#include "my_rpc_service.h"

std::string my_Rpc_service::Request_Id_to_name(int id){
    return "my_rpc_service" + std::to_string(id);
}

int my_Rpc_service::Request_Name_to_Id(std::string name){
    if(name == "my_rpc_service"){
        return 1;
    }
    return 2;
}

void my_Rpc_service::Request_Id_to_name(::google::protobuf::RpcController* controller,
                    const ::myrpc::Id_Request_message* request,
                    ::myrpc::Id_Response_message* response,
                    ::google::protobuf::Closure* done){
    //提取信息
    int id = request->id();
    //运行本地函数
    std::string name = Request_Id_to_name(id);
    //填充响应信息
    response->set_info("sucess Request_Id_to_name");
    response->set_response_code(666);
    response->set_name(name);
    // 响应回调
    if(done != nullptr){
        done->Run();
    }

    //std::cout << "Request_Id_to_name sucess" << std::endl;
}

void my_Rpc_service::Request_Name_to_Id(::google::protobuf::RpcController* controller,
                    const ::myrpc::Name_Request_message* request,
                    ::myrpc::Name_Response_message* response,
                    ::google::protobuf::Closure* done){
    //提取信息
    std::string name = request->name();
    //运行本地函数
    int id = Request_Name_to_Id(name);
    //填充响应信息
    response->set_info("sucess Request_Name_to_Id");    
    response->set_response_code(666);
    response->set_id(id);
    // 响应回调
    if(done != nullptr){
        done->Run();
    }

    //std::cout << "Request_Name_to_Id sucess" << std::endl;
}
