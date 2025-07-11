#ifndef MY_SCHEDULER_H
#define MY_SCHEDULER_H

#include "my_coroutine.h"
#include "my_task_node.h"
#include "my_log.h"
#include "my_timer.h"
#include "my_sql_thread_pool.h"

#include <sys/socket.h>
#include <string.h>
#include <thread>
#include <mutex>
#include <unistd.h>
#include <list>
#include <condition_variable>
#include <netinet/in.h>

class my_Rpc_provider;

class my_Scheduler
{
public:
    my_Scheduler();
    ~my_Scheduler();

    void stop_my_Scheduler();

    static my_Scheduler* get_instance();

    void init(int epoll_fd,int listen_fd,int* pipe_fd,int sql_port, const char *sql_user,const char *sql_pwd, 
            const char *db_name,int thread_pool_num,int conn_pool_num,int coroutine_num,int listen_timeout,int run_timeout,bool log_close);
    void run();

    void add_task(int socket);
    void add_task(my_Task_node* task);
    my_Task_node* get_task();

    void idle();

    void Coroutine_run(my_Coroutine* my_coroutine,my_Scheduler* my_scheduler);

    void read_task(my_Coroutine* coroutine);
    void work_task(my_Coroutine* coroutine);
    void write_task(my_Coroutine* coroutine);
    void close_task(my_Coroutine* coroutine);
    void error_task(my_Coroutine* coroutine);

    my_Coroutine* get_free_coroutine();
    void return_free_coroutine(my_Coroutine* coroutine);

    my_Coroutine* get_busy_coroutine();
    void return_busy_coroutine(my_Coroutine* coroutine);

    my_Coroutine* get_close_coroutine();
    void return_close_coroutine(my_Coroutine* coroutine);

    void addfd(int fd,my_Task_node::task_status status);
    void modfd(int fd,my_Task_node::task_status status);

    void dealwithsignal();

    void set_my_provider(my_Rpc_provider* provider);

public:
    bool stop_scheduler = false;
private:
    std::unordered_map<int,my_Coroutine*> my_socket_to_coroutine_map;
    std::mutex my_socket_to_coroutine_mutex;

    int my_thread_num;
    int my_free_coroutine_num;
    int my_busy_coroutine_num;
    int my_close_coroutine_num;

    //协程池
    std::list<my_Coroutine*> my_free_coroutine_list;
    std::list<my_Coroutine*> my_busy_coroutine_list;
    std::list<my_Coroutine*> my_close_coroutine_list;

    //线程池
    std::vector<std::thread*> my_thread_vector;

    std::mutex my_free_coroutine_mutex;
    std::mutex my_busy_coroutine_mutex;
    std::mutex my_close_coroutine_mutex;

    std::condition_variable my_free_coroutine_cv;
    std::condition_variable my_busy_coroutine_cv;
    std::condition_variable my_close_coroutine_cv;
private:
    //任务
    std::list<my_Task_node*> my_task_list;
    std::mutex my_task_mutex;
    std::condition_variable my_task_cv;
    int my_task_num ;
private:
    bool is_coroutine_pool = false;

    my_Timer *my_timer;
    my_Sql_thread_pool *my_sql_thread_pool;
    my_Log *my_log;
    my_Rpc_provider* my_provider;

    int my_epoll_fd;
    int my_listen_fd;
    int my_pipe_fd[2];
    
    //定时器时间
    bool my_timeout = false;
private:
    static const int READ_BUFFER_SIZE = 250;
};

#endif