#ifndef LEARNER_H_
#define LEARNER_H_

#include "coo_matrix.h"
#include "csr_matrix.h"
#include "id_mapper.h"
#include "slim_sgd.h"
#include "slim_thread_manager.h"

#include <string>
#include <utility>
#include <vector>
#include <fstream>

#include <boost/shared_ptr.hpp>

#include <laser-manager/Tokenizer.h>

#include <boost/serialization/string.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/archive/text_iarchive.hpp>

#include <3rdparty/msgpack/rpc/client.h>

namespace slim { namespace model {

class learner {
public:
    typedef boost::shared_lock<boost::shared_mutex> read_lock;
    typedef boost::unique_lock<boost::shared_mutex> write_lock;

    typedef std::vector<float> TokenVector;
    typedef std::vector<std::pair<int, float> > TokenSparseVector;

    learner(const std::string & penalty_type)
        : _model(penalty_type),
          _tok("../package/resource/dict/title_pca/",
               "../package/resource/laser_resource/terms_dic.dat")
    {
        std::vector<boost::unordered_map<std::string, float> > clustering_container;

        {
            std::ifstream ifs("../package/resource/laser_resource/clustering_result");
            boost::archive::text_iarchive ia(ifs);
            ia >> clustering_container;
        }

        for (int i = 0; i != (int)clustering_container.size(); ++i) {
            TokenVector vec;
            _tok.numeric(clustering_container[i], vec);
            _clustering_container.push_back(vec);
        }
    }

    void add_feedback(const std::string & uuid, const std::string & title) {
        write_lock lock(_rw_mutex);

        std::pair<bool, int> uuid_ret = _uuid_mapper.insert(uuid);

        TokenSparseVector vec;
        _tok.tokenize(title, vec);

        int cluster_id = _assign_clustering(vec);

        // std::cout << _clustering_container.size() << std::endl;
        // std::cout << "uuid: " << uuid_ret.second << std::endl;
        // std::cout << "cluster_id: " << _assign_clustering(vec) << std::endl;

        if (_current_u_c_pairs.find(std::make_pair(uuid_ret.second, cluster_id))
            == _current_u_c_pairs.end()) {
            _current_u_c_pairs.insert(std::make_pair(uuid_ret.second, cluster_id));
            _coo.add(uuid_ret.second, cluster_id, 1.0);
        }
    }

    void coo_to_csr() {
        read_lock lock(_rw_mutex);

        _p_csr_X.reset(new csr_matrix(_coo));
    }

    void train(int thread_num, int top_n) {
        _init_similar_cluster_size();

        slim_thread_manager slim_runner(thread_num, _model, *_p_csr_X, top_n, _similar_cluster, _similar_cluster_mutex);
        std::cout << _p_csr_X->m() << "\t" << _p_csr_X->n() << std::endl;
        slim_runner.start();
        slim_runner.join();

        msgpack::rpc::client cli("127.0.0.1", 38611);
        bool update_flag = cli.call("update_similar_cluster", _similar_cluster).get<bool>();
        std::cout << "update_similar_cluster called..." << std::endl;
    }

    int _assign_clustering(const TokenSparseVector & v) {
        if (v.empty()) {
            return _clustering_container.size();
        }

        double max_sim = 0.0;
        int max_id = -1;
        for (int i = 0; i != (int)_clustering_container.size(); ++i) {
            double sim = _similarity(v, _clustering_container[i]);
            if (sim > max_sim) {
                max_sim = sim;
                max_id = i;
            }
        }
        return max_id;
    }

    double _similarity(const TokenSparseVector & lv, const TokenVector & rv) {
        double sim = 0.0;

        for (int i = 0; i != (int)lv.size(); ++i) {
            sim += rv[lv[i].first] * lv[i].second;
        }

        return sim;
    }

    void _init_similar_cluster_size() {
        _similar_cluster.clear();
        _similar_cluster.resize(_p_csr_X->n());
    }

    boost::shared_mutex _rw_mutex;

    boost::unordered_set<std::pair<int, int> > _current_u_c_pairs;
    coo_matrix _coo;
    id_mapper _uuid_mapper;

    boost::shared_ptr<csr_matrix> _p_csr_X;

    slim_sgd _model;

    sf1r::laser::Tokenizer _tok;
    std::vector<TokenVector> _clustering_container;

    std::vector<std::vector<int> > _similar_cluster;
    boost::mutex _similar_cluster_mutex;
};

}}

#endif
