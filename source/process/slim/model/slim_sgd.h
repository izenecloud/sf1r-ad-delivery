#ifndef SLIM_SGD_H_
#define SLIM_SGD_H_

#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

#include "csr_matrix.h"
#include "weight_vector.h"

namespace slim { namespace model {

class slim_sgd {
public:
    slim_sgd(const std::string & penalty_type,
             bool fit_intercept = true,
             double alpha = 0.0001,
             double l1_ratio = 0.15,
             double eta0 = 0.01,
             double power_t = 0.25)
        : _penalty_type(penalty_type),
          _fit_intercept(fit_intercept),
          _alpha(alpha),
          _l1_ratio(l1_ratio),
          _eta0(eta0),
          _power_t(power_t)
    {
        if (penalty_type == "l1") {
            _l1_ratio = 1.0;
        } else if (penalty_type == "l2") {
            _l1_ratio = 0.0;
        }
    }

    void fit(csr_matrix & X, int column, weight_vector & w, double & intercept) {
        w.clear(X.n());
        intercept = 0.0;

        double t = 1.0;
        double eta = _eta0;
        double u = 0.0;
        double p;
        double update;

        std::vector<double> q(X.n(), 0.0);
        std::vector<double> y(X.m(), 0.0);
        get_y(X, y, column);

        // scanning 1M instances before convergence
        _n_iter = 1000000 / X.m() + 1;
        for (int i = 0; i != _n_iter; ++i) {
            for (int j = 0; j != X.m(); ++j) {
                eta = _eta0 / pow(t, _power_t);

                p = w.dot(X, j) + intercept;
                update = -eta * dloss(p, y[j]);

                if (_penalty_type == "l2" || _penalty_type == "elasticnet") {
                    w.scale(1.0 - ((1.0 - _l1_ratio) * eta * _alpha));
                }

                if (update != 0.0) {
                    w.add(X, j, update);
                    w._w[column] = 0.0;
                    if (_fit_intercept) {
                        // sparse_intercept_decay == 0.01
                        intercept += update * 0.01;
                    }
                }

                if (_penalty_type == "l1" || _penalty_type == "elasticnet") {
                    u += (_l1_ratio * eta * _alpha);
                    l1penalty(w, q, X, j, u);
                }

                t += 1.0;
            }
        }

        w.reset_scale();
    }

    void get_y(csr_matrix & X, std::vector<double> & y, int column) {
        for (int i = 0; i != X.m(); ++i) {
            for (int j = X._indptr[i]; j != X._indptr[i + 1]; ++j) {
                if (X._indices[j] == column) {
                    y[i] = X._data[j];
                    break;
                }
            }
        }
    }

    double loss(double p, double y) {
        return 0.5 * (p - y) * (p - y);
    }

    double dloss(double p, double y) {
        return p - y;
    }

    void l1penalty(weight_vector & w, std::vector<double> & q, csr_matrix & X, int row, double u) {
        for (int i = X._indptr[row]; i != X._indptr[row + 1]; ++i) {
            int col = X._indices[i];
            double & w_col = w._w[col];
            double z = w_col;

            if (w._w_scale * w_col > 0.0) {
                w_col = std::max(0.0, w_col - (u + q[col]) / w._w_scale);
            } else if (w._w_scale * w_col < 0.0) {
                w_col = std::min(0.0, w_col + (u - q[col]) / w._w_scale);
            }

            q[col] += w._w_scale * (w_col - z);
        }
    }

    std::string _penalty_type;
    bool _fit_intercept;
    double _alpha;
    double _l1_ratio;
    int _n_iter;
    double _eta0;
    double _power_t;
};

}}

#endif
