#ifndef SLIM_THREAD_MANAGER_H_
#define SLIM_THREAD_MANAGER_H_

#include "slim_sgd.h"
#include "csr_matrix.h"
#include "weight_vector.h"

#include <boost/thread/thread.hpp>

namespace slim { namespace model {

class slim_thread {
public:
    slim_thread(int start_column,
                int end_column,
                slim_sgd & model,
                csr_matrix & X)
        : start_column(start_column),
          end_column(end_column),
          model(model),
          X(X)
    {
    }

    void operator()()
    {
        for (int i = start_column; i != end_column; ++i) {
            model.fit(X, i, w, intercept);
        }
        std::cout << start_column << "->" << end_column << " finished!" << std::endl;
    }

    int start_column;
    int end_column;
    slim_sgd & model;
    csr_matrix & X;

    weight_vector w;
    double intercept;
};

class slim_thread_manager {
public:
    slim_thread_manager(int thread_num,
                        slim_sgd & model,
                        csr_matrix & X)
        : thread_num(thread_num),
          model(model),
          X(X)
    {
    }

    void start() {
        if (X.n() < thread_num) {
            threads.create_thread(slim_thread(0,
                                              X.n(),
                                              model,
                                              X));
        } else {
            int average_task_num = X.n() / thread_num;
            int extra_num = X.n() % thread_num;

            int start = 0;
            threads.create_thread(slim_thread(start,
                                              start + average_task_num + extra_num,
                                              model,
                                              X));
            start += average_task_num + extra_num;
            while (start != X.n()) {
                threads.create_thread(slim_thread(start,
                                                  start + average_task_num,
                                                  model,
                                                  X));
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

    boost::thread_group threads;
};

}}

#endif
