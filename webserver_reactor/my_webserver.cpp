#include "my_webserver.h"
#include "my_rpc_provider.h"

my_Webserver_reactor::my_Webserver_reactor()
{
    my_port = 8080;
    my_sql_port = 3306;
    my_sql_user = "root";
    my_sql_pwd = "123456789";
    my_db_name = "test";

    stop_my_server = false;
}

my_Webserver_reactor::~my_Webserver_reactor()
{
    stop_my_Webserver_reactor();
}


void my_Webserver_reactor::stop_my_Webserver_reactor(){
    close(my_epoll_fd);
    close(my_listen_fd);
    close(my_pipefd[0]);
    close(my_pipefd[1]);
}

my_Webserver_reactor* my_Webserver_reactor::get_instance(){
    static my_Webserver_reactor instance;
    return &instance;
}

void my_Webserver_reactor::init_my_Webserver_reactor(int port,int sql_port, const char *sql_user,const char *sql_pwd, const char *db_name,int thread_pool_num,int conn_pool_num,int coroutine_num,int listen_timeout,int run_timeout,bool log_close)
{
    my_port = port;
    my_sql_port = sql_port;
    my_sql_user = sql_user;
    my_sql_pwd = sql_pwd;
    my_db_name = db_name;

    my_thread_pool_num = thread_pool_num;
    my_conn_pool_num = conn_pool_num;
    my_coroutine_num = coroutine_num;
    
    my_run_timeout = run_timeout;
    my_listen_timeout = listen_timeout;

    my_log_close = log_close;
}

void my_Webserver_reactor::start_my_Webserver_reactor(){
    int ret = 0;
    my_listen_fd = socket(PF_INET, SOCK_STREAM, 0);
    assert(my_listen_fd >= 0 && "listen fd error1");

    struct linger tmp = {1, 1};
    int flag = 1;//复用端口    // 解决 "address already in use" 错误
    setsockopt(my_listen_fd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    setsockopt(my_listen_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    assert(my_listen_fd >= 0 && "listen fd error2");

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr)); //将address清零
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(my_port);

    ret = bind(my_listen_fd, (sockaddr *)&addr, sizeof(addr));
    assert(ret >= 0 && "bind error");
    ret = listen(my_listen_fd, 1024);
    assert(ret >= 0 && "listen error");
    fcntl(my_listen_fd, F_SETFL, O_NONBLOCK);

    my_epoll_fd = epoll_create(1);
    assert(my_epoll_fd != -1 && "epoll fd error1");

    epoll_event event_listen = {};
    event_listen.data.fd = my_listen_fd;
    event_listen.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    event_listen.events = EPOLLIN | EPOLLRDHUP;
    epoll_ctl(my_epoll_fd, EPOLL_CTL_ADD, my_listen_fd, &event_listen);

    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, my_pipefd);//创建一对socket，用于进程间通信
    assert(ret != -1);
    fcntl(my_pipefd[1], F_SETFL, O_NONBLOCK);

    epoll_event event_pipe = {};
    event_pipe.data.fd = my_pipefd[0];
    event_pipe.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    epoll_ctl(my_epoll_fd, EPOLL_CTL_ADD, my_pipefd[0], &event_pipe);

    my_scheduler = new my_Scheduler();
    my_scheduler->init(my_epoll_fd,my_listen_fd,my_pipefd,my_sql_port,my_sql_user,my_sql_pwd,my_db_name,my_thread_pool_num,my_conn_pool_num,my_coroutine_num,my_listen_timeout,my_run_timeout,my_log_close);
    my_scheduler->set_my_provider(my_provider);

    set_alarm(my_listen_timeout);
    
    delete my_scheduler;
}

void my_Webserver_reactor::set_alarm(int timeout){
    while(1){
        if(my_scheduler->stop_scheduler == false){
            alarm(timeout);
            sleep(timeout);
        } 
    }   
}

void my_Webserver_reactor::set_my_provider(my_Rpc_provider* provider){
    my_provider = provider;
}
