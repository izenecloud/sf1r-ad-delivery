#include "AdAuctionLogMgr.h"
#include <time.h>
#include <boost/lexical_cast.hpp>
#include <glog/logging.h>
#include <fstream>
#include <util/izene_serialization.h>
#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/split.hpp>

namespace bfs = boost::filesystem;

namespace sf1r { namespace sponsored {

static const int STAT_COMPUTE_INTERVAL = 3;
static const int MAX_AD_SLOT = 10;
static const double DEFAULT_AD_CTR = 0.0002;
static const int DEFAULT_CLICK_COST = 5;

static const int MIN_SEARCH_NUM = 1000;

static const int YESTODAY = 0;
static const int YESTODAY2 = 0;

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
    return DEFAULT_AD_CTR * (MAX_AD_SLOT - slot*1.5) / MAX_AD_SLOT;
}

AdAuctionLogMgr::AdAuctionLogMgr()
{

}

void AdAuctionLogMgr::init(const std::string& datapath)
{
    data_path_ = datapath;

    if (!bfs::exists(data_path_))
    {
        bfs::create_directories(data_path_);
    }

    load();
}

void AdAuctionLogMgr::load()
{
    std::ifstream ifs(std::string(data_path_ + "/sponsored_ad_auction.data").c_str());
    std::string data;

    if (ifs.good())
    {
        std::size_t len = 0;
        {
            ifs.read((char*)&len, sizeof(len));
            data.resize(len);
            ifs.read((char*)&data[0], len);
            izenelib::util::izene_deserialization<AdStatContainerT> izd(data.data(), data.size());
            izd.read_image(ad_stat_data_);
        }

        {
            len = 0;
            ifs.read((char*)&len, sizeof(len));
            data.resize(len);
            ifs.read((char*)&data[0], len);
            izenelib::util::izene_deserialization<KeywordStatContainerT> izd(data.data(), data.size());
            izd.read_image(keyword_stat_data_);
        }

        {
            len = 0;
            ifs.read((char*)&len, sizeof(len));
            data.resize(len);
            ifs.read((char*)&data[0], len);
            izenelib::util::izene_deserialization<GlobalInfoDayListT> izd(data.data(), data.size());
            izd.read_image(global_stat_data_);
        }
    }
    ifs.close();
}

void AdAuctionLogMgr::save()
{
    std::ofstream ofs(std::string(data_path_ + "/sponsored_ad_auction.data").c_str());
    std::size_t len = 0;
    char* buf = NULL;
    {
        izenelib::util::izene_serialization<AdStatContainerT> izs(ad_stat_data_);
        izs.write_image(buf, len);
        ofs.write((const char*)&len, sizeof(len));
        ofs.write(buf, len);
        ofs.flush();
    }
    {
        len = 0;
        izenelib::util::izene_serialization<KeywordStatContainerT> izs(keyword_stat_data_);
        izs.write_image(buf, len);
        ofs.write((const char*)&len, sizeof(len));
        ofs.write(buf, len);
        ofs.flush();
    }
    {
        len = 0;
        izenelib::util::izene_serialization<GlobalInfoDayListT> izs(global_stat_data_);
        izs.write_image(buf, len);
        ofs.write((const char*)&len, sizeof(len));
        ofs.write(buf, len);
        ofs.flush();
    }
}

// update the impression info after search request.
void AdAuctionLogMgr::updateAdSearchStat(const std::set<LogBidKeywordId>& keyword_list,
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

        if (slot >= ad_view_info.size())
        {
            ad_view_info.resize(slot + 1);
        }
        ad_view_info[slot].impression_num++;
    }
    for (std::set<LogBidKeywordId>::const_iterator it = keyword_list.begin(); it != keyword_list.end(); ++it)
    {
        keyword_stat_data_[*it][todaystr].searched_num++;
    }
}

