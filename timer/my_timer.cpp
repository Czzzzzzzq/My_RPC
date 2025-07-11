#include <my_timer.h>

my_Timer::my_Timer()
{
    set_timeout(10,30);
}

void my_Timer::set_timeout(int listen_timeout,int run_timeout){
    max_listen_time = listen_timeout;
    max_run_time = run_timeout;
}

void my_Timer::init(int listen_timeout,int run_timeout,int epoll_fd){
    max_listen_time = listen_timeout;
    max_run_time = run_timeout;
    m_epoll_fd = epoll_fd;
}

my_Timer::~my_Timer()
{
    m_sorted_timer.clear();
}

my_Timer* my_Timer::get_instance(){
    static my_Timer timer;
    return &timer;
}

void my_Timer::add_task_timer(my_Task_node* task){
    if(task == nullptr){
        return;
    }
    adjust_timer(task);
    add_timer(task);
}


void my_Timer::add_timer(my_Task_node* task){
    m_sorted_timer.push_back(task);
}

void my_Timer::del_timer(my_Task_node* task){
    if(task->get_socket() != -1){
        close(task->get_socket());
        task->set_socket(-1);
    }
}

void my_Timer::adjust_timer(my_Task_node* task){
    if(task == nullptr){
        return;
    }
    time_t cur = time(NULL);
    task->set_time(cur + max_listen_time,cur + max_run_time);
}

int my_Timer::tick(){
    time_t cur = time(NULL);
    while(!m_sorted_timer.empty()){
        my_Task_node* task = m_sorted_timer.front();
        if(task == nullptr){
            break;
        }
        if(task->get_listen_end_time() >= cur && task->get_run_end_time() >= cur){
            break;
        }else{
            m_sorted_timer.pop_front();
            if(task->get_status() == my_Task_node::CLOSE){
                LOG_INFO("Sucess Found close task:%d",task->get_socket());
                del_timer(task);
                delete task;
            }
            else if(task->get_status() == my_Task_node::WAIT){
                LOG_INFO("Sucess Found wait task:%d",task->get_socket());
                task->set_time(cur + max_listen_time,cur + max_run_time);
                m_sorted_timer.push_back(task);           
                task->set_status(my_Task_node::CLOSE);
            }else if(task->get_status() == my_Task_node::ERROR){
                LOG_INFO("Sucess Found error task:%d",task->get_socket());
                del_timer(task);
                delete task;
            }
        }
    }
    if (!m_sorted_timer.empty() && m_sorted_timer.front()->get_listen_end_time() - cur > 0){
        return m_sorted_timer.front()->get_listen_end_time() - cur;
    }else{
        return max_listen_time;
    }
}



//信号处理函数
void my_Timer::sig_handler(int sig)
{
    int msg = sig;
    send(get_instance()->u_pipefd[1], (char *)&msg, 1, 0);
}

//设置信号函数
void my_Timer::addsig(int sig, void(handler)(int))
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    sigfillset(&sa.sa_mask);
    sigaction(sig, &sa, NULL);
}

//设置epoll事件
void my_Timer::set_epoll_fd(int epoll_fd){
    m_epoll_fd = epoll_fd;
}

void my_Timer::set_pipe_fd(int* pipe_fd){
    u_pipefd[0] = pipe_fd[0];
    u_pipefd[1] = pipe_fd[1];
}
