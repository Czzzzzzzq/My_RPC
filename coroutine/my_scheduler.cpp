#include "my_scheduler.h"
#include "my_rpc_provider.h"


my_Scheduler::my_Scheduler()
{
    my_thread_num = 1;
    my_free_coroutine_num = 0;
    my_busy_coroutine_num = 0;
    my_task_num = 0;
    my_close_coroutine_num = 0;

    stop_scheduler = false;
    is_coroutine_pool = true;

    my_timer = my_Timer::get_instance();
    my_log = my_Log::get_instance();
    my_sql_thread_pool = my_Sql_thread_pool::get_instance();
} 

my_Scheduler::~my_Scheduler()
{
    stop_my_Scheduler();
}
void my_Scheduler::stop_my_Scheduler()
{
    stop_scheduler = true;
    my_log->log_close = true;
    while(!my_free_coroutine_list.empty())
    {
        my_Coroutine* coroutine = my_free_coroutine_list.front();
        my_free_coroutine_list.pop_front();
        if(coroutine != nullptr){
            delete coroutine;
        }
    }
    while(!my_busy_coroutine_list.empty())
    {
        my_Coroutine* coroutine = my_busy_coroutine_list.front();
        my_busy_coroutine_list.pop_front();
        if(coroutine != nullptr){
            delete coroutine;
        }
    }
    while(!my_task_list.empty())
    {
        my_Task_node* task = my_task_list.front();
        my_task_list.pop_front();
        if(task == nullptr){
            delete task;
        }
    }
}


my_Scheduler* my_Scheduler::get_instance()
{
    static my_Scheduler scheduler;
    return &scheduler;
}

void my_Scheduler::init(int epoll_fd,int listen_fd,int* pipe_fd,int sql_port, const char *sql_user,const char *sql_pwd, 
    const char *db_name,int thread_pool_num,int conn_pool_num,int coroutine_num,int listen_timeout,int run_timeout,bool log_close)
{
    my_thread_num = thread_pool_num;
    my_free_coroutine_num = coroutine_num;
    my_busy_coroutine_num = 0;

    my_epoll_fd = epoll_fd;
    my_listen_fd = listen_fd;
    my_pipe_fd[0] = pipe_fd[0];
    my_pipe_fd[1] = pipe_fd[1];

    my_log->init("./web_log/", "log", 5000,log_close);
    my_timer->init(listen_timeout,run_timeout,epoll_fd);
    my_sql_thread_pool->init(string("localhost"),sql_user,sql_pwd,db_name, sql_port,conn_pool_num);

    my_timer->set_pipe_fd(pipe_fd);
    my_timer->addsig(SIGALRM, my_Timer::sig_handler);
    my_timer->addsig(SIGTERM, my_Timer::sig_handler);

    if(is_coroutine_pool == true)
    {
        for(int i = 0; i < my_free_coroutine_num; i++)
        {
            my_Coroutine* coroutine = new my_Coroutine();
            coroutine->init(0, [this](my_Coroutine* coroutine) {
                Coroutine_run(coroutine, this);
            });
            my_free_coroutine_list.push_back(coroutine);
        }
    }

    for(int i = 0; i < my_thread_num; i++)
    {
        std::thread* thread = new std::thread(&my_Scheduler::run, this);
        my_thread_vector.push_back(thread);
        thread->detach();
    }
}

void my_Scheduler::Coroutine_run(my_Coroutine* my_coroutine,my_Scheduler* my_scheduler)
{
    //1.读取数据
    if(my_coroutine->get_task()->get_status() == my_Task_node::READ)
    {
        my_scheduler->read_task(my_coroutine);
    }

    //2.分析数据/调用函数
    if(my_coroutine->get_task()->get_status() == my_Task_node::WORK)
    {
        my_scheduler->work_task(my_coroutine);
    }

    //3.返回数据
    if(my_coroutine->get_task()->get_status() == my_Task_node::WRITE)
    {
        my_scheduler->write_task(my_coroutine);           
    }

    if(my_coroutine->get_task()->get_status() == my_Task_node::ERROR)
    {
        my_scheduler->error_task(my_coroutine);
    }else{
        my_scheduler->close_task(my_coroutine);
    }
    //cout << this_thread::get_id() << endl;
    //cout << my_coroutine->get_id() << endl;


}

