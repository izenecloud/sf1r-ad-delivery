#ifndef AD_SPONSORED_MGR_H
#define AD_SPONSORED_MGR_H

#include "AdCommonDataType.h"
#include <ir/id_manager/IDManager.h>
#include <vector>
#include <map>
#include <deque>
#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <set>

namespace sf1r
{

class TitlePCAWrapper;
class SearchKeywordOperation;
class KeywordSearchResult;
class AdKeywordSearchResult;
class DocumentManager;
class AdSearchService;
namespace faceted
{
    class GroupManager;
}

namespace sponsored
{

class AdAuctionLogMgr;
class AdBidStrategy;
class AdQueryStatisticInfo;
// note: all the bid price , budget is in unit of 0.01 yuan.
class AdSponsoredMgr
{
public:
    enum kBidStrategy
    {
        UniformBid,
        GeneticBid,
        RealtimeBid,
        Unknown
    };

    AdSponsoredMgr();
    ~AdSponsoredMgr();
    void init(const std::string& dict_path,
        const std::string& data_path,
        faceted::GroupManager* grp_mgr,
        DocumentManager* doc_mgr,
        izenelib::ir::idmanager::IDManager* id_manager,
        AdSearchService* searcher);

    void miningAdCreatives(ad_docid_t start_id, ad_docid_t end_id);
    void tokenize(const std::string& str, std::vector<std::string>& tokens);
    void generateBidPhrase(const std::string& ad_title, BidPhraseT& bidphrase_list);
    void generateBidPrice(ad_docid_t adid, std::vector<int>& price_list);

    bool sponsoredAdSearch(const SearchKeywordOperation& actionOperation,
        KeywordSearchResult& searchResult);

    void getAdBidPrice(ad_docid_t adid, const std::string& query, int leftbudget, int& price);
    int getBudgetLeft(ad_docid_t adid);
    double getAdCTR(ad_docid_t adid);
    void getBidPhrase(const std::string& adid, BidPhraseT& bidphrase);
    bool getAdIdFromAdStrId(const std::string& strid, ad_docid_t& adid);
    bool getAdStrIdFromAdId(ad_docid_t adid, std::string& ad_strid);

    void updateAuctionLogData(const std::string& ad_id,
        int click_cost_in_fen, uint32_t click_slot);

    void save();
    void changeDailyBudget(const std::string& ad_campaign_name, int dailybudget);
    void updateAdCampaign(ad_docid_t adid, const std::string& campaign_name);

private:
    typedef boost::unordered_map<std::string, uint32_t>  StrIdMapT;
    typedef std::vector<std::pair<int, double> > BidAuctionLandscapeT;

    double getAdRelevantScore(const BidPhraseT& bidphrase, const BidPhraseT& query_kid_list);
    double getAdQualityScore(ad_docid_t adid, const BidPhraseT& bidphrase, const BidPhraseT& query_kid_list);
    void consumeBudget(ad_docid_t adid, int cost);
    void load();
    void resetDailyLeftBudget();
    bool getBidKeywordId(const std::string& keyword, bool insert, BidKeywordId& id);
    void getBidStatisticalData(const std::set<BidKeywordId>& bidkey_list,
        const std::map<BidKeywordId, BidAuctionLandscapeT>& bidkey_cpc_map,
        std::list<AdQueryStatisticInfo>& ad_statistical_data);

    std::string data_path_;
    // all bid phrase for all ad creatives.
    std::vector<BidPhraseT>  ad_bidphrase_list_;
    std::vector<std::string> keyword_id_value_list_;
    StrIdMapT keyword_value_id_list_;

    // the total budget for specific ad campaign. update each day.
    std::vector<int> ad_budget_list_;
    // the left budget for specific ad campaign. update realtime.
    std::vector<int> ad_budget_left_list_;
    std::vector<std::string>  ad_campaign_name_list_;
    StrIdMapT ad_campaign_name_id_list_;
    std::vector<uint32_t>  ad_campaign_belong_list_; 
    std::vector<std::set<BidKeywordId> >  ad_campaign_bid_keyword_list_;

    std::auto_ptr<TitlePCAWrapper>  bid_title_pca_;

    faceted::GroupManager* grp_mgr_;
    DocumentManager* doc_mgr_;
    izenelib::ir::idmanager::IDManager* id_manager_;
    AdSearchService* ad_searcher_;

    boost::shared_ptr<AdAuctionLogMgr> ad_log_mgr_;
    boost::shared_ptr<AdBidStrategy> ad_bid_strategy_;
    kBidStrategy bid_strategy_type_;
    // bid price for different campaign.
    std::vector<std::map<BidKeywordId, int> > ad_bid_price_list_;
    std::vector<std::vector<std::pair<int, double> > > ad_uniform_bid_price_list_;
};

}
}

#endif
