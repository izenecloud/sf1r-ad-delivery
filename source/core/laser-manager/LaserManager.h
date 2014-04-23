#ifndef SF1R_LASER_MANAGER_LASER_MANAGER_H
#define SF1R_LASER_MANAGER_LASER_MANAGER_H
#include <configuration-manager/LaserConfig.h>
#include <boost/shared_ptr.hpp>
#include <mining-manager/MiningManager.h>
#include <common/ResultType.h>

#include "service/RpcServer.h"
#include "LaserRecommend.h"

namespace sf1r
{
namespace laser
{

class LaserManager
{
public:
    LaserManager(const boost::shared_ptr<AdSearchService>& adSearchService,
        const LaserConfig& laserConfig);
    ~LaserManager();
public:
    bool recommend(const std::string& uuid, 
        RawTextResultFromMIA& itemList, 
        const std::size_t num) const; 
private:
    boost::shared_ptr<AdSearchService> adSearchService_;
    boost::shared_ptr<LaserConfig> conf_;
    boost::scoped_ptr<RpcServer> rpcServer_;
    boost::scoped_ptr<LaserRecommend> recommend_;
};

}
}

#endif
