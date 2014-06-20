#ifndef AD_FEEDBACK_MGR_H
#define AD_FEEDBACK_MGR_H

#include "sponsored-ad-search/AdAuctionLogMgr.h"
#include <util/singleton.h>
#include <boost/thread.hpp>
#include <cache/concurrent_cache.hpp>
#include <3rdparty/msgpack/msgpack.hpp>
#include <libcouchbase/couchbase.h>

namespace sf1r
{

class AdFeedbackMgr
{
public:
    static AdFeedbackMgr* get()
    {
        return izenelib::util::Singleton<AdFeedbackMgr>::get();
    }

    AdFeedbackMgr();
    ~AdFeedbackMgr();

    enum FeedbackActionT
    {
        View,
        Click,
        Buy,
        TotalAction
    };

    struct UserProfile
    {
        std::map<std::string, std::map<std::string, double> > profile_data;
        time_t timestamp;
        MSGPACK_DEFINE(profile_data, timestamp);
    };

    struct FeedbackInfo
    {
        FeedbackInfo()
            :click_cost(0), click_slot(0)
        {
        }
        std::string user_id;
        std::string ad_id;
        std::string hit_bidstr;
        FeedbackActionT action;
        double click_cost;
        uint32_t click_slot;
        UserProfile user_profiles;
    };

    void init(const std::string& dmp_server_ip, uint16_t port);
    void stop();
    bool parserFeedbackLog(const std::string& log_data, FeedbackInfo& feedback_info);
    bool parserFeedbackLogForAVRO(const std::string& log_data, FeedbackInfo& feedback_info);

    void setDMPRsp(const std::string& key, const std::string& value);

private:
    bool getUserProfileFromDMP(const std::string& user_id, UserProfile& user_profile);
    lcb_t get_conn_from_pool();
    void free_conn_to_pool(lcb_t conn);

    typedef izenelib::concurrent_cache::ConcurrentCache<std::string, UserProfile> cache_type;
    std::string dmp_server_ip_;
    uint16_t dmp_server_port_;
    cache_type cached_user_profiles_;
    std::list<lcb_t> dmp_conn_pool_;
    boost::mutex  dmp_pool_lock_;
    boost::mutex  dmp_rsp_lock_;
    std::map<std::string, std::string> tmp_rsp_list_;
};

};

#endif
