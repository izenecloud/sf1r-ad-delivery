#ifndef CSR_MATRIX_H_
#define CSR_MATRIX_H_

#include <vector>
#include <iostream>
#include <stdexcept>
#include <algorithm>

#include "coo_matrix.h"

namespace slim { namespace model {

class csr_matrix {
public:
    csr_matrix(const coo_matrix & sp) {
        // temporarily do not support empty matrix transformation
        if (sp._size == 0) {
            throw std::runtime_error("empty matrix transformation");
        }

        _M = sp._M;
        _N = sp._N;

        _size = sp._size;

        _indptr.resize(_M + 1);
        _indices.resize(_size);
        _data.resize(_size);

        coo_tocsr(sp);
    }

    void coo_tocsr(const coo_matrix & sp) {
        std::fill(_indptr.begin(), _indptr.begin() + _M, 0);

        // compute number of non-zero entries per row of sp
        for (int i = 0; i != _size; ++i) {
            _indptr[sp._row[i]] += 1;
        }

        // cumsum the nnz per row to get indptr
        for (int i = 0, cumsum = 0; i != _M; ++i) {
            int temp = _indptr[i];
            _indptr[i] = cumsum;
            cumsum += temp;
        }
        _indptr[_M] = _size;

        // write col, data into indices, data
        for (int i = 0; i != _size; ++i) {
            int row = sp._row[i];
            int dest = _indptr[row];

            _indices[dest] = sp._col[i];
            _data[dest] = sp._data[i];

            _indptr[row] += 1;
        }

        for (int i = 0, last = 0; i <= _M; ++i) {
            int temp = _indptr[i];
            _indptr[i] = last;
            last = temp;
        }
    }

    int m() {
        return _M;
    }

    int n() {
        return _N;
    }

    int size() {
        return _size;
    }

    std::vector<double> _data;
    std::vector<int> _indices;
    std::vector<int> _indptr;

    int _M;
    int _N;

    int _size;
};

}}

#endif
