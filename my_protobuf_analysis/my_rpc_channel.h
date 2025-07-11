#ifndef MY_RPC_CHANNEL_H
#define MY_RPC_CHANNEL_H

#include "my_protobuf_protocol.pb.h"

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <mutex>
#include <arpa/inet.h>

#include "my_zookeeper.h"

class my_Rpc_channel : public google::protobuf::RpcChannel
{
public:
    my_Rpc_channel();
    ~my_Rpc_channel();

    void CallMethod(const google::protobuf::MethodDescriptor* method,
                    google::protobuf::RpcController* controller,
                    const google::protobuf::Message* request,
                    google::protobuf::Message* response,
                    google::protobuf::Closure* done);
private:
    std::string my_service_name;
    std::string my_method_name;

    std::string my_ip;
    int my_port;
    int index;
    int my_socket;
    struct sockaddr_in server_addr;

    my_Zookeeper *zkclient;

    std::string QurryService(my_Zookeeper *zkclient, std::string service_name, std::string method_name, int &idx);
    bool ConnectServer(std::string ip,int port);
};




#endif