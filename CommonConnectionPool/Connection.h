#pragma once
/*ʵ��mysql���ݿ�Ĳ���*/
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
    MYSQL* _conn; // ��ʾ��mysql server��һ������ mysql�����Ա�
    clock_t _alivetime;//��¼�������״̬ʱ��ʱ��

};
