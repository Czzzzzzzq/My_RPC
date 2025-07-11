#include "my_zookeeper.h"

std::mutex cv_mutex;        // 全局锁，用于保护共享变量的线程安全
std::condition_variable cv; // 条件变量，用于线程间通信
bool is_connected = false;  // 标记ZooKeeper客户端是否连接成功

void global_watcher(zhandle_t* handle, int type,int state, const char* path, void* watcherCtx)
{
    if (type == ZOO_SESSION_EVENT) // 回调的消息类型是和会话相关的消息类型
    {
        if (state == ZOO_CONNECTED_STATE) // zkclient和zkserver连接成功
        {
            std::lock_guard<std::mutex> lock(cv_mutex);
            is_connected = true;
            cv.notify_all();
        }
    }
}

my_Zookeeper::my_Zookeeper()
{
    my_Zookeeper_handle = nullptr;
}

my_Zookeeper::~my_Zookeeper()
{
    if (my_Zookeeper_handle != nullptr)
    {
        zookeeper_close(my_Zookeeper_handle);
    }
}

void my_Zookeeper::Start()
{
    std::string host = "127.0.0.1:2181";

    my_Zookeeper_handle = zookeeper_init(host.c_str(), global_watcher, 30000, nullptr, nullptr, 0);
    if (my_Zookeeper_handle == nullptr)
    {
        std::cout << "zookeeper_init error!" << std::endl;
        exit(1);
    }

    {
        std::unique_lock<std::mutex> lock(cv_mutex);
        while (!is_connected)
        {
            cv.wait(lock);
        }
    }
    std::cout << "ZooKeeper client started!" << std::endl;
}

void my_Zookeeper::Create(const char *path, const char *data, int datalen, int flag)
{
    char path_buffer[128];
    int bufferlen = sizeof(path_buffer);
    int zoo_flag;
    // 先判断path表示的znode节点是否存在
    zoo_flag = zoo_exists(my_Zookeeper_handle, path, 0, NULL);
    if (zoo_flag == ZNONODE) // 表示path的znode节点不存在
    {
        zoo_flag = zoo_create(my_Zookeeper_handle, path, data, datalen,&ZOO_OPEN_ACL_UNSAFE,flag,path_buffer,bufferlen);
        if (zoo_flag == ZOK)
        {
            std::cout << "znode create success! path:" << path << std::endl;
        }
        else
        {
            std::cout << "zoo_flag:" << zoo_flag << std::endl;
        }
    }
    else
    {
        std::cout << "znode already exist! path:" << path << std::endl;
    }
}

void my_Zookeeper::Delete(const char *path, int version)
{
    int zoo_flag;
    // 先判断path表示的znode节点是否存在
    zoo_flag = zoo_exists(my_Zookeeper_handle, path, 0, NULL);
    if (zoo_flag != ZNONODE) // 表示path的znode节点存在
    {
        zoo_flag = zoo_delete(my_Zookeeper_handle, path, version);
        if (zoo_flag == ZOK)
        {
            std::cout << "znode delete success! path:" << path << std::endl;
        }
        else
        {
            std::cout << "zoo_flag:" << zoo_flag << std::endl;
        }
    }
    else
    {
        std::cout << "znode not exist! path:" << path << std::endl;
    }
}

std::string my_Zookeeper::GetData(const char *path)
{
    char buffer[64];
    int bufferlen = sizeof(buffer);
    int zoo_flag;
    // 先判断path表示的znode节点是否存在
    zoo_flag = zoo_get(my_Zookeeper_handle, path, 0, buffer, &bufferlen, NULL);
    if (zoo_flag == ZOK)
    {
        return std::string(buffer, bufferlen);
    }
    else
    {
        std::cout << "zoo_flag:" << zoo_flag << std::endl;
        return "";
    }
}
