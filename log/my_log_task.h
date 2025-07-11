#ifndef MY_LOG_TASK_H
#define MY_LOG_TASK_H

#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <iostream>

using namespace std;

template <typename T>
class my_log_task
{
public:
    //初始化
    my_log_task();
    my_log_task(int max_size); 
    ~my_log_task();

    //维护
    int log_task_size();
    int log_task_capacity();
    void log_task_resize(int size);

    //操作
    bool if_log_task_empty();
    bool if_log_task_full();
    void log_task_push(T task);
    void log_task_pop(T& task);
private:
    vector<T> task_queue;
    int log_task_max_size;
    int log_task_front;
    int log_task_back;

    mutex log_task_mutex;
    condition_variable log_task_cond;

    bool shutdown = false;
};





#endif