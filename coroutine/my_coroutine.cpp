#include "my_coroutine.h"

static thread_local my_Coroutine* my_coroutine_current = nullptr;
static thread_local my_Coroutine* my_coroutine_main = nullptr;
static thread_local my_Coroutine* my_coroutine_idle = nullptr;

static int coroutine_id = 0;
static int coroutine_count = 0;

my_Coroutine::my_Coroutine()
{
    my_coroutine_id = coroutine_id;
    coroutine_id++;
    coroutine_count++;
}

my_Coroutine::~my_Coroutine()
{
    coroutine_count--;
    if(my_coroutine_context != nullptr)
    {
        delete my_coroutine_context;
    }
    if(my_coroutine_stack != nullptr)
    {
        free(my_coroutine_stack);
    }
}

void my_Coroutine::init(int stack_size, std::function<void(my_Coroutine*)> func)
{
    
    my_coroutine_func = func;

    if(stack_size == 0)
    {
        my_coroutine_stack_size = 1024 * 1024 * 4;
    }else{
        my_coroutine_stack_size = stack_size;
    }

    my_coroutine_stack = new char[my_coroutine_stack_size];

    memset(my_coroutine_stack, 0, my_coroutine_stack_size);

    my_coroutine_context = new ucontext_t();

    getcontext(my_coroutine_context);
    my_coroutine_context->uc_link = nullptr;
    my_coroutine_context->uc_stack.ss_sp = my_coroutine_stack;
    my_coroutine_context->uc_stack.ss_size = my_coroutine_stack_size;
    my_coroutine_context->uc_stack.ss_flags = 0;
    makecontext(my_coroutine_context, &my_Coroutine::run, 0);
}

void my_Coroutine::run()
{
    my_Coroutine* coroutine_run = get_current();
    coroutine_run->set_status(RUNNING); 

    if(coroutine_run->get_func() != nullptr)
    {
        coroutine_run->get_func()(coroutine_run);
    }

    coroutine_run->yield();
}

void my_Coroutine::reset(std::function<void(my_Coroutine*)> func)
{
    // 重置协程状态
    my_coroutine_status = READY;
    my_coroutine_func = func;
    my_task = nullptr;

    // 设置执行函数
    getcontext(my_coroutine_context);
    my_coroutine_context->uc_link = nullptr;
    my_coroutine_context->uc_stack.ss_sp = my_coroutine_stack;
    my_coroutine_context->uc_stack.ss_size = my_coroutine_stack_size;
    my_coroutine_context->uc_stack.ss_flags = 0;
    makecontext(my_coroutine_context, &my_Coroutine::run, 0);
}

void my_Coroutine::yield()
{
    if (this->get_task() == nullptr)
    {
        my_coroutine_status = COMPLETE;
    }else{
        my_coroutine_status = YIELD;
    }

    my_Coroutine* coroutine = get_current();

    set_current(my_coroutine_main);
    swapcontext(coroutine->get_context(), my_coroutine_main->get_context());
}

void my_Coroutine::resume()
{
    my_coroutine_status = RUNNING;
    
    set_current(this);
    swapcontext(my_coroutine_main->get_context(), my_coroutine_current->get_context());
}


void my_Coroutine::set_main(my_Coroutine* coroutine)
{
    my_coroutine_main = coroutine;
}

void my_Coroutine::set_idle(my_Coroutine* coroutine)
{
    my_coroutine_idle = coroutine;
}
void my_Coroutine::set_current(my_Coroutine* coroutine){
    my_coroutine_current = coroutine;
}
my_Coroutine* my_Coroutine::get_current()
{
    return my_coroutine_current;
}


//常规
int my_Coroutine::get_id()
{
    return my_coroutine_id;
}

void my_Coroutine::set_id(int id)
{
    my_coroutine_id = id;
}

my_Coroutine::CoroutineStatus my_Coroutine::get_status()
{
    return my_coroutine_status;
}

void my_Coroutine::set_status(CoroutineStatus status)
{
    my_coroutine_status = status;
}

ucontext_t* my_Coroutine::get_context()
{
    return my_coroutine_context;
}   

void my_Coroutine::set_context(ucontext_t context)
{
    *my_coroutine_context = context;
}

std::function<void(my_Coroutine*)> my_Coroutine::get_func()
{
    return my_coroutine_func;
}
void my_Coroutine::set_func(std::function<void(my_Coroutine*)> func)
{
    my_coroutine_func = func;
}

my_Task_node* my_Coroutine::get_task()
{
    return my_task;
}

void my_Coroutine::set_task(my_Task_node* task)
{
    my_task = task;
}

void my_Coroutine::reset_task()
{
    my_task = nullptr;
}
