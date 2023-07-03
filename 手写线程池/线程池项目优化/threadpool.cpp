#include "threadpool.h"
#include <functional>
#include <thread>
#include <iostream>


const int TASK_MAX_THRESHHOLD = INT32_MAX;
const int THREAD_MAX_THRESHHOLD = 100;   //����Ǻ�CPU��������һ��
const int THREAD_MAX_IDLE_TIME = 60;  //��

//�̳߳ع��캯��
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

//�̳߳���������
ThreadPool::~ThreadPool() {
	//��Ա�������޸�
	isPoolRunning_ = false;

	/*   �������������   */
	//notEmpty_.notify_all();  //�������еȴ����߳�

	////�ȴ��̳߳��������е��̷߳���     ������״̬�� ��������ִ��������
	////��Ҫ�߳�ͨѶ
	//std::unique_lock<std::mutex> lock(taskQueMtx_);

	//�޸�Ϊ���������ٻ��ѵȴ����߳�
	std::unique_lock<std::mutex> lock(taskQueMtx_);
	notEmpty_.notify_all();  //�������еȴ����߳�


	exitCond_.wait(lock, [&]()->bool {return threads_.size() == 0; });  //�������̶߳������,

}

//�����̳߳صĹ���ģʽ
void ThreadPool::setMode(PoolMode mode) {
	if (checkRunningState()) return;   //��������������������
	poolMode_ = mode;
}

//����task�������������ֵ
void ThreadPool::setTaskQueMaxThreshHold(int threshhold) {
	if (checkRunningState()) return;   //��������������������
	taskQueMaxThreshHold_ = threshhold;
}

//�����̳߳�cachedģʽ���̵߳�������ֵ
void ThreadPool::setThreadSizeThreshHold(int threshhold){
	if (checkRunningState()) return;   //��������������������
	if (poolMode_ == PoolMode::MODE_CACHED) {
		idleThreadSize_ = threshhold;
	}
}

//���̳߳��ύ����   �û����øýӿڣ��������������������
Result ThreadPool::submitTask(std::shared_ptr<Task> sp) {
	
	//��ȡ��
	std::unique_lock<std::mutex> lock(taskQueMtx_);

	//�߳�ͨ��  �ȴ� ��������п���
	// �û��ύ�����������������1s �����ж��ύʧ�ܣ�����
	//while (taskQue_.size() == taskQueMaxThreshHold_) {
	//	notFull_.wait(lock);  //����ȴ�״̬
	//}
	//���ѭ������д��  ����lambda ������ �Ͳ�������������
	// wait ��ʱ��û�й�ϵ��,һֱ�ȵ���������   wait_for ���������㣬�����ʱ��  wait_until ����һ�������ȴ���ʱ�� 
	if ( !notFull_.wait_for(lock, std::chrono::seconds(1), //�ȴ� 1s  1s������lambda����ִ�з���true  δ�����򷵻�false
		[&]()->bool {return taskQue_.size() < (size_t)taskQueMaxThreshHold_; })) {
		//���������ʾ����û������
		std::cerr << "task queue is full , submit task fail." << std::endl;

		//return task->getResult();   //��������ķ������result����  �߳�ִ����task��task����ͱ���������
		return Result(sp, false);
	}

	//����п��࣬ ������������������
	taskQue_.emplace(sp);
	taskSize_++;

	//��Ϊ���������ˣ� ������п϶����ǿգ� notEmpty_ �Ͻ���֪ͨ�������߳�ִ������
	notEmpty_.notify_all();

	//cachedģʽ ������ȽϽ���  ������С���������  ��Ҫ�������������Ϳ����߳��������ж��Ƿ���Ҫ�����µ��̳߳���
	if (poolMode_ == PoolMode::MODE_CACHED
		&& taskSize_ > idleThreadSize_
		&& curThreadSize_ < threadSizeThreshHold_) {

		std::cout << ">>>create new thread  " << std::this_thread::get_id() << std::endl;
		//�������߳�
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
		int threadId = ptr->getId();
		threads_.emplace(threadId, std::move(ptr));
		threads_[threadId]->start();

		//�޸��߳�������صı���
		curThreadSize_++;
		idleThreadSize_++;
	}

	//����result ����
	//return sp->getResult();
	return Result(sp);
}

//�����̳߳�
void ThreadPool::start(int initThreadSize) {
	//�����̳߳ص�����״̬
	isPoolRunning_ = true;

	//��¼��ʼ�̸߳���
	initThreadSize_ = initThreadSize;
	curThreadSize_ = initThreadSize;

	//�����̶߳���
	for (int i = 0; i < initThreadSize_; i++) {
		//emplace_back �������������Ԫ�أ�������Ҫ��ʽ������ʱ���� ����ֱ������µĶ���
		//����thread�̶߳����ʱ�򣬰��̺߳�������thread�̶߳���
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
		//threads_.emplace_back(ptr);  ���﷢���˿������죬��unique ptr ������
		//threads_.emplace_back(std::move(ptr));
		int threadId = ptr->getId();
		threads_.emplace(threadId, std::move(ptr));

	}

	//���������߳�
	for (int i = 0; i < initThreadSize_; i++) {
		threads_[i]->start(); //��Ҫȥִ��һ���̺߳���   ����� i ���� key
		idleThreadSize_++;//��¼��ʼ�����̵߳�����
	}
}

