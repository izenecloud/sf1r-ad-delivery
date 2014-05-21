#include "AdSponsoredMgr.h"
#include "AdAuctionLogMgr.h"
#include "AdBidStrategy.h"
#include "../AdSearchService.h"
#include <la-manager/TitlePCAWrapper.h>
#include <document-manager/DocumentManager.h>
#include <mining-manager/group-manager/GroupManager.h>
#include "AdResultType.h"
#include <query-manager/SearchKeywordOperation.h>
#include <query-manager/ActionItem.h>
#include <search-manager/HitQueue.h>
#include <common/Utilities.h>

#include <fstream>
#include <util/izene_serialization.h>
#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/unordered_map.hpp>

namespace sf1r
{

namespace sponsored
{

static const int MAX_BIDPHRASE_LEN = 3;
static const int MAX_DIFF_BID_KEYWORD_NUM = 1000000;
static const int MAX_RANKED_AD_NUM = 10;
static const int LOWEST_CLICK_COST = 1;
static const int DEFAULT_AD_BUDGET = 1000;

static bool sort_tokens_func(const std::pair<std::string, double>& left, const std::pair<std::string, double>& right)
{
    return left.second > right.second;
}

struct ScoreSponsoredAdDoc
{
    ScoreSponsoredAdDoc()
        :docId(0), score(0), qscore(0)
    {
    }
    ad_docid_t docId;
    double score;
    double qscore;
};

class ScoreSortedSponsoredAdQueue
{
    class Queue_ : public izenelib::util::PriorityQueue<ScoreSponsoredAdDoc>
    {
    public:
        Queue_(size_t size)
        {
            initialize(size);
        }
    protected:
        bool lessThan(const ScoreSponsoredAdDoc& o1, const ScoreSponsoredAdDoc& o2) const
        {
            if (std::fabs(o1.score - o2.score) < std::numeric_limits<score_t>::epsilon())
            {
                return o1.docId < o2.docId;
            }
            return (o1.score < o2.score);
        }
    };

public:
    ScoreSortedSponsoredAdQueue(size_t size) : queue_(size)
    {
    }

    ~ScoreSortedSponsoredAdQueue() {}

    bool insert(const ScoreSponsoredAdDoc& doc)
    {
        return queue_.insert(doc);
    }

