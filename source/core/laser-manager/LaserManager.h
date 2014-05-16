#ifndef SF1R_LASER_MANAGER_LASER_MANAGER_H
#define SF1R_LASER_MANAGER_LASER_MANAGER_H
#include <boost/shared_ptr.hpp>
#include <mining-manager/MiningManager.h>
#include <mining-manager/MiningTask.h>
#include <common/ResultType.h>

#include "LaserRecommend.h"
#include "LaserRecommendParam.h"
#include "AdIndexManager.h"
#include "Tokenizer.h"
#include "TopNClusteringDB.h"
#include "clusteringmanager/type/TermDictionary.h"

namespace sf1r { namespace laser {
class LaserIndexTask;
class LaserRpcServer;
} }
namespace sf1r {

class LaserManager
{
    typedef std::vector<float> TokenVector;
    typedef std::vector<std::pair<int, float> > TokenSparseVector;
    friend class laser::LaserRpcServer;

    class ThreadContext
    {
    public:
        ThreadContext(const std::vector<TokenVector>* clusteringContainer,
            const std::size_t bIndex,
            const std::size_t eIndex)
            : clusteringContainer_(clusteringContainer)
            , v_(NULL)
            , bIndex_(bIndex)
            , eIndex_(eIndex)
            , maxIndex_(-1)
            , max_(0.0)
            , exit_(false)
            , finish_(true)
        {
        }
    public:
        const std::size_t maxIndex() const
        {
            return maxIndex_;
        }

        const float max() const
        {
            return max_;
        }

        void set(const TokenSparseVector* v)
        {
            boost::unique_lock<boost::shared_mutex> uniqueLock(mutex_);
            v_ = v;
            if (NULL == v_)
            {
                exit_ = true;
            }
            else
            {
                finish_ = false;
            }
        }

        const TokenSparseVector* get() const
        {
            boost::shared_lock<boost::shared_mutex> sharedLock(mutex_);
            return v_;
        }

        void waitFinish() const
        {
            while(true)
            {
                boost::shared_lock<boost::shared_mutex> sharedLock(mutex_);
                if (finish_)
                    break;
            }
        }

        void setFinish()
        {
            boost::unique_lock<boost::shared_mutex> uniqueLock(mutex_);
            v_ = NULL;
            finish_ = true;
        }


        bool isExist() const
        {
            boost::shared_lock<boost::shared_mutex> sharedLock(mutex_);
            return exit_;
        }

    public:
        const std::vector<TokenVector>* clusteringContainer_;
        const TokenSparseVector* v_;
        const std::size_t bIndex_;
        const std::size_t eIndex_;
        std::size_t maxIndex_;
        float max_;
        bool exit_;
        bool finish_;
        mutable boost::shared_mutex mutex_;
    };
public:
    LaserManager(const boost::shared_ptr<AdSearchService>& adSearchService, const std::string& collection);
    ~LaserManager();
public:
    bool recommend(const laser::LaserRecommendParam& param, 
        GetDocumentsByIdsActionItem& actionItem,
        RawTextResultFromMIA& itemList) const;
    void index(const docid_t& docid, const std::string& title);

    MiningTask* getLaserIndexTask();

    const std::size_t getClustering(const std::string& title) const;
private:
    std::size_t assignClustering_(const TokenSparseVector& v) const;
    float similarity_(const TokenSparseVector& lv, const TokenVector& rv) const;

    void assignClusteringFunc_(ThreadContext* context);

    void load_();
    void close_();

    void initThreadPool_();
    void closeThreadPool_();

    friend class laser::LaserIndexTask;
private:
    const std::string workdir_;
    const std::string collection_;
    boost::shared_ptr<AdSearchService> adSearchService_;
    boost::scoped_ptr<laser::LaserRecommend> recommend_;
    boost::scoped_ptr<laser::AdIndexManager> indexManager_;
    boost::shared_ptr<laser::LaserIndexTask> indexTask_;

    std::vector<std::pair<boost::thread*, ThreadContext*> > thread_;
    
    static std::vector<TokenVector>* clusteringContainer_;
    static laser::Tokenizer* tokenizer_;
    static laser::LaserRpcServer* rpcServer_;
    static laser::TopNClusteringDB* topnClustering_;
    static laser::LaserOnlineModelDB* laserOnlineModel_;
    static boost::shared_mutex mutex_;
};

}

#endif
