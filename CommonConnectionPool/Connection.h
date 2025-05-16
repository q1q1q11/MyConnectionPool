#pragma once
/*实现mysql数据库的操作*/
#include <mysql.h>
#include <string>
#include<ctime>
using namespace std;
class Connection
{
public:
    Connection();
    ~Connection();
    bool connect(string ip, unsigned short port, string user, string password, string dbname);
    bool update(string sql);
    MYSQL_RES* query(string sql);
    void refreshAliveTime() { _alivetime = clock(); }
    clock_t getAliveTime() const{ return clock()-_alivetime; }
private:
    MYSQL* _conn; // 表示与mysql server的一条连接 mysql厂家自备
    clock_t _alivetime;//记录进入空闲状态时的时间

};
