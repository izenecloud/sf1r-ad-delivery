#ifndef SF1R_LASER_WEB_SCALE_METAQ_CLIENT_H
#define SF1R_LASER_WEB_SCALE_METAQ_CLIENT_H
#include <sf1r-net/RpcServerConnection.h>
#include <3rdparty/msgpack/rpc/server.h>
#include <glog/logging.h>

namespace sf1r { namespace laser { namespace context {

template <typename T>
class ReqData
{
public:
    ReqData(const std::string& topic, const T& val)
        : topic_(topic)
        , val_(val)
    {
    }

    MSGPACK_DEFINE(topic_, val_);
private:
    const std::string topic_;
    const T val_;
};

class MQClient
{
public:
    MQClient(const std::string& addr, const int port);
    ~MQClient();

public:
    void shutdown();
    void publish(const std::string& topic);
    template <typename T> bool produce(const std::string& topic, const T& val);
private:
    const std::string addr_;
    const int port_;
    RpcServerConnection* conn_;
};
    
template <typename T> bool MQClient::produce(const std::string& topic, const T& val)
{
    ReqData<T> req(topic, val);
    conn_->asynRequest("produce", req);
    return true;
}

} } } 
#endif
