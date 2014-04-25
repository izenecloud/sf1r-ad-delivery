#include "TopNClusterContainer.h"
#include "laser-manager/clusteringmanager/common/utils.h"
#include "laser-manager/clusteringmanager/type/LevelDBClusteringData.h"
#include <am/range/AmIterator.h>
using namespace izenelib::am;
using namespace sf1r::laser::clustering::type;
namespace sf1r
{
namespace laser
{
namespace predict
{
    TopNClusterContainer::TopNClusterContainer()
    {
    }

    TopNClusterContainer::~TopNClusterContainer()
    {
        release();
    }

    void TopNClusterContainer::output()
    {
        if(topNclusterLeveldb_!= NULL && topNclusterLeveldb_->open())
        {
            typedef AMIterator<TopNclusterLeveldbType > AMIteratorType;
            AMIteratorType iter(*topNclusterLeveldb_);
            AMIteratorType end;
            int iterStep = 0;
            for(; iter != end; ++iter)
            {
                cout<<"usr:"<<iter->first<<endl;
                for(ClusterContainer::const_iterator miter = iter->second.begin(); miter != iter->second.end(); miter++)
                {
                    cout<<"cluster id:"<<miter->first<<"\t"<<" value:"<<miter->second<<endl;
                }
                ++iterStep;
            }
        }
    }
    bool TopNClusterContainer::init(std::string topNClusterPath)
    {
        topNClusterPath_ = topNClusterPath+"./topncluster";
        topNclusterLeveldb_ = new TopNclusterLeveldbType(topNClusterPath_);
        if(topNclusterLeveldb_->open())
        {
            return true;
        }
        else
        {
            release();
            return false;
        }
    }

    bool TopNClusterContainer::release()
    {
        if (NULL != topNclusterLeveldb_)
        {
            topNclusterLeveldb_->close();
            delete topNclusterLeveldb_;
            topNclusterLeveldb_ = NULL;
        }
        return true;
    }

    bool TopNClusterContainer::update(const TopNCluster& cluster)
    {
        if(cluster.getUserName().compare("")!=0)
        {
            return topNclusterLeveldb_->insert(cluster.getUserName(), cluster.getTopNCluster());
        }
        return false;

    }

    bool TopNClusterContainer::get(const std::string& user, ClusterContainer& cluster)
    {
        bool res = topNclusterLeveldb_->get(user, cluster);
        return res;
    }
}
}
}
