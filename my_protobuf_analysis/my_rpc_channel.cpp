#include "my_rpc_channel.h"

std::mutex data_mutx;  // 全局互斥锁，用于保护共享数据的线程安全


my_Rpc_channel::my_Rpc_channel()
{
    my_ip = "127.0.0.1";
    my_port = 8080;
    my_socket = -1;
}

my_Rpc_channel::~my_Rpc_channel()
{
    if(my_socket != -1)
    {
        close(my_socket);
    }
}

void my_Rpc_channel::CallMethod(const google::protobuf::MethodDescriptor* method,
                                google::protobuf::RpcController* controller,
                                const google::protobuf::Message* request,
                                google::protobuf::Message* response,
                                google::protobuf::Closure* done)
{
    //获取服务名和方法名
    my_service_name = method->service()->name();
    my_method_name = method->name();
    
    //查询zookeeper服务器
    my_Zookeeper my_zookeeper_client;
    my_zookeeper_client.Start();
    std::string method_data = QurryService(&my_zookeeper_client, my_service_name, my_method_name, index);
    if(method_data.empty())
    {
        controller->SetFailed("method_data is empty");
        return;
    }
    my_ip = method_data.substr(0, method_data.find(":"));
    my_port = atoi(method_data.substr(method_data.find(":") + 1).c_str());
    //连接服务器
    if(!ConnectServer(my_ip, my_port))
    {
        controller->SetFailed("connect server error");
        return;
    }
    //构建请求
    int parameter_length = 0;
    std::string parameter = {};
    if(request->SerializeToString(&parameter))
    {
        parameter_length = parameter.size();
    }else{
        controller->SetFailed("SerializeToString error");
        return;
    }

    //构建头部字段
    myrpc::Header RpcHeader;
    RpcHeader.set_service_name(my_service_name);  // 设置服务名
    RpcHeader.set_method_name(my_method_name);  // 设置方法名
    RpcHeader.set_parameter_size(parameter_length);  // 设置参数长度

    // 将RPC头部信息序列化为字符串，并计算其长度
    int header_size = 0;
    std::string header_str;
    if (RpcHeader.SerializeToString(&header_str)) {  // 序列化头部信息
        header_size = header_str.size();  // 获取序列化后的长度
    } else {
        controller->SetFailed("serialize header error!");  // 头部序列化失败，设置错误信息
        return;
    }

    // 将头部长度和头部信息拼接成完整的RPC请求报文
    std::string send_data;
    {
        google::protobuf::io::StringOutputStream string_output(&send_data);
        google::protobuf::io::CodedOutputStream coded_output(&string_output);
        coded_output.WriteVarint32(static_cast<uint32_t>(header_size));  // 写入头部长度
        coded_output.WriteString(header_str);  // 写入头部信息
    }
    send_data += parameter;  // 拼接请求参数

    //发送请求

    int ret = send(my_socket, send_data.c_str(), send_data.size(), 0);
    if(ret < 0)
    {
         controller->SetFailed("send error");
    }   
    else if(ret == 0)
    {
         controller->SetFailed("server close");
    }

    int READ_BUFFER_SIZE = 1024;
    char read_buf[READ_BUFFER_SIZE] = {0};

    int read_bytes = 0;
    int buffer_ptr = 0;

    while (true)
    {
        read_bytes = recv(my_socket, read_buf + buffer_ptr,READ_BUFFER_SIZE - buffer_ptr, 0);
        if(read_bytes == -1)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }else{
                break;
            }
        }else if(read_bytes == 0){
            controller->SetFailed("server close");
            break;
        }else{
            buffer_ptr += read_bytes;
            break;
        }
    }

    // 在解析前打印响应
    /*
    std::cout << "client Raw response (" << buffer_ptr << " bytes): ";
    for (int i = 0; i < buffer_ptr; i++) {
        printf("%02x ", static_cast<unsigned char>(read_buf[i]));
    }
    std::cout << std::endl;
    std::string str_in = std::string(read_buf, buffer_ptr);
    std::cout << "client Raw response: " << str_in << std::endl;
    myrpc::Id_Response_message response2;
    response2.ParseFromString(str_in);
    std::cout << "response2.name()=" << response2.name() << std::endl;
    */

    if(buffer_ptr <= 0)
    {
        controller->SetFailed("read error");
        return;
    }else{
        if(!response->ParseFromArray(read_buf, buffer_ptr)) {
            controller->SetFailed("parse response error");
        }
    }
    close(my_socket);

    if(done != nullptr)
    {
        done->Run();
    }
    
    return;
}

std::string my_Rpc_channel::QurryService(my_Zookeeper *zkclient, std::string service_name, std::string method_name, int &idx)
{
    std::string method_path = "/" + service_name + "/" + method_name;
    std::string method_data;
    
    std::unique_lock<std::mutex> lock(data_mutx);
    method_data = zkclient->GetData(method_path.c_str());
    lock.unlock();

    return method_data;
}   

bool my_Rpc_channel::ConnectServer(std::string ip,int port)
{
    my_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(my_socket < 0)
    {
        std::cout << "create socket error" << std::endl;
        return false;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    if(connect(my_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {   
        std::cout << "connect error" << std::endl;
        return false;
    }       
    return true; 
}   