    ScoreSponsoredAdDoc pop()
    {
        return queue_.pop();
    }
    const ScoreSponsoredAdDoc& top()
    {
        return queue_.top();
    }
    ScoreSponsoredAdDoc& operator[](size_t pos)
    {
        return queue_[pos];
    }
    ScoreSponsoredAdDoc& getAt(size_t pos)
    {
        return queue_.getAt(pos);
    }
    size_t size()
    {
        return queue_.size();
    }
    void clear() {}

private:
    Queue_ queue_;

};


AdSponsoredMgr::AdSponsoredMgr()
    : grp_mgr_(NULL), doc_mgr_(NULL), id_manager_(NULL)
{
}

AdSponsoredMgr::~AdSponsoredMgr()
{
    save();
}

void AdSponsoredMgr::init(const std::string& res_path,
    const std::string& data_path,
    faceted::GroupManager* grp_mgr,
    DocumentManager* doc_mgr,
    izenelib::ir::idmanager::IDManager* id_manager,
    AdSearchService* searcher)
{
    data_path_ = data_path;
    grp_mgr_ = grp_mgr;
    doc_mgr_ = doc_mgr;
    id_manager_ = id_manager;
    ad_searcher_ = searcher;
    keyword_value_id_list_.rehash(MAX_DIFF_BID_KEYWORD_NUM);
    ad_log_mgr_.reset(new AdAuctionLogMgr());
    ad_log_mgr_->init(data_path_);
    ad_bid_strategy_.reset(new AdBidStrategy());

    load();
}

void AdSponsoredMgr::updateAuctionLogData(const std::string& ad_strid,
    int click_cost_in_fen, uint32_t click_slot)
{
    BidPhraseT bidphrase;
    getBidPhrase(ad_strid, bidphrase);
    ad_log_mgr_->updateAuctionLogData(ad_strid, bidphrase, click_cost_in_fen, click_slot);
    ad_docid_t adid;
    if (!getAdIdFromAdStrId(ad_strid, adid))
    {
        LOG(INFO) << "ad string id not found: " << ad_strid;
        return;
    }
    consumeBudget(adid, click_cost_in_fen);
}

void AdSponsoredMgr::save()
{
    std::ofstream ofs(std::string(data_path_ + "/sponsored_ad.data").c_str());
    std::size_t len = 0;
    char* buf = NULL;
    {
        izenelib::util::izene_serialization<std::vector<BidPhraseT> > izs(ad_bidphrase_list_);
        izs.write_image(buf, len);
        ofs.write((const char*)&len, sizeof(len));
        ofs.write(buf, len);
        ofs.flush();

    }
    {
        len = 0;
        izenelib::util::izene_serialization<std::vector<std::string> > izs(keyword_id_value_list_);
        izs.write_image(buf, len);
        ofs.write((const char*)&len, sizeof(len));
        ofs.write(buf, len);
        ofs.flush();
    }

    {
        len = 0;
        izenelib::util::izene_serialization<StrIdMapT> izs(keyword_value_id_list_);
        izs.write_image(buf, len);
        ofs.write((const char*)&len, sizeof(len));
        ofs.write(buf, len);
        ofs.flush();
    }

    {
        len = 0;
        izenelib::util::izene_serialization<std::vector<int> > izs(ad_budget_list_);
        izs.write_image(buf, len);
        ofs.write((const char*)&len, sizeof(len));
        ofs.write(buf, len);
        ofs.flush();
    }

    {
        len = 0;
        izenelib::util::izene_serialization<std::vector<int> > izs(ad_budget_left_list_);
        izs.write_image(buf, len);
        ofs.write((const char*)&len, sizeof(len));
        ofs.write(buf, len);
        ofs.flush();
    }

    {
        len = 0;
        izenelib::util::izene_serialization<std::vector<std::string> > izs(ad_campaign_name_list_);
        izs.write_image(buf, len);
        ofs.write((const char*)&len, sizeof(len));
        ofs.write(buf, len);
        ofs.flush();
    }

    {
        len = 0;
        izenelib::util::izene_serialization<StrIdMapT> izs(ad_campaign_name_id_list_);
        izs.write_image(buf, len);
        ofs.write((const char*)&len, sizeof(len));
        ofs.write(buf, len);
        ofs.flush();
    }

    {
        len = 0;
        izenelib::util::izene_serialization<std::vector<uint32_t> > izs(ad_campaign_belong_list_);
        izs.write_image(buf, len);
        ofs.write((const char*)&len, sizeof(len));
        ofs.write(buf, len);
        ofs.flush();
    }

    ofs.close();
    if (ad_log_mgr_)
        ad_log_mgr_->save();
}

void AdSponsoredMgr::load()
{
    std::ifstream ifs(std::string(data_path_ + "/sponsored_ad.data").c_str());
    std::string data;

    if (ifs.good())
    {
        std::size_t len = 0;
        {
            ifs.read((char*)&len, sizeof(len));
            data.resize(len);
            ifs.read((char*)&data[0], len);
            izenelib::util::izene_deserialization<std::vector<BidPhraseT> > izd(data.data(), data.size());
            izd.read_image(ad_bidphrase_list_);
        }

        {
            len = 0;
            ifs.read((char*)&len, sizeof(len));
            data.resize(len);
            ifs.read((char*)&data[0], len);
            izenelib::util::izene_deserialization<std::vector<std::string> > izd(data.data(), data.size());
            izd.read_image(keyword_id_value_list_);
        }

        {
            len = 0;
            ifs.read((char*)&len, sizeof(len));
            data.resize(len);
            ifs.read((char*)&data[0], len);
            izenelib::util::izene_deserialization<StrIdMapT> izd(data.data(), data.size());
            izd.read_image(keyword_value_id_list_);
        }

        {
            len = 0;
            ifs.read((char*)&len, sizeof(len));
            data.resize(len);
            ifs.read((char*)&data[0], len);
            izenelib::util::izene_deserialization<std::vector<int> > izd(data.data(), data.size());
            izd.read_image(ad_budget_list_);
        }

        {
            len = 0;
            ifs.read((char*)&len, sizeof(len));
            data.resize(len);
            ifs.read((char*)&data[0], len);
            izenelib::util::izene_deserialization<std::vector<int> > izd(data.data(), data.size());
            izd.read_image(ad_budget_left_list_);
        }

        {
            len = 0;
            ifs.read((char*)&len, sizeof(len));
            data.resize(len);
            ifs.read((char*)&data[0], len);
            izenelib::util::izene_deserialization<std::vector<std::string> > izd(data.data(), data.size());
            izd.read_image(ad_campaign_name_list_);
        }

        {
            len = 0;
            ifs.read((char*)&len, sizeof(len));
            data.resize(len);
            ifs.read((char*)&data[0], len);
            izenelib::util::izene_deserialization<StrIdMapT> izd(data.data(), data.size());
            izd.read_image(ad_campaign_name_id_list_);
        }

        {
            len = 0;
            ifs.read((char*)&len, sizeof(len));
            data.resize(len);
            ifs.read((char*)&data[0], len);
            izenelib::util::izene_deserialization<std::vector<uint32_t> > izd(data.data(), data.size());
            izd.read_image(ad_campaign_belong_list_);
        }
    }
    ifs.close();

    if (ad_log_mgr_)
        ad_log_mgr_->load();
}

void AdSponsoredMgr::miningAdCreatives(ad_docid_t start_id, ad_docid_t end_id)
{
    if (!doc_mgr_)
    {
        LOG(ERROR) << "no doc manager!";
        return;
    }
    ad_bidphrase_list_.resize(end_id + 1);
    std::string ad_title;
    BidPhraseT bidphrase;
    for(ad_docid_t i = start_id; i <= end_id; ++i)
    {
        Document::doc_prop_value_strtype prop_value;
        doc_mgr_->getPropertyValue(i, "Title", prop_value);
        ad_title = propstr_to_str(prop_value);
        generateBidPhrase(ad_title, bidphrase);
        ad_bidphrase_list_[i].swap(bidphrase);
        doc_mgr_->getPropertyValue(i, "Campaign", prop_value);
        std::string campaign = propstr_to_str(prop_value);
        if (campaign.empty())
        {
            doc_mgr_->getPropertyValue(i, "DOCID", prop_value);
            campaign = propstr_to_str(prop_value);
        }
        updateAdCampaign(i, campaign);
    }
}

void AdSponsoredMgr::updateAdCampaign(ad_docid_t adid, const std::string& campaign_name)
{
    uint32_t campaign_id = 0;
    StrIdMapT::const_iterator it = ad_campaign_name_id_list_.find(campaign_name);
    if (it == ad_campaign_name_id_list_.end())
    {
        campaign_id = ad_campaign_name_list_.size();
        ad_campaign_name_list_.push_back(campaign_name);
        ad_campaign_name_id_list_[campaign_name] = campaign_id;
    }
    else
    {
        campaign_id = it->second;
    }
    if (adid >= ad_campaign_belong_list_.size())
    {
        ad_campaign_belong_list_.resize(adid + 1);
    }
    ad_campaign_belong_list_[adid] = campaign_id;
    if (campaign_id >= ad_budget_list_.size())
    {
        ad_budget_list_.resize(campaign_id + 1, DEFAULT_AD_BUDGET);
    }
    if (campaign_id >= ad_campaign_bid_keyword_list_.size())
    {
        ad_campaign_bid_keyword_list_.resize(campaign_id + 1);
    }
    if (adid >= ad_bidphrase_list_.size())
    {
        return;
    }
    const BidPhraseT& bidphrase = ad_bidphrase_list_[adid];
    ad_campaign_bid_keyword_list_[campaign_id].insert(bidphrase.begin(), bidphrase.end());
}

void AdSponsoredMgr::getBidStatisticalData(const std::set<BidKeywordId>& bidkey_list,
    const std::map<BidKeywordId, BidAuctionLandscapeT>& bidkey_cpc_map,
    std::list<AdQueryStatisticInfo>& ad_statistical_data)
{
    std::set<BidKeywordId>::const_iterator bid_it = bidkey_list.begin();
    for(; bid_it != bidkey_list.end(); ++bid_it)
    {
        const BidKeywordId& bid = *bid_it;
        ad_statistical_data.push_back(AdQueryStatisticInfo());
        ad_statistical_data.back().impression_ = ad_log_mgr_->getKeywordAvgDailyImpression(bid);
        ad_statistical_data.back().minBid_ = LOWEST_CLICK_COST;
        std::map<BidKeywordId, BidAuctionLandscapeT>::const_iterator it = bidkey_cpc_map.find(bid);
        if (it == bidkey_cpc_map.end())
            continue;
        for (std::size_t j = 0; j < it->second.size(); ++j)
        {
            ad_statistical_data.back().cpc_.push_back(it->second[j].first);
            ad_statistical_data.back().ctr_.push_back(it->second[j].second);
        }
    }
}

// reset daily left budget each day.
void AdSponsoredMgr::resetDailyLeftBudget()
{
    ad_budget_left_list_ = ad_budget_list_;

    std::list<AdQueryStatisticInfo> ad_statistical_data;
    std::vector<BidKeywordId> keyword_list;
    std::vector<AdAuctionLogMgr::BidAuctionLandscapeT> cost_click_list;
    ad_log_mgr_->getKeywordBidLandscape(keyword_list, cost_click_list);
    std::map<BidKeywordId, BidAuctionLandscapeT> bidkey_cpc_map;
    for(std::size_t i = 0; i < keyword_list.size(); ++i)
    {
        bidkey_cpc_map[keyword_list[i]] = cost_click_list[i];
    }

    // recompute the bid strategy.
    if (bid_strategy_type_ == UniformBid)
    {
        std::vector<std::pair<int, double> > bid_price_list;
        ad_uniform_bid_price_list_.resize(ad_budget_left_list_.size());
        for(std::size_t i = 0; i < ad_budget_left_list_.size(); ++i)
        {
            if (i >= ad_campaign_bid_keyword_list_.size())
                break;
            const std::set<BidKeywordId>& bidkey_list = ad_campaign_bid_keyword_list_[i];
            getBidStatisticalData(bidkey_list, bidkey_cpc_map, ad_statistical_data);
            bid_price_list = ad_bid_strategy_->convexUniformBid(ad_statistical_data, ad_budget_left_list_[i]);
            ad_uniform_bid_price_list_[i].swap(bid_price_list);
        }
    }
    else if (bid_strategy_type_ == GeneticBid)
    {
        ad_bid_price_list_.resize(ad_budget_left_list_.size());
        for (std::size_t i = 0; i < ad_budget_left_list_.size(); ++i)
        {
            if (i >= ad_campaign_bid_keyword_list_.size())
                break;
            const std::set<BidKeywordId>& bidkey_list = ad_campaign_bid_keyword_list_[i];
            getBidStatisticalData(bidkey_list, bidkey_cpc_map, ad_statistical_data);
            std::vector<int> bid_price_list = ad_bid_strategy_->geneticBid(ad_statistical_data, ad_budget_left_list_[i]);
            assert(bid_price_list.size() == bidkey_list.size());
            std::set<BidKeywordId>::const_iterator bid_it = bidkey_list.begin();
            for(std::size_t j = 0; j < bid_price_list.size(); ++j)
            {
                ad_bid_price_list_[i][*bid_it] = bid_price_list[j];
            }
        }
    }
    else
    {
        // realtime bid.
    }
}

void AdSponsoredMgr::changeDailyBudget(const std::string& ad_campaign_name, int dailybudget)
{
    StrIdMapT::const_iterator it = ad_campaign_name_id_list_.find(ad_campaign_name);
    if (it == ad_campaign_name_id_list_.end())
    {
        LOG(INFO) << "no such campaign: " << ad_campaign_name;
        return;
    }
    if (it->second >= ad_budget_list_.size())
    {
        ad_budget_list_.resize(it->second + 1, 0);
    }
    ad_budget_list_[it->second] = dailybudget;
}

void AdSponsoredMgr::consumeBudget(ad_docid_t adid, int cost)
{
    if (adid >= ad_campaign_belong_list_.size())
    {
        LOG(WARNING) << "the ad id not belong to any campaign: " << adid;
        return;
    }
    uint32_t campaign_id = ad_campaign_belong_list_[adid];
    if (campaign_id >= ad_budget_left_list_.size())
    {
        LOG(WARNING) << "no budget was set for this campaign :" << campaign_id;
        return;
    }
    ad_budget_left_list_[campaign_id] -= cost;
}

void AdSponsoredMgr::generateBidPrice(ad_docid_t adid, std::vector<int>& price_list)
{
    // give the init bid price considered the budget, revenue and CTR.
    // Using the interface of the bid optimization module.
}

void AdSponsoredMgr::getAdBidPrice(ad_docid_t adid, const std::string& query,
    int leftbudget, int& price)
{
    price = 0;
    if (adid >= ad_campaign_belong_list_.size())
        return;
    std::size_t campaign_id = ad_campaign_belong_list_[adid];

    // if no real time bid available, just use the daily bid price.
    if (bid_strategy_type_ == RealtimeBid)
    {
        if (adid >= ad_bidphrase_list_.size())
            return;
        if (campaign_id >= ad_budget_left_list_.size())
            return;
        const BidPhraseT& bidphrase = ad_bidphrase_list_[adid];
        int left_budget = ad_budget_left_list_[campaign_id];
        int used_bugget = ad_budget_list_[campaign_id] - left_budget;
        AdQueryStatisticInfo info;
        for (std::size_t i = 0; i < bidphrase.size(); ++i)
        {
            BidKeywordId bid = bidphrase[i];
            info.impression_ = ad_log_mgr_->getKeywordAvgDailyImpression(bid) - ad_log_mgr_->getKeywordCurrentImpression(bid);
            info.minBid_ = LOWEST_CLICK_COST;
            ad_log_mgr_->getKeywordCTR(bid, info.ctr_);
            ad_log_mgr_->getKeywordAvgCost(bid, info.cpc_);
            //info.cpc_.clear();
            //for (std::size_t j = 0; j < cpc_list.size(); ++j)
            //{
            //    info.cpc_.push_back(cpc_list[j]);
            //}
            int tmp_price = ad_bid_strategy_->realtimeBidWithRevenueMax(info, used_bugget, left_budget);
            if (tmp_price > price)
            {
                price = tmp_price;
            }
        }
    }
    else if (bid_strategy_type_ == UniformBid)
    {
        int base = 10000;
        int r = rand() % base;
        if (campaign_id >= ad_uniform_bid_price_list_.size())
        {
            LOG(INFO) << "no bid price for this adid : " << campaign_id << ", maxid :" << ad_uniform_bid_price_list_.size();
            return;
        }
        int possible = 0;
        const std::vector<std::pair<int, double> >& bid_list = ad_uniform_bid_price_list_[campaign_id];
        for(std::size_t i = 0; i < bid_list.size(); ++i)
        {
            int tmp = bid_list[i].first;
            possible += int(bid_list[i].second * base);
            if (r <= possible)
            {
                price = tmp;
                return;
            }
        }
    }
    else if (bid_strategy_type_ == GeneticBid)
    {
        if (campaign_id >= ad_bid_price_list_.size())
        {
            LOG(INFO) << "no bid price for this adid : " << adid << ", maxid :" << ad_bid_price_list_.size();
            return;
        }
        const std::map<BidKeywordId, int>& bid_list = ad_bid_price_list_[campaign_id];
        if (adid >= ad_bidphrase_list_.size())
            return;
        const BidPhraseT& bidphrase = ad_bidphrase_list_[adid];
        for(std::size_t i = 0; i < bidphrase.size(); ++i)
        {
            std::map<BidKeywordId, int>::const_iterator it = bid_list.find(bidphrase[i]);
            if (it == bid_list.end())
                continue;
            if (it->second > price)
            {
                price = it->second;
            }
        }
    }
}

int AdSponsoredMgr::getBudgetLeft(ad_docid_t adid)
{
    if (adid >= ad_campaign_belong_list_.size())
        return 0;
    std::size_t campaign_id = ad_campaign_belong_list_[adid];
    if (campaign_id >= ad_budget_left_list_.size())
        return 0;
    return ad_budget_left_list_[campaign_id];
}

bool AdSponsoredMgr::getBidKeywordId(const std::string& keyword, bool insert, BidKeywordId& id)
{
    boost::unordered_map<std::string, uint32_t>::iterator it = keyword_value_id_list_.find(keyword);
    if (it != keyword_value_id_list_.end())
    {
        id = it->second;
        return true;
    }
    if (!insert)
        return false;
    id = keyword_id_value_list_.size();
    keyword_id_value_list_.push_back(keyword);
    keyword_value_id_list_[keyword] = id;
    return true;
}

void AdSponsoredMgr::getBidPhrase(const std::string& adid, BidPhraseT& bidphrase)
{
    bidphrase.clear();
    ad_docid_t adid_int = 0;
    if (!getAdIdFromAdStrId(adid, adid_int))
        return;
    if (adid_int > ad_bidphrase_list_.size())
        return;
    bidphrase = ad_bidphrase_list_[adid_int];
}

void AdSponsoredMgr::tokenize(const std::string& str, std::vector<std::string>& tokens)
{
    tokens.clear();
    std::string pattern = str;
    boost::to_lower(pattern);

    // TODO: tokenize the str using the bid phrase dictionary.
    //
    //const bool isLongQuery = QueryNormalizer::get()->isLongQuery(pattern);
    //boost::shared_ptr<KNlpWrapper> knlpWrapper = KNlpResourceManager::getResource();
    //if (isLongQuery)
    //    pattern = knlpWrapper->cleanStopword(pattern);
    //else
    //    pattern = knlpWrapper->cleanGarbage(pattern);

    //ProductTokenParam tokenParam(pattern, false);

    //// use Fuzzy Search Threshold 
    //if (actionOperation.actionItem_.searchingMode_.useFuzzyThreshold_)
    //{
    //    tokenParam.useFuzzyThreshold = true;
    //    tokenParam.fuzzyThreshold = actionOperation.actionItem_.searchingMode_.fuzzyThreshold_;
    //    tokenParam.tokensThreshold = actionOperation.actionItem_.searchingMode_.tokensThreshold_;
    //}

    //productTokenizer_->tokenize(tokenParam);

    //for (ProductTokenParam::TokenScoreListIter it = tokenParam.majorTokens.begin();
    //    it != tokenParam.majorTokens.end(); ++it)
    //{
    //    std::string key;
    //    it->first.convertString(key, izenelib::util::UString::UTF_8);
    //    tokens.push_back(key);
    //}
    //for (ProductTokenParam::TokenScoreListIter it = tokenParam.minorTokens.begin();
    //    it != tokenParam.minorTokens.end(); ++it)
    //{
    //    std::string key;
    //    it->first.convertString(key, izenelib::util::UString::UTF_8);
    //    tokens.push_back(key);
    //}
}

void AdSponsoredMgr::generateBidPhrase(const std::string& ad_title, BidPhraseT& bidphrase)
{
    bidphrase.clear();
    std::vector<std::pair<std::string, float> > tokens, subtokens;
    std::string brand;
    std::string model;
    BidKeywordId kid;

    // TODO: tokenize the str using the bid phrase dictionary.
    TitlePCAWrapper::get()->pca(ad_title, tokens, brand, model, subtokens, false);
    if (!brand.empty())
    {
        getBidKeywordId(brand, true, kid);
        bidphrase.push_back(kid);
    }
    if (!model.empty())
    {
        getBidKeywordId(model, true, kid);
        bidphrase.push_back(kid);
    }
    std::sort(tokens.begin(), tokens.end(), sort_tokens_func);
    for (size_t i = 0; i < tokens.size(); ++i)
    {
        if (tokens[i].first.length() > 1)
        {
            getBidKeywordId(tokens[i].first, true, kid);
            bidphrase.push_back(kid);
        }
        if (bidphrase.size() > (std::size_t)MAX_BIDPHRASE_LEN)
            break;
    }
    // further optimize : consider the long tail of bid keyword.
    // give more weight to the keyword with more revenue and low cost
}

double AdSponsoredMgr::getAdCTR(ad_docid_t adid)
{
    std::string ad_strid;
    getAdStrIdFromAdId(adid, ad_strid);
    return ad_log_mgr_->getAdCTR(ad_strid);
}

double AdSponsoredMgr::getAdRelevantScore(const BidPhraseT& bidphrase, const BidPhraseT& query_kid_list)
{
    return bidphrase.size() / query_kid_list.size();
}

double AdSponsoredMgr::getAdQualityScore(ad_docid_t adid, const BidPhraseT& bidphrase, const BidPhraseT& query_kid_list)
{
    return getAdCTR(adid) * getAdRelevantScore(bidphrase, query_kid_list);
}

bool AdSponsoredMgr::sponsoredAdSearch(const SearchKeywordOperation& actionOperation,
        KeywordSearchResult& searchResult)
{
    if (!ad_searcher_)
        return false;
    // search using OR mode for fuzzy search.
    (*const_cast<SearchKeywordOperation*>(&actionOperation)).actionItem_.searchingMode_.mode_ = SearchingMode::SPONSORED_AD_SEARCH_SUFFIX;
    ad_searcher_->search(actionOperation.actionItem_, searchResult);

    std::vector<std::string> query_keyword_list;
    // filter by broad match.
    std::string query = actionOperation.actionItem_.env_.queryString_;
    boost::to_lower(query);
    // tokenize the query to the bid keywords.
    tokenize(query, query_keyword_list);
    BidPhraseT query_kid_list;
    for(std::size_t i = 0; i < query_keyword_list.size(); ++i)
    {
        BidKeywordId id;
        if(getBidKeywordId(query_keyword_list[i], false, id))
        {
            query_kid_list.push_back(id);
        }
    }
    if (query_kid_list.empty())
        return false;
    std::sort(query_kid_list.begin(), query_kid_list.end());
    uint32_t first_kid = query_kid_list[0];
    uint32_t last_kid = query_kid_list[query_kid_list.size() - 1];
    boost::dynamic_bitset<> hit_set(last_kid - first_kid + 1);
    for(std::size_t i = 0; i < query_kid_list.size(); ++i)
    {
        hit_set[query_kid_list[i] - first_kid] = 1;
    }

    ScoreSortedSponsoredAdQueue ranked_queue(MAX_RANKED_AD_NUM);

    const std::vector<ad_docid_t>& result_list = searchResult.topKDocs_;
    std::vector<ad_docid_t> filtered_result_list;
    filtered_result_list.reserve(result_list.size());
    for(std::size_t i = 0; i < result_list.size(); ++i)
    {
        const BidPhraseT& bidphrase = ad_bidphrase_list_[result_list[i]];
        bool missed = false;
        for(std::size_t j = 0; j < bidphrase.size(); ++j)
        {
            if ((bidphrase[j] < first_kid) ||
                !hit_set.test(bidphrase[j] - first_kid))
            {
                missed = true;
                break;
            }
        }
        if (!missed)
        {
            int leftbudget = getBudgetLeft(result_list[i]);
            if (leftbudget > 0)
            {
                ScoreSponsoredAdDoc item;
                item.docId = result_list[i];

                int bidprice = 0;
                getAdBidPrice(item.docId, query, leftbudget, bidprice);
                if (bidprice > 0)
                {
                    item.qscore = getAdQualityScore(item.docId, bidphrase, query_kid_list);
                    item.score = bidprice * item.qscore;
                    ranked_queue.insert(item);
                }
                filtered_result_list.push_back(result_list[i]);
            }
        }
    }
    LOG(INFO) << "result num: " << result_list.size() << ", after broad match and budget filter: " << filtered_result_list.size();

    int count = (int)ranked_queue.size();
    searchResult.topKDocs_.resize(count);
    searchResult.topKRankScoreList_.resize(count);
    std::set<BidKeywordId> all_keyword_list;
    std::vector<std::string> ranked_ad_strlist(count);

    try
    {
        std::vector<double>& topKAdCost = dynamic_cast<AdKeywordSearchResult&>(searchResult).topKAdCost_;
        topKAdCost.resize(count);
        for(int i = count - 1; i >= 0; --i)
        {
            const ScoreSponsoredAdDoc& item = ranked_queue.pop();
            searchResult.topKDocs_[i] = item.docId;
            searchResult.topKRankScoreList_[i] = item.score;

            if (i == count - 1)
            {
                topKAdCost[i] = LOWEST_CLICK_COST;
            }
            else
            {
                topKAdCost[i] = LOWEST_CLICK_COST + searchResult.topKRankScoreList_[i - 1]/item.qscore;
            }
        }
    }
    catch(const std::exception& e)
    {
        LOG(WARNING) << "cast to AdKeywordSearchResult failed." << e.what();
        return false;
    }
    
    searchResult.totalCount_ = count;
    ad_log_mgr_->updateAdSearchStat(all_keyword_list, ranked_ad_strlist);

    return true;
}

bool AdSponsoredMgr::getAdIdFromAdStrId(const std::string& strid, ad_docid_t& id)
{
    uint128_t num_docid = Utilities::md5ToUint128(strid);
    return id_manager_->getDocIdByDocName(num_docid, id, false);
}

bool AdSponsoredMgr::getAdStrIdFromAdId(ad_docid_t adid, std::string& ad_strid)
{
    if (doc_mgr_)
    {
        Document doc;
        if (!doc_mgr_->getDocument(adid, doc))
        {
            return false;
        }
        Document::doc_prop_value_strtype docid_value;
        if (!doc.getProperty("DOCID", docid_value))
        {
            return false;
        }
        ad_strid = propstr_to_str(docid_value);
        return true;
    }
    return false;
}


} // namespace of sponsored

}
