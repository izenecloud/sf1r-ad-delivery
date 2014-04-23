#include <3rdparty/am/leveldb/db.h>
#include <am/leveldb/Table.h>

#include <util/Int2String.h>

#include <stdlib.h>
#include <assert.h>
#include <sys/time.h>

#include <cstdlib>   // for rand()
#include <cctype>    // for isalnum()
#include <algorithm> // for back_inserter


#include <boost/timer.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "common/utils.h"
#include <boost/test/unit_test.hpp>
#include "type/LevelDBClusteringData.h"
#include "service/DataType.h"
#include <sstream>

#include "LaserOnlineModel.h"
#include "PerUserOnlineModel.h"

#include "TopNClusterContainer.h"
#include "PredictTool.h"
#include <queue>
using namespace std;
using namespace leveldb;
using namespace clustering::type;

using namespace clustering;
using namespace predict;
namespace bfs = boost::filesystem;

using std::priority_queue;
using namespace clustering::type;
int featurecount=2000;
int usernum=200;
int clusternum=200;
int perdocnum=200;
LaserOnlineModel* lom_ptr;
TopNClusterContainer* tncc_ptr;
namespace pt = boost::posix_time;
void initUsrClusterModel(int truncted=1)
{
    string TESTHOME = "./usermodel";
    lom_ptr = LaserOnlineModel::get();
    if(truncted)
    {
    bfs::remove_all(TESTHOME);
    lom_ptr->init(TESTHOME);
    for(int i = 0; i < usernum; i++)
    {
        std::vector<float> tmp(featurecount, 0.1*(i+1));
        PerUserOnlineModel puom;
        stringstream ss;
        ss<<"test"<<i;
        puom.setUserName(ss.str());
        puom.setArgs(tmp);
        lom_ptr->update(puom);
    }
    lom_ptr->release();
    }
    lom_ptr->init(TESTHOME);
}
void initUsrCluster(int truncted=1)
{
    string TESTHOME = "./usercluster";
    tncc_ptr = TopNClusterContainer::get();
    if(truncted)
    {
    bfs::remove_all(TESTHOME);
    tncc_ptr->init(TESTHOME);
    for(int i = 0; i < usernum; i++)
    {
        TopNCluster tnc;
        ClusterContainer cc;
        cc.insert(make_pair<clustering::hash_t, float>(i, i*10));
        cc.insert(make_pair<clustering::hash_t, float>(i+1, i*20));
        cc.insert(make_pair<clustering::hash_t, float>(i+2, i*20));
        stringstream ss;
        ss<<"test"<<i;
        tnc.setUserName(ss.str());
        tnc.setClusterContainer(cc);
        tncc_ptr->update(tnc);
    }
    tncc_ptr->release();
    }
    tncc_ptr->init(TESTHOME);
}
void initClustering(int truncted=1)
{
    string HOME_STR2 = "./cluster";
    if(truncted)
    {
    bfs::remove_all(HOME_STR2);
    if(LevelDBClusteringData::get()->init(HOME_STR2))
    {
        for(int i =0; i < clusternum; i++)
        {
            ClusteringData oridata;
            ClusteringInfo oriinfo;
            stringstream ssc;
            ssc<<"cluster"<<i;

            for(int j = 0; j < perdocnum; j++)
            {
                stringstream ss;
                ss<<"test"<<(j*perdocnum+1);
                Document doc(ss.str());
                int count = random() %featurecount;
                set<int> featureset;
                count = 4;
                for(int k = 0; k < count; k++)
                {
                    int h = 0;
                    do
                    {
                        h = random()%featurecount;
                        if(featureset.find(h) == featureset.end())
                        {
                            featureset.insert(h);
                            break;
                        }
                    }while(true);

                    doc.add(h,(float)random()/numeric_limits<int>::max());
                }
                oridata.clusteringData.push_back(doc);
            }
            oriinfo.clusteringHash = oridata.clusteringHash = i;
            oriinfo.clusteringname = ssc.str();
            oriinfo.clusteringDocNum = perdocnum;

            if(!LevelDBClusteringData::get()->save(oridata, oriinfo))
            {
                cout<<"save fail!"<<endl;
            }
       }
    }
    }
    else
    {
        LevelDBClusteringData::get()->init(HOME_STR2);
    }
}

int main(int argc, char* argv[])
{
if(argc != 2)
{
    cout<<"./test_predict istruncted(0[false], 1[ture])"<<endl;
    return 0;
}
initUsrClusterModel(atoi(argv[1]));
initUsrCluster(atoi(argv[1]));
initClustering(atoi(argv[1]));
    for(int j = 0; j < 3; j++)
    {
    pt::ptime beg = pt::microsec_clock::local_time();
    for(int i = 0; i < usernum; i++)
    {
        PerUserOnlineModel puom;
        stringstream ss;
        ss<<"test"<<i;
        bool res = lom_ptr->get(ss.str(),puom);
        ClusterContainer tnc;

        bool res2 = tncc_ptr->get(ss.str(),tnc);
        if(!res || !res2){
            cout<<"error!"<<endl;
            continue;
        }
        MinHeapDocRankType mt = getTopN(tnc,puom, 4);
//        cout<<"user:"<<ss.str()<<" predict:"<<endl;
        while(!mt.empty()){
  //          cout<<"id:"<<mt.top().doc_id_<<"rank"<<mt.top().rank_<<endl;
            mt.pop();
        }
    }
    pt::ptime end = pt::microsec_clock::local_time();
    pt::time_duration diff = end - beg;
    cout<<"cost:"<< diff.total_milliseconds()<<" usernum:"<<usernum<<"m:"<<endl;
    }
    LaserOnlineModel::get()->release();
    TopNClusterContainer::get()->release();
    LevelDBClusteringData::get()->release();
}


//BOOST_AUTO_TEST_SUITE_END()

