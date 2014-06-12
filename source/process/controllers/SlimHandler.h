#ifndef SF1R_SLIM_SLIM_HANDLER_H
#define SF1R_SLIM_SLIM_HANDLER_H

#include "CollectionHandler.h"
#include <slim-manager/SlimRecommendParam.h>
#include <query-manager/ActionItem.h>
#include <util/driver/Request.h>
#include <util/driver/Response.h>

namespace sf1r {

class SlimHandler {
public:
    SlimHandler(
        ::izenelib::driver::Request& request,
        ::izenelib::driver::Response& response,
        const CollectionHandler& collectionHandler
    );

public:
    void recommend();

private:
    bool parseSelect_();
    bool parseSlimRecommendParam_(slim::SlimRecommendParam& param);

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
