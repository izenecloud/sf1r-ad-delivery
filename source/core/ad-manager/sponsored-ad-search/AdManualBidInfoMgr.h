#ifndef AD_SPONSORED_MANUALBIDINFO_MGR_H
#define AD_SPONSORED_MANUALBIDINFO_MGR_H

#include "AdCommonDataType.h"
#include <string>
#include <vector>
#include <map>
#include <boost/unordered_map.hpp>

namespace sf1r
{
namespace sponsored
{

class AdManualBidInfoMgr
{
public:
    AdManualBidInfoMgr();
    ~AdManualBidInfoMgr();
    void init(const std::string& data_path);

    void setManualBidPrice(const std::string& campaign_name, ad_docid_t adid, const std::vector<std::string>& key_list,
        const std::vector<int>& price_list);
    void getManualBidPriceList(const std::string& campaign_name, std::map<std::string, int>& price_list);
    bool hasManualBidPrice(const std::string& campaign_name);
    int getManualBidPrice(const std::string& campaign_name, ad_docid_t adid, const std::string& bidstr);
    void removeManualBidPrice(const std::string& campaign_name, ad_docid_t adid,
        const std::vector<std::string>& key_list);
    void setBidBudget(const std::string& campaign_name, int budget);
    inline int getBidBudget(const std::string& campaign)
    {
        boost::unordered_map<std::string, int>::const_iterator it = ad_campaign_budget_list_.find(campaign);
        if (it == ad_campaign_budget_list_.end())
            return DEFAULT_AD_BUDGET;
        return it->second;
    }
    std::string getManualKeyForAd(ad_docid_t adid, const std::string& bidstr);

    void load();
    void save();

private:
    typedef std::map<std::string, int> AdBidInfoValueT;
    typedef boost::unordered_map<std::string, AdBidInfoValueT>  ManualBidInfoT;
    // the total budget for specific ad campaign. update each day.
    boost::unordered_map<std::string, int> ad_campaign_budget_list_;
    // the list of all manual bid price for each campaign.
    ManualBidInfoT  manual_bidprice_list_; 
    std::string data_path_;
    static const int DEFAULT_AD_BUDGET = 100;
};

}
}

#endif
