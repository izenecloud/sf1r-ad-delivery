#ifndef SF1R_LASER_SPARSE_VECTOR_H
#define SF1R_LASER_SPARSE_VECTOR_H
#include <3rdparty/msgpack/msgpack.hpp>
#include <vector>
namespace sf1r { namespace laser {
class SparseVector
{
public:
    std::vector<int>& index()
    {
        return index_;
    }

    std::vector<float>& value()
    {
        return value_;
    }
    
    const std::vector<int>& index() const
    {
        return index_;
    }

    const std::vector<float>& value() const
    {
        return value_;
    }
    
    
    MSGPACK_DEFINE(index_, value_);
private:
    std::vector<int> index_;
    std::vector<float> value_;
};

} }
#endif
