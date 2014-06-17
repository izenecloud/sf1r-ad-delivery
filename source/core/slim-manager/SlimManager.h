#ifndef SF1R_SLIM_MANAGER_SLIM_MANAGER_H
#define SF1R_SLIM_MANAGER_SLIM_MANAGER_H

#include <boost/shared_ptr.hpp>
#include <document-manager/DocumentManager.h>
#include <query-manager/ActionItem.h>
#include <common/ResultType.h>
#include <ad-manager/AdSearchService.h>
#include <laser-manager/LaserManager.h>
#include <string>

#include "SlimRecommend.h"
#include "SlimRecommendParam.h"

namespace sf1r {

class SlimManager {
public:
    SlimManager(const boost::shared_ptr<AdSearchService>& adSearchService,
                const boost::shared_ptr<DocumentManager>& documentManager,
                const std::string& collection,
                LaserManager* laser);
    ~SlimManager();

public:
    bool recommend(const slim::SlimRecommendParam& param,
                   GetDocumentsByIdsActionItem& actionItem,
                   RawTextResultFromMIA& itemList) const;

private:
    std::string collection_;
    const boost::shared_ptr<AdSearchService> & adSearchService_;
    const boost::shared_ptr<DocumentManager> & documentManager_;
    slim::SlimRecommend* recommend_;
    LaserManager* laser_;
};

}

#endif
