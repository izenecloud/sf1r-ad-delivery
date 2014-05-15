#ifndef SF1R_LASER_INDEX_TASK_H
#define SF1R_LASER_INDEX_TASK_H
#include <mining-manager/MiningTask.h>
#include "LaserManager.h"

namespace sf1r { namespace laser {

class LaserIndexTask : public MiningTask
{
public:
    LaserIndexTask(LaserManager* laserManager)
        : laserManager_(laserManager)
        , docid_(0)
    {
    }

    virtual ~LaserIndexTask()
    {
    }
public:
    virtual bool buildDocument(docid_t docID, const Document& doc)
    {
        if (docid_ < docID)
        {
            docid_ = docID;
        }
        std::string title = "";
        if (doc.getString("Title", title))
        {
            laserManager_->index(docID, title);
        }
        return true;
    }

    virtual bool preProcess(int64_t timestamp)
    {
        docid_ = laserManager_->indexManager_->getLastDocId();
        return true;
    }

    virtual bool postProcess()
    {
        laserManager_->indexManager_->setLastDocId(docid_);
        return true;
    }

    virtual docid_t getLastDocId()
    {
        return docid_;
    }

private:
    LaserManager* laserManager_;
    docid_t docid_;
};

} }
#endif