void my_Scheduler::add_task(int socket)
{
    bool is_new_task;

    {
        std::lock_guard<std::mutex> lock_socket_to_coroutine(my_socket_to_coroutine_mutex);
        auto it = my_socket_to_coroutine_map.find(socket);
        if(it == my_socket_to_coroutine_map.end())
        {
            is_new_task = true;
        }else{
            is_new_task = it->second == nullptr;
        }
    }

    if(is_new_task) //新创建的任务
    {
        my_Task_node* task = new my_Task_node();
        task->set_status(my_Task_node::READ);
        task->set_socket(socket);
        my_timer->add_task_timer(task);

        addfd(task->get_socket(),task->get_status()); //添加到epoll事件表中

        std::unique_lock<std::mutex> lock_task(my_task_mutex); //新添加的任务需要绑定协程 tasklist中是等待绑定的任务
        my_task_list.push_back(task);
        my_task_num++;

        lock_task.unlock();
        my_task_cv.notify_one();

    }else{  //非新创建的任务，将任务对应的协程重新加入工作队列
        my_Coroutine* coroutine;
        {
            std::lock_guard<std::mutex> lock_socket_to_coroutine(my_socket_to_coroutine_mutex);
            coroutine = my_socket_to_coroutine_map[socket];
        }
        return_busy_coroutine(coroutine);
    }
}
void my_Scheduler::add_task(my_Task_node* task)
{
    std::unique_lock<std::mutex> lock_task(my_task_mutex);
    my_task_list.push_back(task);
    my_task_num++;
    lock_task.unlock();
    my_task_cv.notify_one();
}

my_Task_node* my_Scheduler::get_task()
{
    std::unique_lock<std::mutex> lock_task(my_task_mutex);
    if(my_task_num == 0)
    {
        lock_task.unlock();
        return nullptr;
    }
    my_Task_node* task = my_task_list.front();
    my_task_list.pop_front();
    my_task_num--;
    lock_task.unlock();
    my_task_cv.notify_one();

    return task;
}

void my_Scheduler::read_task(my_Coroutine* coroutine){
    my_Task_node* task = coroutine->get_task();
    
    int read_bytes = 0;
    int buffer_ptr = 0;

    char read_buf[READ_BUFFER_SIZE];
    memset(read_buf,0,READ_BUFFER_SIZE);

    if (task->get_socket() < 0) {
        //LOG_ERROR("read task Invalid socket fd: %d", task->get_socket());
        task->set_status(my_Task_node::ERROR);
        task->set_error_status(my_Task_node::SOCKET_ERROR);
        return;
    }

    //LOG_INFO("Start to read data from socket %d", task->get_socket());

    while(true)
    {
        read_bytes = recv(task->get_socket(), read_buf + buffer_ptr,READ_BUFFER_SIZE - buffer_ptr, 0);
        if (read_bytes == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK){
                modfd(task->get_socket(),my_Task_node::READ); //修改为可读事件
                coroutine->yield(); //让出控制权
                continue;
            }
            else {
                //LOG_ERROR("recv error: %d", errno);
                task->set_status(my_Task_node::ERROR);
                task->set_error_status(my_Task_node::READ_ERROR);
                break;
            }
        }
        else if (read_bytes == 0) {
            //LOG_INFO("Connection closed by peer, socket: %d", task->get_socket());
            task->set_status(my_Task_node::ERROR);
            task->set_error_status(my_Task_node::READ_ERROR);
            break;
        }else{
            buffer_ptr += read_bytes;
            if(buffer_ptr >= READ_BUFFER_SIZE){
                buffer_ptr = READ_BUFFER_SIZE - 1;
                //LOG_ERROR("Out of read buffer, socket: %d", task->get_socket());
            }
            break;
        }
    }

    task->set_read_buffer(std::string(read_buf,buffer_ptr)); // += 可多次读取
    task->set_status(my_Task_node::WORK);

    //LOG_INFO("Success to read data from socket %d, read bytes: %d", task->get_socket(), buffer_ptr);
}

void my_Scheduler::work_task(my_Coroutine* coroutine){

    //LOG_INFO("start to work task from socket %d", coroutine->get_task()->get_socket());

    my_Task_node* task = coroutine->get_task();
    
    string str_in = task->get_read_buffer();

    string str_out = my_provider->analysis_write(str_in);

    if(str_out == "error"){
        task->set_status(my_Task_node::ERROR);
        task->set_error_status(my_Task_node::WORK_ERROR);
        return;
    }
    
    task->set_write_buffer(str_out);
    task->set_status(my_Task_node::WRITE);
    //LOG_INFO("Success to work task from socket %d", task->get_socket());
}

