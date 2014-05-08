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
#include "laser-manager/clusteringmanager/type/Term.h"
#include <map>
namespace sf1r { namespace laser { namespace clustering {
class ComputationTool
{
public:
    ComputationTool(int thread, std::queue<type::ClusteringInfo>& path,
                    size_t min_clustering_doc_num_,
                    size_t max_clustering_doc_num_,
                    const boost::unordered_map<std::string, type::Term>& terms,
                    type::ClusteringDataAdapter* clusteringDataAdpater_)
        : min_clustering_doc_num(min_clustering_doc_num_)
        , max_clustering_doc_num(max_clustering_doc_num_)
        , THREAD_NUM_(thread)
        , infos(path)
        , clusteringDataAdpater(clusteringDataAdpater_)
        , terms_(terms)
    {
    }

    void start()
    {
        boost::function0<void> f = boost::bind(&ComputationTool::run, this);
        std::vector<boost::thread*> thread_array;
        for(int i = 0; i < THREAD_NUM_; i++)
        {
            boost::thread* bt = new boost::thread(f);
            thread_array.push_back(bt);
        }
    }

    void join()
    {
        for(std::vector<boost::thread*>::iterator iter = thread_array.begin();
                iter != thread_array.end(); iter++)
        {
            (*iter)->join();
            delete (*iter);
        }
    }

private:
    void run();

    bool getNext(type::ClusteringInfo& info)
    {
        boost::mutex::scoped_lock lock(io_mutex);
        if(infos.empty())
            return false;
        else
        {
            info = infos.front();
            infos.pop();
            return true;
        }
    }

private:
    const std::size_t min_clustering_doc_num;
    const std::size_t max_clustering_doc_num;
    const int THREAD_NUM_;
    std::queue<type::ClusteringInfo>& infos;
    type::ClusteringDataAdapter* clusteringDataAdpater;
    const boost::unordered_map<std::string, type::Term>& terms_;
    boost::mutex io_mutex;

};

} } }
#endif /* ComputationTool_H_ */
