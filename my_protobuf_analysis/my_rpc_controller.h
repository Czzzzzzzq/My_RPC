#ifndef MY_RPC_CONTROLLER_H
#define MY_RPC_CONTROLLER_H

#include "my_protobuf_protocol.pb.h"

#include <string>
#include <google/protobuf/message.h>

class RpcController : public google::protobuf::RpcController {
public:
    RpcController() {Reset();}
    ~RpcController() {}

    void Reset() {
        failed = false;
        error_text.clear();
    }
    bool Failed() const { return failed; }
    std::string ErrorText() const { return error_text; }

    void SetFailed(const std::string& reason) {
        failed = true;
        error_text = reason;
    }

    void NotifyOnCancel(google::protobuf::Closure* callback) {
        if(callback != nullptr)
        {
            callback->Run();
        }
    }

    void StartCancel() {}
    bool IsCanceled() const { return false; }
    
private:
    bool failed;
    std::string error_text;
};

#endif