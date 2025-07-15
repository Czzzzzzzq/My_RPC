#include "my_protobuf_protocol.pb.h"
#include "my_rpc_provider.h"
#include "my_rpc_service.h"


int main(void)
{
    my_Rpc_provider provider;
    
    provider.init("127.0.0.1", 8080, 3306, "root", "123456789", "test", 8, 8, 50, 10, 10, true);

    my_Rpc_service *service = new my_Rpc_service();

    provider.registration_service(service);
    
    provider.run();
    
    return 0;
}