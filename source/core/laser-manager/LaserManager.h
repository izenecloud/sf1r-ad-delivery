#ifndef SF1R_LASER_MANAGER_LASER_MANAGER_H
#define SF1R_LASER_MANAGER_LASER_MANAGER_H
#include <configuration-manager/LaserConfig.h>
#include <boost/shared_ptr.hpp>
#include <mining-manager/MiningManager.h>
#include <common/ResultType.h>

#include "service/RpcServer.h"
#include "LaserRecommend.h"
#include "LaserRecommendParam.h"

namespace sf1r
{

class LaserManager
{
public:
    LaserManager(const boost::shared_ptr<AdSearchService>& adSearchService,
        const LaserConfig& laserConfig);
    ~LaserManager();
public:
    bool recommend(const laser::LaserRecommendParam& param, 
        RawTextResultFromMIA& itemList) const; 
private:
    boost::shared_ptr<AdSearchService> adSearchService_;
    const LaserConfig& conf_;
    boost::shared_ptr<laser::RpcServer> rpcServer_;
    boost::scoped_ptr<laser::LaserRecommend> recommend_;
};

}

#endif
