#include "threadpool.h"
#include <functional>
#include <thread>
#include <iostream>


const int TASK_MAX_THRESHHOLD = INT32_MAX;
const int THREAD_MAX_THRESHHOLD = 100;   //最好是和CPU核心数量一样
const int THREAD_MAX_IDLE_TIME = 60;  //秒

//线程池构造函数
ThreadPool::ThreadPool()
	:initThreadSize_(0)
	, taskSize_(0)
	, taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD)
	, poolMode_(PoolMode::MODE_FIXED)
	, isPoolRunning_(false)
	, idleThreadSize_(0)
	, threadSizeThreshHold_(200)
	, curThreadSize_(0){

}

//线程池析构函数
ThreadPool::~ThreadPool() {
	//成员变量的修改
	isPoolRunning_ = false;

	/*   这样会造成死锁   */
	//notEmpty_.notify_all();  //唤醒所有等待的线程

	////等待线程池里面所有的线程返回     有两种状态： 阻塞，和执行任务中
	////需要线程通讯
	//std::unique_lock<std::mutex> lock(taskQueMtx_);

	//修改为先拿锁，再唤醒等待的线程
	std::unique_lock<std::mutex> lock(taskQueMtx_);
	notEmpty_.notify_all();  //唤醒所有等待的线程


	exitCond_.wait(lock, [&]()->bool {return threads_.size() == 0; });  //等所有线程对象结束,

}

//设置线程池的工作模式
void ThreadPool::setMode(PoolMode mode) {
	if (checkRunningState()) return;   //不允许在启动后再设置
	poolMode_ = mode;
}

//设置task任务队列上限阈值
void ThreadPool::setTaskQueMaxThreshHold(int threshhold) {
	if (checkRunningState()) return;   //不允许在启动后再设置
	taskQueMaxThreshHold_ = threshhold;
}

//设置线程池cached模式下线程的上限阈值
void ThreadPool::setThreadSizeThreshHold(int threshhold){
	if (checkRunningState()) return;   //不允许在启动后再设置
	if (poolMode_ == PoolMode::MODE_CACHED) {
		idleThreadSize_ = threshhold;
	}
}

//给线程池提交任务   用户调用该接口，传入任务对象，生产任务
Result ThreadPool::submitTask(std::shared_ptr<Task> sp) {
	
	//获取锁
	std::unique_lock<std::mutex> lock(taskQueMtx_);

	//线程通信  等待 任务队列有空余
	// 用户提交任务，最长不能阻塞超过1s 否则判断提交失败，返回
	//while (taskQue_.size() == taskQueMaxThreshHold_) {
	//	notFull_.wait(lock);  //进入等待状态
	//}
	//这个循环可以写成  满足lambda 的条件 就不会阻塞在这里
	// wait 与时间没有关系的,一直等到条件满足   wait_for 等条件满足，有最大时间  wait_until 设置一个持续等待的时间 
	if ( !notFull_.wait_for(lock, std::chrono::seconds(1), //等待 1s  1s内满足lambda继续执行返回true  未满足则返回false
		[&]()->bool {return taskQue_.size() < (size_t)taskQueMaxThreshHold_; })) {
		//进入了则表示条件没有满足
		std::cerr << "task queue is full , submit task fail." << std::endl;

		//return task->getResult();   //调用任务的方法获得result对象  线程执行完task，task对象就被析构掉了
		return Result(sp, false);
	}

	//如果有空余， 把任务放入任务队列中
	taskQue_.emplace(sp);
	taskSize_++;

	//因为放入任务了， 任务队列肯定不是空， notEmpty_ 上进行通知，分配线程执行任务
	notEmpty_.notify_all();

	//cached模式 任务处理比较紧急  场景：小而快的任务  需要根据任务数量和空闲线程数量，判断是否需要创建新的线程出来
	if (poolMode_ == PoolMode::MODE_CACHED
		&& taskSize_ > idleThreadSize_
		&& curThreadSize_ < threadSizeThreshHold_) {

		std::cout << ">>>create new thread  " << std::this_thread::get_id() << std::endl;
		//创建新线程
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
		int threadId = ptr->getId();
		threads_.emplace(threadId, std::move(ptr));
		threads_[threadId]->start();

		//修改线程数量相关的变量
		curThreadSize_++;
		idleThreadSize_++;
	}

	//返回result 对象
	//return sp->getResult();
	return Result(sp);
}

//开启线程池
void ThreadPool::start(int initThreadSize) {
	//设置线程池的运行状态
	isPoolRunning_ = true;

	//记录初始线程个数
	initThreadSize_ = initThreadSize;
	curThreadSize_ = initThreadSize;

	//创建线程对象
	for (int i = 0; i < initThreadSize_; i++) {
		//emplace_back 向容器中添加新元素，而不需要显式创建临时对象 就是直接添加新的对象
		//创建thread线程对象的时候，把线程函数给到thread线程对象
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
		//threads_.emplace_back(ptr);  这里发生了拷贝构造，但unique ptr 不可以
		//threads_.emplace_back(std::move(ptr));
		int threadId = ptr->getId();
		threads_.emplace(threadId, std::move(ptr));

	}

	//启动所有线程
	for (int i = 0; i < initThreadSize_; i++) {
		threads_[i]->start(); //需要去执行一个线程函数   这里的 i 就是 key
		idleThreadSize_++;//记录初始空闲线程的数量
	}
}

