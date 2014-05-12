/*
 * testRpcServer.cpp
 *
 *  Created on: Apr 2, 2014
 *      Author: alex
 */
#include <3rdparty/msgpack/rpc/client.h>
#include <iostream>
#include "common/constant.h"
#include "conf/ClusteringCfg.h"
#include "service/DataType.h"
#include "service/CLUSTERINGServerRequest.h"
#include "type/Document.h"
#include "type/ClusteringData.h"
#include "type/ClusteringInfo.h"
#include <vector>
#include <map>

using namespace clustering::conf;
using namespace clustering::rpc;
using namespace clustering::type;
using namespace std;
int main(int argc, char* argv[])
{
    if(argc != 3)
    {
        std::cout<<"test_rpc_server ./conf splitsentense"<<endl;
        exit(0);
    }
    ClusteringCfg::get()->parse(argv[1]);
    cout <<"Test test"<<endl;
    msgpack::rpc::client c(ClusteringCfg::get()->getHost(), ClusteringCfg::get()->getPort());
    // bool result = c.call(CLUSTERINGServerRequest::method_names[CLUSTERINGServerRequest::METHOD_TEST], true).get<bool>();
    // std::cout << result << std::endl;
    SplitTitle st;
    //st.title_ = "依尚羊羔绒上衣";
    st.title_=argv[2];
    cout <<"Test splite Title "<<st.title_<<endl;

    SplitTitleResult sTitleResult = c.call(CLUSTERINGServerRequest::method_names[CLUSTERINGServerRequest::METHOD_SPLITETITLE], st).get<SplitTitleResult>();
    cout <<"Test split Result is"<<endl;
    for(map<size_t, float>::iterator iter = sTitleResult.term_list_.begin(); iter != sTitleResult.term_list_.end(); iter++)
    {
        cout<<"termlist:" << iter->first <<"  "<<iter->second<<endl;
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




