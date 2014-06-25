#include "SlimRecommend.h"

#include <laser-manager/Tokenizer.h>

namespace sf1r { namespace slim {

bool SlimRecommend::recommend(const std::string & title,
                              int id,
                              int topn,
                              std::vector<docid_t> & itemList)
{
    boost::shared_lock<boost::shared_mutex> lock(_rw_mutex);

    if (!title.empty()) {
        int clusterID = laser_->getClustering(title);

        if (clusterID == -1) {
            clusterID = _similar_cluster.size() - 1;
        }

        std::vector<int> clusters;
        if (clusterID != (int)_similar_cluster.size() - 1) {
            clusters.push_back(clusterID);
        }

        for (int i = 0; i != (int)_similar_cluster[clusterID].size(); ++i) {
            if (_similar_cluster[clusterID][i] != (int)_similar_cluster.size() - 1) {
                clusters.push_back(_similar_cluster[clusterID][i]);
            }
        }

        if (clusters.empty()) {
            return false;
        }

        // clusters is ready and not empty...
        boost::unordered_map<int, float> vec;
        tokenizer_->tokenize(title, vec);

        priority_queue queue;

        for (int i = 0; i != (int)clusters.size(); ++i) {
            ADVector advec;
            if (indexManager_->get(clusters[i], advec)) {
                topN(vec, advec, topn, queue);
            }
        }

        while (!queue.empty()) {
            itemList.push_back(queue.top().first);
            queue.pop();
        }

        return true;
    } else if (id != -1) {

    } else {
        // error
    }

    // for (int i = 0; i != (int)_similar_cluster.size(); ++i) {
    //     if (!_similar_cluster[i].empty()) {
    //         std::cout << i << std::endl;
    //     }
    // }

    return true;
}

void SlimRecommend::topN(boost::unordered_map<int, float> & title_vec,
                         ADVector & advec,
                         int topn,
                         priority_queue & queue)
{
    for (int i = 0; i != (int)advec.size(); ++i) {
        AD & ad = advec[i];
        docid_t & docid = ad.first;
        std::vector<std::pair<int, float> > & vec = ad.second;

        float score = 0.0;
        for (int i = 0; i != (int)vec.size(); ++i) {
            boost::unordered_map<int, float>::iterator p = title_vec.find(vec[i].first);
            if (p != title_vec.end()) {
                score += p->second * vec[i].second;
            }
        }

        if ((int)queue.size() >= topn) {
            if (score > queue.top().second) {
                queue.pop();
                queue.push(std::make_pair(docid, score));
            }
        } else {
            queue.push(std::make_pair(docid, score));
        }
    }
}

}}
