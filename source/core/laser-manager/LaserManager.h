#ifndef SF1R_LASER_MANAGER_LASER_MANAGER_H
#define SF1R_LASER_MANAGER_LASER_MANAGER_H
#include <boost/shared_ptr.hpp>
#include <mining-manager/MiningManager.h>
#include <mining-manager/MiningTask.h>
#include <common/ResultType.h>

#include "LaserRecommend.h"
#include "LaserRecommendParam.h"
#include "AdIndexManager.h"
#include "Tokenizer.h"
#include "TopNClusteringDB.h"
#include "clusteringmanager/type/TermDictionary.h"

namespace sf1r { namespace laser {
class LaserIndexTask;
class LaserRpcServer;
} }
namespace sf1r {

class LaserManager
{
    typedef std::vector<float> TokenVector;
    typedef std::vector<std::pair<int, float> > TokenSparseVector;
public:
    LaserManager(const boost::shared_ptr<AdSearchService>& adSearchService, const std::string& collection);
    ~LaserManager();
public:
    bool recommend(const laser::LaserRecommendParam& param, 
        GetDocumentsByIdsActionItem& actionItem,
        RawTextResultFromMIA& itemList) const;
    void index(const docid_t& docid, const std::string& title);

    MiningTask* getLaserIndexTask();

    const std::size_t getClustering(const std::string& title) const;
private:
    std::size_t assignClustering_(const TokenSparseVector& v) const;
    float similarity_(const TokenSparseVector& lv, const TokenVector& rv) const;

    void load_();
    void close_();

    friend class laser::LaserIndexTask;
    friend class laser::LaserRpcServer;
private:
    const std::string workdir_;
    const std::string collection_;
    boost::shared_ptr<AdSearchService> adSearchService_;
    boost::scoped_ptr<laser::LaserRecommend> recommend_;
    boost::scoped_ptr<laser::AdIndexManager> indexManager_;
    boost::shared_ptr<laser::LaserIndexTask> indexTask_;

    
    static std::vector<TokenVector>* clusteringContainer_;
    static laser::Tokenizer* tokenizer_;
    static laser::LaserRpcServer* rpcServer_;
    static laser::TopNClusteringDB* topnClustering_;
    static laser::LaserOnlineModelDB* laserOnlineModel_;
    static boost::shared_mutex mutex_;
};

}

#endif
