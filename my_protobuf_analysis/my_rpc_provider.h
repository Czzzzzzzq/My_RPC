#ifndef MY_RPC_PROVIDER_H
#define MY_RPC_PROVIDER_H

#include "my_protobuf_protocol.pb.h"

#include "my_zookeeper.h"
#include "my_webserver.h"

class my_Rpc_provider{
public:
    my_Rpc_provider();
    ~my_Rpc_provider();

    static my_Rpc_provider* get_instance();   

    void init(std::string ip,int port,int sql_port, const char *sql_user,const char *sql_pwd, const char *db_name,
                     int conn_pool_num, int thread_pool_num,int coroutine_num,int listen_timeout,int run_timeout,bool log_close);
    void registration_service(google::protobuf::Service *service);
    void run();

public:
    std::string analysis_write(std::string);
    std::string ip;
    int port;
private:
    my_Zookeeper* zkclient;
    my_Webserver_reactor *my_webserver;
    my_Sql_thread_pool *my_sql_thread_pool;
    my_Log *my_log;
private:
    struct my_service_struct
    {
        google::protobuf::Service* service;
        std::unordered_map<std::string, const google::protobuf::MethodDescriptor*> method_map;
    };
    std::unordered_map<std::string, my_service_struct>service_map;

};



#endif