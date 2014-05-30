#include "MiningSearchService.h"
#include <mining-manager/MiningManager.h>
#include <node-manager/DistributeRequestHooker.h>
#include <node-manager/MasterManagerBase.h>
#include <node-manager/sharding/ShardingStrategy.h>
#include <ad-manager/AdIndexManager.h>

#include <util/driver/Request.h>
#include <aggregator-manager/SearchWorker.h>

using namespace izenelib::driver;
namespace sf1r
{

MiningSearchService::MiningSearchService()
{
}

MiningSearchService::~MiningSearchService()
{
}

bool MiningSearchService::HookDistributeRequestForSearch(const std::string& coll, uint32_t workerId)
{
    Request::kCallType hooktype = (Request::kCallType)DistributeRequestHooker::get()->getHookType();
    if (hooktype == Request::FromAPI)
    {
        // from api do not need hook, just process as usually.
        return true;
    }
    const std::string& reqdata = DistributeRequestHooker::get()->getAdditionData();
    bool ret = false;
    if (hooktype == Request::FromDistribute)
    {
        searchAggregator_->singleRequest(coll, 0, "HookDistributeRequestForSearch", (int)hooktype, reqdata, ret, workerId);
    }
    else
    {
        ret = true;
    }
    if (!ret)
    {
        LOG(WARNING) << "Request failed, HookDistributeRequestForSearch failed.";
    }
    return ret;
}

bool MiningSearchService::getSearchResult(
    const KeywordSearchActionItem& actionItem,
    KeywordSearchResult& resultItem
)
{
    return false;
}

// xxx
bool MiningSearchService::visitDoc(const uint128_t& scdDocId)
{
    uint64_t internalId = 0;
    searchWorker_->getInternalDocumentId(scdDocId, internalId);
    bool ret = true;
    searchWorker_->visitDoc(internalId, ret);
    return ret;
}

bool MiningSearchService::visitDoc(const std::string& collectionName, uint64_t wdocId)
{
    std::pair<sf1r::workerid_t, sf1r::docid_t> wd = net::aggregator::Util::GetWorkerAndDocId(wdocId);
    sf1r::workerid_t workerId = wd.first;
    sf1r::docid_t docId = wd.second;
    bool ret = true;

    bool is_worker_self = searchAggregator_->isLocalWorker(workerId);
    if (is_worker_self || !bundleConfig_->isMasterAggregator_ || !searchAggregator_->isNeedDistribute())
    {
        searchWorker_->visitDoc(docId, ret);
    }
    else
    {
        if(!HookDistributeRequestForSearch(collectionName, workerId))
            return false;
        ret = true;
        //searchAggregator_->singleRequest(collectionName, "visitDoc", docId, ret, workerId);
        //if (ret)
        //{
        //    LOG(INFO) << "send to remote worker to visitDoc success, clear local hook. remote: " << workerId;
        //    DistributeRequestHooker::get()->processLocalFinished(true);
        //}
    }

    return ret;
}

bool MiningSearchService::clickGroupLabel(
        const std::string& query,
        const std::string& propName,
        const std::vector<std::string>& groupPath)
{
    return miningManager_->clickGroupLabel(query, propName, groupPath);
}

bool MiningSearchService::getFreqGroupLabel(
        const std::string& query,
        const std::string& propName,
        int limit,
        std::vector<std::vector<std::string> >& pathVec,
        std::vector<int>& freqVec)
{
    return miningManager_->getFreqGroupLabel(query, propName, limit, pathVec, freqVec);
}

bool MiningSearchService::setTopGroupLabel(
        const std::string& query,
        const std::string& propName,
        const std::vector<std::vector<std::string> >& groupLabels)
{
    bool result = miningManager_->setTopGroupLabel(query,
                                                   propName, groupLabels);
    if (result)
    {
        searchWorker_->clearSearchCache();
    }

    return result;
}

bool MiningSearchService::setCustomRank(
        const std::string& query,
        const CustomRankDocStr& customDocStr)
{
    bool result = miningManager_->setCustomRank(query, customDocStr);

    searchWorker_->clearSearchCache();

    return result;
}

bool MiningSearchService::getCustomRank(
        const std::string& query,
        std::vector<Document>& topDocList,
        std::vector<Document>& excludeDocList)
{
    return miningManager_->getCustomRank(query, topDocList, excludeDocList);
}

bool MiningSearchService::getCustomQueries(std::vector<std::string>& queries)
{
    return miningManager_->getCustomQueries(queries);
}

bool MiningSearchService::setMerchantScore(const MerchantStrScoreMap& scoreMap)
{
    bool result = miningManager_->setMerchantScore(scoreMap);

    searchWorker_->clearSearchCache();

    return result;
}

bool MiningSearchService::getMerchantScore(
        const std::vector<std::string>& merchantNames,
        MerchantStrScoreMap& merchantScoreMap) const
{
    return miningManager_->getMerchantScore(merchantNames, merchantScoreMap);
}

bool MiningSearchService::GetProductScore(
        const std::string& docIdStr,
        const std::string& scoreType,
        score_t& scoreValue)
{
    return miningManager_->getProductScore(docIdStr, scoreType, scoreValue);
}

bool MiningSearchService::setKeywordBidPrice(const std::string& keyword, const std::string& campaign_name, double bidprice)
{
    LOG(INFO) << "keyword bid price setting: " << keyword << ":" << bidprice << " for campaign: " << campaign_name;
    if (!miningManager_->getAdIndexManager())
    {
        LOG(WARNING) << "ad manager not enabled.";
        return false;
    }
    return miningManager_->getAdIndexManager()->setKeywordBidPrice(keyword, campaign_name, bidprice * 100);
}

bool MiningSearchService::setAdCampaignBudget(const std::string& campaign_name, double budget)
{
    LOG(INFO) << "ad campaign budget setting: " << campaign_name << ":" << budget;
    if (!miningManager_->getAdIndexManager())
    {
        LOG(WARNING) << "ad manager not enabled.";
        return false;
    }

    return miningManager_->getAdIndexManager()->setAdCampaignBudget(campaign_name, budget * 100);
}

bool MiningSearchService::setAdBidPhrase(const std::string& ad_strid, const std::vector<std::string>& bid_phrase_list)
{
    LOG(INFO) << "ad bid phrase setting: " << ad_strid << ":";
    for(std::size_t i = 0; i < bid_phrase_list.size(); ++i) 
    {
        LOG(INFO) << bid_phrase_list[i];
    }
    if (!miningManager_->getAdIndexManager())
    {
        LOG(WARNING) << "ad manager not enabled.";
        return false;
    }

    return miningManager_->getAdIndexManager()->setAdBidPhrase(ad_strid, bid_phrase_list);
}

}
