#include "AdAuctionLogMgr.h"
#include <glog/logging.h>
#include <util/hashFunction.h>
#include <util/izene_serialization.h>

#include <time.h>
#include <fstream>
#include <boost/lexical_cast.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/split.hpp>

namespace bfs = boost::filesystem;

namespace sf1r { namespace sponsored {

static const int MAX_AD_SLOT = 20;
static const double DEFAULT_AD_CTR = 0.003;
static const int DEFAULT_CLICK_COST = 50;

static const int MIN_SEARCH_NUM = 1000;

static const int HISTORY_HOURS = 48;
static const int RECENT_HOURS = 1;
static const int HOURS_OF_DAY = 24;

static const int DEFAULT_AD_BUCKET = 1000000;
static const int DEFAULT_KEYWORD_BUCKET = 1000000;
static const int DEFAULT_LOCK_NUM = 10000;

static std::string gettimestr(int hours)
{
    struct tm time_s;
    time_t current = time(NULL);
    gmtime_r(&current, &time_s);
    // for test
    //time_s.tm_min += hours;
    //if (time_s.tm_min < 0)
    //{
    //    time_s.tm_min += 60;
    //    time_s.tm_hour -= 1;
    //}
    //else if (time_s.tm_min >= 60)
    //{
    //    time_s.tm_min -= 60;
    //    time_s.tm_hour += 1;
    //}
    //
    time_s.tm_hour += hours;
    if (time_s.tm_hour < 0)
    {
        time_s.tm_hour += HOURS_OF_DAY;
        time_s.tm_yday -= 1;
    }
    else if (time_s.tm_hour >= HOURS_OF_DAY)
    {
        time_s.tm_hour -= HOURS_OF_DAY;
        time_s.tm_yday += 1;
    }
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
        boost::lexical_cast<std::string>(time_s.tm_yday) +
        boost::lexical_cast<std::string>(time_s.tm_hour) /* +
        boost::lexical_cast<std::string>(time_s.tm_min)*/;
}

// get the time when the bid began. relative to the current time.
static int getCurrentDayBegin()
{
    struct tm time_s;
    time_t today = time(NULL);
    gmtime_r(&today, &time_s);
    return 0 - time_s.tm_hour;
}

// get the time when the bid period will end.
static int getCurrentDayEnd()
{
    struct tm time_s;
    time_t today = time(NULL);
    gmtime_r(&today, &time_s);
    return HOURS_OF_DAY - time_s.tm_hour;
}

static std::string getHistoryTimeStr()
{
    return gettimestr(0 - RECENT_HOURS);
}

inline uint32_t getHashIndex(const std::string& key)
{
    static izenelib::util::HashIDTraits<std::string, uint32_t> hash_func;
    return hash_func(key);
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
    lock_list_ = new boost::mutex[DEFAULT_LOCK_NUM];
}
AdAuctionLogMgr::~AdAuctionLogMgr()
{
    delete[] lock_list_;
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
            izenelib::util::izene_deserialization<GlobalInfoPeriodListT> izd(data.data(), data.size());
            izd.read_image(global_stat_data_);
        }
        LOG(INFO) << "ad auction log data loaded, ad stat: " << ad_stat_data_.size()
           << ", keyword stat: " << keyword_stat_data_.size();
    } 
    ifs.close();
    if (ad_stat_data_.load_factor() > 0.7)
    {
        int num = ad_stat_data_.load_factor() * ad_stat_data_.bucket_count();
        ad_stat_data_.rehash(std::max(num*2, DEFAULT_AD_BUCKET));
    }
    else if (ad_stat_data_.bucket_count() < (std::size_t)DEFAULT_AD_BUCKET)
    {
        ad_stat_data_.rehash(DEFAULT_AD_BUCKET);
    }

