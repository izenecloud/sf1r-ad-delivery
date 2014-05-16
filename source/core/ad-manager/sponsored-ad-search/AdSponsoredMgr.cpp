#include "AdSponsoredMgr.h"
#include "AdAucationLogMgr.h"
#include "../AdSearchService.h"
#include <la-manager/TitlePCAWrapper.h>
#include <document-manager/DocumentManager.h>
#include <mining-manager/group-manager/GroupManager.h>
#include "AdResultType.h"
#include <query-manager/SearchKeywordOperation.h>
#include <query-manager/ActionItem.h>
#include <search-manager/HitQueue.h>

namespace sf1r
{

namespace sponsored
{

static const int MAX_BIDPHRASE_LEN = 3;
static const int MAX_DIFF_BID_KEYWORD_NUM = 1000000;
static const int MAX_RANKED_AD_NUM = 10;
static const double LOWEST_CLICK_COST = 0.01;

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
    : grp_mgr_(NULL), doc_mgr_(NULL)
{
}

void AdSponsoredMgr::init(const std::string& dict_path,
    faceted::GroupManager* grp_mgr,
    DocumentManager* doc_mgr,
    AdSearchService* searcher)
{
    bid_title_pca_.reset(new TitlePCAWrapper());
    bid_title_pca_->loadDictFiles(dict_path);
    grp_mgr_ = grp_mgr;
    doc_mgr_ = doc_mgr;
    ad_searcher_ = searcher;
    keyword_value_id_list_.rehash(MAX_DIFF_BID_KEYWORD_NUM);
}

void AdSponsoredMgr::miningAdCreatives(ad_docid_t start_id)
{
    ad_docid_t end_id;
    end_id = doc_mgr_->getMaxDocId();
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
    }
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
    // TODO: convert string adid to int ad id.
    if (adid_int > ad_bidphrase_list_.size())
        return;
    bidphrase = ad_bidphrase_list_[adid_int];
}

void AdSponsoredMgr::generateBidPhrase(const std::string& ad_title, BidPhraseT& bidphrase)
{
    bidphrase.clear();
    std::vector<std::pair<std::string, float> > tokens, subtokens;
    std::string brand;
    std::string model;
    BidKeywordId kid;

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

void AdSponsoredMgr::generateBidPrice(ad_docid_t adid, std::vector<double>& price_list)
{
    // give the init bid price considered the budget, revenue and CTR.
    // Using the interface of the bid optimization module.
}

void AdSponsoredMgr::getRealTimeBidPrice(ad_docid_t adid, const std::string& query,
    double leftbudget, double& price)
{
    // if no real time bid available, just use the daily bid price.
}

double AdSponsoredMgr::getBudgetLeft(ad_docid_t adid)
{
    if (adid >= ad_campaign_belong_list_.size())
        return 0;
    std::string campaign_name = ad_campaign_name_list_[ad_campaign_belong_list_[adid]];
    boost::unordered_map<std::string, double>::const_iterator it = ad_budget_left_list_.find(campaign_name);   
    if (it == ad_budget_left_list_.end())
        return 0;
    return it->second;
}

double AdSponsoredMgr::getAdCTR(ad_docid_t adid)
{
    return AdAucationLogMgr::get()->getAdCTR(getAdStrIdFromAdId(adid));
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
    // search using OR mode for fuzzy search.
    (*const_cast<SearchKeywordOperation*>(&actionOperation)).actionItem_.searchingMode_.mode_ = SearchingMode::SUFFIX_MATCH;
    ad_searcher_->search(actionOperation.actionItem_, searchResult);

    std::vector<std::string> query_keyword_list;
    // filter by broad match.
    std::string query = actionOperation.actionItem_.env_.queryString_;
    boost::to_lower(query);
    // tokenize the query to the bid keywords.
    //
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
            double leftbudget = getBudgetLeft(result_list[i]);
            if (leftbudget > 0)
            {
                ScoreSponsoredAdDoc item;
                item.docId = result_list[i];

                double bidprice = 0;
                getRealTimeBidPrice(item.docId, query, leftbudget, bidprice);
                item.qscore = getAdQualityScore(item.docId, bidphrase, query_kid_list);
                item.score = bidprice * item.qscore;
                ranked_queue.insert(item);

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
    AdAucationLogMgr::get()->updateAdSearchStat(all_keyword_list, ranked_ad_strlist);

    return true;
}

ad_docid_t AdSponsoredMgr::getAdIdFromAdStrId(const std::string& strid)
{
    ad_docid_t id;
    return id;
}

std::string AdSponsoredMgr::getAdStrIdFromAdId(ad_docid_t adid)
{
    std::string ad_strid;
    return ad_strid;
}


} // namespace of sponsored

}
