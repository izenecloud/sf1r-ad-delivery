#ifndef SF1R_AD_LASER_RPC_SERVER_H
#define SF1R_AD_LASER_RPC_SERVER_H

#include <glog/logging.h>
#include <3rdparty/msgpack/rpc/server.h>
#include <boost/scoped_ptr.hpp>
#include "TermParser.h"
#include "CLUSTERINGServerRequest.h"
#include "TermParser.h"
#include "type/ClusteringDataAdapter.h"
namespace sf1r 
{ 
namespace laser 
{

class RpcServer : public msgpack::rpc::server::base
{
public:
    RpcServer(
        const std::string& host,
        uint16_t port,
        uint32_t threadNum);

    ~RpcServer();

    bool init(const std::string& clusteringRootPath, 
        const std::string& clusteringDBPath, 
        const std::string& perUserDBPath);

    inline uint16_t getPort() const
    {
        return port_;
    }

    void start();

    void join();

    // start + join
    void run();

    void stop();
public:
    virtual void dispatch(msgpack::rpc::request req);
private:
    std::string host_;
    uint16_t port_;
    uint32_t threadNum_;
};

} 
} 

#endif
