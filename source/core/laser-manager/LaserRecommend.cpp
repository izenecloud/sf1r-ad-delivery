#include "LaserRecommend.h"
#include <glog/logging.h>


namespace sf1r { namespace laser {

bool LaserRecommend::recommend(const std::string& uuid, 
    std::vector<docid_t>& itemList, 
    std::vector<float>& itemScoreList, 
    const std::size_t num) const
{
    LOG(INFO)<<"user request: uuid="<<uuid<<"\t topn="<<num;
    std::map<int, float> clustering;
    std::vector<float> model;
    if (topnClustering_->get(uuid, clustering) && laserOnlineModel_->get(uuid, model))
    {
        priority_queue queue;
        
        for (std::map<int, float>::const_iterator it = clustering.begin();
            it != clustering.end(); ++it)
        {
            AdIndexManager::ADVector advec;
            if (getAD(it->first, advec))
            {
                topN(model, advec, num, queue);
            }
        }

        while (!queue.empty())
        {
           itemList.push_back(queue.top().first);
           itemScoreList.push_back(queue.top().second);
           queue.pop();
        }
        return true;
    }
    else
    {
        return false;
    }
}

bool LaserRecommend::getAD(const std::size_t& clusteringId, AdIndexManager::ADVector& advec) const
{
    return indexManager_->get(clusteringId, advec);
}

void LaserRecommend::topN(const std::vector<float>& model, const AdIndexManager::ADVector& advec, const std::size_t n, priority_queue& queue) const
{
    for (std::size_t i = 0; i < advec.size(); ++i)
    {
        const AdIndexManager::AD& ad = advec[i];
        const docid_t& docid = ad.first;
        const std::vector<std::pair<int, float> >& vec = ad.second;
        std::vector<std::pair<int, float> >::const_iterator it = vec.begin();
        float weight = 0;
        for (; it != vec.end(); ++it)
        {
            weight += model[it->first + 1] * it->second;
        }
        weight += model[0];
        if(queue.size() >= n)
        {
            if(weight > queue.top().second)
            {
                queue.pop();
                queue.push(std::make_pair(docid, weight));
            }
        }
        else
        {
            queue.push(std::make_pair(docid, weight));
        }
    }
}

} }
