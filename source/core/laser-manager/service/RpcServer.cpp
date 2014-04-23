#include "RpcServer.h"
#include <string>
#include <list>
#include "errno.h"
#include "DataType.h"
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "DataType.h"
#include "type/ClusteringData.h"
#include "type/ClusteringInfo.h"
#include "conf/Configuration.h"
#include "type/LevelDBClusteringData.h"
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <3rdparty/json/json.h>
#include <3rdparty/msgpack/msgpack/type/tuple.hpp>
#include "predict/TopNClusterContainer.h"
#include "predict/LaserOnlineModel.h"

using namespace msgpack::type;
using namespace predict;
using namespace clustering::type;
using namespace conf;

namespace clustering
{
namespace rpc
{

RpcServer::RpcServer(const std::string& host, uint16_t port, uint32_t threadNum)
    : host_(host)
    , port_(port)
    , threadNum_(threadNum)
{

}

RpcServer::~RpcServer()
{
    LOG(INFO) << "~RpcServer()" << std::endl;
}

bool RpcServer::init()
{
    RETURN_ON_FAILURE(TermParser::get()->init());
    RETURN_ON_FAILURE(LevelDBClusteringData::get()->init(Configuration::get()->getClusteringDBPath()));
    RETURN_ON_FAILURE(TopNClusterContainer::get()->init(Configuration::get()->getTopNClusterDBPath()));
    RETURN_ON_FAILURE(LaserOnlineModel::get()->init(Configuration::get()->getPerUserDBPath()));
    return true;
}

void RpcServer::start()
{
    LOG(INFO) << host_ << "  " << port_ << "  " << threadNum_ << endl;
    instance.listen(host_, port_);
    instance.start(threadNum_);
}

void RpcServer::join()
{
    instance.join();
}

void RpcServer::stop()
{
    TermParser::get()->release();
    LevelDBClusteringData::get()->release();
    TopNClusterContainer::get()->release();
    instance.end();
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
//        else if (method == CLUSTERINGServerRequest::method_names[CLUSTERINGServerRequest::METHOD_GETCLUSTERINGITEMLIST])
//        {
//            ClusteringData test;
//            Document doc("test");
//            doc.add(1,1.1);
//            doc.add(2,2.2);
//            cda
//            test.clusteringData.push_back(doc);
//            test.clusteringData.push_back(doc);
//            req.result(test);
//        }
        else if (method == CLUSTERINGServerRequest::method_names[CLUSTERINGServerRequest::METHOD_GETCLUSTERINGINFOS])
        {
            msgpack::type::tuple<GetClusteringInfoRequest> params;
            req.params().convert(&params);
            GetClusteringInfosResult gir;
            if(params.get<0>().clusteringhash_ == 0)
            {
                //std::vector<ClusteringInfo> list;
                LevelDBClusteringData::get()->loadClusteringInfos(gir.info_list_);
                /*ClusteringInfo newinfo;
                int i = 0;
                for (std::vector<ClusteringInfo>::const_iterator it = list.begin(); it != list.end(); it++)
                {

                    LOG(INFO)<<"docnum:"<<it->clusteringDocNum<<endl;
                    LOG(INFO)<<"clusterHash"<<it->clusteringHash<<endl;
                    LOG(INFO)<<"pow set"<<it->clusteringPow.size()<<endl;
                    //gir.info_list_.push_back(newinfo);
                    gir.info_list_.push_back(*it);  //NOT
                    //if (i++ > 12)
                      //  break;
                }*/
                LOG(INFO) << "method:" <<method <<" data req total"<< gir.info_list_.size() << std::endl;
                req.result(gir);
            }
            else
            {
                ClusteringInfo newinfo;
                bool res = LevelDBClusteringData::get()->loadClusteringInfo(params.get<0>().clusteringhash_, newinfo);
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
            msgpack::type::tuple<PerUserOnlineModel> params;
            req.params().convert(&params);
            bool res = LaserOnlineModel::get()->update(params.get<0>());
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
        //LOG(INFO) << "exception: " << e.what() << std::endl;
        req.error(std::string(e.what()));
    }
    catch (...)
    {
        LOG(INFO)<<"Rpc error"<<endl;
    }
}

}
}
