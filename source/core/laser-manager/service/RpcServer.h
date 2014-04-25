#ifndef RPC_SERVER_H_
#define RPC_SERVER_H_

//#include "OptimizerServerRequest.h"
#include <glog/logging.h>
#include <3rdparty/msgpack/rpc/server.h>
#include <boost/scoped_ptr.hpp>
#include "TermParser.h"
#include "CLUSTERINGServerRequest.h"
#include "TermParser.h"
#include "laser-manager/clusteringmanager/type/ClusteringDataAdapter.h"
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

    bool init(const std::string& clusteringRootPath, const std::string& dictionaryPath,  
       const std::string& clusteringDBPath, const std::string& topNClusteringPath,
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
    //ClusteringDataAdapter* cda;
    uint16_t port_;
    uint32_t threadNum_;
};

} //namespace laser
} //namespace sf1r

#endif /* RPC_SERVER_H_ */
