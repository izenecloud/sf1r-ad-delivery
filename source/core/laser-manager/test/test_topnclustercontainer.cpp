#include <iostream>
#include <string>
#include "LaserOnlineModel.h"
#include "PerUserOnlineModel.h"
#include "TopNClusterContainer.h"
#include <vector>
#include <sstream>
#include <boost/filesystem.hpp>
namespace bfs = boost::filesystem;

using namespace std;
using namespace clustering;
using namespace predict;
string TESTHOME="./testtopnclustercontainer";
void test()
{
    bfs::remove_all(TESTHOME);
    TopNClusterContainer* tncc_ptr = TopNClusterContainer::get();
    tncc_ptr->init(TESTHOME);
    for(int i = 0; i < 10; i++)
    {
        TopNCluster tnc;
        ClusterContainer cc;
        cc.insert(make_pair<clustering::hash_t, float>(i, i*10));
        cc.insert(make_pair<clustering::hash_t, float>(i+1, i*20));
        stringstream ss;
        ss<<"test"<<i;
        tnc.setUserName(ss.str());
        tnc.setClusterContainer(cc);
        tncc_ptr->update(tnc);
    }
    tncc_ptr->release();
    tncc_ptr->init(TESTHOME);
    for(int i = 0; i < 10; i++)
    {
        ClusterContainer tnc;
        stringstream ss;
        ss<<"test"<<i;
        bool res = tncc_ptr->get(ss.str(),tnc);
        cout<<ss.str()<<" :"<<res;
        for(ClusterContainerIter iter = tnc.begin();
            iter!= tnc.end(); iter++)
            {
                cout<<"key:"<<iter->first<<" value:"<<iter->second;

            }
        cout<<endl;
    }
    tncc_ptr->release();
}
int main()
{
    test();
}
