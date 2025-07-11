#include "my_protobuf_protocol.pb.h"
#include "my_rpc_channel.h"
#include "my_rpc_controller.h"

int main() {

    myrpc::UsersRpcService_Stub client(new my_Rpc_channel());

    for(int i = 0; i < 1000; i++){
        myrpc::Id_Request_message request;
        myrpc::Id_Response_message response;
        RpcController controller;

        request.set_info("info");
        request.set_request_code(1);
        request.set_id(i);

        client.Request_Id_to_name(&controller, &request, &response, nullptr);
        
        if(controller.Failed()){
            std::cout << "controller.ErrorText() = " << controller.ErrorText() << std::endl;
            std::cout << "response: " << response.name() << std::endl;
        }else{
            std::cout << "call Request_Id_to_name success" << std::endl;
            std::cout << "response: " << response.name() << std::endl;
        }
        sleep(1);
    }
    
    for(int i = 0; i < 1000; i++){
        myrpc::Name_Request_message request;
        myrpc::Name_Response_message response;
        RpcController controller;

        request.set_info("info");
        request.set_request_code(1);
        request.set_name("name");

        client.Request_Name_to_Id(&controller, &request, &response, nullptr);

        if(controller.Failed()){
            std::cout << "controller.ErrorText() = " << controller.ErrorText() << std::endl;
        }else{
            std::cout << "call Request_Name_to_Id success" << std::endl;
            std::cout << "response: " << response.id() << std::endl;
        }
        //sleep(1);
    }
    return 0;
}