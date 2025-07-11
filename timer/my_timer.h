#ifndef MY_TIMER_H
#define MY_TIMER_H

#include <my_log.h>
#include <my_task_node.h>

#include <time.h>
#include <list>
#include <unordered_map>
#include <vector>

#include <sys/epoll.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

class my_Timer
{
public:
    my_Timer();
    ~my_Timer();

    void set_timeout(int listen_timeout,int run_timeout);
    void init(int listen_timeout,int run_timeout,int epoll_fd);

    static my_Timer* get_instance();

    void add_task_timer(my_Task_node* task);

    void add_timer(my_Task_node* task);
    void del_timer(my_Task_node* task);
    void adjust_timer(my_Task_node* task);
    int tick();

    void addsig(int sig, void(handler)(int));
    static void sig_handler(int sig);

    void set_epoll_fd(int epoll_fd);
    void set_pipe_fd(int* pipe_fd);
private:
    int u_pipefd[2];

    int max_listen_time;
    int max_run_time;

    int m_epoll_fd = -1;
    std::list<my_Task_node*> m_sorted_timer;
};


#endif

