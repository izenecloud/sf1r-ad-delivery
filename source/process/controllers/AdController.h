#ifndef SF1R_AD_CONTROLLER_H
#define SF1R_AD_CONTROLLER_H

#include "Sf1Controller.h"
namespace sf1r
{
class MiningSearchService;

class AdController : public Sf1Controller
{
public:
    AdController();
    void set_ad_campaign_budget();
    void update_ad_bid_phrase();
    void update_online_status();
    void del_ad_bid_phrase();
protected:
    virtual bool checkCollectionService(std::string& error);

    MiningSearchService* miningSearchService_;

};
}
#endif
