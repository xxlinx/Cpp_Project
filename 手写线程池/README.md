#### **项目名称：基于可变参模板实现的线程池**

git地址：[github](https://github.com/xxlinx/Cpp_Project)

平台工具：VS2022，centos7编译so库，gdb调试分析定位死锁问题

#### **项目描述**

1.  基于可变参模板编程和万能引用&&，实现线程池submitTask接口，支持任意任务函数的任意参数数量的传递
2.  future 类型定制 submitTask 提交对象的返回值
3.  线程对象用map存储，任务对象用queue队列存储
4.  条件变量condition\_variable与互斥锁mutex实现任务提交线程和任务执行线程的线程通信，常说的消费者模式

#### **项目问题**

1.  在ThreadPool资源回收，等待线程池里所有的线程退出的时候，由于抢锁发生死锁，导致进程无法退出
2.  window下测试无问题，但在linux下测试存在死锁问题，应该头文件针对析构函数的处理存在差异
3.  result any 等问题设计
