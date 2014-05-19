#ifndef SF1R_LASER_TOPN_CLUSTERING_H
#define SF1R_LASER_TOPN_CLUSTERING_H
#include <string>
#include <vector>
#include <map>
#include <boost/serialization/access.hpp>
#include <3rdparty/msgpack/msgpack.hpp>
namespace sf1r { namespace laser {

class TopNClustering
{
public:
    void userName(std::string un)
    {
        user_ = un;
    }
    
    const std::string& userName() const
    {
        return user_;
    }

    void set(const std::map<int, float>& clusters)
    {
        clustering_ = clusters;
    }

    const std::map<int, float>& get() const
    {
        return clustering_;
    }

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & clustering_;
    }

    MSGPACK_DEFINE(user_, clustering_);
private:
    std::string user_;
    std::map<int, float> clustering_;
};
} }

#endif
