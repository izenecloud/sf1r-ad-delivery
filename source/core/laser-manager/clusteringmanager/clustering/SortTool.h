/*
 * SortTool.h
 *
 *  Created on: Apr 4, 2014
 *      Author: alex
 */

#ifndef SF1R_LASER_CLUSTERINGSORTTOOL_H_
#define SF1R_LASER_CLUSTERINGSORTTOOL_H_

#include <queue>
#include <string>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <am/sequence_file/ssfr.h>
#include "laser-manager/clusteringmanager/type/Document.h"

#include <glog/logging.h>
namespace sf1r
{
namespace laser
{
namespace clustering
{

//template <class K, class V>
class ClusteringSortTool
{
private:
    std::queue<std::string> paths;
    int threadNum;
    boost::mutex io_mutex;
public:
    void run()
    {
        std::string path;
        while(getNext(path))
        {
            izenelib::am::ssf::Sorter<uint32_t, hash_t>::Sort(path);
        }
    }
    void start()
    {
        boost::function0<void> f = boost::bind(&ClusteringSortTool::run, this);
        std::vector<boost::thread*> thread_array;
        for(int i = 0; i < threadNum; i++)
        {
            boost::thread* bt = new boost::thread(f);
            thread_array.push_back(bt);
        }
        for(std::vector<boost::thread*>::iterator iter = thread_array.begin();
                iter != thread_array.end(); iter++)
        {
            (*iter)->join();
            delete (*iter);
        }
    }

    bool getNext(std::string& path)
    {
        boost::mutex::scoped_lock lock(io_mutex);
        if(paths.empty())
            return false;
        else
        {
            path = paths.front();
            paths.pop();
            return true;
        }
    }
    ClusteringSortTool(int thread, std::queue<std::string> path)
    {
        threadNum = thread;
        paths = path;
    }
};
} /* namespace clustering */
}
}
#endif /* CLUSTERINGSORTTOOL_H_ */