void my_Scheduler::write_task(my_Coroutine* coroutine) {
    my_Task_node* task = coroutine->get_task();

    const std::string& response = task->get_write_buffer();
    const char* data = response.c_str();
    int total_bytes = response.length();
    int bytes_sent = 0;

    if (total_bytes == 0) {
        //LOG_ERROR("send error: buffer is null");
        task->set_status(my_Task_node::ERROR);
        task->set_error_status(my_Task_node::WRITE_ERROR);
        return;
    }
    //LOG_INFO("Start to send response to socket %d, total bytes: %zu", task->get_socket(), total_bytes);
    int ret = 0;
    while (bytes_sent < total_bytes) {

        if (task->get_socket() == -1) {
            task->set_status(my_Task_node::ERROR);
            task->set_error_status(my_Task_node::WRITE_ERROR);
            break;
        }

        ret = send(task->get_socket(),data + bytes_sent, total_bytes - bytes_sent, 0);
        
        if (ret < 0) {
            switch (errno) {
                case EAGAIN:
                    // 暂时无法写入，修改为可写事件并让出协程
                    //LOG_DEBUG("Socket %d write buffer full, waiting for space", task->get_socket());
                    //cout << "Socket " << task->get_socket() << " write buffer full, waiting for space" << endl;
                    modfd(task->get_socket(), my_Task_node::WRITE);
                    coroutine->yield();
                    continue;
                case EBADF:
                case ECONNRESET:
                case EPIPE:
                    // 连接已断开，关闭套接字
                    //LOG_ERROR("Connection error on socket %d: %s", task->get_socket(), strerror(errno));
                    task->set_status(my_Task_node::ERROR);
                    task->set_error_status(my_Task_node::WRITE_ERROR);
                    break;
                default:
                    //LOG_ERROR("Unexpected error while sending data on socket %d: %s", 
                             //task->get_socket(), strerror(errno));
                    task->set_status(my_Task_node::ERROR);
                    task->set_error_status(my_Task_node::WRITE_ERROR);
                    break;
            }
        } 
        else if (ret == 0) {
            // 连接已关闭
            //LOG_INFO("Connection closed by peer while sending data on socket %d", task->get_socket());
            task->set_status(my_Task_node::ERROR);
            task->set_error_status(my_Task_node::WRITE_ERROR);
            return;
        } 
        else {
            // 成功发送部分数据
            bytes_sent += ret;  
            //LOG_INFO("Sent %d bytes, total %zu/%zu on socket %d",     
                     //ret, bytes_sent, total_bytes, task->get_socket());
        }
    }

    if(bytes_sent == total_bytes){
        task->set_status(my_Task_node::CLOSE);
        //LOG_INFO("Success to send entire response to socket %d", task->get_socket());
    }
}

void my_Scheduler::close_task(my_Coroutine* coroutine){
    my_Task_node* task = coroutine->get_task();
    coroutine->reset_task();
    {
        std::lock_guard<std::mutex> lock_socket_to_coroutine(my_socket_to_coroutine_mutex);
        my_socket_to_coroutine_map[task->get_socket()] = nullptr;
        if(task->get_socket() != -1){
            close(task->get_socket());
            task->set_socket(-1);
        }
    }
    //LOG_INFO("Success return free coroutine %d",coroutine->get_id());
}

void my_Scheduler::error_task(my_Coroutine* coroutine){
    my_Task_node* task = coroutine->get_task();
    coroutine->reset_task();

    {
        std::lock_guard<std::mutex> lock_socket_to_coroutine(my_socket_to_coroutine_mutex);
        my_socket_to_coroutine_map[task->get_socket()] = nullptr;
        if(task->get_socket() != -1){
            close(task->get_socket());
            task->set_socket(-1);
        }
    }

    switch (task->get_error_status())
    {
    case my_Task_node::READ_ERROR:
        //LOG_ERROR("Cortoune Id : %d,read error",coroutine->get_id());
        break;
    case my_Task_node::WORK_ERROR:
        //LOG_ERROR("Cortoune Id : %d,work error",coroutine->get_id());
        break;    

    case my_Task_node::WRITE_ERROR:
        //LOG_ERROR("Cortoune Id : %d,write error",coroutine->get_id());
        break;    

    case my_Task_node::SOCKET_ERROR:
        //LOG_ERROR("Cortoune Id : %d,socket error",coroutine->get_id());
        break;    
    default:
        //LOG_ERROR("Cortoune Id : %d,unkonw error",coroutine->get_id());
        break;
    }       

    //LOG_INFO("Success return free coroutine %d",coroutine->get_id());

}

