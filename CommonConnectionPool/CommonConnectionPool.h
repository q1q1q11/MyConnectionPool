#pragma once
#include<string>
#include<queue>
#include<mutex>
#include<atomic>
#include<thread>
#include<memory>
#include<functional>
#include <condition_variable>
#include"Connection.h"
using namespace std;
class ConnectionPool
{
public:
		static ConnectionPool* getConnectionPool();
		shared_ptr<Connection> getConnection();
private:
	ConnectionPool();
	bool loadConfigFile();
	void produceconnectionTask();
	void scannerConnectionTask();
	string _ip;
	unsigned short _port;
	string _username;
	string _password;
	string _dbname;
	int _initSize;
	int _maxSize;
	int _maxIdleTime;
	int _connectionTimeOut;
	queue<Connection*> _connectionQue;
	mutex _queueMutex;//维护连接队列的线程安全互斥锁
	atomic_int _connectionCnt;
	condition_variable cv;

};