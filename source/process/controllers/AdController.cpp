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

void AdController::set_keyword_bidprice()
{
    Value& input = request()[Keys::resource];
    bool ret = miningSearchService_->setKeywordBidPrice(asString(input[Keys::ad_keyword]),
        asString(input[Keys::ad_campaign]), asDouble(input[Keys::bidprice]));
    if (!ret)
    {
        response().addError("Request failed.");
        return;
    }
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

void AdController::set_ad_bid_phrase()
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
        bid_phrase_list[i] = asString(bidphraseV(i));
    }

    bool ret = miningSearchService_->setAdBidPhrase(asString(input[Keys::DOCID]), bid_phrase_list);
    if (!ret)
    {
        response().addError("Request failed.");
        return;
    }
}


}
