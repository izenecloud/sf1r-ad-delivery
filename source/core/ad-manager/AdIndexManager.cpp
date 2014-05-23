/*
 *  AdIndexManager.cpp
 */

#include "AdIndexManager.h"
#include "AdStreamSubscriber.h"
#include "AdSelector.h"
#include "AdClickPredictor.h"
#include "AdFeedbackMgr.h"
#include "AdSearchService.h"
#include "sponsored-ad-search/AdSponsoredMgr.h"
#include <common/ResultType.h>
#include <mining-manager/group-manager/GroupManager.h>
#include <query-manager/ActionItem.h>
#include <query-manager/SearchKeywordOperation.h>
#include <search-manager/SearchBase.h>
#include <search-manager/HitQueue.h>
#include <util/ustring/UString.h>
#include <algorithm>


namespace sf1r
{

static const int MAX_SEARCH_AD_COUNT = 20000;
static const int MAX_SELECT_AD_COUNT = 20;
static const int MAX_RECOMMEND_ITEM_NUM = 10;
static const std::string adlog_topic = "log_b5t_sf1";

using namespace sponsored;

AdIndexManager::AdIndexManager(
        const std::string& ad_resource_path,
        const std::string& ad_data_path,
        bool enable_ad_selector,
        bool enable_ad_rec,
        bool enable_ad_sponsored_search,
        const std::string& dmp_ip,
        uint16_t dmp_port,
        const std::string& stream_log_ip,
        uint16_t stream_log_port,
        boost::shared_ptr<DocumentManager>& dm,
        izenelib::ir::idmanager::IDManager* id_manager,
        NumericPropertyTableBuilder* ntb,
        AdSearchService* searcher,
        faceted::GroupManager* grp_mgr)
    : indexPath_(ad_data_path + "/index.bin"),
      clickPredictorWorkingPath_(ad_data_path + "/ctr_predictor"),
      ad_res_path_(ad_resource_path),
      ad_data_path_(ad_data_path),
      enable_ad_selector_(enable_ad_selector),
      enable_ad_rec_(enable_ad_rec),
      enable_ad_sponsored_search_(enable_ad_sponsored_search),
      documentManager_(dm),
      id_manager_(id_manager),
      numericTableBuilder_(ntb),
      ad_searcher_(searcher),
      groupManager_(grp_mgr)
{
    AdFeedbackMgr::get()->init(dmp_ip, dmp_port, ad_resource_path + "/feedback-schema.json");
    AdStreamSubscriber::get()->init(stream_log_ip, stream_log_port);
}

AdIndexManager::~AdIndexManager()
{
    // unsubscribe should make sure all callback finished and 
    // no any callback will be send later.
    AdStreamSubscriber::get()->unsubscribe(adlog_topic);

    if(adMiningTask_)
        delete adMiningTask_;

    if (ad_click_predictor_)
        ad_click_predictor_->stop();

    AdStreamSubscriber::get()->stop();
}

bool AdIndexManager::buildMiningTask()
{
    ad_dnf_index_.reset(new AdDNFIndexType());

    ad_click_predictor_ = AdClickPredictor::get();
    ad_click_predictor_->init(clickPredictorWorkingPath_);

    std::ifstream ifs(indexPath_.c_str(), std::ios_base::binary);
    if(ifs.good())
    {
        try
        {
            boost::unique_lock<boost::shared_mutex> lock(rwMutex_);
            ad_dnf_index_->load_binary(ifs);
        }
        catch(const std::exception& e)
        {
            LOG(ERROR) << "exception in read file: " << e.what()
                << ", path: "<< indexPath_<<endl;
        }
    }

    if (enable_ad_selector_)
    {
        ad_selector_.reset(new AdSelector());
        ad_selector_->init(ad_res_path_ + "/ad_selector", ad_data_path_ + "/ad_selector",
            ad_data_path_ + "/ad_rec", enable_ad_rec_, ad_click_predictor_, groupManager_,
            documentManager_.get());
    }

    if (enable_ad_sponsored_search_)
    {
        ad_sponsored_mgr_.reset(new AdSponsoredMgr());
        ad_sponsored_mgr_->init(ad_res_path_ + "/ad_sponsored", ad_data_path_ + "ad_sponsored",
            groupManager_, documentManager_.get(), id_manager_, ad_searcher_);
    }

    adMiningTask_ = new AdMiningTask(indexPath_, documentManager_, ad_dnf_index_, ad_selector_, rwMutex_);
    adMiningTask_->setPostProcessFunc(boost::bind(&AdIndexManager::postMining, this, _1, _2));

    bool ret = AdStreamSubscriber::get()->subscribe(adlog_topic, boost::bind(&AdIndexManager::onAdStreamMessage, this, _1));
    if (!ret)
    {
        LOG(ERROR) << "subscribe the click log failed !!!!!!";
    }

    return true;
}

void AdIndexManager::onAdStreamMessage(const std::vector<AdMessage>& msg_list)
{
    static int cnt = 0;
    if (cnt % 10000 == 0)
    {
        for (size_t i = 0; i < msg_list.size(); ++i)
        {
            LOG(INFO) << "stream data: " << msg_list[i].body;
            // print only first log
            break;
        }
        LOG(INFO) << "got ad stream data. size: " << msg_list.size() << ", total " << cnt;
    }
    cnt += msg_list.size();
    AdFeedbackMgr::FeedbackInfo feedback_info;
    AdClickPredictor::AssignmentT ad_feature;
    AdClickPredictor::AssignmentT user_feature;
    // read from stream msg and convert it to assignment list.
    for (size_t i = 0; i < msg_list.size(); ++i)
    {
        bool ret = AdFeedbackMgr::get()->parserFeedbackLog(msg_list[i].body, feedback_info);
        if (!ret)
            continue;

        std::map<std::string, std::map<std::string, double> >::const_iterator it = feedback_info.user_profiles.profile_data.begin();
        user_feature.clear();
        for(; it != feedback_info.user_profiles.profile_data.end(); ++it)
        {
            std::map<std::string, double>::const_iterator it2 = it->second.begin();
            for(; it2 != it->second.end(); ++it2)
            {
                user_feature.push_back(std::make_pair(it->first, it2->first));
            }
        }
        bool is_clicked = feedback_info.action > AdFeedbackMgr::View;
        if (enable_ad_sponsored_search_)
        {
            if (is_clicked && !feedback_info.ad_id.empty())
            {
                ad_sponsored_mgr_->updateAuctionLogData(feedback_info.ad_id,
                    feedback_info.click_cost, feedback_info.click_slot);
            }
        }
        if (enable_ad_selector_)
        {
            docid_t docid = 0;
            uint128_t num_docid = Utilities::md5ToUint128(feedback_info.ad_id);
            bool ret = id_manager_->getDocIdByDocName(num_docid, docid, false);
            if (ret)
            {
                ad_selector_->getAdFeatureList(docid, ad_feature);
                ad_click_predictor_->update(user_feature, ad_feature,
                    is_clicked);
            }
            if (is_clicked)
            {
                if (!feedback_info.ad_id.empty())
                {
                    ad_selector_->updateClicked(docid);
                }
            }
            ad_selector_->updateSegments(user_feature, AdSelector::UserSeg);
            if (enable_ad_rec_)
            {
                ad_selector_->trainOnlineRecommender(feedback_info.user_id,
                    user_feature, feedback_info.ad_id, is_clicked);
            }
        }
    }
}

void AdIndexManager::postMining(docid_t startid, docid_t endid)
{
    LOG(INFO) << "ad mining finished from: " << startid << " to " << endid;
    if (ad_selector_)
    {
        ad_selector_->miningAdSegmentStr(startid, endid);
    }
    if (ad_sponsored_mgr_)
    {
        ad_sponsored_mgr_->miningAdCreatives(startid, endid);
    }
}

void AdIndexManager::rankAndSelect(const FeatureT& userinfo,
    std::vector<docid_t>& docids,
    std::vector<float>& topKRankScoreList,
    std::size_t& totalCount)
{
    LOG(INFO) << "begin rank ads from searched result : " << docids.size();
    typedef std::vector<docid_t>::iterator InputIterator;

    std::string propName = "Price";
    std::string propName_mode = "BidMode";
    PropSharedLockSet propSharedLockSet;

    boost::shared_ptr<NumericPropertyTableBase> numericTable =
        numericTableBuilder_->createPropertyTable(propName);
    boost::shared_ptr<NumericPropertyTableBase> numericTable_mode =
        numericTableBuilder_->createPropertyTable(propName_mode);

    if ( numericTable )
        propSharedLockSet.insertSharedLock(numericTable.get());
    if ( numericTable_mode )
        propSharedLockSet.insertSharedLock(numericTable_mode.get());

    boost::shared_ptr<HitQueue> scoreItemQueue;
    std::vector<docid_t>  cpc_ads_result;
    std::vector<AdSelector::FeatureMapT> cpc_ads_features;
    cpc_ads_result.reserve(docids.size());
    cpc_ads_features.reserve(docids.size());

    uint32_t heapSize = std::min((std::size_t)MAX_SELECT_AD_COUNT, docids.size());
    scoreItemQueue.reset(new ScoreSortedHitQueue(heapSize));

    for(InputIterator it = docids.begin();
            it != docids.end(); it++ )
    {
        if(!documentManager_->isDeleted(*it))
        {
            float price = 0.0;
            float score = 0.0;
            int32_t mode = 0;
            if(numericTable_mode)
            {
                numericTable_mode->getInt32Value(*it, mode, false);
            }
            if(mode == 0)
            {
                if(numericTable)
                {
                    numericTable->getFloatValue(*it, price, false);
                }
                score = price;
            }
            else if(mode == 1)
            {
                cpc_ads_result.push_back(*it);
                continue;
            }
            ScoreDoc scoreItem(*it, score);
            scoreItemQueue->insert(scoreItem);
        }
    }
    LOG(INFO) << "begin select ads from cpc cand result : " << cpc_ads_result.size();
    // select some ads using some strategy to maximize the CPC.
    std::vector<double> score_list;
    if (ad_selector_)
    {
        ad_selector_->select(userinfo, MAX_SELECT_AD_COUNT,
            cpc_ads_result, score_list, propSharedLockSet);
    }
    LOG(INFO) << "end select ads from result.";
    // calculate eCPM
    for (std::size_t i = 0; i < cpc_ads_result.size(); ++i)
    {
        float price = 0;
        if(numericTable)
        {
            numericTable->getFloatValue(cpc_ads_result[i], price, false);
        }
        double score = score_list[i] * price * 1000;
        ScoreDoc item(cpc_ads_result[i], score);
        scoreItemQueue->insert(item);
    }

    unsigned int scoreSize = scoreItemQueue->size();

    docids.resize(scoreSize);
    topKRankScoreList.resize(scoreSize);

    for (int i = scoreSize - 1; i >= 0; --i)
    {
        const ScoreDoc& item = scoreItemQueue->pop();
        docids[i] = item.docId;
        topKRankScoreList[i] = item.score;
    }
    totalCount = docids.size();
}

bool AdIndexManager::searchByQuery(const SearchKeywordOperation& actionOperation,
    KeywordSearchResult& searchResult)
{
    if (!ad_searcher_ || !ad_selector_)
        return false;
    // using the DNF as Attributes.
    (*const_cast<SearchKeywordOperation*>(&actionOperation)).actionItem_.searchingMode_.mode_ = SearchingMode::SUFFIX_MATCH;
    bool ret = ad_searcher_->search(actionOperation.actionItem_, searchResult);
    const std::vector<std::pair<std::string, std::string> > userinfo;
    if (ret)
    {
        searchResult.adCachedTopKDocs_ = searchResult.topKDocs_;
        rankAndSelect(userinfo, searchResult.topKDocs_, searchResult.topKRankScoreList_, searchResult.totalCount_);
    }
    return ret;
}

bool AdIndexManager::searchByDNF(const FeatureT& info,
    std::vector<docid_t>& docids,
    std::vector<float>& topKRankScoreList,
    std::size_t& totalCount)
{
    boost::unordered_set<uint32_t> dnfIDs;

    {
        boost::shared_lock<boost::shared_mutex> lock(rwMutex_);
        ad_dnf_index_->retrieve(info, dnfIDs);
    }

    LOG(INFO)<< "dnfIDs.size(): "<< dnfIDs.size() << std::endl;

    docids.resize(dnfIDs.size());

    std::size_t i = 0;
    for(boost::unordered_set<uint32_t>::iterator it = dnfIDs.begin();
            it != dnfIDs.end(); it++ )
    {
        docids[i++] = *it;
    }

    rankAndSelect(info, docids, topKRankScoreList, totalCount);
    return true;
}

bool AdIndexManager::searchByRecommend(const SearchKeywordOperation& actionOperation,
    KeywordSearchResult& searchResult)
{
    if (!ad_searcher_ || !ad_selector_)
        return false;
    const std::vector<std::pair<std::string, std::string> > userinfo;
    std::vector<double> score_list;
    std::vector<std::string> rec_items;
    ad_selector_->selectFromRecommend(userinfo, MAX_RECOMMEND_ITEM_NUM,
        rec_items, score_list);
    std::string rec_query_str;
    for (size_t i = 0; i < rec_items.size(); ++i)
    {
        rec_query_str += rec_items[i] + " ";
    }
    LOG(INFO) << "recommend feature items are: ";
    //KeywordSearchActionItem rec_search_query(actionOperation.actionItem_);
    //rec_search_query.env_.queryString_ = rec_query_str;
    //rec_search_query.searchingMode_.mode_ = SearchingMode::SUFFIX_MATCH;
    (*const_cast<SearchKeywordOperation*>(&actionOperation)).actionItem_.searchingMode_.mode_ = SearchingMode::SUFFIX_MATCH;
    (*const_cast<SearchKeywordOperation*>(&actionOperation)).actionItem_.env_.queryString_ = rec_query_str;
    bool ret = ad_searcher_->search(actionOperation.actionItem_, searchResult);
    if (ret)
    {
        rankAndSelect(userinfo, searchResult.topKDocs_, searchResult.topKRankScoreList_, searchResult.totalCount_);
    }
    return ret;
}

bool AdIndexManager::sponsoredAdSearch(const SearchKeywordOperation& actionOperation,
    KeywordSearchResult& searchResult)
{
    if (!ad_sponsored_mgr_)
        return false;
    return ad_sponsored_mgr_->sponsoredAdSearch(actionOperation, searchResult);
}


} //namespace sf1r
