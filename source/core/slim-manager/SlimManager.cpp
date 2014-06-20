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
    , recommend_(NULL)
    , laser_(laser)
{
    recommend_ = new slim::SlimRecommend(laser_->indexManager_);
}

SlimManager::~SlimManager()
{
    if (recommend_ != NULL) {
        delete recommend_;
        recommend_ = NULL;
    }
}

bool SlimManager::recommend(const slim::SlimRecommendParam& param,
                            GetDocumentsByIdsActionItem& actionItem,
                            RawTextResultFromMIA& res) const
{
    std::vector<docid_t> docIdList;
    if (!recommend_->recommend(param.uuid_, param.topN_, docIdList)) {
        res.error_ = "Internal ERROR in Recommend Engine, no data for uuid: " + param.uuid_;
        return false;
    }

    for (int i = 0; i != (int)docIdList.size(); ++i) {
        actionItem.idList_.push_back(docIdList[i]);
    }

    adSearchService_->getDocumentsByIds(actionItem, res);
    return true;
}

}
