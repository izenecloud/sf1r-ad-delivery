#include "RpcServer.h"
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
#include "DataType.h"
#include "laser-manager/clusteringmanager/type/ClusteringData.h"
#include "laser-manager/clusteringmanager/type/ClusteringInfo.h"
#include "laser-manager/clusteringmanager/type/ClusteringDataStorage.h"
#include "laser-manager/predict/TopNClusterContainer.h"
#include "laser-manager/predict/LaserOnlineModel.h"

#include <glog/logging.h>

using namespace msgpack::type;
using namespace sf1r::laser::predict;
using namespace sf1r::laser::clustering::type;
using namespace sf1r::laser::clustering::rpc;
using namespace boost;
namespace sf1r
{
namespace laser
{

static shared_ptr<RpcServer> rpcServer;
static shared_mutex mutex;
shared_ptr<RpcServer>& RpcServer::getInstance()
{
    unique_lock<shared_mutex> uniqueLock(mutex);
    if (NULL == rpcServer.get())
    {
        rpcServer.reset(new RpcServer());
    }
    return rpcServer;
}


RpcServer::RpcServer()
    : isStarted_(false)
{
}

RpcServer::~RpcServer()
{
    stop();
}

bool RpcServer::init(const std::string& clusteringRootPath,
       const std::string& clusteringDBPath,
       const std::string& pcaPath)
{
    RETURN_ON_FAILURE(TermParser::get()->init(clusteringRootPath, pcaPath));
    RETURN_ON_FAILURE(ClusteringDataStorage::get()->init(clusteringDBPath));
    RETURN_ON_FAILURE(TopNClusterContainer::get()->init(clusteringDBPath));
    RETURN_ON_FAILURE(LaserOnlineModel::get()->init(clusteringDBPath));
    return true;
}

void RpcServer::start(const std::string& host, 
        int port, 
        int threadNum, 
        const std::string& clusteringRootPath, 
        const std::string& clusteringDBRootPath, 
        const std::string& pcaPath)
{
    boost::unique_lock<boost::shared_mutex> uniqueLock(mutex_);
    if (isStarted_)
    {
        return;
    }

    if (init(clusteringRootPath, clusteringDBRootPath, pcaPath))
    {
        LOG(INFO) << host << "  " << port << "  " << threadNum << endl;
        instance.listen(host, port);
        instance.start(threadNum);
        isStarted_ = true;
    }
    else
    {
        LOG(ERROR)<<"init leveldb storage error.";
    }
}

void RpcServer::stop()
{
    LOG(INFO)<<"release all leveldb storage";
    TermParser::get()->release();
    ClusteringDataStorage::get()->release();
    TopNClusterContainer::get()->release();
    LOG(INFO)<<"stop msgpack server";
    instance.end();
    instance.join();
}

void RpcServer::dispatch(msgpack::rpc::request req)
{
    try
    {
        std::string method;
        req.method().convert(&method);
        //LOG(INFO) << "method:  " << method << std::endl;

        if (method == CLUSTERINGServerRequest::method_names[CLUSTERINGServerRequest::METHOD_TEST])
        {
            msgpack::type::tuple<bool> params;
            req.params().convert(&params);
            bool data = params.get<0>();
            LOG(INFO) << "method:" <<method <<" data"<< data << std::endl;
            TestData res;
            res.test_.push_back(0);
            res.test_.push_back(2);
            req.result(res);
        }
        else if (method == CLUSTERINGServerRequest::method_names[CLUSTERINGServerRequest::METHOD_SPLITETITLE])
        {
            // SplitTitle params;
            msgpack::type::tuple<SplitTitle> params;
            req.params().convert(&params);
            req.result(TermParser::get()->parse(params.get<0>()));
        }
        else if (method == CLUSTERINGServerRequest::method_names[CLUSTERINGServerRequest::METHOD_GETCLUSTERINGINFOS])
        {
            msgpack::type::tuple<GetClusteringInfoRequest> params;
            req.params().convert(&params);
            GetClusteringInfosResult gir;
            if(params.get<0>().clusteringhash_ == 0)
            {
                ClusteringDataStorage::get()->loadClusteringInfos(gir.info_list_);
                LOG(INFO) << "method:" <<method <<" data req total"<< gir.info_list_.size() << std::endl;
                req.result(gir);
            }
            else
            {
                ClusteringInfo newinfo;
                bool res = ClusteringDataStorage::get()->loadClusteringInfo(params.get<0>().clusteringhash_, newinfo);
                LOG(INFO) << "method:" <<method <<" data req one"<< res << std::endl;
                if(res == true)
                {
                    gir.info_list_.push_back(newinfo);
                }

                req.result(gir);
            }
        }
        else if (method == CLUSTERINGServerRequest::method_names[CLUSTERINGServerRequest::METHOD_UPDATETOPNCLUSTER])
        {
            msgpack::type::tuple<TopNCluster> params;
            req.params().convert(&params);
            bool res = TopNClusterContainer::get()->update(params.get<0>());
            req.result(res);
        }
        else if (method == CLUSTERINGServerRequest::method_names[CLUSTERINGServerRequest::METHOD_UPDATEPERUSERMODEL])
        {
            msgpack::type::tuple<std::string, std::vector<float> > params;
            req.params().convert(&params);
            PerUserOnlineModel userModel;
            userModel.setUserName(params.get<0>());
            userModel.setArgs(params.get<1>());
            bool res = LaserOnlineModel::get()->update(userModel);
            req.result(res);
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

}
}
