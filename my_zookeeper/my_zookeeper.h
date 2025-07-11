#ifndef MY_ZOOKEEPER_H
#define MY_ZOOKEEPER_H

#include <zookeeper/zookeeper.h>

#include <string>
#include <semaphore.h>
#include <mutex>
#include <condition_variable>
#include <iostream>

class my_Zookeeper
{
public:
    my_Zookeeper();
    ~my_Zookeeper();

    void Start();
    void Create(const char *path, const char *data, int datalen, int flag = 0);
    void Delete(const char *path, int version = -1);
    std::string GetData(const char *path);

private:
    zhandle_t* my_Zookeeper_handle; 
};



#endif