#include "LaserHandler.h"
#include <laser-mananger/LaserManager.h>

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
        , miningSchema_(collectionHandler.miningSchema_)
    {
    }
    
    void LaserHandler::recommend()
    {
        boost::shared_ptr<LaserManager>& laserManager = miningSearchService_->GetMiningManager()->GetLaserManager();
        RawTextResultFromMIA result;
        std::string uuid;
        std::size_t num;
        laserManager.recommend(uuid, num, result);
        if (!result.error_.empty())
        {
            response_.addError(result.error_);
            return ;
        }
        
        const std::vector<DisplayProperty>& propertyList,

        DocumentsRenderer renderer(miningSchema_);
        renderer.renderDocuments(
                actionItem_.displayPropertyList_,
                result,
                response_[Keys::resources]);
    }
}
