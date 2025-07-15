#ifndef MY_COROUTINE_H
#define MY_COROUTINE_H

#include "my_task_node.h"
#include "my_coroutine.h"

#include <ucontext.h>
#include <functional>
#include <time.h>
#include <iostream>
#include <cstring>

class my_Coroutine
{
public:
    enum CoroutineStatus
    {
        READY,
        RUNNING,
        YIELD,
        COMPLETE
    };
public:
    my_Coroutine();
    ~my_Coroutine();

    void init(int stack_size, std::function<void(my_Coroutine*)> func);
    
    static void run();

    void yield();
    void resume();
    void set_main(my_Coroutine* coroutine);
    void set_idle(my_Coroutine* coroutine);
    void reset(std::function<void(my_Coroutine*)> func);

    void set_current(my_Coroutine* coroutine);
    static my_Coroutine* get_current();

    int get_id();
    void set_id(int id);

    CoroutineStatus get_status();
    void set_status(CoroutineStatus status);

    ucontext_t* get_context();
    void set_context(ucontext_t context);

    std::function<void(my_Coroutine*)> get_func();
    void set_func(std::function<void(my_Coroutine*)> func);

    my_Task_node* get_task();
    void set_task(my_Task_node* task);
    void reset_task();

private:
    int my_coroutine_id = 0;
    CoroutineStatus my_coroutine_status = READY;

    ucontext_t* my_coroutine_context = nullptr;
    char* my_coroutine_stack = nullptr;
    int my_coroutine_stack_size;
    std::function<void(my_Coroutine*)> my_coroutine_func;

    my_Task_node* my_task = nullptr;
};

#endif