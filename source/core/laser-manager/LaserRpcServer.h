#ifndef SF1R_LASER_RPC_SERVER_H
#define SF1R_LASER_RPC_SERVER_H

#include <3rdparty/msgpack/rpc/server.h>
#include "service/CLUSTERINGServerRequest.h"
#include "LaserManager.h"
#include <boost/thread/mutex.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <vector>
namespace sf1r { namespace laser {

class LaserRpcServer : public msgpack::rpc::server::base
{
public:
    LaserRpcServer();
    ~LaserRpcServer();

public:
    void start(const std::string& host, 
        int port, 
        int threadNum);

    void stop();
    
    void registerCollection(const std::string& collection, const LaserManager* laserManager); 
    void removeCollection(const std::string& collection);
    const LaserManager* get(const std::string& collection); 

    virtual void dispatch(msgpack::rpc::request req);

private:
    boost::unordered_map<std::string, const LaserManager*> laserManagers_;
    boost::shared_mutex mutex_;
};

} } 
#endif /* RPC_SERVER_H_ */