void AdAuctionLogMgr::updateAuctionLogData(const std::string& ad_id, const std::vector<LogBidKeywordId>& keyword_list,
    int click_cost_in_fen, uint32_t click_slot)
{
    std::string todaystr = getdaystr(0);
    AdViewInfoT& ad_view_info = ad_stat_data_[ad_id][todaystr];
    if (click_slot > (uint32_t)MAX_AD_SLOT)
    {
        LOG(WARNING) << "the click slot is invalid : " << click_slot;
        return;
    }

    if (click_slot >= ad_view_info.size())
    {
        ad_view_info.resize(click_slot + 1);
    }
    ad_view_info[click_slot].click_num++;

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

double AdAuctionLogMgr::getAdCTR(const std::string& adid)
{
    std::string yestodaystr = getdaystr(YESTODAY);
    std::string daybefore_yestodaystr = getdaystr(YESTODAY2);
    const AdViewInfoT& tmp1 = ad_stat_data_[adid][yestodaystr];
    const AdViewInfoT& tmp2 = ad_stat_data_[adid][daybefore_yestodaystr];
    int total_impression = 0;
    int total_click = 0;
    for (std::size_t i = 0; i < tmp1.size(); ++i)
    {
        total_impression += tmp1[i].impression_num;
        // we leverage the low ranked slot click since it means the more possible click if rank on the first.
        total_click += tmp1[i].click_num * (1 + 0.2*i);
    }
    for (std::size_t i = 0; i < tmp2.size(); ++i)
    {
        total_impression += tmp2[i].impression_num;
        // we leverage the low ranked slot click since it means the more possible click if rank on the first.
        total_click += tmp2[i].click_num * (1 + 0.2*i);
    }
    if (total_impression < MIN_SEARCH_NUM || total_click < 1)
        return DEFAULT_AD_CTR;
    return (double)total_click/total_impression;
}

std::size_t AdAuctionLogMgr::getAvgKeywordClickedNum(LogBidKeywordId keyid)
{
    if (keyword_stat_data_.find(keyid) == keyword_stat_data_.end())
        return 0;
    std::size_t clicked = 0;
    const KeywordViewInfo& keyinfo1 = keyword_stat_data_[keyid][getdaystr(YESTODAY)];
    const KeywordViewInfo& keyinfo2 = keyword_stat_data_[keyid][getdaystr(YESTODAY2)];
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

std::size_t AdAuctionLogMgr::getAvgTotalClickedNum()
{
    std::size_t total_click = 0;
    const GlobalInfo& ginfo = global_stat_data_[getdaystr(YESTODAY)];
    for(std::size_t i = 0; i < ginfo.click_num_list.size(); ++i)
    {
        total_click += ginfo.click_num_list[i];
    }
    return total_click;
}

int AdAuctionLogMgr::getKeywordCurrentImpression(LogBidKeywordId keyid)
{
    if (keyword_stat_data_.find(keyid) == keyword_stat_data_.end())
        return 0;
    const KeywordViewInfo& keyinfo = keyword_stat_data_[keyid][getdaystr(0)];
    return keyinfo.searched_num;
}

int AdAuctionLogMgr::getKeywordAvgDailyImpression(LogBidKeywordId keyid)
{
    if (keyword_stat_data_.find(keyid) == keyword_stat_data_.end())
        return 0;
    int impression = 0;
    for(int i = -1; i >= -3; --i)
    {
        const KeywordViewInfo& keyinfo = keyword_stat_data_[keyid][getdaystr(i)];
        impression += keyinfo.searched_num;
    }
    return impression / 3;
}

void AdAuctionLogMgr::getKeywordAvgCost(LogBidKeywordId keyid, std::vector<int>& cost_list)
{
    cost_list.clear();
    if (keyword_stat_data_.find(keyid) == keyword_stat_data_.end())
        return;
    const KeywordViewInfo& keyinfo = keyword_stat_data_[keyid][getdaystr(YESTODAY)];
    cost_list.resize(keyinfo.view_info.size());
    for(std::size_t slot = 0; slot < keyinfo.view_info.size(); ++slot)
    {
        const KeywordViewInfo::KeywordViewSlotInfo& key_slotinfo = keyinfo.view_info[slot];
        int total_cost = 0;
        int total_num = 0;
        std::map<int, int>::const_iterator it = key_slotinfo.cost_list.begin();
        for(; it != key_slotinfo.cost_list.end(); ++it)
        {
            total_cost += it->first * it->second;
            total_num += it->second;
        }
        if (total_num < 1)
            cost_list[slot] = getDefaultCostForSlot(slot);
        else
        {
            cost_list[slot] = total_cost/total_num;
        }
    }
}

int AdAuctionLogMgr::getKeywordAvgCost(LogBidKeywordId keyid, uint32_t slot)
{
    if (keyword_stat_data_.find(keyid) == keyword_stat_data_.end())
        return getDefaultCostForSlot(slot);

    const KeywordViewInfo& keyinfo = keyword_stat_data_[keyid][getdaystr(YESTODAY)];
    if (slot >= keyinfo.view_info.size())
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
    if (total_num < 1)
        return getDefaultCostForSlot(slot);
    return total_cost/total_num;
}

int AdAuctionLogMgr::getAdAvgCost(const std::string& adid)
{
    const AdViewInfoT& adview1 = ad_stat_data_[adid][getdaystr(YESTODAY)];
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

void AdAuctionLogMgr::getKeywordCTR(LogBidKeywordId kid, std::vector<double>& ctr_list)
{
    ctr_list.clear();
    if (keyword_stat_data_.find(kid) == keyword_stat_data_.end())
        return;
    const KeywordViewInfo& keyinfo = keyword_stat_data_[kid][getdaystr(YESTODAY)];
    if (keyinfo.searched_num < MIN_SEARCH_NUM)
        return;
    ctr_list.resize(keyinfo.view_info.size(), 0);
    for (std::size_t i = 0; i < keyinfo.view_info.size(); ++i)
    {
        ctr_list[i] = keyinfo.view_info[i].click_num / keyinfo.searched_num;
    }
}

double AdAuctionLogMgr::getKeywordCTR(LogBidKeywordId kid, uint32_t slot)
{
    if (keyword_stat_data_.find(kid) == keyword_stat_data_.end())
        return DEFAULT_AD_CTR;
    std::size_t clicked = 0;
    const KeywordViewInfo& keyinfo = keyword_stat_data_[kid][getdaystr(YESTODAY)];
    if (slot < keyinfo.view_info.size())
    {
        clicked += keyinfo.view_info[slot].click_num;
    }

    if (clicked < 1 || (keyinfo.searched_num < MIN_SEARCH_NUM))
        return DEFAULT_AD_CTR;
    return (double)clicked/(keyinfo.searched_num);
}

void AdAuctionLogMgr::getKeywordStatData(LogBidKeywordId kid, int& current_impression, int& avg_impression,
    std::vector<int>& cost_list,
    std::vector<double>& ctr_list)
{
}

void AdAuctionLogMgr::getAdLandscape(std::vector<std::string>& ad_list,
    std::vector<BidAuctionLandscapeT>& cost_click_list)
{
    // ad -> slot -> (cpc, ctr)
    ad_list.clear();
    cost_click_list.clear();

    std::string yestodaystr = getdaystr(YESTODAY);
    AdStatContainerT::const_iterator cit = ad_stat_data_.begin();

    for(; cit != ad_stat_data_.end(); ++cit)
    {
        if (cit->second.empty())
            continue;
        AdViewInfoDayListT::const_iterator day_it = cit->second.find(yestodaystr);
        if (day_it == cit->second.end())
            continue;
        const AdViewInfoT& adview = day_it->second;
        if (adview.empty())
            continue;

        ad_list.push_back(cit->first);
        cost_click_list.push_back(BidAuctionLandscapeT(adview.size()));
        int all_slot_clicked = 0;
        for (std::size_t slot = 0; slot < adview.size(); ++slot)
        {
            const AdViewSlotInfo& ad_slotinfo = adview[slot];
            all_slot_clicked += ad_slotinfo.click_num;
            int total_cost = 0;
            int total_click = 0;
            std::map<int, int>::const_iterator cost_it = ad_slotinfo.keyword_cpc.begin();
            for(; cost_it != ad_slotinfo.keyword_cpc.end(); ++cost_it)
            {
                total_cost += cost_it->first * cost_it->second;
                total_click += cost_it->second;
            }
            int avg_cpc = getDefaultCostForSlot(slot);
            double ctr = getDefaultCTRForSlot(slot);
            if (total_click > 1 && ad_slotinfo.impression_num > MIN_SEARCH_NUM)
            {
                avg_cpc = total_cost/total_click;
                ctr = (double)total_click / ad_slotinfo.impression_num;
            }
            cost_click_list.back()[slot] = std::make_pair(avg_cpc, ctr);
        }
        if (all_slot_clicked == 0)
        {
            ad_list.pop_back();
            cost_click_list.pop_back();
        }
    }

}

void AdAuctionLogMgr::getKeywordBidLandscape(std::vector<LogBidKeywordId>& keyword_list,
    std::vector<BidAuctionLandscapeT>& cost_click_list)
{
    // key -> slot -> (cost per click, click ratio)
    
    keyword_list.clear();
    cost_click_list.clear();

    std::string yestodaystr = getdaystr(YESTODAY);
    for(KeywordStatContainerT::const_iterator kit = keyword_stat_data_.begin();
        kit != keyword_stat_data_.end(); ++kit)
    {
        if (kit->second.empty())
            continue;
        KeywordViewInfoDayListT::const_iterator day_it = kit->second.find(yestodaystr);
        if (day_it == kit->second.end())
            continue;
        const KeywordViewInfo& keyview = day_it->second;
        std::size_t slot_num = keyview.view_info.size();
        if (keyview.searched_num < MIN_SEARCH_NUM || slot_num == 0)
            continue;

        keyword_list.push_back(kit->first);
        cost_click_list.push_back(BidAuctionLandscapeT(slot_num));

        int all_slot_clicked = 0;
        for(std::size_t slot = 0; slot < slot_num; ++slot)
        {
            const KeywordViewInfo::KeywordViewSlotInfo& slotinfo = keyview.view_info[slot];

            all_slot_clicked += slotinfo.click_num;
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
        if (all_slot_clicked == 0)
        {
            keyword_list.pop_back();
            cost_click_list.pop_back();
        }
    }
}


} }


