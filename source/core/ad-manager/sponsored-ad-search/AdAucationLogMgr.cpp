#include "AdAucationLogMgr.h"
#include <time.h>
#include <boost/lexical_cast.hpp>
#include <glog/logging.h>

namespace sf1r { namespace sponsored {

static const int STAT_COMPUTE_INTERVAL = 3;
static const int MAX_AD_SLOT = 10;
static const double DEFAULT_AD_CTR = 0.002;
static const int DEFAULT_CLICK_COST = 5;

static const int MIN_SEARCH_NUM = 1000;

static std::string getdaystr(int day)
{
    struct tm time_s;
    time_t today = time(NULL);
    gmtime_r(&today, &time_s);
    time_s.tm_yday += day;
    if (time_s.tm_yday < 0)
    {
        time_s.tm_yday += 365;
        time_s.tm_year -= 1;
    }
    else if (time_s.tm_yday > 365)
    {
        time_s.tm_year += 1;
        time_s.tm_yday -= 365;
    }
    return boost::lexical_cast<std::string>(time_s.tm_year) +
        boost::lexical_cast<std::string>(time_s.tm_yday);
}

inline int getDefaultCostForSlot(int slot)
{
    return DEFAULT_CLICK_COST * (MAX_AD_SLOT - slot)/MAX_AD_SLOT;
}

inline double getDefaultCTRForSlot(int slot)
{
    return DEFAULT_AD_CTR * (MAX_AD_SLOT - slot) / MAX_AD_SLOT;
}

AdAucationLogMgr::AdAucationLogMgr()
{

}

// update the impression info after search request.
void AdAucationLogMgr::updateAdSearchStat(const std::set<BidKeywordId>& keyword_list,
    const std::vector<std::string>& ranked_ad_list)
{
    std::string todaystr = getdaystr(0);
    for(std::size_t slot = 0; slot < ranked_ad_list.size(); ++slot)
    {
        const std::string& ad_id = ranked_ad_list[slot];
        AdViewInfoT& ad_view_info = ad_stat_data_[ad_id][todaystr];
        if (slot > (std::size_t)MAX_AD_SLOT)
        {
            LOG(WARNING) << "the click slot is invalid : " << slot;
            return;
        }

        if (slot > ad_view_info.size())
        {
            ad_view_info.resize(slot + 1);
        }
        ad_view_info[slot].impression_num++;
    }
    for (std::set<BidKeywordId>::const_iterator it = keyword_list.begin(); it != keyword_list.end(); ++it)
    {
        if (*it > keyword_stat_data_.size())
        {
            keyword_stat_data_.resize(*it + 1);
        }
        keyword_stat_data_[*it][todaystr].searched_num++;
    }
}

void AdAucationLogMgr::updateAucationLogData(const std::string& ad_id, const BidPhraseT& keyword_list,
    int click_cost, uint32_t click_slot)
{
    std::string todaystr = getdaystr(0);
    AdViewInfoT& ad_view_info = ad_stat_data_[ad_id][todaystr];
    if (click_slot > (uint32_t)MAX_AD_SLOT)
    {
        LOG(WARNING) << "the click slot is invalid : " << click_slot;
        return;
    }

    if (click_slot > ad_view_info.size())
    {
        ad_view_info.resize(click_slot + 1);
    }
    ad_view_info[click_slot].click_num++;

    int click_cost_in_fen = (int)click_cost * 100;
    std::pair<std::map<int, int>::iterator, bool> insert_it = ad_view_info[click_slot].keyword_cpc.insert(std::make_pair(click_cost_in_fen, 1));
    if (insert_it.second)
    {
        // first click on this cost.
    }
    else
    {
        insert_it.first->second++;
    }

    for (std::size_t i = 0; i < keyword_list.size(); ++i)
    {
        if (keyword_list[i] > keyword_stat_data_.size())
        {
            keyword_stat_data_.resize(keyword_list[i] + 1);
        }
        KeywordViewInfo& key_view_info = keyword_stat_data_[keyword_list[i]][todaystr];
        key_view_info.view_info.resize(click_slot + 1);
        key_view_info.view_info[click_slot].click_num++;

        std::pair<std::map<int, int>::iterator, bool> insert_it = key_view_info.view_info[click_slot].cost_list.insert(std::make_pair(click_cost_in_fen, 1));
        if (insert_it.second)
        {
        }
        else
        {
            insert_it.first->second++;
        }
    }

    GlobalInfo& ginfo = global_stat_data_[todaystr];
    ginfo.click_num_list.resize(click_slot + 1, 0);
    ginfo.click_num_list[click_slot]++;
}

double AdAucationLogMgr::getAdCTR(const std::string& adid)
{
    std::string yestodaystr = getdaystr(-1);
    std::string daybefore_yestodaystr = getdaystr(-2);
    const AdViewInfoT& tmp1 = ad_stat_data_[adid][yestodaystr];
    const AdViewInfoT& tmp2 = ad_stat_data_[adid][daybefore_yestodaystr];
    int total_impression = 0;
    int total_click = 0;
    for (std::size_t i = 0; i < tmp1.size(); ++i)
    {
        total_impression += tmp1[i].impression_num;
        // we leverage the low ranked slot click since it means the more possible click if rank on the first.
        total_click += tmp1[i].click_num * (1 + 0.1*i);
    }
    for (std::size_t i = 0; i < tmp2.size(); ++i)
    {
        total_impression += tmp2[i].impression_num;
        // we leverage the low ranked slot click since it means the more possible click if rank on the first.
        total_click += tmp2[i].click_num * (1 + 0.1*i);
    }
    if (total_impression < MIN_SEARCH_NUM || total_click < 1)
        return DEFAULT_AD_CTR;
    return (double)total_click/total_impression;
}

std::size_t AdAucationLogMgr::getAvgKeywordClickedNum(BidKeywordId keyid)
{
    if (keyid > keyword_stat_data_.size())
        return 0;
    std::size_t clicked = 0;
    const KeywordViewInfo& keyinfo1 = keyword_stat_data_[keyid][getdaystr(-1)];
    const KeywordViewInfo& keyinfo2 = keyword_stat_data_[keyid][getdaystr(-2)];
    for (std::size_t i = 0; i < keyinfo1.view_info.size(); ++i)
    {
        clicked += keyinfo1.view_info[i].click_num;
    }
    for (std::size_t i = 0; i < keyinfo2.view_info.size(); ++i)
    {
        clicked += keyinfo2.view_info[i].click_num;
    }

    return clicked/2;
}

std::size_t AdAucationLogMgr::getAvgTotalClickedNum()
{
    std::size_t total_click = 0;
    const GlobalInfo& ginfo = global_stat_data_[getdaystr(-1)];
    for(std::size_t i = 0; i < ginfo.click_num_list.size(); ++i)
    {
        total_click += ginfo.click_num_list[i];
    }
    return total_click;
}

int AdAucationLogMgr::getKeywordAvgCost(BidKeywordId keyid, std::size_t slot)
{
    if (keyid > keyword_stat_data_.size())
        return getDefaultCostForSlot(slot);

    const KeywordViewInfo& keyinfo = keyword_stat_data_[keyid][getdaystr(-1)];
    if (slot > keyinfo.view_info.size())
        return getDefaultCostForSlot(slot);
    const KeywordViewInfo::KeywordViewSlotInfo& key_slotinfo = keyinfo.view_info[slot];

    int total_cost = 0;
    int total_num = 0;
    std::map<int, int>::const_iterator it = key_slotinfo.cost_list.begin();
    for(; it != key_slotinfo.cost_list.end(); ++it)
    {
        total_cost += it->first * it->second;
        total_num += it->second;
    }
    return total_cost/total_num;
}

int AdAucationLogMgr::getAdAvgCost(const std::string& adid)
{
    const AdViewInfoT& adview1 = ad_stat_data_[adid][getdaystr(-1)];
    int total_cost = 0;
    int total_click = 0;
    for (std::size_t i = 0; i < adview1.size(); ++i)
    {
        std::map<int, int>::const_iterator it = adview1[i].keyword_cpc.begin();
        while(it != adview1[i].keyword_cpc.end())
        {
            total_cost += it->first*it->second;
            total_click += it->second;
            ++it;
        }
    }
    if (total_click < 1)
        return DEFAULT_CLICK_COST;
    return total_cost / total_click;
}

double AdAucationLogMgr::getKeywordCTR(BidKeywordId kid)
{
    if (kid > keyword_stat_data_.size())
        return DEFAULT_AD_CTR;
    std::size_t clicked = 0;
    const KeywordViewInfo& keyinfo1 = keyword_stat_data_[kid][getdaystr(-1)];
    const KeywordViewInfo& keyinfo2 = keyword_stat_data_[kid][getdaystr(-2)];
    for (std::size_t i = 0; i < keyinfo1.view_info.size(); ++i)
    {
        clicked += keyinfo1.view_info[i].click_num;
    }
    for (std::size_t i = 0; i < keyinfo2.view_info.size(); ++i)
    {
        clicked += keyinfo2.view_info[i].click_num;
    }

    if (clicked < 1 || (keyinfo1.searched_num + keyinfo2.searched_num < MIN_SEARCH_NUM))
        return DEFAULT_AD_CTR;
    return clicked/(keyinfo1.searched_num + keyinfo2.searched_num);
}

void AdAucationLogMgr::getKeywordBidLandscape(std::vector<BidKeywordId>& keyword_list,
    std::vector<std::vector<std::pair<int, double> > >& cost_click_list)
{
    // key -> slot -> (cost per click, click ratio)
    
    keyword_list.clear();
    cost_click_list.clear();

    std::string yestodaystr = getdaystr(-1);
    for(std::size_t kid = 0; kid < keyword_stat_data_.size(); ++kid)
    {
        if (keyword_stat_data_[kid].empty())
            continue;
        const KeywordViewInfo& keyview = keyword_stat_data_[kid][yestodaystr];
        if (keyview.searched_num < MIN_SEARCH_NUM)
            continue;
        keyword_list.push_back(kid);
        std::size_t slot_num = keyview.view_info.size();
        cost_click_list.push_back(std::vector<std::pair<int, double> >(slot_num));

        for(std::size_t slot = 0; slot < slot_num; ++slot)
        {
            const KeywordViewInfo::KeywordViewSlotInfo& slotinfo = keyview.view_info[slot];

            int key_slot_total_cost = 0;
            std::size_t key_slot_total_click = 0;
            std::map<int, int>::const_iterator cost_it = slotinfo.cost_list.begin();
            while(cost_it != slotinfo.cost_list.end())
            {
                if (cost_it->second > 0)
                {
                    key_slot_total_cost += cost_it->first*cost_it->second;
                    key_slot_total_click += cost_it->second;
                }
                ++cost_it;
            }
            int avg_cpc = getDefaultCostForSlot(slot);
            double click_ratio = getDefaultCTRForSlot(slot);
            if (key_slot_total_click > 1)
            {
                avg_cpc = key_slot_total_cost/key_slot_total_click;
                click_ratio = (double)key_slot_total_click/keyview.searched_num;
            }

            cost_click_list.back()[slot] = std::make_pair(avg_cpc, click_ratio);
        }
    }
}


} }


