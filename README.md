# MyConnectionPool
c++手写连接池

一、技术栈

MySQL数据库编程、单例模式、queue队列容器、C++11多线程编程、线程互斥、线程同步通信和unique_lock、基于CAS的原子整形、智能指针shared_ptr、lambda表达式、生产者-消费者线程模型

二、编译方式

vs下添加相应path后可直接编译运行。

三、项目实现细节

1.关于线程方面的编程是通过c++语言级别实现的，并没有调用操作系统api，因此跨平台性较好。

2.sql数据库（基于C/S设计）访问瓶颈：访问数据库本质上是对磁盘进行操作，访问次数过多导致I/O过多。

3.增加连接池是为了减少tcp三次握手四次挥手导致的性能损耗。（在连接池中拿出一条可用的连接，通信完成后不关闭，再放回连接池中）

4.连接池——生产者线程（生产connection），定时线程（当client超过定时未发起请求时清理连接池中的connection至initsize）；sever app——消费者线程。这里的生产者消费者模型是通过mutex与条件变量实现的（也可以通过信号量）实现。

5.shared_ptr<Connection> sp();智能指针自动管理析构，但在这里我们只要归还连接给connectionpool，因此要自定义lambda表达式，不delete当前指向的连接，只归还。用智能指针自动管理连接的归还。

自定义删除器：

// 构造一个 shared_ptr，管理 rawPtr，同时指定一个 lambda 作为“删除器”

shared_ptr<Connection> sp(

    rawPtr,

    [&](Connection* pcon) {

        // 这段代码会在 sp 的最后一个引用被释放时调用

        // 因为要操作队列，要确保线程安全，故加锁

        unique_lock<mutex> lock(_queueMutex);


        // 更新连接的“活跃时间”，避免超时被回收

        pcon->refreshAliveTime();

        // 把连接重新放回队列尾，等待下次复用

        _connectionQue.push(pcon);
    }

);

6.cv.wait(lock)

wait(lock) 的原子操作

当执行到 cv.wait(lock) 时，内部会做两件事，且保证原子性（不会有其它线程偷偷插入）：

解锁：释放 lock 上的互斥量，让别的线程（通常是生产者）有机会进入临界区，修改共享数据并调用 cv.notify_one() 或 notify_all()。

阻塞当前线程：当前线程挂起，直到被另一线程的 notify 唤醒（或出现虚假唤醒）。

被唤醒后的重锁:一旦 wait 因通知或超时返回，它会重新获取那个 lock，然后才会把控制权交回给调用者。这保证了：

你拿到锁时，共享数据（队列状态）依然受保护，能安全地检查或修改它。

与此同时，没有竞争条件：在你被唤醒到真正拿到锁的这段空档，其他线程暂时不会修改这份数据。

7.scanner扫描连接池中空闲时间大于maxidletime的连接进行回收。基于队列的性质，可知队头连接的空闲时间不大于maxidletime时，后面的连接空闲时间也一定不会大于maxidletime。

8.main.cpp执行压力测试

不使用连接池vs使用连接池

单线程vs四线程
