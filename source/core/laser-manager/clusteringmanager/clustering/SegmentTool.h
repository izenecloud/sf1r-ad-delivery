/*
 * SegmentTool.h
 *
 *  Created on: Apr 4, 2014
 *      Author: alex
 */

#ifndef SEGMENTTOOL_H_
#define SEGMENTTOOL_H_

#include <queue>
#include <string>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <am/sequence_file/ssfr.h>
#include <util/functional.h>
#include <boost/unordered_map.hpp>
#include <map>
#include "laser-manager/clusteringmanager/common/utils.h"
namespace sf1r
{
namespace laser
{
namespace clustering
{

enum PSTATUS
{
    NEW=0,
    HOLD,
    TOAPPEND
};
//template <class K, class V>
class SegmentTool
{
public:
    typedef boost::unordered_map<std::string, PSTATUS> StatusMapType;
    typedef boost::unordered_map<std::string, std::vector<OriDocument> > DocumentMapType;
    typedef vector<OriDocument> DocumentVecType;
    typedef boost::unordered_map<hash_t, std::pair<std::string, int> > CoverNumType;
private:
    boost::unordered_map<hash_t, CoverNumType > cat_limit_count;
    StatusMapType operationMap_;
    DocumentMapType documentPool_;
    queue<std::string> paths;
    int threadNum;
    type::TermDictionary& term_dictionary;
    ilplib::knlp::TitlePCA tok;
    float threhold;
    size_t max_clustering_doc_num;
    bool running;
    boost::mutex documentPools_mutex_;
    //boost::shared_mutex stop_mutex;
    std::vector<boost::thread*> thread_array;
public:
    bool empty()
    {
       // boost::mutex::scoped_lock lock(documentPools_mutex_);
        bool res = ( documentPool_.size() == 0);
        return res;
    }
    bool getNext(std::string& cat, DocumentVecType &vec,  CoverNumType& ccnt)
    {
        boost::mutex::scoped_lock lock(documentPools_mutex_);
        if(cat.compare("") != 0 && ccnt.size() > 0)
            cat_limit_count[Hash_(cat)] = ccnt;
        if(documentPool_.size() == 0)
        {
            cat = "";
            return false;
        }
        if(cat.compare("") != 0)
        {
            StatusMapType::iterator iter = operationMap_.find(cat);
            if(iter == operationMap_.end())
            {
            }
            else
            {
                if(iter->second == TOAPPEND)
                {
                    vec = documentPool_[cat];
                    documentPool_[cat].clear();
                    if(vec.size() == 0)
                    {
                    //    cat_limit_count[Hash_(cat)] = ccnt;
                        operationMap_.erase(iter);
                    }
                    else
                    {
                        iter->second = HOLD;
                        return true;
                    }
                }
                else
                {
                    operationMap_.erase(iter);
                }
            }
        }
        {
            for(DocumentMapType::iterator iter = documentPool_.begin(); iter != documentPool_.end(); iter++)
            {
                if(operationMap_.find(iter->first) == operationMap_.end())
                {
                    vec = iter->second;
                    cat = iter->first;
                    iter->second.clear();
                    ccnt = cat_limit_count[Hash_(cat)];
                    operationMap_.insert(make_pair<std::string, PSTATUS>(iter->first, HOLD));
                    documentPool_.erase(iter);
                    return true;
                }
            }
            cat = "";
            return false;
        }
    }
    void mergeterm(CoverNumType& termlist )
    {
        boost::mutex::scoped_lock lock(documentPools_mutex_);
        for(CoverNumType::iterator iter = termlist.begin(); iter != termlist.end(); iter++)
        {
            term_dictionary.get(iter->second.first, iter->second.second);
        }
    }
    void pushback(std::string cat, DocumentVecType& vec)
    {
        boost::mutex::scoped_lock lock(documentPools_mutex_);
        if(operationMap_.find(cat) != operationMap_.end() )
        {
            operationMap_[cat] = TOAPPEND;
        }
        else
        {
        }
        documentPool_[cat].insert(documentPool_[cat].end(),vec.begin(), vec.end()); 
    }
    void calc(DocumentVecType& docv, CoverNumType& ccnt, CoverNumType& termlist )
    {
        typedef izenelib::util::second_greater<std::pair<std::string, float> > greater_than;
        if(docv.size() == 0)
            return ;
        izene_writer_pointer iwp = type::ClusteringListDes::get()->get_cat_mid_writer(
                                   docv[0].category);
        for(DocumentVecType::iterator iter = docv.begin(); iter != docv.end(); iter++)
        {
            std::vector<std::pair<std::string, float> > tks;
            std::pair<std::string, float> tmp;
            std::vector<std::pair<std::string, float> > subtks;
            std::string brand, mdt;
            tok.pca(iter->title, tks, brand, mdt, subtks, false);
            double tot = 0, now = 0;
            for (size_t i = 0; i < tks.size(); ++i)
            {
                if (mdt == tks[i].first)
                    tks[i].second = 0;
                tot += tks[i].second;
            }

            if(tot == 0)
            {
                continue;
            }
            
            std::sort(tks.begin(), tks.end(), greater_than());
            
            std::stringstream category_merge;
            category_merge << iter->category;
            type::Document d(iter->docid);
            size_t cat_limit_c = std::numeric_limits<size_t>::max();
            hash_t cat_hash_value = 0;
            for (size_t i = 0; i < tks.size(); ++i)
            {
                if(tks[i].first.length() == 1)
                    continue;
                hash_t th = Hash_(tks[i].first);
                if(termlist.find(th) == termlist.end())
                {
                    termlist[th] = make_pair<std::string, int>(tks[i].first, 1);
                }
                else
                {
                    termlist[th].second++;
                }
                d.add(th, tks[i].second/tot);
                now += tks[i].second;
                if (now > tot * threhold)   //check whether the document number in this category reach the limit
                {
                    if (cat_limit_c > max_clustering_doc_num)   // temporary number{
                    {
                        category_merge << "<" << tks[i].first;
                        hash_t cat_hash = Hash_(category_merge.str());
                        if(ccnt.find(cat_hash) != ccnt.end())
                        {
                            cat_limit_c = ccnt[cat_hash].second;
                        }
                        else
                        {
                            ccnt[cat_hash]=make_pair<std::string, int> (category_merge.str(), 0);
                            cat_limit_c = 0;
                        }
                        if (cat_limit_c < max_clustering_doc_num)
                        {
                            ccnt[cat_hash].second++;
                            cat_hash_value = cat_hash;
                        }
                        else
                        {
                        }
                    }
                }
                else
                {
                    category_merge << "<" << tks[i].first;
                    now += tks[i].second;
                }

            }
                //hash_t h = CatDictionary::get()->getCatHash(category_merge.str());
            if (cat_hash_value > 0)
                iwp->Append(cat_hash_value, d);
        }

    }
    void run()
    {
        std::string cat = "";
        DocumentVecType vec;
        CoverNumType termlist;
        CoverNumType ccnt;
        while(getRunning())
            //|| !empty())
        {
            getNext(cat, vec, ccnt);
            if(vec.size() > 0)
            {
                calc(vec, ccnt, termlist );
            }
            else
            {
                boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
            }
            vec.clear();
        }
        mergeterm(termlist);
    }
    void start()
    {
        boost::function0<void> f = boost::bind(&SegmentTool::run, this);
        for(int i = 0; i < threadNum; i++)
        {
            boost::thread* bt = new boost::thread(f);
            thread_array.push_back(bt);
        }
    }
    void stop()
    {
        setRunning(false);
        if(!empty())
        {
            boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

        }
        for(std::vector<boost::thread*>::iterator iter = thread_array.begin();
                iter != thread_array.end(); iter++)
        {
            (*iter)->join();
            delete (*iter);
            *iter = NULL;
        }
        thread_array.clear();
        for(boost::unordered_map<hash_t, CoverNumType >::iterator iter = cat_limit_count.begin(); iter != cat_limit_count.end(); iter++)
        {
            for(CoverNumType::iterator in =iter->second.begin(); in != iter->second.end(); in++){
                type::CatDictionary::get()->addCatHash(in->second.first, in->first, in->second.second);
            }
        }
        cat_limit_count.clear();
    }
    void setRunning(bool run)
    {
//        boost::unique_lock< boost::shared_mutex > lock(stop_mutex);
        running = run;
    }
    bool getRunning()
    {
   //     boost::shared_lock<boost::shared_mutex> lock(stop_mutex);
        return running;
    }
    SegmentTool(int thread, type::TermDictionary& t, std::string& pcapath, float threhold_,int max_clustering_doc_num_ )
      :threadNum(thread),term_dictionary(t), tok(pcapath), threhold(threhold_), max_clustering_doc_num(max_clustering_doc_num_)
    {
        running = true;
    }
};
} /* namespace clustering */
}
}
#endif /* SEGMENTTOOL_H_ */
