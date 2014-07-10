#include "MQClient.h"
#include <sf1r-net/RpcServerConnectionConfig.h>

namespace sf1r { namespace laser { namespace context {

MQClient::MQClient(const std::string& addr, const int port)
    : addr_(addr)
    , port_(port)
    , conn_(NULL)
{
    RpcServerConnectionConfig config(addr_, port_, 2);
    conn_ = new RpcServerConnection();
    conn_->init(config);
}

MQClient::~MQClient()
{
    shutdown();
    if (NULL != conn_)
    {
        conn_->flushRequests();
        delete conn_;
        conn_ = NULL;
    }
}

void MQClient::publish(const std::string& topic)
{
    bool res = false;
    conn_->syncRequest("publish", topic, res);
}

void MQClient::shutdown()
{
}

} } }
