#include"pch.h"
#include"CommonConnectionPool.h"
#include<string>
#include"Connection.h"
#include"public.h"
#include<iostream>
#include<thread>
#include <condition_variable>
ConnectionPool* ConnectionPool::getConnectionPool()
{
	static ConnectionPool pool;
	return &pool;
}
bool ConnectionPool::loadConfigFile()
{
	FILE* pf = fopen("mysql.ini", "r");
	if (pf == nullptr)
	{
		LOG("mysql.ini file is not exist!");
		return false;
	}
	while (!feof(pf))
	{
		char line[1024] = { 0 };
		fgets(line, 1024, pf);
		string str = line;
		int idx = str.find('=',0);
		if (idx == -1)//寻找配置文件中每一行"="的开始索引
		{
			continue;
		}
		//password=root\n
		int endidx = str.find('\n',idx);
		string key = str.substr(0, idx);
		string value = str.substr(idx+1,endidx-idx-1);
		if (key == "ip")
		{
			_ip = value;
		}
		else if (key == "port")
		{
			_port= atoi(value.c_str());
		}
		else if (key == "username")
		{
			_username = value;
		}
		else if (key == "password")
		{
			_password = value;
		}
		else if (key == "dbname")
		{
			_dbname = value;
		}
		else if (key == "initSize")
		{
			_initSize = atoi(value.c_str());
		}
		else if (key == "maxSize")
		{
			_maxSize = atoi(value.c_str());
		}
		else if (key == "maxIdleTime")
		{
			_maxIdleTime = atoi(value.c_str());
		}
		else if (key == "_connectionTimeOut")
		{
			_connectionTimeOut = atoi(value.c_str());
		}
		return true;
	}
}
ConnectionPool::ConnectionPool()
{
	if (!loadConfigFile())
	{
		return;
	}
	for (int i = 0; i < _initSize; i++)
	{
		Connection* p = new Connection();
		p->connect(_ip,_port,_username,_password,_dbname);
		p->refreshAliveTime();
		_connectionQue.push(p);
		_connectionCnt++;
	}
	//thread=>pthread_create linux
	thread produce(std::bind(&ConnectionPool::produceconnectionTask,this));//生产者线程 将成员函数作为线程入口 绑定后才能成为线程构造函数的参数
	//线程一启动，就会在新线程里执行 this->produceconnectionTask()。
	produce.detach();

	//启动一个新的定时线程，扫描超过maxidletime时间的空闲连接，进行回收
	thread scanner(std::bind(&ConnectionPool::scannerConnectionTask, this));
	scanner.detach();
}
void ConnectionPool::produceconnectionTask() 
{
	for (;;)
	{
		unique_lock<mutex> lock(_queueMutex);
		while (!_connectionQue.empty())
		{
			cv.wait(lock);
		}

		if (_connectionCnt < _maxSize)
		{
			Connection* p = new Connection();
			p->connect(_ip, _port, _username, _password, _dbname);
			p->refreshAliveTime();
			_connectionQue.push(p);
			_connectionCnt++;
		}
		cv.notify_all();//通知消费者线程可以获取连接了
	}
}
shared_ptr<Connection> ConnectionPool::getConnection()
{
	unique_lock<mutex> lock(_queueMutex);
	while (_connectionQue.empty())
	{
		if (cv_status::timeout == cv.wait_for(lock, chrono::milliseconds(_connectionTimeOut)))
		{
			if (_connectionQue.empty())
			{
				LOG("获取空闲连接超时...获取连接失败！");
					return nullptr;
			}
		}
		
	}
	//智能指针析构时，会把资源释放，connection会被close掉，这不是我们想要的。
	shared_ptr<Connection> sp(_connectionQue.front(),
		[&](Connection* pcon) {
			//在服务器应用线程中调用，因此要注意线程安全
			unique_lock<mutex> lock(_queueMutex);
			pcon->refreshAliveTime(); 
			_connectionQue.push(pcon);
		}
	);
	_connectionQue.pop();
	cv.notify_all();//通知生产者线程检查queue是否为空，可进行生产。
	return sp;
}
void ConnectionPool::scannerConnectionTask()
{
	for (;;)
	{
		// 通过sleep模拟定时效果
		this_thread::sleep_for(chrono::seconds(_maxIdleTime));

		// 扫描整个队列，释放多余的连接
		unique_lock<mutex> lock(_queueMutex);
		while (_connectionCnt > _initSize)
		{
			Connection* p = _connectionQue.front();
			if (p->getAliveTime() >= (_maxIdleTime * 1000))
			{
				_connectionQue.pop();
				_connectionCnt--;
				delete p; // 调用~Connection()释放连接
			}
			else
			{
				break; // 队头的连接没有超过_maxIdleTime，其它连接肯定没有
			}
		}
	}

}