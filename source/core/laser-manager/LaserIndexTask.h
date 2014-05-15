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
    {
    }

    virtual ~LaserIndexTask()
    {
    }
public:
    virtual bool buildDocument(docid_t docID, const Document& doc)
    {
        std::string title = "";
        if (doc.getString("Title", title))
        {
            laserManager_->index(docID, title);
        }
        return true;
    }

    virtual bool preProcess(int64_t timestamp)
    {
        return true;
    }

    virtual bool postProcess()
    {
        return true;
    }

    virtual docid_t getLastDocId()
    {
        return 0;
    }

private:
    LaserManager* laserManager_;
};

} }
#endif
