#ifndef SF1R_LASER_MANAGER_LASER_MANAGER_H
#define SF1R_LASER_MANAGER_LASER_MANAGER_H
#include <boost/shared_ptr.hpp>
#include <mining-manager/MiningManager.h>
#include <mining-manager/MiningTask.h>
#include <common/ResultType.h>
#include <ir/id_manager/IDManager.h>

#include "LaserRecommend.h"
#include "LaserRecommendParam.h"
#include "AdIndexManager.h"
#include "Tokenizer.h"
#include "TopNClusteringDB.h"
#include "LaserOnlineModel.h"
#include "LaserOfflineModel.h"
#include "clusteringmanager/type/TermDictionary.h"

namespace sf1r { namespace laser {
class LaserIndexTask;
class LaserRpcServer;
class LaserRecommend;
class LaserModelFactory;
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
        izenelib::ir::idmanager::IDManager& idManager,
        const std::string& collectionDataPath,
        const std::string& collection,
        const LaserPara& para);
    ~LaserManager();
public:
    bool recommend(const laser::LaserRecommendParam& param, 
        GetDocumentsByIdsActionItem& actionItem,
        RawTextResultFromAIA& itemList) const;
    void index(const docid_t& docid, const std::string& title);

    MiningTask* getLaserIndexTask();

    const std::size_t getClustering(const std::string& title) const;

    bool convertDocId(const std::string& docStr, docid_t& docId) const;

    void getAdInfoById(const std::string& DOCID, 
        std::string& adId,
        std::string& clusteringId,
        std::vector<int>& index, 
        std::vector<float>& value) const;
    
    void tokenize(const std::string& text, 
        std::vector<int>& index, 
        std::vector<float>& value) const;

    const std::vector<std::vector<float> >& getClustering() const
    {
        return *clusteringContainer_;
    }

    friend void loadLaserDependency();
    friend void closeLaserDependency();
private:
    std::size_t assignClustering_(const TokenSparseVector& v) const;
    float similarity_(const TokenSparseVector& lv, const TokenVector& rv) const;

    bool isNeedClusteringKnowlege() const;
    void loadClusteringKnowledge();

    friend class laser::LaserIndexTask;
    friend class laser::LaserRpcServer;
    friend class laser::LaserRecommend;
    friend class laser::LaserModelFactory;
    friend class laser::AdIndexManager;
private:
    const std::string collection_;
    const std::string workdir_;
    const std::string sysdir_;
    std::string resdir_;
    const LaserPara& para_;
    const boost::shared_ptr<AdSearchService>& adSearchService_;
    const boost::shared_ptr<DocumentManager>& documentManager_;
    mutable izenelib::ir::idmanager::IDManager& idManager_;
    laser::LaserRecommend* recommend_;
    laser::AdIndexManager* indexManager_;
    laser::LaserIndexTask* indexTask_;
    
    laser::Tokenizer* tokenizer_;
    std::vector<TokenVector>* clusteringContainer_;
    std::vector<std::vector<int> >* similarClustering_;
    
    static laser::LaserRpcServer* rpcServer_;
    static boost::shared_mutex mutex_;
};

}

#endif
