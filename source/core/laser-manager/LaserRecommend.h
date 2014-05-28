#ifndef SF1R_LASER_RECOMMEND_H
#define SF1R_LASER_RECOMMEND_H
#include <string>
#include <vector>
#include "AdIndexManager.h"
#include "TopNClusteringDB.h"
#include "LaserModelDB.h"
#include <util/functional.h>
#include <common/inttypes.h>
#include <queue>

namespace sf1r { namespace laser {
class LaserRecommend
{
typedef izenelib::util::second_greater<std::pair<docid_t, float> > greater_than;
typedef std::priority_queue<std::pair<docid_t, float>, std::vector<std::pair<docid_t, float> >, greater_than> priority_queue;

public:
    LaserRecommend(const AdIndexManager* index,
        const TopNClusteringDB* topnClustering,
        const LaserModelDB* laserOnlineModel,
        const std::vector<std::vector<int> >* similarClustering)
        : indexManager_(index)
        , topnClustering_(topnClustering)
        , laserOnlineModel_(laserOnlineModel)
        , similarClustering_(similarClustering)
    {
    }

public:
    bool recommend(const std::string& uuid, 
        std::vector<docid_t>& itemList, 
        std::vector<float>& itemScoreList, 
        const std::size_t num) const;
private:
    bool getAD(const std::size_t& clusteringId, AdIndexManager::ADVector& advec) const;
    bool getSimilarAd(const std::size_t& clusteringId, AdIndexManager::ADVector& advec) const;
    void getSimilarClustering(const std::size_t& clusteringId, std::vector<int>& idvec) const;

    void topN(const std::vector<float>& model, 
        const float offset, 
        const AdIndexManager::ADVector& advec, 
        const std::size_t n, 
        priority_queue& queue) const;
private:
    const AdIndexManager* indexManager_;
    const TopNClusteringDB* topnClustering_;
    const LaserModelDB* laserOnlineModel_;
    const std::vector<std::vector<int> >* similarClustering_;
};

} }

#endif
