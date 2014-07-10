#ifndef SF1R_LASER_AD_CLUSTERING_INFO_H
#define SF1R_LASER_AD_CLUSTERING_INFO_H
#include <3rdparty/msgpack/msgpack.hpp>
#include "SparseVector.h"

namespace sf1r { namespace laser {
class AdClusteringsInfo
{
public:
    std::vector<SparseVector>& get()
    {
        return infos_;
    }

    MSGPACK_DEFINE(infos_);
private:
    std::vector<SparseVector> infos_;
};

} }
#endif
