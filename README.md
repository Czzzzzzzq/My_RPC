# 项目介绍

这个项目是基于C++实现的一个RPC分布式网络通信框架项目，将本地方法调用重构为基于TCP网络通信的RPC远程方法调用。框架通过使用ZooKeeper实现服务的注册与发现。利用ucontext_t实现非对称协程调度和设计线程池并实现协程调度器来实现高并发。此外通信过程中使用Protobuf自定义通信协议，并将数据进行序列化和反序列化。

# Zookeeper
学习连接：https://blog.csdn.net/dream_ambition/article/details/136027023?ops_request_misc=elastic_search_misc&request_id=17992b31513aec0916df927d0145ee8f&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~top_positive~default-1-136027023-null-null.142^v102^pc_search_result_base3&utm_term=zookeeper&spm=1018.2226.3001.4187

一些API总结：https://www.wolai.com/3oyyNhcyBWnTmhg2G5fzJv

# Protobuf
|类名|角色|
|-|-|
|`Service`|服务接口的抽象基类，定义服务方法的调用入口|
|`RpcController`|控制 RPC 调用的生命周期，处理错误和取消操作|
|`RpcChannel`|抽象通信层，负责消息的发送和接收|
|`Descriptor`|描述消息结构的元数据|
|`ServiceDescriptor`|描述服务结构的元数据|
|`MethodDescriptor`|描述服务方法的元数据|
|`Message`|所有消息的基类，提供序列化和反序列化功能|


Protobuf将自动生成基本的服务类，名称就是服务名。

UsersRpcService和UsersRpcService_Stub。UsersRpcService继承于Service，而UsersRpcService_Stub继承于UsersRpcService**。**



**Service类有一些基本的函数定义，例如CallMethod，GetDescriptor，GetRequestPrototype，GetResponsePrototype。**



**UsersRpcService类是设计给服务端的**

**UsersRpcService类中针对于用户自定义的函数将体现在其框架自动生成的CallMethod中，其参数为通用的Message，它的初衷是基类指针指向派生类对象以调用派生类的函数和成员。**此外用户自定义的函数为纯虚函数，在服务端需要用户设计一个类例如my_rpc_service继承**UsersRpcService**类来重新定义这些纯虚函数，即实现这些远程调用的函数的功能。

此外为了能管理多个服务，设计my_rpc_provider，其中存储了各种服务以及该服务对应可以使用的函数方法，并使用zookeeper以注册服务。



**UsersRpcService类是设计给客户端的**

其中**UsersRpcService类**中的Callmethod方法是调用其中一个成员RpcChannel的CallMethod方法，该方法也是纯虚函数，需要重新设计一个类，例如my_Rpc_channel继承RpcChannel，并重新定义CallMehtod方法，实现远程调用的功能。

其中同样使用Zookeeper来实现服务发现和寻址的功能。
