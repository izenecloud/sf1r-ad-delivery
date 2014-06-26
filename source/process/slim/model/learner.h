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

    void add_feedback_title(const std::string & uuid, const std::string & title) {
        write_lock lock(_rw_mutex_title);

        std::pair<bool, int> uuid_ret = _uuid_mapper_title.insert(uuid);

        TokenSparseVector vec;
        _tok.tokenize(title, vec);

        int cluster_id = _assign_clustering(vec);

        // std::cout << _clustering_container.size() << std::endl;
        // std::cout << "uuid: " << uuid_ret.second << std::endl;
        // std::cout << "cluster_id: " << _assign_clustering(vec) << std::endl;

        if (_current_u_c_pairs.find(std::make_pair(uuid_ret.second, cluster_id))
            == _current_u_c_pairs.end()) {
            _current_u_c_pairs.insert(std::make_pair(uuid_ret.second, cluster_id));
            _coo_title.add(uuid_ret.second, cluster_id, 1.0);
        }
    }

    void add_feedback_tareid(const std::string & uuid, int tareid) {
        write_lock lock(_rw_mutex_tareid);

        std::pair<bool, int> uuid_ret = _uuid_mapper_tareid.insert(uuid);

        if (_current_u_t_pairs.find(std::make_pair(uuid_ret.second, tareid))
            == _current_u_t_pairs.end()) {
            _current_u_t_pairs.insert(std::make_pair(uuid_ret.second, tareid));
            _coo_tareid.add(uuid_ret.second, tareid, 1.0);
        }
    }

    void coo_to_csr() {
        coo_to_csr_title();
        coo_to_csr_tareid();
    }

    void coo_to_csr_title() {
        read_lock lock(_rw_mutex_title);

        _p_csr_X_title.reset(new csr_matrix(_coo_title));
    }

    void coo_to_csr_tareid() {
        read_lock lock(_rw_mutex_tareid);

        _p_csr_X_tareid.reset(new csr_matrix(_coo_tareid));
    }

    void train(int thread_num, int top_n) {
        _init_similar_cluster_size();
        _init_similar_tareid_size();

        slim_thread_manager slim_runner_title(thread_num, _model, *_p_csr_X_title, top_n, _similar_cluster, _similar_cluster_mutex);
        std::cout << "training similar cluster for csr matrix of size "<< _p_csr_X_title->m() << "\t" << _p_csr_X_title->n() << std::endl;
        slim_runner_title.start();
        slim_runner_title.join();

        slim_thread_manager slim_runner_tareid(thread_num, _model, *_p_csr_X_tareid, top_n, _similar_tareid, _similar_tareid_mutex);
        std::cout << "training similar tareid for csr matrix of size "<< _p_csr_X_tareid->m() << "\t" << _p_csr_X_tareid->n() << std::endl;
        slim_runner_tareid.start();
        slim_runner_tareid.join();

        msgpack::rpc::client cli("127.0.0.1", 38611);
        bool update_flag_title = cli.call("update_similar_cluster", _similar_cluster).get<bool>();
        std::cout << "update_similar_cluster called... " << update_flag_title << std::endl;
        bool update_flag_tareid = cli.call("update_similar_tareid", _similar_tareid).get<bool>();
        std::cout << "update_similar_tareid called... " << update_flag_tareid << std::endl;
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
        _similar_cluster.resize(_p_csr_X_title->n());
    }

    void _init_similar_tareid_size() {
        _similar_tareid.clear();
        _similar_tareid.resize(_p_csr_X_tareid->n());
    }

    boost::shared_mutex _rw_mutex_title;
    boost::shared_mutex _rw_mutex_tareid;

    boost::unordered_set<std::pair<int, int> > _current_u_c_pairs;
    boost::unordered_set<std::pair<int, int> > _current_u_t_pairs;
    coo_matrix _coo_title;
    coo_matrix _coo_tareid;
    id_mapper _uuid_mapper_title;
    id_mapper _uuid_mapper_tareid;

    boost::shared_ptr<csr_matrix> _p_csr_X_title;
    boost::shared_ptr<csr_matrix> _p_csr_X_tareid;

    slim_sgd _model;

    sf1r::laser::Tokenizer _tok;
    std::vector<TokenVector> _clustering_container;

    std::vector<std::vector<int> > _similar_cluster;
    boost::mutex _similar_cluster_mutex;

    std::vector<std::vector<int> > _similar_tareid;
    boost::mutex _similar_tareid_mutex;
};

}}

#endif
