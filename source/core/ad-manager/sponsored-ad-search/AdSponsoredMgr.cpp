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
#include <util/ClockTimer.h>

#include <fstream>
#include <util/izene_serialization.h>
#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/split.hpp>

namespace bfs = boost::filesystem;
using namespace izenelib::util;
namespace sf1r
{

namespace sponsored
{

static const int MAX_BIDPHRASE_LEN = 3;
static const int MAX_DIFF_BID_KEYWORD_NUM = 1000000;
static const int MAX_RANKED_AD_NUM = 20;
static const int LOWEST_CLICK_COST = 1;
static const int DEFAULT_AD_BUDGET = 1000;
static const double MIN_AD_SCORE = 1e-6;

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
    std::string hit_bidstr;
    std::string hit_orig_bidstr;
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
    : grp_mgr_(NULL), doc_mgr_(NULL), id_manager_(NULL), bid_strategy_type_(UniformBid)
{
}

AdSponsoredMgr::~AdSponsoredMgr()
{
    save();
}

void AdSponsoredMgr::init(const std::string& res_path,
    const std::string& data_path,
    const std::string& commondata_path,
    const std::string& adtitle_prop,
    const std::string& adbidphrase_prop,
    const std::string& adcampaign_prop,
    faceted::GroupManager* grp_mgr,
    DocumentManager* doc_mgr,
    izenelib::ir::idmanager::IDManager* id_manager,
    AdSearchService* searcher)
{
    data_path_ = data_path;

    if (!bfs::exists(data_path_))
    {
        bfs::create_directories(data_path_);
    }
    ad_title_prop_ = adtitle_prop;
    ad_bidphrase_prop_ = adbidphrase_prop;
    ad_campaign_prop_ = adcampaign_prop;

    grp_mgr_ = grp_mgr;
    doc_mgr_ = doc_mgr;
    id_manager_ = id_manager;
    ad_searcher_ = searcher;
    keyword_value_id_list_.rehash(MAX_DIFF_BID_KEYWORD_NUM);
    ad_log_mgr_.reset(new AdAuctionLogMgr());
    ad_log_mgr_->init(commondata_path);
    ad_bid_strategy_.reset(new AdBidStrategy());

    manual_bidinfo_mgr_.init(data_path_);

    load();
    resetDailyLogStatisticalData(false);
}

void AdSponsoredMgr::updateAuctionLogData(const std::string& ad_strid, const std::string& hit_orig_bidstr,
    int click_cost_in_fen, uint32_t click_slot)
{
    std::string hit_bidstr = preprocessBidPhraseStr(hit_orig_bidstr);
    ad_log_mgr_->updateAuctionLogData(ad_strid, hit_bidstr, click_cost_in_fen, click_slot);
    ad_docid_t adid;
    if (!getAdIdFromAdStrId(ad_strid, adid))
    {
        LOG(INFO) << "ad string id not found: " << ad_strid;
        return;
    }
    LOG(INFO) << "ad :" << ad_strid << " used budget: " << click_cost_in_fen << " at slot: " << click_slot;
    consumeBudget(adid, click_cost_in_fen);
}

void AdSponsoredMgr::save()
{
    std::ofstream ofs(std::string(data_path_ + "/sponsored_ad.data").c_str());
    std::size_t len = 0;
    char* buf = NULL;
    {
        izenelib::util::izene_serialization<std::vector<BidPhraseListT> > izs(ad_bidphrase_list_);
        izs.write_image(buf, len);
        ofs.write((const char*)&len, sizeof(len));
        ofs.write(buf, len);
        ofs.flush();

    }
    {
        len = 0;
        izenelib::util::izene_serialization<std::vector<BidPhraseStrListT> > izs(ad_orig_bidstr_list_);
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

    std::ofstream txt_data(std::string(data_path_ + "/sponsored_ad.txt").c_str());
    for(std::size_t i = 0; i < ad_bidphrase_list_.size(); ++i)
    {
        txt_data << i << ":";
        for(std::size_t j = 0; j < ad_bidphrase_list_[i].size(); ++j)
        {
            for(std::size_t k = 0; k < ad_bidphrase_list_[i][j].size(); ++k)
            {
                txt_data << ad_bidphrase_list_[i][j][k] << ",";
            }
            txt_data << "-:-";
        }
        txt_data << std::endl;
    }
    for(std::size_t i = 0; i < keyword_id_value_list_.size(); ++i)
    {
        txt_data << i << ":" << keyword_id_value_list_[i] << std::endl;
    }

    txt_data.close();

    {
        len = 0;
        izenelib::util::izene_serialization<std::vector<int> > izs(ad_budget_used_list_);
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
    {
        len = 0;
        izenelib::util::izene_serialization<std::vector<std::map<BidPhraseStrT, int> > > izs(ad_bid_price_list_);
        izs.write_image(buf, len);
        ofs.write((const char*)&len, sizeof(len));
        ofs.write(buf, len);
        ofs.flush();
    }
    {
        len = 0;
        izenelib::util::izene_serialization<std::vector<std::vector<std::pair<int, double> > > > izs(ad_uniform_bid_price_list_);
        izs.write_image(buf, len);
        ofs.write((const char*)&len, sizeof(len));
        ofs.write(buf, len);
        ofs.flush();
    }

    ofs.close();
    if (ad_log_mgr_)
        ad_log_mgr_->save();
    manual_bidinfo_mgr_.save();
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
            izenelib::util::izene_deserialization<std::vector<BidPhraseListT> > izd(data.data(), data.size());
            izd.read_image(ad_bidphrase_list_);
        }
        {
            len = 0;
            ifs.read((char*)&len, sizeof(len));
            data.resize(len);
            ifs.read((char*)&data[0], len);
            izenelib::util::izene_deserialization<std::vector<BidPhraseStrListT> > izd(data.data(), data.size());
            izd.read_image(ad_orig_bidstr_list_);
        }

        LOG(INFO) << "ad bidphrase loaded : " << ad_bidphrase_list_.size();
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

        LOG(INFO) << "keyword info loaded: " << keyword_id_value_list_.size();
        {
            len = 0;
            ifs.read((char*)&len, sizeof(len));
            data.resize(len);
            ifs.read((char*)&data[0], len);
            izenelib::util::izene_deserialization<std::vector<int> > izd(data.data(), data.size());
            izd.read_image(ad_budget_used_list_);
        }

        LOG(INFO) << "campaign budget info loaded: " << ad_budget_used_list_.size();

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
        ad_campaign_bid_phrase_list_.resize(ad_campaign_name_id_list_.size());
        for (std::size_t i = 0; i < ad_bidphrase_list_.size(); ++i)
        {
            uint32_t campaign_id = ad_campaign_belong_list_[i];
            const BidPhraseListT& bidphrase_list = ad_bidphrase_list_[i];
            BidPhraseStrListT bidstr_list;
            getBidPhraseStrList(bidphrase_list, bidstr_list);
            ad_campaign_bid_phrase_list_[campaign_id].insert(bidstr_list.begin(), bidstr_list.end());
        }
        LOG(INFO) << "campaign name info loaded: " << ad_campaign_name_list_.size();

        {
            len = 0;
            ifs.read((char*)&len, sizeof(len));
            data.resize(len);
            ifs.read((char*)&data[0], len);
            izenelib::util::izene_deserialization<std::vector<std::map<BidPhraseStrT, int> > > izd(data.data(), data.size());
            izd.read_image(ad_bid_price_list_);
        }

        {
            len = 0;
            ifs.read((char*)&len, sizeof(len));
            data.resize(len);
            ifs.read((char*)&data[0], len);
            izenelib::util::izene_deserialization<std::vector<std::vector<std::pair<int, double> > > > izd(data.data(), data.size());
            izd.read_image(ad_uniform_bid_price_list_);
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
    ad_orig_bidstr_list_.resize(end_id + 1);
    ad_status_bitmap_.resize(end_id + 1, false);
    std::string ad_title;
    std::vector<std::string> ad_bid_phrase_strlist;
    std::vector<std::string> ad_processed_bid_phrase_strlist;
    std::vector<std::string> pricestr_list;
    std::vector<int> priceint_list;
    BidPhraseListT  bidphrase_list;
    BidPhraseT bidphrase;
    for(ad_docid_t i = start_id; i <= end_id; ++i)
    {
        Document::doc_prop_value_strtype prop_value;
        doc_mgr_->getPropertyValue(i, ad_bidphrase_prop_, prop_value);
        const std::string bidstr = propstr_to_str(prop_value);
        doc_mgr_->getPropertyValue(i, ad_campaign_prop_, prop_value);
        std::string campaign = propstr_to_str(prop_value);
        if (campaign.empty())
        {
            doc_mgr_->getPropertyValue(i, "DOCID", prop_value);
            campaign = propstr_to_str(prop_value);
        }

        if (!bidstr.empty())
        {
            ad_bid_phrase_strlist.clear();
            ad_processed_bid_phrase_strlist.clear();
            pricestr_list.clear();
            priceint_list.clear();

            boost::split(ad_bid_phrase_strlist, bidstr, boost::is_any_of(",;"));
            bidphrase_list.resize(ad_bid_phrase_strlist.size());

            doc_mgr_->getPropertyValue(i, "KeywordsPrice", prop_value);
            const std::string bidprice_str = propstr_to_str(prop_value);
            if (!bidprice_str.empty())
            {
                boost::split(pricestr_list, bidprice_str, boost::is_any_of(","));
            }
            if (pricestr_list.size() != ad_bid_phrase_strlist.size())
            {
                LOG(WARNING) << "bid price list number not matched with bid string.";
                pricestr_list.resize(ad_bid_phrase_strlist.size(), 0);
            }

            ad_processed_bid_phrase_strlist.resize(ad_bid_phrase_strlist.size());
            priceint_list.resize(ad_bid_phrase_strlist.size());
            for(std::size_t j = 0; j < ad_bid_phrase_strlist.size(); ++j)
            {
                generateBidPhrase(ad_bid_phrase_strlist[j], bidphrase);
                getBidPhraseStr(bidphrase, ad_processed_bid_phrase_strlist[j]);
                bidphrase_list[j].swap(bidphrase);
                priceint_list[j] = int(boost::lexical_cast<double>(pricestr_list[j]) * 100);
            }
            manual_bidinfo_mgr_.setManualBidPrice(campaign, ad_processed_bid_phrase_strlist, priceint_list);
            ad_orig_bidstr_list_[i].swap(ad_bid_phrase_strlist);
        }
        else
        {
            doc_mgr_->getPropertyValue(i, ad_title_prop_, prop_value);
            ad_title = propstr_to_str(prop_value);
            generateBidPhrase(ad_title, bidphrase);
            bidphrase_list.resize(1);
            bidphrase_list[0].swap(bidphrase);
            getBidPhraseStrList(bidphrase_list, ad_orig_bidstr_list_[i]);
        }

        ad_bidphrase_list_[i].swap(bidphrase_list);
        updateAdCampaign(i, campaign);
        if (i % 100000 == 0)
        {
            LOG(INFO) << "mining ad creative for sponsored search : " << i;
        }
    }
    resetDailyLogStatisticalData(false);
    save();
}

void AdSponsoredMgr::updateAdBidPhrase(ad_docid_t adid, const std::vector<std::string>& bid_phrase_list,
    BidPhraseListT& bidid_list)
{
    bidid_list.clear();
    BidPhraseT phrase;
    for(std::size_t j = 0; j < bid_phrase_list.size(); ++j)
    {
        generateBidPhrase(bid_phrase_list[j], phrase);
        bidid_list.push_back(phrase);
    }

    ad_bidphrase_list_[adid].swap(bidid_list);
    ad_orig_bidstr_list_[adid] = bid_phrase_list;
}

void AdSponsoredMgr::getBidPhraseStr(const BidPhraseT& bidphrase, BidPhraseStrT& bidstr)
{
    bidstr.clear();
    for(std::size_t j = 0; j < bidphrase.size(); ++j)
    {
        std::string keystr;
        getBidKeywordStrFromId(bidphrase[j], keystr);
        bidstr += keystr + " ";
    }
}

void AdSponsoredMgr::getBidPhraseStrList(const BidPhraseListT& bidphrase_list, BidPhraseStrListT& bidstr_list)
{
    bidstr_list.resize(bidphrase_list.size());
    for(std::size_t i = 0; i < bidphrase_list.size(); ++i)
    {
        getBidPhraseStr(bidphrase_list[i], bidstr_list[i]);
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
    if (campaign_id >= ad_campaign_bid_phrase_list_.size())
    {
        ad_campaign_bid_phrase_list_.resize(campaign_id + 1);
    }
    if (adid >= ad_bidphrase_list_.size())
    {
        return;
    }
    const BidPhraseListT& bidphrase_list = ad_bidphrase_list_[adid];
    BidPhraseStrListT bidstr_list;
    getBidPhraseStrList(bidphrase_list, bidstr_list);
    ad_campaign_bid_phrase_list_[campaign_id].insert(bidstr_list.begin(), bidstr_list.end());
}

void AdSponsoredMgr::getBidStatisticalData(const std::set<BidPhraseStrT>& bidstr_list,
    const std::map<std::string, BidAuctionLandscapeT>& bidkey_cpc_map,
    const std::map<std::string, int>& manual_bidprice_list,
    std::vector<AdQueryStatisticInfo>& ad_statistical_data)
{
    ad_statistical_data.clear();
    std::set<BidPhraseStrT>::const_iterator bid_it = bidstr_list.begin();
    for(; bid_it != bidstr_list.end(); ++bid_it)
    {
        const BidPhraseStrT& bidstr = *bid_it;
        ad_statistical_data.push_back(AdQueryStatisticInfo());
        ad_statistical_data.back().impression_ = ad_log_mgr_->getKeywordAvgDailyImpression(bidstr);
        ad_statistical_data.back().minBid_ = LOWEST_CLICK_COST;
        std::map<std::string, int>::const_iterator manual_it = manual_bidprice_list.find(bidstr);
        if (manual_it != manual_bidprice_list.end())
        {
            ad_statistical_data.back().bid_ = manual_it->second;
        }
        std::map<std::string, BidAuctionLandscapeT>::const_iterator it = bidkey_cpc_map.find(bidstr);
        if (it == bidkey_cpc_map.end())
            continue;
        for (std::size_t j = 0; j < it->second.size(); ++j)
        {
            ad_statistical_data.back().cpc_.push_back(it->second[j].first);
            ad_statistical_data.back().ctr_.push_back(it->second[j].second);
        }
    }
}

// refresh daily left budget periodically since the bid price and budget can be changed hourly.
void AdSponsoredMgr::resetDailyLogStatisticalData(bool reset_used)
{
    LOG(INFO) << "begin compute auto bid strategy.";
    if (reset_used)
    {
        LOG(INFO) << "budget used reset.";
        ad_budget_used_list_.clear();
    }
    ad_budget_used_list_.resize(ad_campaign_name_list_.size(), 0);

    struct tm time_now;
    time_t current = time(NULL);
    gmtime_r(&current, &time_now);

    double left_ratio = (24 - time_now.tm_hour)/(double)24;
    int ad_daily_budget = 0;

    std::vector<AdQueryStatisticInfo> ad_statistical_data;
    std::vector<BidPhraseStrT> bidstr_list;
    std::vector<AdAuctionLogMgr::BidAuctionLandscapeT> cost_click_list;
    ad_log_mgr_->getKeywordBidLandscape(bidstr_list, cost_click_list);
    std::map<std::string, BidAuctionLandscapeT> bidkey_cpc_map;
    for(std::size_t i = 0; i < bidstr_list.size(); ++i)
    {
        bidkey_cpc_map[bidstr_list[i]] = cost_click_list[i];
    }

    ad_ctr_list_.resize(ad_bidphrase_list_.size());
    for(std::size_t i = 0; i < ad_bidphrase_list_.size(); ++i)
    {
        ad_ctr_list_[i] = computeAdCTR(i);
    }
    LOG(INFO) << "compute ad ctr finished.";
    std::ofstream bid_price_file(std::string(data_path_ + "/bid_price.txt").c_str());

    // recompute the bid strategy.
    if (bid_strategy_type_ == UniformBid)
    {
        std::vector<std::pair<int, double> > bid_price_list;
        ad_uniform_bid_price_list_.resize(ad_campaign_name_list_.size());
        for(std::size_t i = 0; i < ad_campaign_name_list_.size(); ++i)
        {
            if (i >= ad_campaign_bid_phrase_list_.size())
                break;

            // because the budget may change every hour, 
            // the daily budget should be computed to reflect the change.
            ad_daily_budget = (manual_bidinfo_mgr_.getBidBudget(ad_campaign_name_list_[i]) - ad_budget_used_list_[i])/left_ratio;

            const CampaignBidStrListT& bidstr_list = ad_campaign_bid_phrase_list_[i];
            std::map<std::string, int> manual_bidprice_list;
            manual_bidinfo_mgr_.getManualBidPriceList(ad_campaign_name_list_[i], manual_bidprice_list);
            getBidStatisticalData(bidstr_list, bidkey_cpc_map, manual_bidprice_list, ad_statistical_data);
            bid_price_list = ad_bid_strategy_->convexUniformBid(ad_statistical_data, ad_daily_budget);
            bid_price_file << "campaign: " << ad_campaign_name_list_[i] << ": ";
            for(std::size_t j = 0; j < bid_price_list.size(); ++j)
            {
                bid_price_file << bid_price_list[j].first << "(" << bid_price_list[j].second << "),"; 
            }
            bid_price_file << std::endl;
            ad_uniform_bid_price_list_[i].swap(bid_price_list);
            if (i % 10000 == 0)
                LOG(INFO) << "bid strategy computed: " << i;
        }
    }
    else if (bid_strategy_type_ == GeneticBid)
    {
        ad_bid_price_list_.resize(ad_campaign_name_list_.size());
        for (std::size_t i = 0; i < ad_campaign_name_list_.size(); ++i)
        {
            if (i >= ad_campaign_bid_phrase_list_.size())
                break;
            const CampaignBidStrListT& bidstr_list = ad_campaign_bid_phrase_list_[i];
            std::map<std::string, int> manual_bidprice_list;
            manual_bidinfo_mgr_.getManualBidPriceList(ad_campaign_name_list_[i], manual_bidprice_list);
            getBidStatisticalData(bidstr_list, bidkey_cpc_map, manual_bidprice_list, ad_statistical_data);

            ad_daily_budget = (manual_bidinfo_mgr_.getBidBudget(ad_campaign_name_list_[i]) - ad_budget_used_list_[i])/left_ratio;

            std::vector<int> bid_price_list = ad_bid_strategy_->geneticBid(ad_statistical_data, ad_daily_budget);
            assert(bid_price_list.size() == bidstr_list.size());
            CampaignBidStrListT::const_iterator bid_it = bidstr_list.begin();
            bid_price_file << "campaign: " << ad_campaign_name_list_[i] << ": ";
            for(std::size_t j = 0; j < bid_price_list.size(); ++j)
            {
                bid_price_file << *bid_it << "-" << bid_price_list[j] << ", ";
                ad_bid_price_list_[i][*bid_it] = bid_price_list[j];
            }
            bid_price_file << std::endl;
            if (i % 10000 == 0)
                LOG(INFO) << "bid strategy computed: " << i;
        }
    }
    else
    {
        // realtime bid.
    }
    bid_price_file.close();
    LOG(INFO) << "end compute auto bid strategy.";
}

bool AdSponsoredMgr::updateAdOnlineStatus(const std::string& ad_strid, bool is_online)
{
    uint32_t adid = 0;
    if (!getAdIdFromAdStrId(ad_strid, adid))
        return false;
    if (!is_online)
    {
        ad_status_bitmap_.set(adid);
    }
    else
    {
        ad_status_bitmap_.reset(adid);
    }
    return true;
}

void AdSponsoredMgr::setManualBidPrice(const std::string& campaign_name,
    const std::vector<std::string>& bidphrase_list,
    const std::vector<int>& price_list)
{
    if (bidphrase_list.size() != price_list.size())
    {
        LOG(ERROR) << "the manual bidprice list size not match.";
        return;
    }
    // 
    std::vector<std::string> bidstr_list(bidphrase_list.size());
    for (std::size_t i = 0; i < bidphrase_list.size(); ++i)
    {
        bidstr_list[i] = preprocessBidPhraseStr(bidphrase_list[i]);
    }
    manual_bidinfo_mgr_.setManualBidPrice(campaign_name, bidstr_list, price_list);
}

std::string AdSponsoredMgr::preprocessBidPhraseStr(const std::string& orig)
{
    std::vector<string> tokens_list;
    generateBidPhrase(orig, tokens_list);
    if (tokens_list.size() > (std::size_t)MAX_BIDPHRASE_LEN)
        tokens_list.resize(MAX_BIDPHRASE_LEN);

    std::string new_str;
    for(std::size_t i = 0; i < tokens_list.size(); ++i)
    {
        new_str += tokens_list[i] + " ";
    }
    return new_str;
}

void AdSponsoredMgr::changeDailyBudget(const std::string& ad_campaign_name, int dailybudget)
{
    manual_bidinfo_mgr_.setBidBudget(ad_campaign_name, dailybudget);
    manual_bidinfo_mgr_.save();
}

void AdSponsoredMgr::consumeBudget(ad_docid_t adid, int cost)
{
    if (adid >= ad_campaign_belong_list_.size())
    {
        LOG(WARNING) << "the ad id not belong to any campaign: " << adid;
        return;
    }
    uint32_t campaign_id = ad_campaign_belong_list_[adid];
    if (campaign_id >= ad_budget_used_list_.size())
    {
        LOG(WARNING) << "no budget was set for this campaign :" << campaign_id;
        return;
    }
    ad_budget_used_list_[campaign_id] += cost;
}

void AdSponsoredMgr::generateBidPrice(ad_docid_t adid, std::vector<int>& price_list)
{
    // give the init bid price considered the budget, revenue and CTR.
    // Using the interface of the bid optimization module.
}

void AdSponsoredMgr::getAdBidPrice(ad_docid_t adid, const std::string& query, const std::string& hit_bidstr,
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
        if (campaign_id >= ad_budget_used_list_.size() || campaign_id >= ad_campaign_name_list_.size())
            return;
        int used_budget = ad_budget_used_list_[campaign_id];
        int left_budget = manual_bidinfo_mgr_.getBidBudget(ad_campaign_name_list_[campaign_id]) - used_budget;
        AdQueryStatisticInfo info;
        int current_impression = ad_log_mgr_->getKeywordCurrentImpression(hit_bidstr);
        ad_log_mgr_->getKeywordStatData(hit_bidstr, info.impression_,
            info.cpc_, info.ctr_);
        info.impression_ -= current_impression;
        info.minBid_ = LOWEST_CLICK_COST;
        int tmp_price = ad_bid_strategy_->realtimeBidWithRevenueMax(info, used_budget, left_budget);
        if (tmp_price > price)
        {
            price = tmp_price;
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
        const std::map<BidPhraseStrT, int>& bid_list = ad_bid_price_list_[campaign_id];
        if (adid >= ad_bidphrase_list_.size())
            return;
        std::map<BidPhraseStrT, int>::const_iterator it = bid_list.find(hit_bidstr);
        if (it != bid_list.end())
        {
            price = it->second;
        }
    }
}

int AdSponsoredMgr::getBudgetLeft(ad_docid_t adid)
{
    if (adid >= ad_campaign_belong_list_.size())
        return 0;
    std::size_t campaign_id = ad_campaign_belong_list_[adid];
    if (campaign_id >= ad_budget_used_list_.size())
        return 0;
    int left_budget = manual_bidinfo_mgr_.getBidBudget(ad_campaign_name_list_[campaign_id]) - ad_budget_used_list_[campaign_id];
    return left_budget;
}

bool AdSponsoredMgr::getBidKeywordIdFromStr(const BidKeywordStrT& keyword, bool insert, BidKeywordId& id)
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

void AdSponsoredMgr::getBidKeywordStrFromId(const BidKeywordId& id, BidKeywordStrT& keyword)
{
    if (id >= keyword_id_value_list_.size())
    {
        keyword.clear();
        return;
    }
    keyword = keyword_id_value_list_[id];
}


void AdSponsoredMgr::getBidPhrase(const std::string& adid,
    BidPhraseListT& bidphrase_list)
{
    bidphrase_list.clear();
    ad_docid_t adid_int = 0;
    if (!getAdIdFromAdStrId(adid, adid_int))
        return;
    if (adid_int > ad_bidphrase_list_.size())
        return;
    bidphrase_list = ad_bidphrase_list_[adid_int];
}

void AdSponsoredMgr::getBidPhrase(const std::string& adid,
    BidPhraseListT& bidphrase_list,
    BidPhraseStrListT& bidstr_list)
{
    bidstr_list.clear();
    getBidPhrase(adid, bidphrase_list);
    getBidPhraseStrList(bidphrase_list, bidstr_list);
}

void AdSponsoredMgr::generateBidPhrase(const std::string& ad_title, std::vector<std::string>& bidphrase)
{
    bidphrase.clear();
    std::string pattern = ad_title;
    boost::to_lower(pattern);

    std::vector<std::pair<std::string, float> > tokens, subtokens;
    std::string brand;
    std::string model;

    TitlePCAWrapper::get()->pca(pattern, tokens, brand, model, subtokens, false);
    std::sort(tokens.begin(), tokens.end(), sort_tokens_func);
    for (size_t i = 0; i < tokens.size(); ++i)
    {
        bidphrase.push_back(tokens[i].first);
    }
}

void AdSponsoredMgr::generateBidPhrase(const std::string& ad_title, BidPhraseT& bidphrase)
{
    if (ad_title.empty())
    {
        LOG(INFO) << "empty ad tiltle";
        return;
    }
    bidphrase.clear();
    std::string pattern = ad_title;
    boost::to_lower(pattern);
    std::vector<std::pair<std::string, float> > tokens, subtokens;
    std::string brand;
    std::string model;
    BidKeywordId kid;

    TitlePCAWrapper::get()->pca(pattern, tokens, brand, model, subtokens, false);
    if (tokens.empty())
    {
        LOG(INFO) << "empty tokens for title: " << ad_title;
        return;
    }
    std::sort(tokens.begin(), tokens.end(), sort_tokens_func);
    for (size_t i = 0; i < tokens.size(); ++i)
    {
        getBidKeywordIdFromStr(tokens[i].first, true, kid);
        bidphrase.push_back(kid);
        if (bidphrase.size() > (std::size_t)MAX_BIDPHRASE_LEN)
            break;
    }
    // further optimize : consider the long tail of bid keyword.
    // give more weight to the keyword with more revenue and low cost
}

double AdSponsoredMgr::computeAdCTR(ad_docid_t adid)
{
    std::string ad_strid;
    if (!getAdStrIdFromAdId(adid, ad_strid))
        return 0;
    return ad_log_mgr_->getAdCTR(ad_strid);
}

bool AdSponsoredMgr::sponsoredAdSearch(const SearchKeywordOperation& actionOperation,
        KeywordSearchResult& searchResult)
{
    if (!ad_searcher_)
        return false;
    // search using OR mode for fuzzy search.
    KeywordSearchActionItem modified_action = actionOperation.actionItem_;
    modified_action.searchingMode_.mode_ = SearchingMode::SPONSORED_AD_SEARCH_SUFFIX;

    std::vector<std::string> query_keyword_list;
    const std::string& query = actionOperation.actionItem_.env_.queryString_;
    std::string modified_query;
    // filter by broad match.
    // tokenize the query to the bid keywords.
    generateBidPhrase(query, query_keyword_list);
    LOG(INFO) << "query for bid phrase: " << query_keyword_list.size();
    BidPhraseT query_kid_list;
    for(std::size_t i = 0; i < query_keyword_list.size(); ++i)
    {
        LOG(INFO) << query_keyword_list[i];
        BidKeywordId id;
        if(getBidKeywordIdFromStr(query_keyword_list[i], false, id))
        {
            query_kid_list.push_back(id);
            modified_query += query_keyword_list[i] + " ";
        }
    }
    if (query_kid_list.empty())
    {
        LOG(INFO) << "no hit keyword.";
        searchResult.totalCount_ = 0;
        searchResult.topKDocs_.resize(0);
        searchResult.topKRankScoreList_.resize(0);
        return true;
    }

    modified_action.env_.queryString_ = modified_query;

    LOG(INFO) << "modified sponsored search query: " << modified_query;
    ad_searcher_->search(modified_action, searchResult);
    (*const_cast<SearchKeywordOperation*>(&actionOperation)).actionItem_.disableGetDocs_ = modified_action.disableGetDocs_;

    std::sort(query_kid_list.begin(), query_kid_list.end());
    uint32_t first_kid = query_kid_list[0];
    uint32_t last_kid = query_kid_list[query_kid_list.size() - 1];
    boost::dynamic_bitset<> hit_set(last_kid - first_kid + 1);
    std::cout << "query keyword id: ";
    for(std::size_t i = 0; i < query_kid_list.size(); ++i)
    {
        hit_set[query_kid_list[i] - first_kid] = 1;
        std::cout << query_kid_list[i] << ",";
    }
    std::cout << std::endl;

    ScoreSortedSponsoredAdQueue ranked_queue(MAX_RANKED_AD_NUM);

    const std::vector<ad_docid_t>& result_list = searchResult.topKDocs_;
    std::vector<ad_docid_t> filtered_result_list;
    filtered_result_list.reserve(result_list.size());
    LOG(INFO) << "begin filter by broad match.";
    ClockTimer t1, t2, t3;
    double t1_total = 0;
    double t2_total = 0;
    double t3_total = 0;
    for(std::size_t i = 0; i < result_list.size(); ++i)
    {
        if (ad_status_bitmap_.test(result_list[i]))
        {
            // this item is offline.
            continue;
        }
        int best_match_index = -1;
        std::size_t best_match_size = 0;
        const BidPhraseListT& bidphrase_list = ad_bidphrase_list_[result_list[i]];
        if (bidphrase_list.empty())
            continue;
        t1.restart();
        //std::cout << "bid for ad: " << result_list[i] << ": ";
        for(std::size_t j = 0; j < bidphrase_list.size(); ++j)
        {
            bool missed = false;
            for(std::size_t k = 0; k < bidphrase_list[j].size(); ++k)
            {
                //std::cout << bidphrase[j] << ",";
                if ((bidphrase_list[j][k] < first_kid) ||
                    !hit_set.test(bidphrase_list[j][k] - first_kid))
                {
                    missed = true;
                    break;
                }
            }
            if (!missed)
            {
                if (bidphrase_list[j].size() > best_match_size)
                {
                    best_match_size = bidphrase_list[j].size();
                    best_match_index = j;
                }
            }
        }
        t1_total += t1.elapsed();
        //std::cout << std::endl;
        if (best_match_index == -1)
        {
            continue;
        }
        t2.restart();
        int leftbudget = getBudgetLeft(result_list[i]);
        if (leftbudget > 0)
        {
            t3.restart();
            ScoreSponsoredAdDoc item;
            item.docId = result_list[i];
            item.hit_orig_bidstr = ad_orig_bidstr_list_[item.docId][best_match_index];
            getBidPhraseStr(bidphrase_list[best_match_index], item.hit_bidstr);

            int bidprice = 0;
            getAdBidPrice(item.docId, query, item.hit_bidstr, leftbudget, bidprice);
            bidprice = std::max(LOWEST_CLICK_COST, bidprice);
            item.qscore = getAdQualityScore(item.docId, bidphrase_list[best_match_index], query_kid_list);
            item.score = bidprice * item.qscore;
            if (item.score > MIN_AD_SCORE)
            {
                ranked_queue.insert(item);
            }
            filtered_result_list.push_back(result_list[i]);
            t3_total += t3.elapsed();
        }
        t2_total += t2.elapsed();
    }
    LOG(INFO) << "result num: " << result_list.size() << ", after broad match and budget filter: " << filtered_result_list.size();
    LOG(INFO) << "time cost: " << t1_total << ", " << t2_total << ", " << t3_total;
 
    int count = (int)ranked_queue.size();
    searchResult.topKDocs_.resize(count);
    searchResult.topKRankScoreList_.resize(count);
    std::set<std::string>  viewed_bidstr_list;
    std::vector<std::string> ranked_ad_strlist(count);

    try
    {
        std::vector<double>& topKAdCost = dynamic_cast<AdKeywordSearchResult&>(searchResult).topKAdCost_;
        std::vector<std::string>& topKAdBidStrList = dynamic_cast<AdKeywordSearchResult&>(searchResult).topKAdBidStrList_;
        topKAdCost.resize(count);
        topKAdBidStrList.resize(count);
        for(int i = count - 1; i >= 0; --i)
        {
            const ScoreSponsoredAdDoc& item = ranked_queue.pop();
            searchResult.topKDocs_[i] = item.docId;
            searchResult.topKRankScoreList_[i] = item.score;
            topKAdBidStrList[i] = item.hit_orig_bidstr;
            viewed_bidstr_list.insert(item.hit_bidstr);

            getAdStrIdFromAdId(item.docId, ranked_ad_strlist[i]);

            if (i == count - 1)
            {
                topKAdCost[i] = LOWEST_CLICK_COST*100;
            }
            else
            {
                topKAdCost[i] = 100*(LOWEST_CLICK_COST + searchResult.topKRankScoreList_[i + 1]/item.qscore);
            }
        }
    }
    catch(const std::exception& e)
    {
        LOG(WARNING) << "cast to AdKeywordSearchResult failed." << e.what();
        return false;
    }
    
    searchResult.totalCount_ = count;
    ad_log_mgr_->updateAdSearchStat(viewed_bidstr_list, ranked_ad_strlist);
    LOG(INFO) << "end of sponsored search.";

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