//�����̺߳���   �̳߳ص������̴߳��������������������
void ThreadPool::threadFunc(int threadid) {     //�̺߳���ִ�з��أ���Ӧ���߳�Ҳ�ͽ�����

	//std::cout << "begin threadFunc tid: " <<  std::this_thread::get_id() << std::endl;
	//std::cout << "end threadFunc tid: " << std::this_thread::get_id() << std::endl;
	//�߳�Ҫ���ϵ�ѭ��


	auto lastTime = std::chrono::high_resolution_clock().now();

	//�������������ִ���꣬�̳߳زſ��Ի����߳���Դ

	for (;;) {
		std::shared_ptr<Task> task;
		{	
			//�Ȼ�ȡ��
			std::unique_lock<std::mutex> lock(taskQueMtx_);

			std::cout << "tid: " << std::this_thread::get_id() <<"���Ի�ȡ����..."<< std::endl;

			//��cachedģʽ�£��п����Ѿ������˺ܶ��̣߳����ǿ���ʱ�䳬��60s��Ӧ�ðѶ�����߳̽������յ����̲߳���������Խ��Խ�ã�
			//����initThreadSize_ �������߳�Ҫ���л���

			//������Ͳ��������������ͱ���Ҫִ��
			//�� + ˫���ж�   ��������
			while (taskQue_.size() == 0) {   //û������->���еȴ�

				//����Ϊ0 ����̳߳�Ҫ�����������߳���Դ
				if (!isPoolRunning_) {
					
					threads_.erase(threadid);
					std::cout << "threadid" << std::this_thread::get_id() << "  exit!" << std::endl;
					exitCond_.notify_all();

					return;  //�̺߳����������߳̽���
				}

				if (poolMode_ == PoolMode::MODE_CACHED) {   //  cachedģʽ
					//ÿһ���ӷ���һ��  ��ô���ֳ�ʱ���أ������������ִ�з���
				//��ʱ����
					if (std::cv_status::timeout == notEmpty_.wait_for(lock, std::chrono::seconds(1))) {
						auto now = std::chrono::high_resolution_clock().now();
						auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);
						if (dur.count() >= THREAD_MAX_IDLE_TIME && curThreadSize_ > initThreadSize_) {
							//��ʼ���յ�ǰ���߳�
							//��¼�߳���������ر�����ֵ���޸�
							//���̶߳�����߳��б�������ɾ��  ***(�Ѹ�)*** û�а취threadFunc ��Ӧ�����ĸ� thread����
							//��Ҫ��һ��ӳ���ϵ threadid -> thread���� -> ɾ��
							threads_.erase(threadid);
							curThreadSize_--;
							idleThreadSize_--;

							std::cout << "threadid" << std::this_thread::get_id() << "  exit!" << std::endl;
							return;
						}
					}
				}
				else {  //fixedģʽ
					//�ȴ�notEmpty ����
					notEmpty_.wait(lock);
				}

				/*****  �ԱȲο�  *****/
				//�̳߳�Ҫ�������ͻ�����Դ
				//if (!isPoolRunning_) {
				//	threads_.erase(threadid);
				//	std::cout << "threadid" << std::this_thread::get_id() << "  exit!" << std::endl;
				//	exitCond_.notify_all();
				//	return;
				//}
			}

			idleThreadSize_--;  //������һ���߳� ���е�-1
			std::cout << "tid: " << std::this_thread::get_id() << "��ȡ����ɹ�������" << std::endl;

			//���� ���������ȡһ���������
			task = taskQue_.front();
			taskQue_.pop();
			taskSize_--;

			//�����ʣ������֪ͨ�����߳�
			if (taskQue_.size() > 0) {
				notEmpty_.notify_all();
			}

			//ȡ��һ������Ҫ֪ͨ ֪ͨ���Լ�����������
			notFull_.notify_all();

		}//��������� ��Ӧ�ð����ͷŵ�   ���Ǽ���������� {  }  ��Ŀ��
	
		//��ǰ�̸߳�����ִ���������
		if (task != nullptr) {
			//task->run();  //ִ�����񣬰�����ķ���ֵ setval �������� Result
			task->exec();
			//ִ������֪ͨ

		}	
		//������һ���̴߳�������鶼�����
		
		idleThreadSize_++;

		lastTime = std::chrono::high_resolution_clock().now();   //�����߳�ִ���������ʱ��
	}
	
}

bool ThreadPool::checkRunningState() const {
	return isPoolRunning_;
}


/**************    �̷߳���ʵ��    *************/

int Thread::generateId_ = 0;

//�̹߳���
Thread::Thread(ThreadFunc func) 
	:func_(func), threadId_(generateId_++)
{

}

//�߳�����
Thread::~Thread() {

}

//�����߳�
void Thread::start() {
	
	//����һ���߳���ִ��һ���̺߳���
	std::thread t(func_, threadId_);  //c++11  ���̶߳���t  ���̺߳���func_
	t.detach();  //���÷����߳�    pthread_detach   

}

int Thread::getId() const{
	return threadId_;
}

/********      Task ������ʵ��       ********/
Task::Task():result_(nullptr) {}

void Task::exec() {
	if (result_ != nullptr) {
		result_->setVal(run());  // ���﷢����̬����
	}
}

void Task::setResult(Result* res) {
	result_ = res;
}

/********      Result ������ʵ��       ********/
Result::Result(std::shared_ptr<Task> task, bool isValid) 
	:isValid_(isValid), task_(task){
	task_->setResult(this);   //this ���ǵ�ǰResult ����
}

Any Result::get() {   //�û�����
	if (!isValid_) {
		return "";
	}

	sem_.wait();  //task�������û��ִ���꣬����������û����߳�
	return std::move(any_);
}

void Result::setVal(Any any) {   //
	//�洢task ����ֵ 
	this->any_ = std::move(any);
	sem_.post();  //�Ѿ���ȡ������ķ���ֵ�������ź�����Դ

}