#include "LaserHandler.h"
#include <laser-manager/LaserManager.h>
#include <bundles/mining/MiningSearchService.h>
#include <common/Keys.h>
#include "renderers/DocumentsRenderer.h"
#include "parsers/SelectParser.h"

using namespace sf1r::laser;

namespace sf1r
{
LaserHandler::LaserHandler(
    ::izenelib::driver::Request& request,
    ::izenelib::driver::Response& response,
    const CollectionHandler& collectionHandler)
    : request_(request)
    , response_(response)
    , miningSearchService_(collectionHandler.miningSearchService_)
    , indexSchema_(collectionHandler.indexSchema_)
    , miningSchema_(collectionHandler.miningSchema_)
    , zambeziConfig_(collectionHandler.zambeziConfig_)
{
}
    
void LaserHandler::recommend()
{
    LaserManager* laserManager = miningSearchService_->GetMiningManager()->GetLaserManager();
    laser::LaserRecommendParam param;
    if (parseSelect_()  && parseLaserRecommendParam_(param))
    {
        RawTextResultFromMIA result;
        laserManager->recommend(param, result);
        if (!result.error_.empty())
        {
            response_.addError(result.error_);
            return ;
        }
        

        DocumentsRenderer renderer(miningSchema_);
        renderer.renderDocuments(
            displayPropertyList_,
            result,
            response_[driver::Keys::resources]);
    }
}

bool LaserHandler::parseSelect_()
{
    using std::swap;

    SelectParser selectParser(indexSchema_, zambeziConfig_, SearchingMode::NotUseSearchingMode); //  TODO update
    if (selectParser.parse(request_[driver::Keys::select]))
    {
        response_.addWarning(selectParser.warningMessage());
        swap(
            displayPropertyList_,
            selectParser.mutableProperties()
        );

        return true;
    }
    else
    {
        response_.addError(selectParser.errorMessage());
        return false;
    }
}

bool LaserHandler::parseLaserRecommendParam_(laser::LaserRecommendParam& param)
{
    const Value& recommendParam = request_["recommend"];
    param.uuid_ = asString(recommendParam["uuid"]);
    param.topN_ = asInt(recommendParam["topn"]);
    return true;
}

}
