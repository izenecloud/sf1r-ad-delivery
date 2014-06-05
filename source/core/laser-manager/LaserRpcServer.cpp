#include "LaserRpcServer.h"
#include <string>
#include <list>
#include <errno.h>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <3rdparty/json/json.h>
#include <3rdparty/msgpack/msgpack/type/tuple.hpp>
#include <boost/thread/locks.hpp> 
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "service/DataType.h"

#include <glog/logging.h>

using namespace msgpack::type;
using namespace sf1r::laser::clustering::rpc;
using namespace boost;
namespace sf1r { namespace laser {

LaserRpcServer::LaserRpcServer()
{
}

LaserRpcServer::~LaserRpcServer()
{
}

void LaserRpcServer::start(const std::string& host, 
        int port, 
        int threadNum) 
{
    LOG(INFO) << host << "  " << port << "  " << threadNum << endl;
    instance.listen(host, port);
    instance.start(threadNum);
}

void LaserRpcServer::stop()
{
    // stop msgpack at first
    LOG(INFO)<<"stop msgpack server";
    instance.end();
    instance.join();
}
    
void LaserRpcServer::registerCollection(const std::string& collection, 
    const LaserManager* laserManager)
{
    boost::unique_lock<boost::shared_mutex> uniqueLock(mutex_);
    laserManagers_[collection] = laserManager;
}

void LaserRpcServer::removeCollection(const std::string& collection)
{
    boost::unique_lock<boost::shared_mutex> uniqueLock(mutex_);
    if (laserManagers_.end() != laserManagers_.find(collection))
        laserManagers_.erase(collection);
}


const LaserManager* LaserRpcServer::get(const std::string& collection)
{
    boost::unordered_map<std::string, const LaserManager*>::const_iterator it = laserManagers_.find(collection);
    if (laserManagers_.end() == it)
    {
        return NULL;
    }
    return it->second;
}

void LaserRpcServer::dispatch(msgpack::rpc::request req)
{
    // in case of remove collection
    try
    {
        std::string method, collection;
        if (!parseRequest(req, method, collection))
        {
            req.error(msgpack::rpc::ARGUMENT_ERROR);
            return;
        }
        
        boost::shared_lock<boost::shared_mutex> sharedLock(mutex_);
        const LaserManager* laserManager = get(collection);
        if (NULL == laserManager)
        {
            req.error("no collection: " + collection);
        }
        
        if (method == "test")
        {
            std::pair<int, float> pp;
            pp = std::make_pair(1, 2.0);
            req.result(pp);
        }
        if (method == "splitTitle")
        {
            msgpack::type::tuple<std::string> params;
            req.params().convert(&params);
            {
                clustering::rpc::SplitTitleResult res;
                laserManager->tokenizer_->tokenize(params.get<0>(), res.term_list_);
                req.result(res);
            }
        }
        else if (method == "getClusteringInfos")
        {
            GetClusteringInfosResult gir;
            for (std::size_t i = 0; i < laserManager->clusteringContainer_->size(); ++i)
            {
                ClusteringInfo clustering;
                clustering.clusteringIndex = i;
                const LaserManager::TokenVector& vec = (*laserManager->clusteringContainer_)[i];
                for (std::size_t k = 0; k < vec.size(); ++k)
                {
                    if (vec[k] > 1e-7)
                    {
                        clustering.pow[k] = vec[k];
                    }
                }
                gir.info_list_.push_back(clustering);
            }
            req.result(gir);
        }
        else if ("updateTopnClustering" == method ||
                 "updatePerUserModel" == method)
        {
            laserManager->recommend_->dispatch(method, req);
        }
        else
        {
            req.error(msgpack::rpc::NO_METHOD_ERROR);
        }
    }
    catch (const msgpack::type_error& e)
    {
        LOG(INFO) << "msgpack type_error " << e.what() << std::endl;
        req.error(msgpack::rpc::ARGUMENT_ERROR);
    }
    catch (const std::exception& e)
    {
        req.error(std::string(e.what()));
    }
    catch (...)
    {
        LOG(INFO)<<"Rpc error"<<endl;
    }
}

bool LaserRpcServer::parseRequest(msgpack::rpc::request& req, std::string& method, std::string& collection) const
{
    std::string str;
    req.method().convert(&str);
    std::size_t pos = str.find("|");
    if (std::string::npos == pos)
    {
        return false;
    }
    method = str.substr(0, pos);
    collection = str.substr(pos + 1);
    return true;
}

} }
