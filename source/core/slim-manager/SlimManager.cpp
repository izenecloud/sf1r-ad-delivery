#include "SlimManager.h"
#include "SlimRecommend.h"

namespace sf1r {

SlimManager::SlimManager(const boost::shared_ptr<AdSearchService>& adSearchService,
                         const boost::shared_ptr<DocumentManager>& documentManager,
                         const std::string& collection,
                         LaserManager* laser)
    : collection_(collection)
    , adSearchService_(adSearchService)
    , documentManager_(documentManager)
    , laser_(laser)
{
    rpcServer_ = new slim::SlimRpcServer(_similar_cluster, _similar_tareid, _rw_mutex);
    rpcServer_->start("127.0.0.1", 38611, 2);

    recommend_ = new slim::SlimRecommend(laser_->indexManager_,
                                         laser_->tokenizer_,
                                         _similar_cluster,
                                         _similar_tareid,
                                         _rw_mutex,
                                         laser);
}

SlimManager::~SlimManager()
{
    if (recommend_ != NULL) {
        delete recommend_;
        recommend_ = NULL;
    }

    if (rpcServer_ != NULL) {
        rpcServer_->stop();
        delete rpcServer_;
        rpcServer_ = NULL;
    }
}

bool SlimManager::recommend(const slim::SlimRecommendParam& param,
                            GetDocumentsByIdsActionItem& actionItem,
                            RawTextResultFromMIA& res) const
{
    std::vector<docid_t> docIdList;
    if (!recommend_->recommend(param.title_, param.id_, param.topn_, docIdList)) {
        res.error_ = "Internal ERROR in Recommend Engine";
        return false;
    }

    for (int i = 0; i != (int)docIdList.size(); ++i) {
        actionItem.idList_.push_back(docIdList[i]);
    }

    adSearchService_->getDocumentsByIds(actionItem, res);
    return true;
}

}
