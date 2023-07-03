#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <unordered_map>
#include <future>
#include <thread>
#include <iostream>

const int TASK_MAX_THRESHHOLD = 2;// INT32_MAX;
const int THREAD_MAX_THRESHHOLD = 100;   //最好是和CPU核心数量一样
const int THREAD_MAX_IDLE_TIME = 60;  //秒


//线程池支持的模式
enum class PoolMode {
	MODE_FIXED,  //固定数量的线程
	MODE_CACHED,  //线程数量可动态增长
};

//线程类型
class Thread {
public:
	//线程函数对象类型
	using ThreadFunc = std::function<void(int)>;

	//线程构造
	Thread(ThreadFunc func):func_(func), threadId_(generateId_++){
	}

	//线程析构
	~Thread() = default;

	//启动线程
	void start() {

		//创建一个线程来执行一个线程函数
		std::thread t(func_, threadId_);  //c++11  有线程对象t  和线程函数func_
		t.detach();  //设置分离线程    pthread_detach   

	}

	//获取线程id
	int getId() const {
		return threadId_;
	}

private:
	ThreadFunc func_;
	static int generateId_;
	int threadId_;  //保存线程id  局部返回的设置
};
//静态成员变量要在类外初始化
int Thread::generateId_ = 0;


//线程池类型
class ThreadPool {
public:
	//线程池构造函数
	ThreadPool()
		:initThreadSize_(0)
		, taskSize_(0)
		, taskQueMaxThreshHold_(TASK_MAX_THRESHHOLD)
		, poolMode_(PoolMode::MODE_FIXED)
		, isPoolRunning_(false)
		, idleThreadSize_(0)
		, threadSizeThreshHold_(200)
		, curThreadSize_(0) {

	}
	//线程池析构函数
	~ThreadPool() {
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
	void setMode(PoolMode mode) {
		if (checkRunningState()) return;   //不允许在启动后再设置
		poolMode_ = mode;
	}

	//设置初始线程数量
	//void setInitThreadSize(int size);

	//设置task任务队列上限阈值
	void setTaskQueMaxThreshHold(int threshhold) {
		if (checkRunningState()) return;   //不允许在启动后再设置
		taskQueMaxThreshHold_ = threshhold;
	}

	//设置线程池cached模式下线程的阈值
	void setThreadSizeThreshHold(int threshhold) {
		if (checkRunningState()) return;   //不允许在启动后再设置
		if (poolMode_ == PoolMode::MODE_CACHED) {
			idleThreadSize_ = threshhold;
		}
	}

	//给线程池提交任务
	//使用可变参模板编程 让submitTask 可以接收任意函数和任意数量的参数
	//返回值 future< 返回值的类型 >      decltype  根据表达式推断类型
	template<typename Func, typename... Args>        //万能引用 &&
	auto submitTask(Func&& func, Args&&... args) -> std::future<decltype(func(args...))> {
		//打包任务，放入任务队列里面
		using RType = decltype(func(args...));
		auto task = std::make_shared<std::packaged_task<RType()>>(
			std::bind(std::forward<Func>(func), std::forward<Args>(args)...)
			);
		std::future<RType> result = task->get_future();

		//获取锁
		std::unique_lock<std::mutex> lock(taskQueMtx_);

		//线程通信  等待 任务队列有空余
		// 用户提交任务，最长不能阻塞超过1s 否则判断提交失败，返回
		if (!notFull_.wait_for(lock, std::chrono::seconds(1), //等待 1s  1s内满足lambda继续执行返回true  未满足则返回false
			[&]()->bool {return taskQue_.size() < (size_t)taskQueMaxThreshHold_; })) {//进入了则表示条件没有满足 任务提交失败
			
			std::cerr << "task queue is full , submit task fail." << std::endl;
			auto task = std::make_shared<std::packaged_task<RType()>>(
				[]()->RType {return RType(); }   //作为0值的返回值
			);
			(*task)();  //去执行task  任务需要执行才不会阻塞
			return task->get_future();
		}

		//如果有空余， 把任务放入任务队列中
		//using Task = std::function<void()>;
		taskQue_.emplace([task]() {   //对应于  void() 
			//去执行下面的任务
			(*task)();  //去执行task  会得到一个0值  对应与142行
			});
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
		return result;
	}
	


	//开启线程池
	void start(int initThreadSize = std::thread::hardware_concurrency()) {   //默认为系统的核心数
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

	//把线程池的拷贝构造和赋值 屏蔽掉
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

private:
	//定义线程函数     线程池的所有线程从任务队列里面消费任务
	void threadFunc(int threadid) {     //线程函数执行返回，相应的线程也就结束了

		//std::cout << "begin threadFunc tid: " <<  std::this_thread::get_id() << std::endl;
		//std::cout << "end threadFunc tid: " << std::this_thread::get_id() << std::endl;
		//线程要不断的循环


		auto lastTime = std::chrono::high_resolution_clock().now();

		//等所有任务必须执行完，线程池才可以回收线程资源

		for (;;) {
			Task task;
			{
				//先获取锁
				std::unique_lock<std::mutex> lock(taskQueMtx_);

				std::cout << "tid: " << std::this_thread::get_id() << "尝试获取任务..." << std::endl;

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
				task();  //执行function<void()>
				//执行完需通知

			}
			//到这里一个线程处理的事情都完成了

			idleThreadSize_++;

			lastTime = std::chrono::high_resolution_clock().now();   //更新线程执行完任务的时间
		}

	}

	//检查pool的运行状态
	bool checkRunningState() const {
		return isPoolRunning_;
	}

private:
	//std::vector<std::unique_ptr<Thread>> threads_;  //线程列表
	std::unordered_map<int, std::unique_ptr<Thread>> threads_;    //线程列表  因为要根据threadid 找对应的对象 然后去删除Thread对象

	size_t initThreadSize_;   //初始的线程数量
	std::atomic_int curThreadSize_; //记录当前线程池里面线程的总数量
	std::atomic_int idleThreadSize_; //记录空闲线程的数量
	int threadSizeThreshHold_;//线程数量上限阈值

	//Task任务 -> 函数对象
	using Task = std::function<void()>; //  一个中间层 void()
	std::queue<Task> taskQue_;          //任务队列
	std::atomic_uint taskSize_;		    //任务的数量
	int taskQueMaxThreshHold_;		    //任务队列数量上限的阈值

	std::mutex taskQueMtx_; //保证任务队形的线程安全
	std::condition_variable notFull_;//表示任务队列不满
	std::condition_variable notEmpty_;//表示任务队列不空
	std::condition_variable exitCond_;  //等待线程资源全部回收

	PoolMode poolMode_;  //记录当前线程池工作模式
	std::atomic_bool isPoolRunning_; //表示当前线程池的启动状态
};

#endif