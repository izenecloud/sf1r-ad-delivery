/*
 * SortTool.h
 *
 *  Created on: Apr 4, 2014
 *      Author: alex
 */

#ifndef ComputationTool_H_
#define ComputationTool_H_
#include <queue>
#include <string>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include "am/sequence_file/ssfr.h"
#include "type/ClusteringInfo.h"
#include "common/utils.h"
#include "common/constant.h"
#include "type/Document.h"
#include "type/ClusteringData.h"
#include "type/ClusteringInfo.h"
#include "type/ClusteringDataAdapter.h"
#include "type/Term.h"
#include <map>
using std::queue;
using std::string;
using std::map;
using namespace clustering::type;
namespace clustering
{

class ComputationTool
{
private:
    size_t min_clustering_doc_num;
    size_t max_clustering_doc_num;
    int threadNum;
    queue<ClusteringInfo> infos;
    ClusteringDataAdapter* clusteringDataAdpater;
    const map<hash_t, Term>& terms_;
    boost::mutex io_mutex;

public:
    ComputationTool(int thread, queue<ClusteringInfo> path,
                    size_t min_clustering_doc_num_,
                    size_t max_clustering_doc_num_,
                    const map<hash_t, Term>& terms,
                    ClusteringDataAdapter* clusteringDataAdpater_)
        :min_clustering_doc_num(min_clustering_doc_num_),
        max_clustering_doc_num(max_clustering_doc_num_),
        threadNum(thread),
        infos(path),
        clusteringDataAdpater(clusteringDataAdpater_),
        terms_(terms)
    {
    }

    void run()
    {
        ClusteringInfo info;
        while(getNext(info))
        {
            izene_reader_pointer reader = openFile<izene_reader>(info.clusteringMidPath, true);
            izene_writer_pointer writer = openFile<izene_writer>(info.clusteringResPath, true);
            izene_writer_pointer clustering_term_pow_writer = openFile<izene_writer>(info.clusteringPowPath, true);
            hash_t cat;
            Document value("");
            hash_t older = 0;
            bool next = true;
            bool last = false;
            ClusteringInfo clusteringInfo;
            ClusteringData clusteringData;
            std::vector<Document>& c_cat_vector = clusteringData.clusteringData;
            c_cat_vector.reserve(max_clustering_doc_num);
            map<hash_t, float>& mean_pow = clusteringInfo.clusteringPow;
            while ((next = reader->Next(cat, value)) || last)
            {
                if (older != cat || !next)   //new clustering or the last document
                {
                    if (c_cat_vector.size() > min_clustering_doc_num)
                    {
                        clusteringData.clusteringHash = clusteringInfo.clusteringHash = older;
                        clusteringInfo.clusteringname = "";
                        for (std::vector<Document>::iterator iter =
                                    c_cat_vector.begin(); iter != c_cat_vector.end();
                                iter++)
                        {
                            writer->Append(cat, *iter);
                        }
                        int c_count = c_cat_vector.size();
                        for (map<hash_t, float>::iterator iter = mean_pow.begin();
                                iter != mean_pow.end(); iter++)
                        {
                            iter->second /= c_count;
                        }
                        clusteringInfo.clusteringDocNum = c_count;
                        //save to kv-db or else
                        clusteringDataAdpater->save(clusteringData, clusteringInfo);
                        clustering_term_pow_writer->Append(cat, mean_pow);
                    }
                    mean_pow.clear();
                    std::vector<Document> swp;
                    c_cat_vector.swap(swp);
                    c_cat_vector.reserve(max_clustering_doc_num);
                    older = cat;
                }
                if (next == false)
                {
                    break;
                }
                last = next;
                Document new_value(value.doc_id);
                map<hash_t, float> tf = value.terms;
                for (map<hash_t, float>::iterator iter = tf.begin();
                        iter != tf.end(); iter++)
                {
                    //erase the term which is not in the dictionary
                    std::map<hash_t, Term>::const_iterator finder = terms_.find(iter->first);
                    if (finder != terms_.end())
                    {
                        new_value.add(finder->second.index, iter->second);
                    }
                }
                if (new_value.terms.size() < 2)
                {
                    //cout<<"REMOVE DOC:"<<value.doc_id<<endl;
                }
                else
                {
                    for (map<hash_t, float>::iterator iter =
                                new_value.terms.begin(); iter != new_value.terms.end();
                            iter++)
                    {
                        mean_pow[iter->first] += iter->second;
                    }
                    c_cat_vector.push_back(new_value);
                }
            }
            closeFile<izene_writer>(clustering_term_pow_writer);
            closeFile<izene_reader>(reader);
            closeFile<izene_writer>(writer);
        }
    }
    void start()
    {
        boost::function0<void> f = boost::bind(&ComputationTool::run, this);
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

    bool getNext(ClusteringInfo& info)
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
};

} /* namespace clustering */
#endif /* ComputationTool_H_ */
