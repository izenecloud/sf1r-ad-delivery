#ifndef AD_SPONSORED_MGR_H
#define AD_SPONSORED_MGR_H

#include "AdCommonDataType.h"
#include <vector>
#include <map>
#include <deque>
#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>
#include <string>

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
class AdSponsoredMgr
{
public:
    AdSponsoredMgr();
    ~AdSponsoredMgr();
    void init(const std::string& dict_path,
        const std::string& data_path,
        faceted::GroupManager* grp_mgr,
        DocumentManager* doc_mgr,
        AdSearchService* searcher);

    void miningAdCreatives(ad_docid_t start_id);
    void tokenize(const std::string& str, std::vector<std::string>& tokens);
    void generateBidPhrase(const std::string& ad_title, BidPhraseT& bidphrase_list);
    void generateBidPrice(ad_docid_t adid, std::vector<double>& price_list);

    bool sponsoredAdSearch(const SearchKeywordOperation& actionOperation,
        KeywordSearchResult& searchResult);

    void getRealTimeBidPrice(ad_docid_t adid, const std::string& query, double leftbudget, double& price);
    double getBudgetLeft(ad_docid_t adid);
    double getAdCTR(ad_docid_t adid);
    double getAdRelevantScore(const BidPhraseT& bidphrase, const BidPhraseT& query_kid_list);
    double getAdQualityScore(ad_docid_t adid, const BidPhraseT& bidphrase, const BidPhraseT& query_kid_list);
    void getBidPhrase(const std::string& adid, BidPhraseT& bidphrase);
    bool getAdIdFromAdStrId(const std::string& strid, ad_docid_t& adid);
    bool getAdStrIdFromAdId(ad_docid_t adid, std::string& ad_strid);

    void updateAuctionLogData(const std::string& ad_id,
        int click_cost_in_fen, uint32_t click_slot);

    void save();

private:
    void load();
    bool getBidKeywordId(const std::string& keyword, bool insert, BidKeywordId& id);
    typedef boost::unordered_map<std::string, uint32_t>  StrIdMapT;
    std::string data_path_;
    // all bid phrase for all ad creatives.
    std::vector<BidPhraseT>  ad_bidphrase_list_;
    std::vector<std::string> keyword_id_value_list_;
    StrIdMapT keyword_value_id_list_;

    // the total budget for specific ad campaign. update each day.
    boost::unordered_map<std::string, double> ad_budget_list_;
    // the left budget for specific ad campaign. update realtime.
    boost::unordered_map<std::string, double> ad_budget_left_list_;
    std::vector<std::string>  ad_campaign_name_list_;
    StrIdMapT ad_campaign_name_id_list_;
    std::vector<uint32_t>  ad_campaign_belong_list_; 

    std::auto_ptr<TitlePCAWrapper>  bid_title_pca_;

    faceted::GroupManager* grp_mgr_;
    DocumentManager* doc_mgr_;
    AdSearchService* ad_searcher_;

    boost::shared_ptr<AdAuctionLogMgr> ad_log_mgr_;
};

}
}

#endif