    if (keyword_stat_data_.load_factor() > 0.7)
    {
        int num = keyword_stat_data_.load_factor() * keyword_stat_data_.bucket_count();
        keyword_stat_data_.rehash(std::max(num*2, DEFAULT_KEYWORD_BUCKET));
    }
    else if (keyword_stat_data_.bucket_count() < (std::size_t)DEFAULT_KEYWORD_BUCKET)
    {
        keyword_stat_data_.rehash(DEFAULT_KEYWORD_BUCKET);
    }
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
        izenelib::util::izene_serialization<GlobalInfoPeriodListT> izs(global_stat_data_);
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
    std::string timestr = gettimestr(0);
    for(std::size_t slot = 0; slot < ranked_ad_list.size(); ++slot)
    {
        const std::string& ad_id = ranked_ad_list[slot];
        boost::mutex& m = lock_list_[getHashIndex(ad_id) % DEFAULT_LOCK_NUM];
        boost::unique_lock<boost::mutex> guard(m);
        AdViewInfoPeriodListT& ad_stat = ad_stat_data_[ad_id];
        if (ad_stat.find(timestr) == ad_stat.end())
        {
            // a new time interval coming, merge the old time data.
            updateHistoryAdLogData(ad_id, ad_stat);
        }

        AdViewInfoT& ad_view_info = ad_stat[timestr];
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
        boost::mutex& m = lock_list_[getHashIndex(*it) % DEFAULT_LOCK_NUM];
        boost::unique_lock<boost::mutex> guard(m);
        KeywordViewInfoPeriodListT& key_stat = keyword_stat_data_[*it];
        if (key_stat.find(timestr) == key_stat.end())
        {
            updateHistoryKeywordLogData(*it, key_stat);
        }

        key_stat[timestr].searched_num++;
    }
}

