#ifndef SF1R_LASER_LASER_HANDLER_H
#define SF1R_LASER_LASER_HANDLER_H

#include "CollectionHandler.h"
#include <laser-manager/LaserRecommendParam.h>
#include <query-manager/ActionItem.h>
#include <util/driver/Request.h>
#include <util/driver/Response.h>
namespace sf1r
{
class LaserHandler
{
public:
    LaserHandler(
        ::izenelib::driver::Request& request,
        ::izenelib::driver::Response& response,
        const CollectionHandler& collectionHandler
    );

public:
    void recommend();

private:
    bool parseSelect_();
    bool parseLaserRecommendParam_(laser::LaserRecommendParam& param);
private:
    ::izenelib::driver::Request& request_;
    ::izenelib::driver::Response& response_;
    MiningSearchService* miningSearchService_;
    const IndexBundleSchema& indexSchema_;
    const MiningSchema& miningSchema_;
    const ZambeziConfig& zambeziConfig_;
    GetDocumentsByIdsActionItem actionItem_;
};
}
#endif
