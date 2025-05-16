#include "pch.h"
#include "public.h"
#include "Connection.h"
#include <mysql.h>
#include <iostream>
using namespace std;
Connection::Connection()
{
    _conn = mysql_init(nullptr);
}

Connection::~Connection()
{
    if (_conn != nullptr)
        mysql_close(_conn);
}

bool Connection::connect(string ip, unsigned short port, string username, string password, string dbname)
{
    MYSQL* p = mysql_real_connect(_conn, ip.c_str(), username.c_str(), password.c_str(), dbname.c_str(), port, nullptr, 0);
    return p != nullptr;
}

bool Connection::update(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG("����ʧ��:" + sql);
        LOG("����ԭ��: " + string(mysql_error(_conn)));
        return false;
    }
    return true;
}
MYSQL_RES* Connection::query(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG("����ʧ��:" + sql);
        return nullptr;
    }
    return mysql_use_result(_conn);
}