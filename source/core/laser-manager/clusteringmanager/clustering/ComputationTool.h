/*
 * SortTool.h
 *
 *  Created on: Apr 4, 2014
 *      Author: alex
 */

#ifndef SF1R_LASER_ComputationTool_H_
#define SF1R_LASER_ComputationTool_H_
#include <queue>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <am/sequence_file/ssfr.h>
#include "laser-manager/clusteringmanager/type/ClusteringInfo.h"
#include "laser-manager/clusteringmanager/common/utils.h"
#include "laser-manager/clusteringmanager/common/constant.h"
#include "laser-manager/clusteringmanager/type/Document.h"
#include "laser-manager/clusteringmanager/type/ClusteringData.h"
#include "laser-manager/clusteringmanager/type/ClusteringInfo.h"
#include "laser-manager/clusteringmanager/type/ClusteringDataAdapter.h"
#include "SegmentTool.h"

namespace sf1r { namespace laser { namespace clustering {
class ComputationTool
{
public:
    ComputationTool(int thread, std::queue<type::ClusteringInfo>& path,
                    size_t min_clustering_doc_num_,
                    size_t max_clustering_doc_num_,
                    const boost::unordered_map<std::string, std::pair<int, int> >& terms,
                    type::ClusteringDataAdapter* clusteringDataAdpater_)
        : min_clustering_doc_num(min_clustering_doc_num_)
        , max_clustering_doc_num(max_clustering_doc_num_)
        , THREAD_NUM_(thread)
        , infos(path)
        , clusteringDataAdpater(clusteringDataAdpater_)
        , terms_(terms)
    {
        thread_.reserve(THREAD_NUM_);
    }

    void start()
    {
        boost::function0<void> f = boost::bind(&ComputationTool::run, this);
        for(int i = 0; i < THREAD_NUM_; i++)
        {
            boost::thread* bt = new boost::thread(f);
            thread_.push_back(bt);
        }
    }

    void join()
    {
        for(std::vector<boost::thread*>::iterator iter = thread_.begin();
                iter != thread_.end(); iter++)
        {
            (*iter)->join();
            delete (*iter);
        }
    }

private:
    void run();

    bool getNext(type::ClusteringInfo& info)
    {
        boost::unique_lock<boost::shared_mutex> uniqueLock(io_mutex);
        if(infos.empty())
            return false;
        else
        {
            info = infos.front();
            infos.pop();
            return true;
        }
    }

    int map(const std::string& clustering)
    {
        int ret = get(clustering);
        if (-1 == ret)
        {
            return set(clustering);
        }
        return ret;
    }

    int get(const std::string& clustering)
    {
        boost::shared_lock<boost::shared_mutex> sharedLock(clusteringIdMapMutex_);
        SegmentTool::Dictionary::const_iterator it = clusteringIdMap_.find(clustering);
        if (clusteringIdMap_.end() == it)
        {
            return -1;
        }
        return it->second;
    }

    int set(const std::string& clustering)
    {
        boost::unique_lock<boost::shared_mutex> uniqueLock(clusteringIdMapMutex_);
        int id = clusteringIdMap_.size();
        clusteringIdMap_[clustering] = id;
        return id;
    }
private:
    const std::size_t min_clustering_doc_num;
    const std::size_t max_clustering_doc_num;
    const int THREAD_NUM_;
    std::vector<boost::thread*> thread_;
    std::queue<type::ClusteringInfo>& infos;
    type::ClusteringDataAdapter* clusteringDataAdpater;
    const boost::unordered_map<std::string, std::pair<int, int> >& terms_;
    SegmentTool::Dictionary clusteringIdMap_;
    boost::shared_mutex clusteringIdMapMutex_;
    boost::shared_mutex io_mutex;

};

} } }
#endif /* ComputationTool_H_ */