my_Coroutine* my_Scheduler::get_free_coroutine()
{
    std::unique_lock<std::mutex> lock_free_coroutine(my_free_coroutine_mutex);
    if(my_free_coroutine_num == 0){
        lock_free_coroutine.unlock();
        return nullptr;
    }
    my_Coroutine* coroutine = my_free_coroutine_list.front();
    my_free_coroutine_list.pop_front();
    my_free_coroutine_num--;
    lock_free_coroutine.unlock();
    my_free_coroutine_cv.notify_one();
    return coroutine;
}

void my_Scheduler::return_free_coroutine(my_Coroutine* coroutine)
{
    std::unique_lock<std::mutex> lock_free_coroutine(my_free_coroutine_mutex);
    my_free_coroutine_list.push_back(coroutine);
    my_free_coroutine_num++;
    lock_free_coroutine.unlock();
    my_free_coroutine_cv.notify_one();
}

my_Coroutine* my_Scheduler::get_busy_coroutine(){
    std::unique_lock<std::mutex> lock_busy_coroutine(my_busy_coroutine_mutex);
    if(my_busy_coroutine_num == 0){
        lock_busy_coroutine.unlock();
        return nullptr;
    }
    my_Coroutine* coroutine = my_busy_coroutine_list.front();
    my_busy_coroutine_list.pop_front();
    my_busy_coroutine_num--;
    lock_busy_coroutine.unlock();
    my_busy_coroutine_cv.notify_one();
    return coroutine;
}
void my_Scheduler::return_busy_coroutine(my_Coroutine* coroutine){
    std::unique_lock<std::mutex> lock_busy_coroutine(my_busy_coroutine_mutex);
    my_busy_coroutine_list.push_back(coroutine);
    my_busy_coroutine_num++;
    lock_busy_coroutine.unlock();
    my_busy_coroutine_cv.notify_one();
}

my_Coroutine* my_Scheduler::get_close_coroutine(){
    std::unique_lock<std::mutex> lock_close_coroutine(my_close_coroutine_mutex);
    if(my_close_coroutine_num == 0){
        lock_close_coroutine.unlock();
        return nullptr;
    }
    my_Coroutine* coroutine = my_close_coroutine_list.front();
    my_close_coroutine_list.pop_front();
    my_close_coroutine_num--;
    lock_close_coroutine.unlock();
    my_close_coroutine_cv.notify_one();
    return coroutine;
}
void my_Scheduler::return_close_coroutine(my_Coroutine* coroutine){
    std::unique_lock<std::mutex> lock_close_coroutine(my_close_coroutine_mutex);
    my_close_coroutine_list.push_back(coroutine);
    my_close_coroutine_num++;
    lock_close_coroutine.unlock();
    my_close_coroutine_cv.notify_one();
}


void my_Scheduler::run()
{
    my_Coroutine* main_coroutine = new my_Coroutine();
    main_coroutine->init(0,nullptr);
    main_coroutine->set_main(main_coroutine);

    my_Coroutine* idle_coroutine = new my_Coroutine();
    idle_coroutine->init(0,std::bind(&my_Scheduler::idle,this));
    idle_coroutine->set_idle(idle_coroutine);   

    while(!stop_scheduler)
    {
        //定时器处理
        if(my_timeout){
            my_timeout = false;
            my_timer->tick();
        }

        // 尝试从繁忙队列获取协程并处理，并将处理完的协程返回关闭队列
        my_Coroutine* coroutine_run = nullptr;
        if (my_busy_coroutine_num > 0) {
            coroutine_run = get_busy_coroutine();
        }
        if (coroutine_run) {
            coroutine_run->resume();
            if(coroutine_run->get_task() == nullptr){
                return_close_coroutine(coroutine_run);
            }
            continue;
        }

        // 处理关闭协程
        while(my_close_coroutine_num > 0){
            my_Coroutine* coroutine = get_close_coroutine();
            if(coroutine){
                if(is_coroutine_pool == true){
                    coroutine->reset([this](my_Coroutine* coroutine) { Coroutine_run(coroutine, this); });
                    return_free_coroutine(coroutine);
                }else{
                    delete coroutine;
                }
            }
        }        

        // 等待任务队列有任务
        if(my_task_num == 0 && !stop_scheduler){
            sleep(0.5);
        }
        idle_coroutine->resume();
    }   
    
    delete idle_coroutine;
    delete main_coroutine;
}

