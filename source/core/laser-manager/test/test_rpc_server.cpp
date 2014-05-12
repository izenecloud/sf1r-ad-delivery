/*
 * testRpcServer.cpp
 *
 *  Created on: Apr 2, 2014
 *      Author: alex
 */
#include <3rdparty/msgpack/rpc/client.h>
#include <iostream>
#include "common/constant.h"
#include "conf/Configuration.h"
#include "service/DataType.h"
#include "service/CLUSTERINGServerRequest.h"
#include "type/Document.h"
#include "type/ClusteringData.h"
#include "type/ClusteringInfo.h"
#include <vector>
#include <list>
#include <map>

using namespace clustering::rpc;
using namespace clustering::type;
using namespace conf;
using namespace std;
int main(int argc, char* argv[])
{
    if(argc != 2)
    {
        std::cout<<"test_rpc_server ./conf"<<endl;
        exit(0);
    }
    Configuration::get()->parse(argv[1]);
    cout <<"Test test"<<endl;
    msgpack::rpc::client c(Configuration::get()->getHost(), Configuration::get()->getPort());
    // bool result = c.call(CLUSTERINGServerRequest::method_names[CLUSTERINGServerRequest::METHOD_TEST], true).get<bool>();
    // std::cout << result << std::endl;
    SplitTitle st;
    st.title_ = "依尚羊羔绒上衣";
    cout <<"Test splite Title "<<st.title_<<endl;

    SplitTitleResult sTitleResult = c.call(CLUSTERINGServerRequest::method_names[CLUSTERINGServerRequest::METHOD_SPLITETITLE], st).get<SplitTitleResult>();
    cout <<"Test split Result is"<<endl;
    for(map<size_t, float>::iterator iter = sTitleResult.term_list_.begin(); iter != sTitleResult.term_list_.end(); iter++)
    {
        cout<<"termlist:" << iter->first <<"  "<<iter->second<<endl;
    }
    GetClusteringInfoRequest gcir;
    gcir.clusteringhash_ = 0;
    GetClusteringInfosResult gir = c.call(CLUSTERINGServerRequest::method_names[CLUSTERINGServerRequest::METHOD_GETCLUSTERINGINFOS], gcir).get<GetClusteringInfosResult>();
    for(vector<ClusteringInfo>::iterator iter = gir.info_list_.begin(); iter!= gir.info_list_.end(); iter++)
    {
        cout<<"name"<<iter->clusteringname<<" docnum:"<<iter->clusteringDocNum<<"dochash"<<iter->clusteringHash<<" pow size:"<<iter->clusteringPow.size()<<endl;
    }
    if(gir.info_list_.size() > 0)
    {
        cout<<"test single get"<<endl;
        gcir.clusteringhash_  = gir.info_list_[0].clusteringHash;
//        gcir.clusteringhash_ = 999520201;
        GetClusteringInfosResult gir2 = c.call(CLUSTERINGServerRequest::method_names[CLUSTERINGServerRequest::METHOD_GETCLUSTERINGINFOS], gcir).get<GetClusteringInfosResult>();

        if(gir2.info_list_.size()>0)
        {
            ClusteringInfo ci = gir2.info_list_[0];
            cout<<"name"<<ci.clusteringname<<" docnum:"<<ci.clusteringDocNum<<"dochash"<<ci.clusteringHash<<" pow size:"<<ci.clusteringPow.size()<<endl;
        }
    }
    /*GetClusteringItemListRequest gcir;
    gcir.clusteringhash_ = 1;
    ClusteringData gciR = c.call(CLUSTERINGServerRequest::method_names[CLUSTERINGServerRequest::METHOD_GETCLUSTERINGITEMLIST], gcir).get<ClusteringData>();
    for(vector<Document>::iterator iter = gciR.clusteringData.begin(); iter != gciR.clusteringData.end(); iter++)
    {
        cout<<"document id"<<iter->doc_id<<" "<<endl;
        for(map<clustering::hash_t, float>::iterator mapiter = iter->terms.begin(); mapiter != iter->terms.end(); mapiter++)
        {
            cout<<"term id"<< mapiter->first<<" value:"<<mapiter->second<<endl;
        }
    }*/
}




