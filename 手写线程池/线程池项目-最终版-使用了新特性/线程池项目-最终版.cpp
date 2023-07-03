#include <iostream>
#include<functional>
#include <thread>
#include <future>
#include "threadpool.h"
#include <chrono>

using namespace std;

/*  如何让线程池提交任务更方便  
1.  pool.submitTask(sum1, 10, 20);
	pool.submitTask(sum2, 1, 2, 3);  //第一个参数接收一个函数，后面的无固定个数的参数为该函数对应的参数
	submitTask: 可变参模板编程

2.  为了接收返回值Result以及相关的类型，造了一个东西
	c++11 线程库  thread   package_task(function函数对象)  打包一个任务
	async 可以直接获取返回值
*/

int sum1(int a, int b) {
	//比较耗时的话， 在 res.get() 处会阻塞
	this_thread::sleep_for(chrono::seconds(2));
	// 比较耗时
	return a + b;
}
int sum2(int a, int b, int c) {
	this_thread::sleep_for(chrono::seconds(2));
	return a + b + c;
}

//io线程
void io_thread(int listenfd) {

}
//work线程
void worker_thread(int clienfd) {

}

int main() {

	ThreadPool pool;
	//pool.setMode(PoolMode::MODE_CACHED);   //默认fixed
	pool.start(2);   //默认为系统的核心数量

	future<int> r1 = pool.submitTask(sum1, 1, 2);
	future<int> r2 = pool.submitTask(sum2, 1, 2, 3);
	future<int> r3 = pool.submitTask([](int b, int e)->int {
		int sum = 0;
		for (int i = b; i <= e; i++)
			sum += i;
		return sum;
		}, 1, 100);
	future<int> r4 = pool.submitTask([](int b, int e)->int {
		int sum = 0;
		for (int i = b; i <= e; i++)
			sum += i;
		return sum;
		}, 1, 100);
	future<int> r5 = pool.submitTask([](int b, int e)->int {
		int sum = 0;
		for (int i = b; i <= e; i++)
			sum += i;
		return sum;
		}, 1, 100);

	//future<int> r4 = pool.submitTask(sum1, 1, 2);

	cout << r1.get() << endl;
	cout << r2.get() << endl;
	cout << r3.get() << endl;
	cout << r4.get() << endl;
	cout << r5.get() << endl;

	//packaged_task<int(int, int)> task(sum1);
	////future <=> Result
	//future<int> res = task.get_future();
	////task(10, 20);
	//thread t(std::move(task), 10, 20);
	//t.detach();

	//cout << res.get() << endl;  //30

	/*thread t1(sum1, 10, 20);
	thread t2(sum2, 1, 2, 3);

	t1.join();
	t2.join();*/
}