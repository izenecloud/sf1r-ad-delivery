#include "AdController.h"
#include "CollectionHandler.h"
#include <bundles/mining/MiningSearchService.h>
#include <common/Keys.h>
#include <common/Utilities.h>

namespace sf1r
{

using namespace izenelib::driver;
using driver::Keys;

AdController::AdController()
    : miningSearchService_(NULL)
{
}

bool AdController::checkCollectionService(std::string& error)
{
    miningSearchService_ = collectionHandler_->miningSearchService_;
    if (! miningSearchService_)
    {
        error = "Request failed, no mining search service found.";
        return false;
    }
    return true;
}

void AdController::set_ad_campaign_budget()
{
    Value& input = request()[Keys::resource];
    bool ret = miningSearchService_->setAdCampaignBudget(asString(input[Keys::ad_campaign]), asDouble(input[Keys::bidbudget]));
    if (!ret)
    {
        response().addError("Request failed.");
        return;
    }
}

void AdController::update_ad_bid_phrase()
{
    Value& input = request()[Keys::resource];
    const Value& bidphraseV = input[Keys::bidphrase];
    if (bidphraseV.type() != Value::kArrayType)
    {
        response().addError("Require resource[bidphrase] as an array of bid phrase list.");
        return;
    }
    std::vector<std::string> bid_phrase_list;
    bid_phrase_list.resize(bidphraseV.size());
    std::vector<int> bid_price_list;
    bid_price_list.resize(bidphraseV.size());
    for(std::size_t i = 0; i < bid_phrase_list.size(); ++i)
    {
        const Value& o = bidphraseV(i);
        bid_phrase_list[i] = asString(o[Keys::ad_keyword]);
        // convert the price unit from yuan to fen.
        bid_price_list[i] = int(asDouble(o[Keys::bidprice]) * 100);
    }

    bool ret = miningSearchService_->updateAdBidPhrase(asString(input[Keys::DOCID]), bid_phrase_list, bid_price_list);
    if (!ret)
    {
        response().addError("Request failed.");
        return;
    }
}

void AdController::del_ad_bid_phrase()
{
    Value& input = request()[Keys::resource];
    const Value& bidphraseV = input[Keys::bidphrase];
    if (bidphraseV.type() != Value::kArrayType)
    {
        response().addError("Require resource[bidphrase] as an array of bid phrase list.");
        return;
    }
    std::vector<std::string> bid_phrase_list;
    bid_phrase_list.resize(bidphraseV.size());
    for(std::size_t i = 0; i < bid_phrase_list.size(); ++i)
    {
        const Value& o = bidphraseV(i);
        bid_phrase_list[i] = asString(o);
    }

    miningSearchService_->delAdBidPhrase(asString(input[Keys::DOCID]), bid_phrase_list);
}

void AdController::update_online_status()
{
    Value& input = request()[Keys::resource];
    if (input.type() != Value::kArrayType)
    {
        response().addError("Require resource as an array of online ad list.");
        return;
    }
    std::vector<std::string> ad_strid_list;
    ad_strid_list.resize(input.size());
    std::vector<bool> is_online_list;
    is_online_list.resize(input.size());
    for(std::size_t i = 0; i < input.size(); ++i)
    {
        ad_strid_list[i] = asString(input(i)[Keys::DOCID]);
        is_online_list[i] = asBool(input(i)[Keys::is_online_ad]);
    }
    bool ret = miningSearchService_->updateAdOnlineStatus(ad_strid_list, is_online_list);
    if (!ret)
    {
        response().addError("Request failed.");
        return;
    }
}

}
