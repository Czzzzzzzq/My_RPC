#ifndef MY_TASK_NODE_H
#define MY_TASK_NODE_H

#include <sys/types.h>
#include <sys/socket.h>
#include <string>
#include <unistd.h>

class my_coroutine;

class my_Task_node
{
public:
    enum task_status
    {
        READ=0,
        WORK,
        WRITE,
        CLOSE,
        ERROR,
        WAIT,
        COMPLETE
    };
    enum error_status{
        NO_ERROR=0,
        SOCKET_ERROR,
        READ_ERROR,
        READ_AGAIN,
        ANALYSIS_ERROR,
        WORK_ERROR,
        WRITE_ERROR,
        CLOSE_ERROR,
        TIMEOUT_ERROR
    };
public:
    my_Task_node();
    ~my_Task_node();
    void reset();

    task_status get_status();
    void set_status(task_status status);

    error_status get_error_status();
    void set_error_status(error_status status);

    int get_socket();
    void set_socket(int socket);

    std::string get_read_buffer();
    void set_read_buffer(std::string read_buffer);
    std::string get_write_buffer();
    void set_write_buffer(std::string write_buffer);

    int get_listen_end_time();
    int get_run_end_time();
    void set_time(int listen_end_time,int run_end_time);
private:
    task_status my_task_status;
    error_status my_task_error_status;
    
    int my_task_socket;

    std::string my_task_read_buffer;
    std::string my_task_write_buffer;

    int my_task_listen_end_time;
    int my_task_run_end_time;
};


#endif
