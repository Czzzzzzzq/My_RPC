#include "my_sql_thread_pool.h"

my_Sql_thread_pool::my_Sql_thread_pool()
{
    my_sql_thread_poll_MAX_num = 0;
    my_sql_thread_poll = nullptr;
}

my_Sql_thread_pool::~my_Sql_thread_pool()
{
    destroy_pool();
}

my_Sql_thread_pool *my_Sql_thread_pool::get_instance(){
    static my_Sql_thread_pool connPool;
    return &connPool;
}

void my_Sql_thread_pool::init(string url, string user, string passward, 
                            string database_name, int port, int maxconn){
    my_url = url;
    my_user = user;
    my_passward = passward;
    my_database_name = database_name;
    my_port = port;
    my_sql_thread_poll_MAX_num = maxconn;
    my_sql_thread_poll = new std::list<MYSQL*>;

    int real_conn = 0;
    for(int i = 0;i < my_sql_thread_poll_MAX_num;i++){
        MYSQL* conn_ptr = nullptr;
        conn_ptr = mysql_init(conn_ptr);
        if(conn_ptr == nullptr){
            LOG_ERROR("mysql init error,errno = %d",errno);
        }
        conn_ptr = mysql_real_connect(conn_ptr,my_url.c_str(),my_user.c_str(),
                                my_passward.c_str(),my_database_name.c_str(),
                                my_port,NULL,0);
        if (conn_ptr == nullptr){
            LOG_ERROR("mysql real connect error,errno = %d",errno);
        }
        my_sql_thread_poll->push_back(conn_ptr);
        real_conn++;
    }
    my_sql_thread_poll_MAX_num = real_conn;
    my_sql_thread_poll_cur_num = real_conn;
}

MYSQL* my_Sql_thread_pool::get_Mysql_conn(){
    my_Sql_thread_pool* connPool = my_Sql_thread_pool::get_instance();
    MYSQL* conn = nullptr;

    std::unique_lock<std::mutex> lock(my_sql_thread_poll_mutex);
    my_sql_thread_poll_cond.wait(lock,[this](){return my_sql_thread_poll_cur_num > 0;});
    conn = connPool->my_sql_thread_poll->front();
    connPool->my_sql_thread_poll->pop_front();
    my_sql_thread_poll_cur_num--;
    lock.unlock();
    return conn;
}

bool my_Sql_thread_pool::release_Mysql_conn(MYSQL* conn){
    if(conn == nullptr){
        return false;
    }
    std::lock_guard<std::mutex> lock(my_sql_thread_poll_mutex);
    my_sql_thread_poll->push_back(conn);
    my_sql_thread_poll_cur_num++;
    my_sql_thread_poll_cond.notify_one();
    return true;
}

void my_Sql_thread_pool::destroy_pool(){
    if(my_sql_thread_poll == nullptr){
        return;
    }

    std::lock_guard<std::mutex> lock(my_sql_thread_poll_mutex);
    for(auto conn: *my_sql_thread_poll){
        mysql_close(conn);
    }
    my_sql_thread_poll_MAX_num = 0;
    my_sql_thread_poll->clear();
    mysql_library_end();
}