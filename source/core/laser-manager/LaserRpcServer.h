#ifndef SF1R_LASER_RPC_SERVER_H
#define SF1R_LASER_RPC_SERVER_H

#include <3rdparty/msgpack/rpc/server.h>
#include "service/CLUSTERINGServerRequest.h"
#include "TopNClusteringDB.h"
#include "LaserOnlineModelDB.h"
#include "Tokenizer.h"
#include <boost/thread/mutex.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <vector>
namespace sf1r { namespace laser {

class LaserRpcServer : public msgpack::rpc::server::base
{
public:
    LaserRpcServer(const Tokenizer* tokenzer,
        const std::vector<boost::unordered_map<std::string, float> >* clusteringContainer,
        TopNClusteringDB* topnClustering,
        LaserOnlineModelDB* laserOnlineModel);
    ~LaserRpcServer();

public:
    void start(const std::string& host, 
        int port, 
        int threadNum);

    void stop();
    
    virtual void dispatch(msgpack::rpc::request req);
private:

private:
    const Tokenizer* tokenizer_;
    const std::vector<boost::unordered_map<std::string, float> >* clusteringContainer_;
    TopNClusteringDB* topnClustering_;
    LaserOnlineModelDB* laserOnlineModel_;
};

} } 
#endif /* RPC_SERVER_H_ */
