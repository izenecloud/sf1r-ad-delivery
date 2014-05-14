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
using namespace sf1r::laser::clustering::type;
using namespace sf1r::laser::clustering::rpc;
using namespace boost;
namespace sf1r { namespace laser {

LaserRpcServer::LaserRpcServer(const Tokenizer* tokenzer,
        const std::vector<boost::unordered_map<std::string, float> >* clusteringContainer)
    : tokenizer_(tokenzer)
    , clusteringContainer_(clusteringContainer)
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

        /*if (method == CLUSTERINGServerRequest::method_names[CLUSTERINGServerRequest::METHOD_TEST])
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
            TermParser::get()->parse(params.get<0>(), res);
            req.result(res);
        }
        else if (method == CLUSTERINGServerRequest::method_names[CLUSTERINGServerRequest::METHOD_GETCLUSTERINGINFOS])
        {
            msgpack::type::tuple<GetClusteringInfoRequest> params;
            req.params().convert(&params);
            GetClusteringInfosResult gir;
            if(params.get<0>().clusteringhash_ == 0)
            {
                std::vector<clustering::type::ClusteringInfo> info_list_;
                ClusteringDataStorage::get()->loadClusteringInfos(info_list_);
                for (std::size_t i = 0; i < info_list_.size(); ++i)
                {
                    const boost::unordered_map<std::string, float>& pow = info_list_[i].getClusteringVector();
                    clustering::rpc::ClusteringInfo vec;
                    vec.clusteringIndex = info_list_[i].getClusteringIndex();
                    TermParser::get()->numeric(pow, vec.pow);
                    gir.info_list_.push_back(vec);
                }
                LOG(INFO) << "method:" <<method <<" data req total"<< gir.info_list_.size() << std::endl;
                req.result(gir);
            }
            else
            {
                clustering::type::ClusteringInfo newinfo;
                bool res = ClusteringDataStorage::get()->loadClusteringInfo(params.get<0>().clusteringhash_, newinfo);
                LOG(INFO) << "method:" <<method <<" data req one"<< res << std::endl;
                if(res == true)
                {
                    clustering::rpc::ClusteringInfo vec;
                    vec.clusteringIndex = newinfo.getClusteringIndex();
                    TermParser::get()->numeric(newinfo.getClusteringVector(), vec.pow);
                    gir.info_list_.push_back(vec);
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
        }*/
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
