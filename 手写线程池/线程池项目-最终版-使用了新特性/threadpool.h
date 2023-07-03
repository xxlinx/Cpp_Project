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
const int THREAD_MAX_THRESHHOLD = 100;   //����Ǻ�CPU��������һ��
const int THREAD_MAX_IDLE_TIME = 60;  //��


//�̳߳�֧�ֵ�ģʽ
enum class PoolMode {
	MODE_FIXED,  //�̶��������߳�
	MODE_CACHED,  //�߳������ɶ�̬����
};

//�߳�����
class Thread {
public:
	//�̺߳�����������
	using ThreadFunc = std::function<void(int)>;

	//�̹߳���
	Thread(ThreadFunc func):func_(func), threadId_(generateId_++){
	}

	//�߳�����
	~Thread() = default;

	//�����߳�
	void start() {

		//����һ���߳���ִ��һ���̺߳���
		std::thread t(func_, threadId_);  //c++11  ���̶߳���t  ���̺߳���func_
		t.detach();  //���÷����߳�    pthread_detach   

	}

	//��ȡ�߳�id
	int getId() const {
		return threadId_;
	}

private:
	ThreadFunc func_;
	static int generateId_;
	int threadId_;  //�����߳�id  �ֲ����ص�����
};
//��̬��Ա����Ҫ�������ʼ��
int Thread::generateId_ = 0;


//�̳߳�����
class ThreadPool {
public:
	//�̳߳ع��캯��
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
	//�̳߳���������
	~ThreadPool() {
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
	void setMode(PoolMode mode) {
		if (checkRunningState()) return;   //��������������������
		poolMode_ = mode;
	}

	//���ó�ʼ�߳�����
	//void setInitThreadSize(int size);

	//����task�������������ֵ
	void setTaskQueMaxThreshHold(int threshhold) {
		if (checkRunningState()) return;   //��������������������
		taskQueMaxThreshHold_ = threshhold;
	}

	//�����̳߳�cachedģʽ���̵߳���ֵ
	void setThreadSizeThreshHold(int threshhold) {
		if (checkRunningState()) return;   //��������������������
		if (poolMode_ == PoolMode::MODE_CACHED) {
			idleThreadSize_ = threshhold;
		}
	}

	//���̳߳��ύ����
	//ʹ�ÿɱ��ģ���� ��submitTask ���Խ������⺯�������������Ĳ���
	//����ֵ future< ����ֵ������ >      decltype  ���ݱ��ʽ�ƶ�����
	template<typename Func, typename... Args>        //�������� &&
	auto submitTask(Func&& func, Args&&... args) -> std::future<decltype(func(args...))> {
		//������񣬷��������������
		using RType = decltype(func(args...));
		auto task = std::make_shared<std::packaged_task<RType()>>(
			std::bind(std::forward<Func>(func), std::forward<Args>(args)...)
			);
		std::future<RType> result = task->get_future();

		//��ȡ��
		std::unique_lock<std::mutex> lock(taskQueMtx_);

		//�߳�ͨ��  �ȴ� ��������п���
		// �û��ύ�����������������1s �����ж��ύʧ�ܣ�����
		if (!notFull_.wait_for(lock, std::chrono::seconds(1), //�ȴ� 1s  1s������lambda����ִ�з���true  δ�����򷵻�false
			[&]()->bool {return taskQue_.size() < (size_t)taskQueMaxThreshHold_; })) {//���������ʾ����û������ �����ύʧ��
			
			std::cerr << "task queue is full , submit task fail." << std::endl;
			auto task = std::make_shared<std::packaged_task<RType()>>(
				[]()->RType {return RType(); }   //��Ϊ0ֵ�ķ���ֵ
			);
			(*task)();  //ȥִ��task  ������Ҫִ�вŲ�������
			return task->get_future();
		}

		//����п��࣬ ������������������
		//using Task = std::function<void()>;
		taskQue_.emplace([task]() {   //��Ӧ��  void() 
			//ȥִ�����������
			(*task)();  //ȥִ��task  ��õ�һ��0ֵ  ��Ӧ��142��
			});
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
		return result;
	}
	


	//�����̳߳�
	void start(int initThreadSize = std::thread::hardware_concurrency()) {   //Ĭ��Ϊϵͳ�ĺ�����
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

	//���̳߳صĿ�������͸�ֵ ���ε�
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

private:
	//�����̺߳���     �̳߳ص������̴߳��������������������
	void threadFunc(int threadid) {     //�̺߳���ִ�з��أ���Ӧ���߳�Ҳ�ͽ�����

		//std::cout << "begin threadFunc tid: " <<  std::this_thread::get_id() << std::endl;
		//std::cout << "end threadFunc tid: " << std::this_thread::get_id() << std::endl;
		//�߳�Ҫ���ϵ�ѭ��


		auto lastTime = std::chrono::high_resolution_clock().now();

		//�������������ִ���꣬�̳߳زſ��Ի����߳���Դ

		for (;;) {
			Task task;
			{
				//�Ȼ�ȡ��
				std::unique_lock<std::mutex> lock(taskQueMtx_);

				std::cout << "tid: " << std::this_thread::get_id() << "���Ի�ȡ����..." << std::endl;

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
				task();  //ִ��function<void()>
				//ִ������֪ͨ

			}
			//������һ���̴߳�������鶼�����

			idleThreadSize_++;

			lastTime = std::chrono::high_resolution_clock().now();   //�����߳�ִ���������ʱ��
		}

	}

	//���pool������״̬
	bool checkRunningState() const {
		return isPoolRunning_;
	}

private:
	//std::vector<std::unique_ptr<Thread>> threads_;  //�߳��б�
	std::unordered_map<int, std::unique_ptr<Thread>> threads_;    //�߳��б�  ��ΪҪ����threadid �Ҷ�Ӧ�Ķ��� Ȼ��ȥɾ��Thread����

	size_t initThreadSize_;   //��ʼ���߳�����
	std::atomic_int curThreadSize_; //��¼��ǰ�̳߳������̵߳�������
	std::atomic_int idleThreadSize_; //��¼�����̵߳�����
	int threadSizeThreshHold_;//�߳�����������ֵ

	//Task���� -> ��������
	using Task = std::function<void()>; //  һ���м�� void()
	std::queue<Task> taskQue_;          //�������
	std::atomic_uint taskSize_;		    //���������
	int taskQueMaxThreshHold_;		    //��������������޵���ֵ

	std::mutex taskQueMtx_; //��֤������ε��̰߳�ȫ
	std::condition_variable notFull_;//��ʾ������в���
	std::condition_variable notEmpty_;//��ʾ������в���
	std::condition_variable exitCond_;  //�ȴ��߳���Դȫ������

	PoolMode poolMode_;  //��¼��ǰ�̳߳ع���ģʽ
	std::atomic_bool isPoolRunning_; //��ʾ��ǰ�̳߳ص�����״̬
};

#endif