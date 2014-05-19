#ifndef SF1R_LASER_ONLINE_MODEL_H
#define SF1R_LASER_ONLINE_MODEL_H
#include <3rdparty/msgpack/msgpack.hpp>
#include <vector>
#include <string>
#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>
namespace sf1r { namespace laser {
class LaserOnlineModel
{
public:
    LaserOnlineModel()
    {
    }

    LaserOnlineModel(const std::vector<float>& args)
    : args_(args)
    {
    }

public:
    void userName(const std::string& un)
    {
        user_ = un;
    }
    
    const std::string& userName() const
    {
        return user_;
    }
    
    void set(const std::vector<float>& args)
    {
        args_ = args;
    }

    const std::vector<float>& get() const
    {
        return args_;
    }

    MSGPACK_DEFINE(user_, args_);
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & args_;
    }
private:
    std::vector<float> args_;
    std::string user_;
};
} }
#endif
