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

void loadLaserDependency();
void closeLaserDependency();
class LaserManager
{
    typedef std::vector<float> TokenVector;
    typedef std::vector<std::pair<int, float> > TokenSparseVector;
public:
    LaserManager(const boost::shared_ptr<AdSearchService>& adSearchService, 
        const boost::shared_ptr<DocumentManager>& documentManager,
        const std::string& collection);
    ~LaserManager();
public:
    bool recommend(const laser::LaserRecommendParam& param, 
        GetDocumentsByIdsActionItem& actionItem,
        RawTextResultFromMIA& itemList) const;
    void index(const docid_t& docid, const std::string& title);

    MiningTask* getLaserIndexTask();

    const std::size_t getClustering(const std::string& title) const;

    void deleteDocuments(const std::vector<docid_t>& lastDeletedDocIdList);

    friend void loadLaserDependency();
    friend void closeLaserDependency();
private:
    std::size_t assignClustering_(const TokenSparseVector& v) const;
    float similarity_(const TokenSparseVector& lv, const TokenVector& rv) const;


    friend class laser::LaserIndexTask;
    friend class laser::LaserRpcServer;
private:
    const std::string collection_;
    boost::shared_ptr<AdSearchService> adSearchService_;
    const boost::shared_ptr<DocumentManager>& documentManager_;
    laser::LaserRecommend* recommend_;
    laser::AdIndexManager* indexManager_;
    laser::LaserIndexTask* indexTask_;

    
    static std::string workdir_;
    static std::vector<TokenVector>* clusteringContainer_;
    static std::vector<std::vector<int> >* similarClustering_;
    static laser::Tokenizer* tokenizer_;
    static laser::LaserRpcServer* rpcServer_;
    static laser::TopNClusteringDB* topnClustering_;
    static laser::LaserOnlineModelDB* laserOnlineModel_;
    static boost::shared_mutex mutex_;
};

}

#endif
