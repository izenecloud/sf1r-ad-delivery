/*
 *  AdIndexManager.h
 */

#ifndef SF1_AD_INDEX_MANAGER_H_
#define SF1_AD_INDEX_MANAGER_H_

#include "AdMiningTask.h"
#include <boost/lexical_cast.hpp>
#include <common/PropSharedLockSet.h>
#include <search-manager/NumericPropertyTableBuilder.h>
#include <ir/be_index/InvIndex.hpp>
#include <ir/id_manager/IDManager.h>
#include <util/cronexpression.h>


#define CPM 0
#define CPC 1

namespace sf1r
{

class MiningTask;
class DocumentManager;
class AdMessage;
class AdClickPredictor;
class SearchKeywordOperation;
class KeywordSearchResult;
class SearchWorker;
class AdSearchService;
class AdSelector;
class AdIndexConfig;
namespace faceted
{
    class GroupManager;
}

namespace sponsored
{
    class AdSponsoredMgr;
}

class AdIndexManager
{
public:
    typedef std::vector<std::pair<std::string, std::string> > FeatureT;
    AdIndexManager(
            const std::string& ad_resource_path,
            const std::string& ad_data_path,
            const AdIndexConfig& adconfig,
            boost::shared_ptr<DocumentManager>& dm,
            izenelib::ir::idmanager::IDManager* id_manager,
            NumericPropertyTableBuilder* ntb,
            AdSearchService* searcher,
            faceted::GroupManager* grp_mgr);

    ~AdIndexManager();

    bool buildMiningTask();

    inline AdMiningTask* getMiningTask()
    {
        return adMiningTask_;
    }

    void onAdStreamMessage(const std::vector<AdMessage>& msg_list);

    void rankAndSelect(
        const FeatureT& userinfo,
        std::vector<docid_t>& docids,
        std::vector<float>& topKRankScoreList,
        std::size_t& totalCount);
    bool searchByQuery(const SearchKeywordOperation& actionOperation,
        KeywordSearchResult& searchResult);
    bool searchByDNF(const FeatureT& info,
            std::vector<docid_t>& docids,
            std::vector<float>& topKRankScoreList,
            std::size_t& totalCount
            );

    bool searchByRecommend(const SearchKeywordOperation& actionOperation,
        KeywordSearchResult& searchResult);

    bool sponsoredAdSearch(const SearchKeywordOperation& actionOperation,
        KeywordSearchResult& searchResult);

    void postMining(docid_t startid, docid_t endid);

    bool setKeywordBidPrice(const std::string& keyword, const std::string& campaign_name, int bidprice);
    bool setAdCampaignBudget(const std::string& campaign_name, int budget);
    bool setAdBidPhrase(const std::string& ad_strid, const std::vector<std::string>& bid_phrase_list);
    bool updateAdOnlineStatus(const std::string& ad_strid, bool is_online);
    void refreshBidStrategy(int calltype);

private:

    typedef izenelib::ir::be_index::DNFInvIndex AdDNFIndexType;
    std::string indexPath_;

    std::string ad_res_path_;
    std::string ad_data_path_;
    const AdIndexConfig& adconfig_;
    std::string adlog_topic_;

    boost::shared_ptr<DocumentManager>& documentManager_;
    izenelib::ir::idmanager::IDManager* id_manager_;
    AdMiningTask* adMiningTask_;
    AdClickPredictor* ad_click_predictor_;
    NumericPropertyTableBuilder* numericTableBuilder_;
    AdSearchService* ad_searcher_;
    faceted::GroupManager* groupManager_;
    boost::shared_mutex  rwMutex_;
    boost::shared_ptr<AdDNFIndexType> ad_dnf_index_;
    boost::shared_ptr<AdSelector> ad_selector_;
    boost::shared_ptr<sponsored::AdSponsoredMgr> ad_sponsored_mgr_;
    izenelib::util::CronExpression refresh_schedule_exp_;
};

} //namespace sf1r
#endif
