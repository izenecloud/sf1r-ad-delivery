#ifndef MSGPACK_LASER_ONLINE_MODEL_H
#define MSGPACK_LASER_ONLINE_MODEL_H
#include <3rdparty/msgpack/msgpack.hpp>
#include <vector>
#include <string>
#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>
namespace predict
{
    class PerUserOnlineModel
    {
    public:
        PerUserOnlineModel()
        {
        }

        PerUserOnlineModel(const std::vector<float>& args)
            : args_(args)
        {
        }
        PerUserOnlineModel(const PerUserOnlineModel& per)
        {
            args_ = per.args_;
        }
        void setUserName(std::string un)
        {
            user_ = un;
        }
        void setArgs(const std::vector<float>& args)
        {
            args_ = args;
        }

        const std::string& getUserName() const
        {
            return user_;
        }

    public:
        const std::vector<float>& getArgs() const
        {
            return args_;
        }
        MSGPACK_DEFINE(user_, args_);
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
//            ar & user_;
            ar & args_;
        }
    private:
        std::vector<float> args_;
        std::string user_;

   };
}
#endif
