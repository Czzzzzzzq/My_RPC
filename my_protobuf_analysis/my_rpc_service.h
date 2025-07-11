#ifndef MY_RPC_SERVICE_H
#define MY_RPC_SERVICE_H

#include "my_protobuf_protocol.pb.h"

class my_Rpc_service : public myrpc::UsersRpcService{
public:

    std::string Request_Id_to_name(int id);

    int Request_Name_to_Id(std::string name);

    void Request_Id_to_name(::google::protobuf::RpcController* controller,
                       const ::myrpc::Id_Request_message* request,
                       ::myrpc::Id_Response_message* response,
                       ::google::protobuf::Closure* done);

    void Request_Name_to_Id(::google::protobuf::RpcController* controller,
                        const ::myrpc::Name_Request_message* request,
                       ::myrpc::Name_Response_message* response,
                       ::google::protobuf::Closure* done);
};


#endif