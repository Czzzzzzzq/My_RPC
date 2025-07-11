#ifndef MY_SQL_THREAD_POOL_H
#define MY_SQL_THREAD_POOL_H

#include <my_log.h>

#include <mysql/mysql.h>
#include <mutex>
#include <semaphore.h>
#include <list>
#include <assert.h>

using namespace std;

class my_Sql_thread_pool
{
public:
    my_Sql_thread_pool();
    ~my_Sql_thread_pool();

    static my_Sql_thread_pool* get_instance();

    void init(string url, string User, string PassWord,string DataBaseName, int Port, int MaxConn);

    MYSQL* get_Mysql_conn();
    bool release_Mysql_conn(MYSQL* conn);
    void destroy_pool();

private:
    int my_sql_thread_poll_MAX_num;
    int my_sql_thread_poll_cur_num;
    
    condition_variable my_sql_thread_poll_cond;
    mutex my_sql_thread_poll_mutex;

    list<MYSQL*> *my_sql_thread_poll;

    string my_url;
    string my_user;
    string my_passward;
    string my_database_name;
    int my_port;
};


#endif