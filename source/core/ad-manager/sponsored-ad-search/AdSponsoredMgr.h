#ifndef AD_SPONSORED_MGR_H
#define AD_SPONSORED_MGR_H

#include "AdCommonDataType.h"
#include <ir/id_manager/IDManager.h>

#include "AdManualBidInfoMgr.h"
#include <vector>
#include <map>
#include <deque>
#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/dynamic_bitset.hpp>
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
        const std::string& commondata_path,
        const std::string& adtitle_prop,
        const std::string& adbidphrase_prop,
        const std::string& adcampaign_prop,
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

    void getAdBidPrice(ad_docid_t adid, const std::string& query, const std::string& hit_bidstr,
        int leftbudget, int& price);
    int getBudgetLeft(ad_docid_t adid);
    void getBidPhrase(const std::string& adid, BidPhraseListT& bidphrase_list);


    bool getAdIdFromAdStrId(const std::string& strid, ad_docid_t& adid);
    bool getAdStrIdFromAdId(ad_docid_t adid, std::string& ad_strid);

    void updateAuctionLogData(const std::string& ad_id, const std::string& hit_bidstr,
        int click_cost_in_fen, uint32_t click_slot);

    void save();
    void changeDailyBudget(const std::string& ad_campaign_name, int dailybudget);

    //bool setManualBidPrice(const std::string& ad_strid,
    //    const std::vector<std::string>& key_list,
    //    const std::vector<int>& price_list);

    void delAdBidPhrase(const std::string& ad_strid, const std::vector<std::string>& bid_phrase_list);
    void updateAdBidPhrase(const std::string& ad_strid, const std::vector<std::string>& bid_phrase_list,
        const std::vector<int>& price_list);
    bool updateAdOnlineStatus(const std::vector<std::string>& ad_strid_list, const std::vector<bool>& is_online_list);
    void resetDailyLogStatisticalData(bool reset_used);
    inline double getAdCTR(ad_docid_t adid)
    {
        if (adid >= ad_ctr_list_.size())
            return 0;
        return ad_ctr_list_[adid];
    }


private:
    typedef boost::unordered_map<std::string, uint32_t>  StrIdMapT;
    typedef std::vector<std::pair<int, double> > BidAuctionLandscapeT;
    typedef std::string BidKeywordStrT;
    typedef std::set<BidPhraseStrT> CampaignBidStrListT;

    double computeAdCTR(ad_docid_t adid);
    void replaceAdBidPhrase(ad_docid_t adid, const std::vector<std::string>& bid_phrase_list,
        BidPhraseListT& bidid_list);
    void updateAdCampaign(ad_docid_t adid, const std::string& campaign_name);

    std::string preprocessBidPhraseStr(const std::string& orig);
    void generateBidPhrase(const std::string& ad_title, std::vector<std::string>& bidphrase);
    inline double getAdRelevantScore(const BidPhraseT& bidphrase, const BidPhraseT& query_kid_list)
    {
        return bidphrase.size(); 
    }
    inline double getAdQualityScore(ad_docid_t adid, const BidPhraseT& bidphrase, const BidPhraseT& query_kid_list)
    {
        return getAdCTR(adid) * getAdRelevantScore(bidphrase, query_kid_list);
    }
    void consumeBudget(ad_docid_t adid, int cost);
    void load();
    bool getBidKeywordIdFromStr(const BidKeywordStrT& keyword, bool insert, BidKeywordId& id);
    void getBidKeywordStrFromId(const BidKeywordId& id, BidKeywordStrT& keyword);
    void replaceBidPhrase(const std::string& adid, BidPhraseListT& bidphrase_list, BidPhraseStrListT& bidstr_list);
    void getBidPhraseStr(const BidPhraseT& bidphrase, BidPhraseStrT& bidstr);
    void getBidPhraseStrList(const BidPhraseListT& bidphrase_list, BidPhraseStrListT& bidstr_list);
    void getBidPhrase(const std::string& adid,
        BidPhraseListT& bidphrase_list,
        BidPhraseStrListT& bidstr_list);

    void getBidStatisticalData(const CampaignBidStrListT& bidstr_list,
        const std::map<std::string, BidAuctionLandscapeT>& bidkey_cpc_map,
        //const std::map<std::string, int>& manual_bidprice_list,
        std::vector<AdQueryStatisticInfo>& ad_statistical_data);

    std::string data_path_;
    std::string ad_title_prop_;
    std::string ad_bidphrase_prop_;
    std::string ad_campaign_prop_;
    // all bid phrase for all ad creatives.
    std::vector<BidPhraseListT>  ad_bidphrase_list_;
    std::vector<BidPhraseStrListT>  ad_orig_bidstr_list_;
    std::vector<double>  ad_ctr_list_;
    std::vector<std::string> keyword_id_value_list_;
    StrIdMapT keyword_value_id_list_;

    // the used budget for specific ad campaign. update realtime.
    std::vector<int> ad_budget_used_list_;
    std::vector<std::string>  ad_campaign_name_list_;
    StrIdMapT ad_campaign_name_id_list_;
    std::vector<uint32_t>  ad_campaign_belong_list_; 
    std::vector<CampaignBidStrListT>  ad_campaign_bid_phrase_list_;

    faceted::GroupManager* grp_mgr_;
    DocumentManager* doc_mgr_;
    izenelib::ir::idmanager::IDManager* id_manager_;
    AdSearchService* ad_searcher_;

    boost::shared_ptr<AdAuctionLogMgr> ad_log_mgr_;
    boost::shared_ptr<AdBidStrategy> ad_bid_strategy_;
    kBidStrategy bid_strategy_type_;
    // bid price for different campaign.
    std::vector<std::map<BidPhraseStrT, int> > ad_bid_price_list_;
    std::vector<std::vector<std::pair<int, double> > > ad_uniform_bid_price_list_;

    AdManualBidInfoMgr manual_bidinfo_mgr_;
    boost::dynamic_bitset<> ad_status_bitmap_;
};

}
}

#endif
