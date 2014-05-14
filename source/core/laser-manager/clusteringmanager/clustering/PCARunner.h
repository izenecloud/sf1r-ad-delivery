#ifndef SF1R_LASER_PCA_RUNNER_H
#define SF1R_LASER_PCA_RUUNER_H

#include <queue>
#include <list>
#include <string>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <am/sequence_file/ssfr.h>
#include <util/functional.h>
#include <boost/unordered_map.hpp>
#include <knlp/title_pca.h>
#include "laser-manager/clusteringmanager/type/TermDictionary.h"

namespace sf1r { namespace laser { namespace clustering {

class Document
{
public:
    std::string title;
    std::string category;
};

class PCARunner
{
    friend class PCAProcess;

    typedef boost::unordered_map<std::string, int> Dictionary;
    typedef std::pair<int, boost::unordered_map<std::string, float> > Clustering;
    typedef boost::unordered_map<std::string, Clustering> ClusteringContainer;
    typedef std::list<Document> DocumentVecType;
    
    class ThreadContext
    {
    public:
        ThreadContext()
        {
            mutex_ = new boost::shared_mutex();
            docs_ = new DocumentVecType();
            cat_ = new Dictionary();
            terms_ = new Dictionary();
            clusteringContainer_ = new ClusteringContainer();
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
            if (NULL != clusteringContainer_)
            {
                delete clusteringContainer_;
                clusteringContainer_ = NULL;
            }
        }
    public:
        boost::shared_mutex* mutex_;
        DocumentVecType* docs_;
        Dictionary* cat_;
        Dictionary* terms_;
        ClusteringContainer* clusteringContainer_;
    };

public: 
    
    PCARunner(const std::size_t thread, 
        type::TermDictionary& t, 
        const std::string& pcapath, 
        float threshold, 
        const std::size_t maxDocPerClustering, 
        const std::size_t minDocPerClustering )
      : term_dictionary(t)
      , tok(pcapath)
      , context_(NULL)
      , thread_(NULL)
      , exit_(false)
      , THREAD_NUM_(thread)
      , THRESHOLD_(threshold)
      , MAX_DOC_PER_CLUSTERING_(maxDocPerClustering)
      , MIN_DOC_PER_CLUSTERING_(minDocPerClustering)
    {
        context_ = new std::vector<ThreadContext*>(THREAD_NUM_);
        for (std::size_t i =0; i < THREAD_NUM_; ++i)
        {
            ThreadContext* context = new ThreadContext();
            (*context_)[i] = context;
        }
        thread_ = new std::vector<boost::thread*>(THREAD_NUM_);
        clusteringContainer_ = new ClusteringContainer();
    }

    ~PCARunner()
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

        if (NULL != clusteringContainer_)
        {
            delete clusteringContainer_;
            clusteringContainer_ = NULL;
        }
    }

public:
    void push_back(Document& doc)
    {
        boost::unordered_map<std::string, std::list<Document> >::iterator it = cache_.find(doc.category);
        if (cache_.end() != it)
        {
            it->second.push_back(doc);
            if (it->second.size() > CACHE_THRESHOLD)
            {
                push_back(it);
                it->second.clear();
            }
        }
        else
        {
            cache_[doc.category].push_back(doc);
        }
    }
    
    
    void startStatistics()
    {
        for (std::size_t i = 0; i < THREAD_NUM_; ++i)
        {
            (*thread_)[i] = new boost::thread(boost::bind(&PCARunner::statistics_, this, (*context_)[i]));
        }
    }
    
    void stopStatistics()
    {
        stop_();
        mergeTerm_();
    }


    void startClustering()
    {
        exit_ = false;
        for (std::size_t i = 0; i < THREAD_NUM_; ++i)
        {
            (*thread_)[i] = new boost::thread(boost::bind(&PCARunner::clustering_, this, (*context_)[i]));
        }
    }
    
    void stopClustering()
    {
        stop_();
        mergeClustering_();
    }

    const ClusteringContainer& getClusteringResult() const
    {
        return *clusteringContainer_;
    }
private:
    void statistics_(ThreadContext* context);
    DocumentVecType::iterator distribution_(DocumentVecType& docv, 
        const std::size_t size, 
        Dictionary& ccnt, 
        Dictionary& termList);

    void clustering_(ThreadContext* context);
    DocumentVecType::iterator doClustering_(DocumentVecType& docv, 
        const std::size_t size, 
        const Dictionary& ccnt, 
        const type::TermDictionary& dict, 
        ClusteringContainer& clusteringContainer);

    void push_back(boost::unordered_map<std::string, std::list<Document> >::const_iterator cache)
    {
        const std::string cat = cache->first;
        int index = Hash_(cat) % THREAD_NUM_;
        boost::shared_mutex* mutex = (*context_)[index]->mutex_;
        DocumentVecType* docs = (*context_)[index]->docs_;
        //docs->insert(docs->end(),vec.begin(), vec.end());
        std::list<Document>::const_iterator it = cache->second.begin();
        boost::unique_lock<boost::shared_mutex> uniqueLock(*mutex);
        for (; it != cache->second.end(); ++it)
        {
            docs->push_back(*it);
        }
    }
    
    bool isExit_()
    {
        boost::shared_lock<boost::shared_mutex> sharedLock(mutex_);
        return exit_;
    }
    
    
    void mergeTerm_()
    {
        LOG(INFO)<<"merge terms...";
        for (std::size_t i = 0; i < THREAD_NUM_; ++i)
        {
            Dictionary* terms = (*context_)[i]->terms_;
            for (Dictionary::const_iterator it = terms->begin(); it != terms->end(); ++it)
            {
                term_dictionary.set(it->first, it->second);
            }
        }
    }
    
    void mergeClustering_()
    {
        LOG(INFO)<<"merge clustering...";
        for (std::size_t i = 0; i < THREAD_NUM_; ++i)
        {
            const ClusteringContainer* container = (*context_)[i]->clusteringContainer_;
            ClusteringContainer::const_iterator it = container->begin();
            for (; it != container->end(); ++it)
            {
                (*clusteringContainer_)[it->first] = it->second;
            }
        }
    }

    void tokenize_(const std::string title, std::vector<std::pair<std::string, float> >& tks);
   
    void stop_()
    {
        {
            boost::unordered_map<std::string, std::list<Document> >::iterator it = cache_.begin();
            for (; it != cache_.end(); ++it)
            {
                push_back(it);
            }
            cache_.clear();
        }
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
    }

    void pushClustering_(const std::string& cateMerge, 
        const std::vector<std::pair<std::string, float> >& tks,
        const type::TermDictionary& dict,
        ClusteringContainer& clusteringContainer) const;
private:
    type::TermDictionary& term_dictionary;
    ilplib::knlp::TitlePCA tok;
    std::vector<ThreadContext*>* context_;
    std::vector<boost::thread*>* thread_;
    boost::unordered_map<std::string, std::list<Document> > cache_;
    boost::shared_mutex mutex_;
    bool exit_;

    ClusteringContainer* clusteringContainer_;
    
    const std::size_t THREAD_NUM_;
    const float THRESHOLD_;
    const std::size_t MAX_DOC_PER_CLUSTERING_;
    const std::size_t MIN_DOC_PER_CLUSTERING_;
    const static std::size_t CACHE_THRESHOLD = 1024;
};
} } }
#endif /* SEGMENTTOOL_H_ */
