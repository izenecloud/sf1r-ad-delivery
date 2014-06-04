#include "HierarchicalModel.h"
#include "LaserOnlineModel.h"
#include "LaserOfflineModel.h"
#include "AdIndexManager.h"
#include <util/functional.h>
#include <algorithm>    // std::sort
    
namespace sf1r { namespace laser {

HierarchicalModel::HierarchicalModel(const AdIndexManager& adIndexer, 
    const std::string& workdir,
    const std::size_t ncandidate)
    : LaserGenericModel(adIndexer, workdir)
    , workdir_(workdir)
    , ncandidate_(ncandidate)
    , pClusteringDb_(NULL)
{
    if (!boost::filesystem::exists(workdir_))
    {
        boost::filesystem::create_directory(workdir_);
    }
    pClusteringDb_ = new LaserModelDB<std::size_t, LaserOnlineModel>(workdir_ + "per-clustering-online-model");
}

HierarchicalModel::~HierarchicalModel()
{
    if (NULL != pClusteringDb_)
    {
        delete pClusteringDb_;
        pClusteringDb_ = NULL;
    }
}

bool HierarchicalModel::candidate(
    const std::string& text,
    const std::vector<std::pair<int, float> >& context, 
    std::vector<std::pair<docid_t, std::vector<std::pair<int, float> > > >& ad,
    std::vector<float>& score) const
{
    LaserModelDB<std::size_t, LaserOnlineModel>::iterator it = pClusteringDb_->begin();
    std::vector<std::pair<std::size_t, float> > clustering;
    for (; it != pClusteringDb_->end(); ++it)
    {
        LaserOnlineModel pClustering;
        if (pClusteringDb_->get(it->first, pClustering))
        {
            static const std::pair<docid_t, std::vector<std::pair<int, float> > > perAd;
            
            // ad empty, per-clustering convert to per ad
            clustering.push_back(std::make_pair(it->first, pClustering.score(text, context, perAd, (float)0.0)));
        }
    }
    
    typedef izenelib::util::second_greater<std::pair<std::size_t, float> > greater_than;
    std::sort(clustering.begin(), clustering.end());
    for (std::size_t i = 0; i < clustering.size(); ++i)
    {
        adIndexer_.get(clustering[i].first, ad);
        if (ad.size() >= ncandidate_)
            break;
    }
    score.assign(ad.size(), 0);
    return true;
}

} }
