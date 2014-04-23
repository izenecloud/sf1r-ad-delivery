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
using namespace std;
using namespace leveldb;
using namespace clustering::type;
using namespace clustering;
namespace bfs = boost::filesystem;
string HOME_STR = "/opt/clustering/leveldb";
string HOME_STR2 = "/opt/clustering/leveldb2";

void test()
{
    leveldb::DB* db;
    leveldb::Options options;

    //options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options, HOME_STR, &db);
    if(!status.ok())
    {
        cout<<status.ToString()<<endl;
        return ;
    }
    string value;
    std::string catname = "cattest";
    hash_t cathash = Hash_(catname);
    stringstream ss;
    ss<<"clusteringdata_"<<cathash;

    db->Get(leveldb::ReadOptions(), ss.str(), &value);
    //cout<<"value"<<value;
    msgpack::unpacked msg;
    ClusteringData newdata;
    msgpack::unpack(&msg, value.data(), value.size());
    msg.get().convert(&newdata);
    cout<<newdata.clusteringData.size()<<endl;
    newdata.output();
    delete db;
}
//test the basic operation of LevelDBClusteringData
void test2()
{
    bfs::remove_all(HOME_STR2);
    LevelDBClusteringData levelDBClusteringData;

    if(levelDBClusteringData.init(HOME_STR2))
    {
        ClusteringData oridata;
        ClusteringInfo oriinfo;
        Document doc("doctest");
        doc.add(1,1.1);
        doc.add(2,2.2);
        Document doc2("doctest2");
        doc2.add(1,2.1);
        doc2.add(2,3.2);
        oridata.clusteringData.push_back(doc);
        oridata.clusteringData.push_back(doc2);
        std::string catname = "cattest";
        hash_t cathash = Hash_(catname);
        oriinfo.clusteringDocNum = 2;
        oriinfo.clusteringHash = oridata.clusteringHash = cathash;
        oriinfo.clusteringname = catname;
        cout<<"to save the data"<<endl;
        if(!levelDBClusteringData.save(oridata, oriinfo))
        {
            cout<<"save fail!"<<endl;
        }
        cout<<"to read the data"<<endl;
        ClusteringData newdata;
        ClusteringInfo newinfo;
        if(!levelDBClusteringData.loadClusteringData(cathash, newdata))
        {
            cout<<"load leveldb clustering data fail!"<<endl;
        }
        else
        {
            newdata.output();
        }

        if(!levelDBClusteringData.loadClusteringInfo(cathash, newinfo))
        {
            cout<<"load leveldb clustering info fail!"<<endl;
        }

        levelDBClusteringData.release();
    }
}

void test4()
{
    // bfs::remove_all(HOME_STR2);
    LevelDBClusteringData levelDBClusteringData;

    if(levelDBClusteringData.init(HOME_STR2))
    {
        //ClusteringData oridata;
        //ClusteringInfo oriinfo;
        //vector<int> clusteringArray;
        srand(time(NULL));
        time_t beg = time(0);
//        for(int i = 0; i < 20000; i++)
//        {
//            if(i%1000==0){
//                cout<<"save round"<<i<<endl;
//            }
//            ClusteringData oridata;
//            ClusteringInfo oriinfo;
//            int max = rand()%450+50;
//            stringstream st;
//            st<<i;
//            for(int j = 0; j < max; j++)
//            {
//                stringstream ss;
//                ss<<i<<"--"<<j;
//                int maxfeature = rand()%5+2;
//                Document doc(ss.str());
//                for(int k = 0; k < maxfeature; k++){
//                    doc.add(rand()%10000, rand()%1000);
//                }
//                oridata.clusteringData.push_back(doc);
//            }
//            oriinfo.clusteringDocNum = max;
//            oriinfo.clusteringHash = oridata.clusteringHash = i;
//            oriinfo.clusteringname = st.str();
//             if(!levelDBClusteringData.save(oridata, oriinfo))
//            {
//                cout<<"save fail!"<<endl;
//            }
//
//        }
        time_t end = time(0);
        cout<<"save data spend"<<(end - beg)<<endl;
        cout<<"to read the data"<<endl;

        ClusteringInfo newinfo;
        beg = time(0);
        int loadtime = 200;
        for(int i = 0; i < loadtime; i++)
        {
            ClusteringData newdata;
            hash_t cathash = rand()%20000;
            if(!levelDBClusteringData.loadClusteringData(cathash, newdata))
            {
                cout<<"load leveldb clustering data fail!"<<endl;
            }
            else
            {
                newdata.output();
            }
        }
        end = time(0);
        cout<<"load data spend "<<(end - beg)<< " average "<<((float)(end-beg))/loadtime<<endl;
        //        else
//        {
//            newdata.output();
//        }
//
//        if(!levelDBClusteringData.loadClusteringInfo(cathash, newinfo))
//        {
//            cout<<"load leveldb clustering info fail!"<<endl;
//        }

        levelDBClusteringData.release();
    }
}

void testTravelData()
{
    leveldb::DB* db;
    leveldb::Options options;

    //options.create_if_missing = true;
    //HOME_STR = "/opt/clustering/leveldb";
    leveldb::Status status = leveldb::DB::Open(options, HOME_STR2, &db);
    if(!status.ok())
    {
        cout<<status.ToString()<<endl;
        return ;
    }
    string value;
    //cout<<"value"<<value;
    leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
    for (it->SeekToFirst(); it->Valid(); it->Next())
    {
        if(it->key().ToString().find("data") == string::npos)
        {
            continue;
        }
        cout << it->key().ToString() << ": "  << endl;

        string value = it->value().ToString() ;
        cout<<"value size:"<<value.size()<<endl;
        ClusteringData newdata;
        deserialize<ClusteringData>(value, newdata);
        cout<<newdata.clusteringData.size()<<endl;
        newdata.output();

    }
    delete it;
    delete db;
}
void testTravelInfo()
{
    leveldb::DB* db;
    leveldb::Options options;

    //options.create_if_missing = true;
    //HOME_STR = "/opt/clustering/leveldb";
    time_t beg = time(0);
    leveldb::Status status = leveldb::DB::Open(options, HOME_STR, &db);
    if(!status.ok())
    {
        cout<<status.ToString()<<endl;
        return ;
    }
    string value;
    leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
    for (it->SeekToFirst(); it->Valid(); it->Next())
    {
        if(it->key().ToString().find("info") == string::npos)
        {
            continue;
        }
        cout << it->key().ToString() << ": "  << endl;
        string value = it->value().ToString() ;
        cout<<"value size:"<<value.size()<<endl;
        ClusteringInfo newinfo;
        deserialize<ClusteringInfo>(value, newinfo);
        cout<<newinfo.clusteringDocNum<< "pow size:"<<newinfo.clusteringPow.size()<<endl;
        //   newdata.output();
        time_t end = time(0);
        cout<<"testTravelInfo spend"<<(end - beg)<<endl;
    }
    delete it;
    delete db;
}
int main()
{
    //test4();
    testTravelInfo();
}


//BOOST_AUTO_TEST_SUITE_END()

