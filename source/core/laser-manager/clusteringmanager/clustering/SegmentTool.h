#ifndef SF1R_LASER_SEGMENT_TOOL_H
#define SF1R_LASER_SEGMENT_TOOL_H

#include <queue>
#include <string>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <am/sequence_file/ssfr.h>
#include <util/functional.h>
#include <boost/unordered_map.hpp>
#include <map>
#include "laser-manager/clusteringmanager/common/utils.h"
namespace sf1r { namespace laser { namespace clustering {

class SegmentTool
{
    typedef std::vector<OriDocument> DocumentVecType;
    typedef boost::unordered_map<std::string, int> CatDictionary;
    
    class ThreadContext
    {
    public:
        ThreadContext()
        {
            mutex_ = new boost::shared_mutex();
            docs_ = new DocumentVecType();
            cat_ = new CatDictionary();
            terms_ = new CatDictionary();
        }
        
        ~ThreadContext()
        {
            if (NULL != mutex_)
            {
                delete mutex_;
                mutex_ = NULL;
            }

            if (NULL != docs_)
            {
                delete docs_;
                docs_ = NULL;
            }

            if (NULL != cat_)
            {
                delete cat_;
                cat_ = NULL;
            }
            
            if (NULL != terms_)
            {
                delete terms_;
                terms_ = NULL;
            }
        }
    public:
        boost::shared_mutex* mutex_;
        DocumentVecType* docs_;
        CatDictionary* cat_;
    };

public: 
    SegmentTool(int thread, type::TermDictionary& t, std::string& pcapath, float threshold, std::size_t maxDocPerClustering )
      : THREAD_NUM_(thread)
      , THRESHOLD_(threshold)
      , MAX_DOC_PER_CLUSTERING_(maxDocPerClustering)
      , term_dictionary(t)
      , tok(pcapath)
      , exit_(false)
      , context_(NULL)
      , thread_(NULL)
    {
        context_ = new std::vector<ThreadContext*>(THREAD_NUM_);
        thread_ = new std::vector<boost::thread*>(THREAD_NUM_);
    }

    ~SegmentTool()
    {
        if (NULL != context_)
        {
            delete context_;
            context_ = NULL;
        }

        if (NULL != thread_)
        {
            delete thread_;
            thread_ = NULL;
        }
    }
public:
    void push_back(std::string cat, DocumentVecType& vec)
    {
        int index = Hash_(cat) % THREAD_NUM_;
        boost::shared_mutex* mutex = (*context_)[index].mutex_;
        DocumentVecType* docs = (*context_)[index].docs_;
        boost::mutex::unique_lock<boost::shared_mutex> uniqueLock(mutex);
        docs->insert(documentPool_[cat].end(),vec.begin(), vec.end()); 
    }
    
    
    void start()
    {
        for (int i = 0; i < THREAD_NUM_; ++i)
        {
            (*context_)[i] = new ThreadContext();
            boost::function0<int> f = boost::bind(&SegmentTool::run, this, i);
            (*thread_)[i] = new boost::thread(f);
        }
    }
    
    void stop()
    {
        {
            boost::unique_lock<boost::shared_mutex> uniqueLock(mutex_)
            exit_ = true;
        }
        for (std::size_t i = 0; i < THREAD_NUM_; ++i)
        {
            boost::thread* thread = (*thread_)[i];
            thread->join();
            delete thread;
        }

        mergeTerm_();

        for (std::size_t i = 0; i < THREAD_NUM_; ++i)
        {
            delete (*context_)[i];
        }
    }

     
private:
    bool isExist_() const
    {
        boost::shared_lock<boost::shared_mutex> sharedLock(mutex_);
        return exit_;
    }
    
    void calc_(DocumentVecType& docv, HashMap& ccnt, HashMap& termList);
    
    void mergeTerm_()
    {
        for (std::size_t i = 0; i < THREAD_NUM_; ++i)
        {
            CatDictionary* terms = (*context_)[i].terms_;
            for (CatDictionary::const_iterator it = terms->begin(); it != terms->end(); ++it)
            {
                term_dictionary.set(it->first, it->second);
            }
        }
    }
    
private:
    queue<std::string> paths;
    type::TermDictionary& term_dictionary;
    ilplib::knlp::TitlePCA tok;
    std::vector<boost::thread*>* thread_;
    std::vector<ThreadContext*>* context_;
    boost::shared_mutex mutex_;
    bool exit_;
    
    const float THRESHOLD_;
    const std::size_t MAX_DOC_PER_CLUSTERING_;
    const int THREAD_NUM_;
};
} } }
#endif /* SEGMENTTOOL_H_ */
