// 线程池项目.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。

#include <iostream>
#include <chrono>
#include <thread>
#include "threadpool.h"

using namespace std;


using uLong = unsigned long long;

/*
有些场景，是希望获取线程执行完的返回值的
举例 计算1 + ...+ 30000 的和
thread1: 1 + + 1w  thread2: 10001 + + 2w  thread3: 20001 + + 3w
main thread 计算3个线程的和
*/


class MyTask :public Task {

public:
    MyTask(uLong begin, uLong end) : begin_(begin), end_(end) {

    }
    //问题一： 怎么设计run返回值以满足任意类型
    //c++17 Any 类型(可以接收任意的其他类型) template  能让一个类型 指向 其他任意的类型
    //关键：（基类类型可以指向派生类型）应该是一个基类指针，用基类指针去指向派生类类型
    Any run() {    //run 方法的最终都是在线程里面执行任务
        std::cout << "tid: " <<  std::this_thread::get_id() << "  begin!"<<std::endl;
    
        std::this_thread::sleep_for(std::chrono::seconds(1));
        uLong sum = 0;
        for (uLong i = begin_; i <= end_; i++) {
            sum += i;
        }

        std::cout << "tid: " << std::this_thread::get_id() << "  end!" << std::endl;

        return sum;
    }
private:
    uLong begin_;
    uLong end_;
};

int main() {
    {
        ThreadPool pool;
        pool.setMode(PoolMode::MODE_CACHED);
        //开始启动线程
        pool.start(2);

        Result res1 = pool.submitTask(std::make_shared<MyTask>(1, 100000000));
        Result res2 = pool.submitTask(std::make_shared<MyTask>(1, 100000000));
        pool.submitTask(std::make_shared<MyTask>(1, 100000000));
        pool.submitTask(std::make_shared<MyTask>(1, 100000000));
        pool.submitTask(std::make_shared<MyTask>(1, 100000000));
        pool.submitTask(std::make_shared<MyTask>(1, 100000000));

        uLong sum1 = res1.get().cast_<uLong>();
        std::cout << sum1 << std::endl;
    }

    std::cout << " main is over" << std::endl;

    getchar();

#if 0
    //问题 ThreadPool 对象析构了以后， 怎么样把线程池相关的资源全部回收 ？？？
    {
        ThreadPool pool;
        //用户自己设置工作模式
        pool.setMode(PoolMode::MODE_CACHED);
        // 开始启动线程池
        pool.start(4);

        //问题二：如何设计这里的result机制呢
        Result res1 = pool.submitTask(std::make_shared<MyTask>(1, 100000000));
        Result res2 = pool.submitTask(std::make_shared<MyTask>(100000001, 200000000));
        Result res3 = pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));

        pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));
        pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));
        pool.submitTask(std::make_shared<MyTask>(200000001, 300000000));

        // 随着task被执行完，task对象没了，依赖于task对象的Result对象也没了
        uLong sum1 = res1.get().cast_<uLong>();  // get返回了一个Any类型，怎么转成具体的类型呢？
        uLong sum2 = res2.get().cast_<uLong>();
        uLong sum3 = res3.get().cast_<uLong>();

        // Master - Slave线程模型
        // Master线程用来分解任务，然后给各个Slave线程分配任务
        // 等待各个Slave线程执行完任务，返回结果
        // Master线程合并各个任务结果，输出
        cout << (sum1 + sum2 + sum3) << endl;

    }
   // std::this_thread::sleep_for(std::chrono::seconds(5));
    getchar();
#endif

    return 0;
}
