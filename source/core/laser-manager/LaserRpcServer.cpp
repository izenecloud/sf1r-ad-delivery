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
#include "laser-manager/predict/TopNClusterContainer.h"
#include "laser-manager/predict/LaserOnlineModel.h"

#include <glog/logging.h>

using namespace msgpack::type;
using namespace sf1r::laser::predict;
using namespace sf1r::laser::clustering::rpc;
using namespace boost;
namespace sf1r { namespace laser {

LaserRpcServer::LaserRpcServer(const Tokenizer* tokenzer,
        const std::vector<boost::unordered_map<std::string, float> >* clusteringContainer,
        TopNClusteringDB* topnClustering,
        LaserOnlineModelDB* laserOnlineModel)
    : tokenizer_(tokenzer)
    , clusteringContainer_(clusteringContainer)
    , topnClustering_(topnClustering)
    , laserOnlineModel_(laserOnlineModel)
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

void LaserRpcServer::dispatch(msgpack::rpc::request req)
{
    try
    {
        std::string method;
        req.method().convert(&method);

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
            clustering::rpc::SplitTitleResult res;
            tokenizer_->tokenize(params.get<0>().title_, res.term_list_);
            req.result(res);
        }
        else if (method == CLUSTERINGServerRequest::method_names[CLUSTERINGServerRequest::METHOD_GETCLUSTERINGINFOS])
        {
            GetClusteringInfosResult gir;
            for (std::size_t i = 0; i < clusteringContainer_->size(); ++i)
            {
                ClusteringInfo clustering;
                clustering.clusteringIndex = i;
                tokenizer_->numeric((*clusteringContainer_)[i], clustering.pow);
                gir.info_list_.push_back(clustering);
            }
            req.result(gir);
        }
        else if (method == CLUSTERINGServerRequest::method_names[CLUSTERINGServerRequest::METHOD_UPDATETOPNCLUSTER])
        {
            msgpack::type::tuple<TopNClustering> params;
            req.params().convert(&params);
            const TopNClustering& clustering = params.get<0>();
            bool res = topnClustering_->update(clustering.userName(), clustering.get());
            req.result(res);
        }
        else if (method == CLUSTERINGServerRequest::method_names[CLUSTERINGServerRequest::METHOD_UPDATEPERUSERMODEL])
        {
            msgpack::type::tuple<std::string, std::vector<float> > params;
            req.params().convert(&params);
            PerUserOnlineModel userModel;
            bool res = laserOnlineModel_->update(params.get<0>(), params.get<1>());
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
