#ifndef SLIM_THREAD_MANAGER_H_
#define SLIM_THREAD_MANAGER_H_

#include "slim_sgd.h"
#include "csr_matrix.h"
#include "weight_vector.h"

#include <vector>
#include <queue>

#include <boost/thread/thread.hpp>

namespace slim { namespace model {

class less_than {
public:
    bool operator()(const std::pair<double, int> & lhs, const std::pair<double, int> & rhs)
    {
        return lhs.first > rhs.first;
    }
};

class slim_thread {
public:
    slim_thread(int start_column,
                int end_column,
                slim_sgd & model,
                csr_matrix & X,
                int top_n,
                std::vector<std::vector<int> > & similar_cluster,
                boost::mutex & similar_cluster_mutex)
        : start_column(start_column),
          end_column(end_column),
          model(model),
          X(X),
          top_n(top_n),
          _similar_cluster(similar_cluster),
          _similar_cluster_mutex(similar_cluster_mutex)
    {
    }

    void operator()()
    {
        for (int i = start_column; i != end_column; ++i) {
            model.fit(X, i, w, intercept);
            _find_top_n(w._w, i);
        }
        std::cout << start_column << "->" << end_column << " finished!" << std::endl;
    }

    void _find_top_n(std::vector<double> & v, int col)
    {
        std::priority_queue<std::pair<double, int>, std::vector<std::pair<double, int> >, less_than> pq;
        for (int i = 0; i != (int)v.size(); ++i) {
            if (v[i] == 0.0) {
                continue;
            }

            if ((int)pq.size() < top_n) {
                pq.push(std::make_pair(v[i], i));
            } else if (v[i] > pq.top().first) {
                pq.pop();
                pq.push(std::make_pair(v[i], i));
            }
        }

        _similar_cluster_mutex.lock();

        int top_size = pq.size();
        _similar_cluster[col].resize(top_size);
        while (!pq.empty()) {
            _similar_cluster[col][--top_size] = pq.top().second;
            pq.pop();
        }

        _similar_cluster_mutex.unlock();
    }

    int start_column;
    int end_column;
    slim_sgd & model;
    csr_matrix & X;
    int top_n;

    weight_vector w;
    double intercept;

    std::vector<std::vector<int> > & _similar_cluster;
    boost::mutex & _similar_cluster_mutex;
};

class slim_thread_manager {
public:
    slim_thread_manager(int thread_num,
                        slim_sgd & model,
                        csr_matrix & X,
                        int top_n,
                        std::vector<std::vector<int> > & similar_cluster,
                        boost::mutex & similar_cluster_mutex)
        : thread_num(thread_num),
          model(model),
          X(X),
          top_n(top_n),
          _similar_cluster(similar_cluster),
          _similar_cluster_mutex(similar_cluster_mutex)
    {
    }

    void start() {
        if (X.n() < thread_num) {
            threads.create_thread(slim_thread(0,
                                              X.n(),
                                              model,
                                              X,
                                              top_n,
                                              _similar_cluster,
                                              _similar_cluster_mutex));
        } else {
            int average_task_num = X.n() / thread_num;
            int extra_num = X.n() % thread_num;

            int start = 0;
            threads.create_thread(slim_thread(start,
                                              start + average_task_num + extra_num,
                                              model,
                                              X,
                                              top_n,
                                              _similar_cluster,
                                              _similar_cluster_mutex));
            start += average_task_num + extra_num;
            while (start != X.n()) {
                threads.create_thread(slim_thread(start,
                                                  start + average_task_num,
                                                  model,
                                                  X,
                                                  top_n,
                                                  _similar_cluster,
                                                  _similar_cluster_mutex));
                start += average_task_num;
            }
        }
    }

    void join() {
        threads.join_all();
    }

    int thread_num;
    slim_sgd & model;
    csr_matrix & X;
    int top_n;

    boost::thread_group threads;

    std::vector<std::vector<int> > & _similar_cluster;
    boost::mutex & _similar_cluster_mutex;
};

}}

#endif