void AdAuctionLogMgr::updateAuctionLogData(const std::string& ad_id, const std::string& keyword_str,
    int click_cost_in_fen, uint32_t click_slot)
{
    std::string timestr = gettimestr(0);
    {
        boost::mutex& m = lock_list_[getHashIndex(ad_id) % DEFAULT_LOCK_NUM];
        boost::unique_lock<boost::mutex> guard(m);
        AdViewInfoPeriodListT& ad_stat = ad_stat_data_[ad_id];
        if (ad_stat.find(timestr) == ad_stat.end())
        {
            // a new time interval coming, merge the old time data.
            updateHistoryAdLogData(ad_id, ad_stat);
        }

        AdViewInfoT& ad_view_info = ad_stat[timestr];
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
    }
    {
        boost::mutex& m = lock_list_[getHashIndex(keyword_str) % DEFAULT_LOCK_NUM];
        boost::unique_lock<boost::mutex> guard(m);
        KeywordViewInfoPeriodListT& key_stat = keyword_stat_data_[keyword_str];
        if (key_stat.find(timestr) == key_stat.end())
        {
            updateHistoryKeywordLogData(keyword_str, key_stat);
        }

        KeywordViewInfo& key_view_info = key_stat[timestr];
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

    GlobalInfoPeriodListT::iterator git = global_stat_data_.find(timestr);
    if (git == global_stat_data_.end())
    {
        updateHistoryGlobalLogData();
    }
    GlobalInfo& ginfo = global_stat_data_[timestr];
    ginfo.click_num_list.resize(click_slot + 1, 0);
    ginfo.click_num_list[click_slot]++;
}

void AdAuctionLogMgr::updateHistoryGlobalLogData()
{
    GlobalInfo history_ginfo;
    for (std::size_t i = 1; i <= (std::size_t)HISTORY_HOURS; ++i)
    {
        std::string timestr = gettimestr(0 - i);
        GlobalInfoPeriodListT::const_iterator period_it = global_stat_data_.find(timestr);
        if (period_it == global_stat_data_.end())
            continue;
        const GlobalInfo& gview = period_it->second;
        if (gview.click_num_list.size() > history_ginfo.click_num_list.size())
        {
            history_ginfo.click_num_list.resize(gview.click_num_list.size(), 0);
        }
        for(std::size_t slot = 0; slot < gview.click_num_list.size(); ++slot)
        {
            history_ginfo.click_num_list[slot] += gview.click_num_list[slot];
        }
    }
    {
        HistoryRWLock::WriteHolder guard(history_rw_lock_);
        global_history_stat_data_.swap(history_ginfo);
    }
}

void AdAuctionLogMgr::updateHistoryAdLogData(const std::string& adid, const AdViewInfoPeriodListT& ad_period_info)
{
    AdViewInfoT history_adview;
    for (std::size_t i = 1; i <= (std::size_t)HISTORY_HOURS; ++i)
    {
        std::string timestr = gettimestr(0 - i);
        AdViewInfoPeriodListT::const_iterator period_it = ad_period_info.find(timestr);
        if (period_it == ad_period_info.end())
            continue;
        const AdViewInfoT& adview = period_it->second;
        if (adview.size() > history_adview.size())
        {
            history_adview.resize(adview.size());
        }
        for(std::size_t slot = 0; slot < adview.size(); ++slot)
        {
            history_adview[slot].impression_num += adview[slot].impression_num;
            history_adview[slot].click_num += adview[slot].click_num;
            std::map<int, int>::const_iterator cpc_it = adview[slot].keyword_cpc.begin();
            for(; cpc_it != adview[slot].keyword_cpc.end(); ++cpc_it)
            {
                std::pair<std::map<int, int>::iterator, bool> inserted = history_adview[slot].keyword_cpc.insert(std::make_pair(cpc_it->first, cpc_it->second));
                if (inserted.second)
                {
                }
                else
                {
                    inserted.first->second += cpc_it->second;
                }
            }
        }
    }
    {
        HistoryRWLock::WriteHolder guard(history_rw_lock_);
        ad_history_stat_data_[adid].swap(history_adview);
    }
}

void AdAuctionLogMgr::updateHistoryKeywordLogData(const std::string& kid, const KeywordViewInfoPeriodListT& keyword_period_info)
{
    KeywordViewInfo history_keyview;
    for (std::size_t i = 1; i <= (std::size_t)HISTORY_HOURS; ++i)
    {
        std::string timestr = gettimestr(0 - i);
        KeywordViewInfoPeriodListT::const_iterator period_it = keyword_period_info.find(timestr);
        if (period_it == keyword_period_info.end())
            continue;
        const KeywordViewInfo& keyview = period_it->second;
        history_keyview.searched_num += keyview.searched_num;
        if (keyview.view_info.size() > history_keyview.view_info.size())
        {
            history_keyview.view_info.resize(keyview.view_info.size());
        }
        for(std::size_t slot = 0; slot < keyview.view_info.size(); ++slot)
        {
            history_keyview.view_info[slot].click_num += keyview.view_info[slot].click_num;
            std::map<int, int>::const_iterator cpc_it = keyview.view_info[slot].cost_list.begin();
            for(; cpc_it != keyview.view_info[slot].cost_list.end(); ++cpc_it)
            {
                std::pair<std::map<int, int>::iterator, bool> inserted = history_keyview.view_info[slot].cost_list.insert(std::make_pair(cpc_it->first, cpc_it->second));
                if (!inserted.second)
                {
                    inserted.first->second += cpc_it->second;
                }
            }
        }
    }
    {
        HistoryRWLock::WriteHolder guard(history_rw_lock_);
        keyword_history_stat_data_[kid].swap(history_keyview);
    }

    LOG(INFO) << "end update history stat for keyword.";
}

double AdAuctionLogMgr::getAdCTR(const std::string& adid)
{
    HistoryRWLock::ReadHolder guard(history_rw_lock_);
    AdHistoryStatContainerT::const_iterator it = ad_history_stat_data_.find(adid);
    if (it == ad_history_stat_data_.end())
        return DEFAULT_AD_CTR;
    const AdViewInfoT& tmp1 = it->second;
    int total_impression = 0;
    int total_click = 0;
    for (std::size_t i = 0; i < tmp1.size(); ++i)
    {
        total_impression += tmp1[i].impression_num;
        // we leverage the low ranked slot click since it means the more possible click if rank on the first.
        total_click += tmp1[i].click_num * (1 + 0.2*i);
    }
    if (total_impression < MIN_SEARCH_NUM || total_click < 1)
        return DEFAULT_AD_CTR;
    return (double)total_click/total_impression;
}

std::size_t AdAuctionLogMgr::getKeywordAvgDailyClickedNum(LogBidKeywordId kid)
{
    HistoryRWLock::ReadHolder guard(history_rw_lock_);
    KeywordHistoryStatContainerT::const_iterator it = keyword_history_stat_data_.find(kid);
    if (it == keyword_history_stat_data_.end())
        return 0;
    std::size_t clicked = 0;
    const KeywordViewInfo& keyinfo = it->second;
    for (std::size_t i = 0; i < keyinfo.view_info.size(); ++i)
    {
        clicked += keyinfo.view_info[i].click_num;
    }

    return clicked / (HISTORY_HOURS / HOURS_OF_DAY);
}

std::size_t AdAuctionLogMgr::getAvgTotalClickedNum()
{
    HistoryRWLock::ReadHolder guard(history_rw_lock_);
    std::size_t total_click = 0;
    const GlobalInfo& ginfo = global_history_stat_data_;
    for(std::size_t i = 0; i < ginfo.click_num_list.size(); ++i)
    {
        total_click += ginfo.click_num_list[i];
    }
    return total_click;
}

int AdAuctionLogMgr::getKeywordCurrentImpression(LogBidKeywordId keyid)
{
    boost::mutex& m = lock_list_[getHashIndex(keyid) % DEFAULT_LOCK_NUM];
    boost::unique_lock<boost::mutex> guard(m);
    KeywordStatContainerT::const_iterator it = keyword_stat_data_.find(keyid);
    if (it == keyword_stat_data_.end())
        return 0;
    int start = getCurrentDayBegin();
    int total = 0;
    for(int i = start; i <= 0; ++i)
    {
        KeywordViewInfoPeriodListT::const_iterator period_it = it->second.find(gettimestr(i));
        if (period_it == it->second.end())
            continue;
        total += period_it->second.searched_num;
    }
    return total;
}

int AdAuctionLogMgr::getKeywordAvgDailyImpression(LogBidKeywordId kid)
{
    HistoryRWLock::ReadHolder guard(history_rw_lock_);
    KeywordHistoryStatContainerT::const_iterator it = keyword_history_stat_data_.find(kid);
    if (it == keyword_history_stat_data_.end())
        return 0;
    return it->second.searched_num / (HISTORY_HOURS / HOURS_OF_DAY);
}

void AdAuctionLogMgr::getKeywordAvgCost(LogBidKeywordId kid, std::vector<int>& cost_list)
{
    cost_list.clear();
    HistoryRWLock::ReadHolder guard(history_rw_lock_);
    KeywordHistoryStatContainerT::const_iterator it = keyword_history_stat_data_.find(kid);
    if (it == keyword_history_stat_data_.end())
        return;
    const KeywordViewInfo& keyinfo = it->second;
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

int AdAuctionLogMgr::getKeywordAvgCost(LogBidKeywordId kid, uint32_t slot)
{
    HistoryRWLock::ReadHolder guard(history_rw_lock_);
    KeywordHistoryStatContainerT::const_iterator it = keyword_history_stat_data_.find(kid);
    if (it == keyword_history_stat_data_.end())
        return getDefaultCostForSlot(slot);

    const KeywordViewInfo& keyinfo = it->second;
    if (slot >= keyinfo.view_info.size())
        return getDefaultCostForSlot(slot);
    const KeywordViewInfo::KeywordViewSlotInfo& key_slotinfo = keyinfo.view_info[slot];

    int total_cost = 0;
    int total_num = 0;
    std::map<int, int>::const_iterator slot_it = key_slotinfo.cost_list.begin();
    for(; slot_it != key_slotinfo.cost_list.end(); ++slot_it)
    {
        total_cost += slot_it->first * (slot_it->second);
        total_num += slot_it->second;
    }
    if (total_num < 1)
        return getDefaultCostForSlot(slot);
    return total_cost/total_num;
}

int AdAuctionLogMgr::getAdAvgCost(const std::string& adid)
{
    HistoryRWLock::ReadHolder guard(history_rw_lock_);
    AdHistoryStatContainerT::const_iterator stat_it = ad_history_stat_data_.find(adid);
    if (stat_it == ad_history_stat_data_.end())
        return DEFAULT_CLICK_COST;
    const AdViewInfoT& adview1 = stat_it->second;
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
    HistoryRWLock::ReadHolder guard(history_rw_lock_);
    ctr_list.clear();
    KeywordHistoryStatContainerT::const_iterator it = keyword_history_stat_data_.find(kid);
    if (it == keyword_history_stat_data_.end())
        return;
    const KeywordViewInfo& keyinfo = it->second;
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
    HistoryRWLock::ReadHolder guard(history_rw_lock_);
    KeywordHistoryStatContainerT::const_iterator it = keyword_history_stat_data_.find(kid);
    if (it == keyword_history_stat_data_.end())
        return getDefaultCTRForSlot(slot);
    std::size_t clicked = 0;
    const KeywordViewInfo& keyinfo = it->second;
    if (slot < keyinfo.view_info.size())
    {
        clicked += keyinfo.view_info[slot].click_num;
    }

    if (clicked < 1 || (keyinfo.searched_num < MIN_SEARCH_NUM))
        return getDefaultCTRForSlot(slot);
    return (double)clicked/(keyinfo.searched_num);
}

void AdAuctionLogMgr::getKeywordStatData(LogBidKeywordId kid, int& avg_impression,
    std::vector<int>& cost_list,
    std::vector<double>& ctr_list)
{
    cost_list.clear();
    ctr_list.clear();
    avg_impression = 0;
    HistoryRWLock::ReadHolder guard(history_rw_lock_);
    KeywordHistoryStatContainerT::const_iterator stat_it = keyword_history_stat_data_.find(kid);
    if (stat_it == keyword_history_stat_data_.end())
        return;
    const KeywordViewInfo& keyinfo = stat_it->second;
    avg_impression = keyinfo.searched_num / (HISTORY_HOURS/HOURS_OF_DAY);
    if (keyinfo.searched_num < MIN_SEARCH_NUM)
    {
        return;
    }
    cost_list.resize(keyinfo.view_info.size(), 0);
    ctr_list.resize(keyinfo.view_info.size(), 0);
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

        ctr_list[slot] = key_slotinfo.click_num / keyinfo.searched_num;
    }
}

void AdAuctionLogMgr::getAdLandscape(std::vector<std::string>& ad_list,
    std::vector<BidAuctionLandscapeT>& cost_click_list)
{
    // ad -> slot -> (cpc, ctr)
    ad_list.clear();
    cost_click_list.clear();

    HistoryRWLock::ReadHolder guard(history_rw_lock_);
    AdHistoryStatContainerT::const_iterator cit = ad_history_stat_data_.begin();

    for(; cit != ad_history_stat_data_.end(); ++cit)
    {
        const AdViewInfoT& adview = cit->second;
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

    HistoryRWLock::ReadHolder guard(history_rw_lock_);
    for(KeywordHistoryStatContainerT::const_iterator kit = keyword_history_stat_data_.begin();
        kit != keyword_history_stat_data_.end(); ++kit)
    {
        const KeywordViewInfo& keyview = kit->second;
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


