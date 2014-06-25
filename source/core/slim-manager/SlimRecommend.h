#ifndef SF1R_SLIM_RECOMMEND_H
#define SF1R_SLIM_RECOMMEND_H

#include <string>
#include <vector>
#include <common/inttypes.h>
#include <laser-manager/LaserManager.h>
#include <laser-manager/AdIndexManager.h>
#include <util/functional.h>
#include <common/inttypes.h>
#include <queue>

namespace sf1r { namespace slim {

class SlimRecommend {
    typedef std::pair<docid_t, std::vector<std::pair<int, float> > > AD;
    typedef std::vector<AD> ADVector;

    typedef izenelib::util::second_greater<std::pair<docid_t, float> > greater_than;
    typedef std::priority_queue<std::pair<docid_t, float>, std::vector<std::pair<docid_t, float> >, greater_than> priority_queue;

public:
    SlimRecommend(laser::AdIndexManager* index,
                  laser::Tokenizer* tok,
                  std::vector<std::vector<int> > & similar_cluster,
                  boost::shared_mutex & rw_mutex,
                  LaserManager * laser)
        : indexManager_(index),
          tokenizer_(tok),
          _similar_cluster(similar_cluster),
          _rw_mutex(rw_mutex),
          laser_(laser)
    {
    }

    bool recommend(const std::string & title,
                   int id,
                   int topn,
                   std::vector<docid_t> & itemList);

    void topN(boost::unordered_map<int, float> & title_vec,
              ADVector & advec,
              int topn,
              priority_queue & queue);

private:
    laser::AdIndexManager* indexManager_;
    laser::Tokenizer* tokenizer_;
    std::vector<std::vector<int> > & _similar_cluster;

    boost::shared_mutex & _rw_mutex;

    LaserManager * laser_;
};

}}

#endif
