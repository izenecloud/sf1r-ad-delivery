#ifndef SF1R_LASER_MANAGER_LASER_MANAGER_H
#define SF1R_LASER_MANAGER_LASER_MANAGER_H
#include <boost/shared_ptr.hpp>
#include <mining-manager/MiningManager.h>
#include <common/ResultType.h>

#include "LaserRpcServer.h"
#include "LaserRecommend.h"
#include "LaserRecommendParam.h"
#include "AdIndexManager.h"
#include "Tokenizer.h"
#include "clusteringmanager/type/TermDictionary.h"

namespace sf1r
{

class LaserManager
{
typedef boost::unordered_map<std::string, float> TokenVector;
public:
    LaserManager(const boost::shared_ptr<AdSearchService>& adSearchService);
    ~LaserManager();
public:
    bool recommend(const laser::LaserRecommendParam& param, 
        RawTextResultFromMIA& itemList) const;
    void index(const std::string& docid, const std::string& title);

private:
    std::size_t assignClustering_(const TokenVector& v) const;
    float similarity_(const TokenVector& lv, const TokenVector& rv) const;

    void load_();
    void close_();
private:
    const std::string workdir_;
    boost::shared_ptr<AdSearchService> adSearchService_;
    boost::scoped_ptr<laser::LaserRecommend> recommend_;
    boost::scoped_ptr<laser::AdIndexManager> indexManager_;
    
    static std::vector<boost::unordered_map<std::string, float> >* clusteringContainer_;
    static laser::Tokenizer* tokenizer_;
    static laser::LaserRpcServer* rpcServer_;
    static boost::shared_mutex mutex_;
};

}

#endif
