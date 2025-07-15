#include "my_rpc_provider.h"

my_Rpc_provider::my_Rpc_provider()
{
}

my_Rpc_provider::~my_Rpc_provider()
{
    delete zkclient;
}  

my_Rpc_provider* my_Rpc_provider::get_instance()
{
    static my_Rpc_provider instance;
    return &instance;
}

void my_Rpc_provider::init(std::string ip,int port,int sql_port, const char *sql_user,const char *sql_pwd, const char *db_name,
                     int conn_pool_num, int thread_pool_num,int coroutine_num,int listen_timeout,int run_timeout,bool log_close)
{
    my_webserver = my_Webserver_reactor::get_instance();
    my_webserver->init_my_Webserver_reactor(port,sql_port,sql_user,sql_pwd,db_name,conn_pool_num,thread_pool_num,coroutine_num,listen_timeout,run_timeout,log_close);


    this->port = port;
    this->ip = ip;
    zkclient = new my_Zookeeper();
    zkclient->Start();  // 连接ZooKeeper服务器

}
void my_Rpc_provider::run()
{
    my_webserver->set_my_provider(this);
    my_webserver->start_my_Webserver_reactor();
}


void my_Rpc_provider::registration_service(google::protobuf::Service *service)
{
    my_service_struct service_struct;

    const google::protobuf::ServiceDescriptor *service_descriptor = service->GetDescriptor();
    
    std::string service_name = service_descriptor->name();
    int method_count = service_descriptor->method_count();

    cout << "service_name=" << service_name << endl;
    cout << "method_count=" << method_count << endl;
    for (int i = 0; i < method_count; ++i) {
        const google::protobuf::MethodDescriptor *method_descriptor = service_descriptor->method(i);
        std::string method_name = method_descriptor->name();
        std::cout << "method_name=" << method_name << std::endl;
        service_struct.method_map[method_name] = method_descriptor;  // 将方法名和方法描述符存入map
    }
    service_struct.service = service;  // 保存服务实例指针

    service_map[service_name] = service_struct;  // 将服务信息存入服务map

    cout << "service_map.size()=" << service_map.size() << endl;

    // service_name为永久节点，method_name为临时节点
    for (auto &sp : service_map) {
        // service_name 在ZooKeeper中的目录是"/"+service_name
        std::string service_path = "/" + sp.first;
        zkclient->Create(service_path.c_str(), nullptr, 0);  // 创建服务节点
        for (auto &mp : sp.second.method_map) {
            std::string method_path = service_path + "/" + mp.first;
            char method_path_data[128] = {0};
            sprintf(method_path_data, "%s:%d", ip.c_str(), port);  // 将IP和端口信息存入节点数据
            // ZOO_EPHEMERAL表示这个节点是临时节点，在客户端断开连接后，ZooKeeper会自动删除这个节点
            zkclient->Create(method_path.c_str(), method_path_data, strlen(method_path_data), ZOO_EPHEMERAL);
        }
    }
}

std::string my_Rpc_provider::analysis_write(std::string message_request)
{
    google::protobuf::io::ArrayInputStream raw_input(message_request.data(), message_request.size());
    google::protobuf::io::CodedInputStream coded_input(&raw_input);

    uint32_t header_size;
    coded_input.ReadVarint32(&header_size);  // 解析header_size

    // 根据header_size读取数据头的原始字符流，反序列化数据，得到RPC请求的详细信息
    std::string header_str;
    myrpc::Header header;

    std::string service_name;
    std::string method_name;
    uint32_t paramenter_length;

    // 设置读取限制
    google::protobuf::io::CodedInputStream::Limit msg_limit = coded_input.PushLimit(header_size);
    coded_input.ReadString(&header_str, header_size);
    
    coded_input.PopLimit(msg_limit);

    if (header.ParseFromString(header_str)) {
        service_name = header.service_name();
        method_name = header.method_name();
        paramenter_length = header.parameter_size();
    } else {
        cout<<"header parse error"<<endl;
        return "error";
    }

    std::string paramenter_str;  // RPC参数
    // 直接读取paramenter_length长度的字符串数据
    bool read_paramenter_success = coded_input.ReadString(&paramenter_str, paramenter_length);
    if (!read_paramenter_success) {
        cout<<"read paramenter error"<<endl;
        return "error";
    }

    // 获取service对象和method对象
    auto service_ite = service_map.find(service_name);
    if (service_ite == service_map.end()) {             
        std::cout << service_name << " is not exist!" << std::endl;
        return "error";
    }

    auto method_ite = service_ite->second.method_map.find(method_name);
    if (method_ite == service_ite->second.method_map.end()) {
        std::cout << service_name << "." << method_name << " is not exist!" << std::endl;
        return "error";
    }

    google::protobuf::Service *service = service_ite->second.service;  // 获取服务对象
    const google::protobuf::MethodDescriptor *method = method_ite->second;  // 获取方法对象

    if(service == nullptr){
        std::cout << service_name <<" ptr is not exist!" << std::endl;
        return "error";
    }
    if(method == nullptr){
        std::cout << method_name <<" ptr is not exist!" << std::endl;
        return "error";
    }

    // 生成RPC方法调用请求的request和响应的response参数
    google::protobuf::Message *request = service->GetRequestPrototype(method).New();  // 动态创建请求对象
    if (!request->ParseFromString(paramenter_str)) {
        std::cout << service_name << "." << method_name << " parse error!" << std::endl;
        delete request;
        return "error";
    }
    google::protobuf::Message *response = service->GetResponsePrototype(method).New();  // 动态创建响应对象
    
    // 在框架上根据远端RPC请求，调用当前RPC节点上发布的方法
    service->CallMethod(method, nullptr, request, response, nullptr);  // 调用服务方法

    string message_response;
    assert(response != nullptr);
    bool ok = response->SerializeToString(&message_response);
    if (!ok) {
        std::cout << "response serialize error" << std::endl;
    }
    /*
    cout << "message_response=" << message_response << endl;
    myrpc::Id_Response_message response2;
    response2.ParseFromString(message_response);
    cout << "response2.name()=" << response2.name() << endl;
    */
    delete request;
    delete response;
    return message_response;
}
