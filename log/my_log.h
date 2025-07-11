#ifndef MY_LOG_H
#define MY_LOG_H

#include <iostream>
#include <string>
#include <cstring>
#include <my_log_task.h>
#include <unistd.h>

#include <chrono>
#include <ctime>
#include <cstdarg>
#include <thread>
#include <mutex>
#include <condition_variable>


using namespace std;

class my_Log
{
public:
    my_Log();
    ~my_Log();

    static my_Log *get_instance();
    
    void thread_log_write();

    bool init(const char* path, const char* name, int max_log_size = 1024,bool close = false);
    void write_log(int type,const char* format, ...);
    void flush();

    bool log_close = false;
    FILE* file_ptr;
    int max_log_size;
private:
    my_log_task<string>* log_task_queue;
    std::mutex log_mutex;

    condition_variable log_cond;

    char log_name[40];
    char log_path[40];
    char log_all_name[100];

    long long count;
};


#define LOG_DEBUG(format,...) if(false == my_Log::get_instance()->log_close) {my_Log::get_instance()->write_log(0, format, ##__VA_ARGS__);}
#define LOG_INFO(format,...) if(false == my_Log::get_instance()->log_close) {my_Log::get_instance()->write_log(1, format, ##__VA_ARGS__); }
#define LOG_WARN(format,...) if(false == my_Log::get_instance()->log_close) {my_Log::get_instance()->write_log(2, format, ##__VA_ARGS__); }
#define LOG_ERROR(format,...) if(false == my_Log::get_instance()->log_close) {my_Log::get_instance()->write_log(3, format, ##__VA_ARGS__);}


#endif