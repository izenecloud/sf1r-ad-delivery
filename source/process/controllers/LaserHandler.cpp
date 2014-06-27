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
    , actionItem_()
{
    actionItem_.env_.encodingType_ = "UTF-8";
    actionItem_.env_.ipAddress_ = request.header()[Keys::remote_ip].getString();
    actionItem_.collectionName_ = asString(request[Keys::collection]);
}
    
void LaserHandler::recommend()
{
    LaserManager* laserManager = miningSearchService_->GetMiningManager()->GetLaserManager();
    laser::LaserRecommendParam param;
    if (parseSelect_()  && parseLaserRecommendParam_(param))
    {
        RawTextResultFromAIA result;
        laserManager->recommend(param, actionItem_, result);
        if (!result.error_.empty())
        {
            response_.addError(result.error_);
            return ;
        }
        
        DocumentsRenderer renderer(miningSchema_);
        renderer.renderDocuments(
            actionItem_.displayPropertyList_,
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
            actionItem_.displayPropertyList_,
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
    const Value& recommendParam = request_[driver::Keys::recommend];
    param.uuid_ = asString(recommendParam[driver::Keys::uuid]);
    param.topN_ = asInt(recommendParam[driver::Keys::topn]);
    return true;
}

}