void my_Scheduler::idle()
{
    epoll_event events[10];
    while(!stop_scheduler)
    {   
        // 等待事件
        int event_num = epoll_wait(my_epoll_fd, events, 10, 0);
        for(int i = 0; i < event_num; i++){
            int socket_fd = events[i].data.fd;
            if(socket_fd == my_listen_fd){
                struct sockaddr_in client_addr;
                socklen_t client_addr_len = sizeof(client_addr);
                int accept_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &client_addr_len);
                if(accept_fd == -1){
                    //LOG_ERROR("idle Invalid socket fd: %d", accept_fd);
                    continue;
                }
                //添加新任务
                add_task(accept_fd);
            }
            else if((socket_fd == my_pipe_fd[0])) {
                //处理信号
                dealwithsignal();
            }else{  
                //添加旧任务
                add_task(socket_fd);
            }
        }
        //将接受到的任务和空闲协程绑定
        while(my_task_num > 0){
            // 获取需要处理的任务
            my_Task_node* task = nullptr;
            if (my_task_num > 0) {
                task = get_task();
            }
            if(!task){
                break;
            }

            my_Coroutine* coroutine = nullptr;
            // 获取空闲协程
            if(is_coroutine_pool == true){
                if (my_free_coroutine_num > 0) {
                    coroutine = get_free_coroutine();
                }
                // 无空闲协程
                if (!coroutine) {
                    add_task(task);
                    break;
                }
            }else{
                coroutine = new my_Coroutine();
                coroutine->init(0, [this](my_Coroutine* coroutine) {
                    Coroutine_run(coroutine, this);
                });
            }

            // 绑定任务
            {   
                std::lock_guard<std::mutex> lock_socket_to_coroutine(my_socket_to_coroutine_mutex);
                my_socket_to_coroutine_map[task->get_socket()] = coroutine;
                coroutine->set_task(task);
            }
            return_busy_coroutine(coroutine);

            sleep(0.5);

        }

        my_Coroutine::get_current()->yield();
    }
}

//将内核事件表注册事件
void my_Scheduler::addfd(int fd,my_Task_node::task_status status)
{
    epoll_event event = {};
    event.data.fd = fd;
    if(status == my_Task_node::READ)
    {
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    }
    else if(status == my_Task_node::WRITE)
    {
        event.events = EPOLLOUT | EPOLLET | EPOLLRDHUP;
    }
    epoll_ctl(my_epoll_fd, EPOLL_CTL_ADD, fd, &event);
    fcntl(fd, F_SETFL, O_NONBLOCK);
}


void my_Scheduler::modfd(int fd,my_Task_node::task_status status)
{
    epoll_event event = {};
    event.data.fd = fd;
    if(status == my_Task_node::READ)
    {
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    }
    else if(status == my_Task_node::WRITE)
    {
        event.events = EPOLLOUT | EPOLLET | EPOLLRDHUP;
    }
    epoll_ctl(my_epoll_fd, EPOLL_CTL_MOD, fd, &event);
    fcntl(fd, F_SETFL, O_NONBLOCK);
}

void my_Scheduler::dealwithsignal(){
    cout << "dealwithtime" << endl;
    
    int ret = 0;
    char signals[1024];
    ret = recv(my_pipe_fd[0], signals, sizeof(signals), 0);
    for (int i = 0; i < ret; ++i)
    {
        switch (signals[i])
        {
            case SIGALRM:   
            {
                my_timeout = true;
                break;
            }
            case SIGTERM:
            case SIGINT:
            {
                //cout << "SIGTERM or SIGINT" << endl;
                stop_scheduler = true;
                stop_my_Scheduler();
                break;
            }
        }
    }
}

void my_Scheduler::set_my_provider(my_Rpc_provider* provider){
    my_provider = provider;
}
