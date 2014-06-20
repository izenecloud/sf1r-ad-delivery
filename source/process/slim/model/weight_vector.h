#ifndef WEIGHT_VECTOR_H_
#define WEIGHT_VECTOR_H_

#include <vector>
#include <cmath>

#include "csr_matrix.h"

namespace slim { namespace model {

class weight_vector {
public:
    weight_vector()
        : _w_scale(1.0)
    {
    }

    void clear(int size) {
        _w.clear();
        _w.resize(size, 0.0);
    }

    void add(csr_matrix & X, int row, double c) {
        for (int i = X._indptr[row]; i != X._indptr[row + 1]; ++i) {
            int idx = X._indices[i];
            double val = X._data[i];
            _w[idx] += val * (c / _w_scale);
        }
    }

    double dot(csr_matrix & X, int row) {
        double innerprod = 0.0;
        for (int i = X._indptr[row]; i != X._indptr[row + 1]; ++i) {
            int idx = X._indices[i];
            double val = X._data[i];
            innerprod += _w[idx] * val;
        }
        innerprod *= _w_scale;
        return innerprod;
    }

    void scale(double c) {
        _w_scale *= c;
        if (_w_scale <= 1e-9) {
            reset_scale();
        }
    }

    void reset_scale() {
        for (int i = 0; i != (int)_w.size(); ++i) {
            _w[i] *= _w_scale;
        }
        _w_scale = 1.0;
    }

    std::vector<double> _w;
    double _w_scale;
};

}}

#endif
