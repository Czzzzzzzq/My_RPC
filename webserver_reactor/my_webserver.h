#ifndef MY_WEB_SERVER_H
#define MY_WEB_SERVER_H

#include "my_scheduler.h"

#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <netinet/in.h>

#include <mutex>
#include <list>
#include <assert.h>
#include <fcntl.h>
#include <unordered_map>
#include <thread>

class my_Rpc_provider;

class my_Webserver_reactor
{
public:
    my_Webserver_reactor(); 
    ~my_Webserver_reactor();

    static my_Webserver_reactor* get_instance();

    void init_my_Webserver_reactor(int port,int sql_port, const char *sql_user,const char *sql_pwd, const char *db_name,int thread_pool_num,int conn_pool_num,int coroutine_num,int listen_timeout,int run_timeout,bool log_close);
    void start_my_Webserver_reactor();
    void stop_my_Webserver_reactor();

    void set_alarm(int timeout);

    void set_my_provider(my_Rpc_provider* provider);
private:
    bool stop_my_server;
private:
    int my_port;
    int my_sql_port;
    const char* my_sql_user;
    const char* my_sql_pwd;
    const char* my_db_name;

    int my_epoll_fd;
    int my_listen_fd;
    int my_pipefd[2];

    int my_conn_pool_num;
    int my_thread_pool_num;
    int my_coroutine_num;
    int my_listen_timeout;
    int my_run_timeout;
    bool my_log_close;

    my_Scheduler *my_scheduler;
    my_Rpc_provider* my_provider;
};


#endif