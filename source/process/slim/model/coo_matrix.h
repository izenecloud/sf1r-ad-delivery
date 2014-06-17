#ifndef COO_MATRIX_H_
#define COO_MATRIX_H_

#include <vector>
#include <string>
#include <iostream>
#include <fstream>

namespace slim { namespace model {

class coo_matrix {
public:
    coo_matrix()
        : _M(0),
          _N(0),
          _size(0)
    {
    }

    void add(int i, int j, double v) {
        _row.push_back(i);
        _col.push_back(j);
        _data.push_back(v);

        if (i > _M - 1) {
            _M = i + 1;
        }

        if (j > _N - 1) {
            _N = j + 1;
        }

        ++_size;
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

    std::vector<int> _row;
    std::vector<int> _col;
    std::vector<double> _data;

    int _M;
    int _N;

    int _size;
};

}}

#endif