//定义线程函数   线程池的所有线程从任务队列里面消费任务
void ThreadPool::threadFunc(int threadid) {     //线程函数执行返回，相应的线程也就结束了

	//std::cout << "begin threadFunc tid: " <<  std::this_thread::get_id() << std::endl;
	//std::cout << "end threadFunc tid: " << std::this_thread::get_id() << std::endl;
	//线程要不断的循环


	auto lastTime = std::chrono::high_resolution_clock().now();

	//等所有任务必须执行完，线程池才可以回收线程资源

	for (;;) {
		std::shared_ptr<Task> task;
		{	
			//先获取锁
			std::unique_lock<std::mutex> lock(taskQueMtx_);

			std::cout << "tid: " << std::this_thread::get_id() <<"尝试获取任务..."<< std::endl;

			//在cached模式下，有可能已经创建了很多线程，但是空闲时间超过60s，应该把多余的线程结束回收掉（线程并不是数量越多越好）
			//超过initThreadSize_ 数量的线程要进行回收

			//有任务就不会进来，有任务就必须要执行
			//锁 + 双重判断   避免死锁
			while (taskQue_.size() == 0) {   //没有任务->进行等待

				//任务为0 如果线程池要结束，回收线程资源
				if (!isPoolRunning_) {
					
					threads_.erase(threadid);
					std::cout << "threadid" << std::this_thread::get_id() << "  exit!" << std::endl;
					exitCond_.notify_all();

					return;  //线程函数结束，线程结束
				}

				if (poolMode_ == PoolMode::MODE_CACHED) {   //  cached模式
					//每一秒钟返回一次  怎么区分超时返回，还是有任务待执行返回
				//超时返回
					if (std::cv_status::timeout == notEmpty_.wait_for(lock, std::chrono::seconds(1))) {
						auto now = std::chrono::high_resolution_clock().now();
						auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);
						if (dur.count() >= THREAD_MAX_IDLE_TIME && curThreadSize_ > initThreadSize_) {
							//开始回收当前的线程
							//记录线程数量的相关变量的值的修改
							//把线程对象从线程列表容器中删除  ***(难改)*** 没有办法threadFunc 对应的是哪个 thread对象
							//需要有一个映射关系 threadid -> thread对象 -> 删除
							threads_.erase(threadid);
							curThreadSize_--;
							idleThreadSize_--;

							std::cout << "threadid" << std::this_thread::get_id() << "  exit!" << std::endl;
							return;
						}
					}
				}
				else {  //fixed模式
					//等待notEmpty 条件
					notEmpty_.wait(lock);
				}

				/*****  对比参考  *****/
				//线程池要结束，就回收资源
				//if (!isPoolRunning_) {
				//	threads_.erase(threadid);
				//	std::cout << "threadid" << std::this_thread::get_id() << "  exit!" << std::endl;
				//	exitCond_.notify_all();
				//	return;
				//}
			}

			idleThreadSize_--;  //启用了一个线程 空闲的-1
			std::cout << "tid: " << std::this_thread::get_id() << "获取任务成功！！！" << std::endl;

			//不空 从任务队列取一个任务出来
			task = taskQue_.front();
			taskQue_.pop();
			taskSize_--;

			//如果有剩余任务，通知其他线程
			if (taskQue_.size() > 0) {
				notEmpty_.notify_all();
			}

			//取出一个任务要通知 通知可以继续生产任务
			notFull_.notify_all();

		}//到这里这个 就应该把锁释放掉   就是加这个作用域 {  }  的目的
	
		//当前线程负责处理执行这个任务
		if (task != nullptr) {
			//task->run();  //执行任务，把任务的返回值 setval 方法给到 Result
			task->exec();
			//执行完需通知

		}	
		//到这里一个线程处理的事情都完成了
		
		idleThreadSize_++;

		lastTime = std::chrono::high_resolution_clock().now();   //更新线程执行完任务的时间
	}
	
}

bool ThreadPool::checkRunningState() const {
	return isPoolRunning_;
}


/**************    线程方法实现    *************/

int Thread::generateId_ = 0;

//线程构造
Thread::Thread(ThreadFunc func) 
	:func_(func), threadId_(generateId_++)
{

}

//线程析构
Thread::~Thread() {

}

//启动线程
void Thread::start() {
	
	//创建一个线程来执行一个线程函数
	std::thread t(func_, threadId_);  //c++11  有线程对象t  和线程函数func_
	t.detach();  //设置分离线程    pthread_detach   

}

int Thread::getId() const{
	return threadId_;
}

/********      Task 方法的实现       ********/
Task::Task():result_(nullptr) {}

void Task::exec() {
	if (result_ != nullptr) {
		result_->setVal(run());  // 这里发生多态调用
	}
}

void Task::setResult(Result* res) {
	result_ = res;
}

/********      Result 方法的实现       ********/
Result::Result(std::shared_ptr<Task> task, bool isValid) 
	:isValid_(isValid), task_(task){
	task_->setResult(this);   //this 就是当前Result 对象
}

Any Result::get() {   //用户调用
	if (!isValid_) {
		return "";
	}

	sem_.wait();  //task任务如果没有执行完，这里会阻塞用户的线程
	return std::move(any_);
}

void Result::setVal(Any any) {   //
	//存储task 返回值 
	this->any_ = std::move(any);
	sem_.post();  //已经获取了任务的返回值，增加信号量资源

}