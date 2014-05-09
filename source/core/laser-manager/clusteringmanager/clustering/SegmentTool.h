#ifndef SF1R_LASER_SEGMENT_TOOL_H
#define SF1R_LASER_SEGMENT_TOOL_H

#include <queue>
#include <list>
#include <string>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <am/sequence_file/ssfr.h>
#include <util/functional.h>
#include <boost/unordered_map.hpp>
#include <knlp/title_pca.h>
#include "laser-manager/clusteringmanager/common/utils.h"
#include "laser-manager/clusteringmanager/type/TermDictionary.h"
#include "OriDocument.h"

namespace sf1r { namespace laser { namespace clustering {

class SegmentTool
{
public:
    typedef std::list<OriDocument> DocumentVecType;

private:
    typedef boost::unordered_map<std::string, int> Dictionary;
    
    class ThreadContext
    {
    public:
        ThreadContext()
        {
            mutex_ = new boost::shared_mutex();
            docs_ = new DocumentVecType();
            cat_ = new Dictionary();
            terms_ = new Dictionary();
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
        Dictionary* cat_;
        Dictionary* terms_;
    };

public: 
    
    SegmentTool(int thread, type::TermDictionary& t, std::string& pcapath, float threshold, std::size_t maxDocPerClustering )
      : term_dictionary(t)
      , tok(pcapath)
      , context_(NULL)
      , thread_(NULL)
      , exit_(false)
      , THREAD_NUM_(thread)
      , THRESHOLD_(threshold)
      , MAX_DOC_PER_CLUSTERING_(maxDocPerClustering)
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

    friend class ComputationTool;
public:
    void push_back(std::string cat, OriDocument& doc)
    {
        int index = Hash_(cat) % THREAD_NUM_;
        boost::shared_mutex* mutex = (*context_)[index]->mutex_;
        DocumentVecType* docs = (*context_)[index]->docs_;
        boost::unique_lock<boost::shared_mutex> uniqueLock(*mutex);
        //docs->insert(docs->end(),vec.begin(), vec.end()); 
        docs->push_back(doc);
    }
    
    
    void start()
    {
        for (std::size_t i = 0; i < THREAD_NUM_; ++i)
        {
            ThreadContext* context = new ThreadContext();
            (*context_)[i] = context;
            (*thread_)[i] = new boost::thread(boost::bind(&SegmentTool::run, this, context));
        }
    }
    
    void stop()
    {
        {
            boost::unique_lock<boost::shared_mutex> uniqueLock(mutex_);
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
    void run(ThreadContext* context);
    
    bool isExit_()
    {
        boost::shared_lock<boost::shared_mutex> sharedLock(mutex_);
        return exit_;
    }
    
    void calc_(DocumentVecType& docv, std::size_t size, Dictionary& ccnt, Dictionary& termList);
    
    void mergeTerm_()
    {
        for (std::size_t i = 0; i < THREAD_NUM_; ++i)
        {
            Dictionary* terms = (*context_)[i]->terms_;
            for (Dictionary::const_iterator it = terms->begin(); it != terms->end(); ++it)
            {
                term_dictionary.set(it->first, it->second);
            }
        }
    }
    
private:
    type::TermDictionary& term_dictionary;
    ilplib::knlp::TitlePCA tok;
    std::vector<ThreadContext*>* context_;
    std::vector<boost::thread*>* thread_;
    boost::shared_mutex mutex_;
    bool exit_;
    
    const std::size_t THREAD_NUM_;
    const float THRESHOLD_;
    const std::size_t MAX_DOC_PER_CLUSTERING_;
};
} } }
#endif /* SEGMENTTOOL_H_ */
