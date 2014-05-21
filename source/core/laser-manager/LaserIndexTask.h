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
        {
            boost::unique_lock<boost::shared_mutex> uniqueLock(mutex_);
            if (docid_ < docID)
            {
                docid_ = docID;
            }
        }
        std::string title = "";
        try
        {
            // throw exception
            if (doc.getString("Title", title))
            {
                laserManager_->index(docID, title);
            }
        }
        catch (std::exception& e)
        {
            LOG(INFO)<<e.what();
        }
        return true;
    }

    virtual bool preProcess(int64_t timestamp)
    {
        laserManager_->indexManager_->preIndex();
        docid_ = laserManager_->indexManager_->getLastDocId();
        return true;
    }

    virtual bool postProcess()
    {
        laserManager_->indexManager_->setLastDocId(docid_);
        laserManager_->indexManager_->postIndex();
        return true;
    }

    virtual docid_t getLastDocId()
    {
        return docid_;
    }

private:
    LaserManager* laserManager_;
    docid_t docid_;
    boost::shared_mutex mutex_;
};

} }
#endif
