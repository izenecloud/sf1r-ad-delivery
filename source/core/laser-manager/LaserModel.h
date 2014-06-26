#ifndef SF1R_LASER_MODEL_H
#define SF1R_LASER_MODEL_H
#include <vector>
#include <string>
#include <vector>
#include <common/inttypes.h>
#include <3rdparty/msgpack/msgpack.hpp>
#include <3rdparty/msgpack/rpc/server.h>

namespace sf1r { namespace laser {
class LaserModel
{
public:
    virtual ~LaserModel()
    {
    }
public:
    float dot(const std::vector<float>& model, 
        const std::vector<std::pair<int, float> >& args) const
    {
        float score = 0.0;
        for (std::size_t i = 0; i < args.size(); ++i)
        {
            score += model[args[i].first] * args[i].second;
        }
        return score;
    }
    
    virtual bool candidate(
        const std::string& text,
        const std::size_t ncandidate,
        const std::vector<std::pair<int, float> >& context, 
        std::vector<std::pair<docid_t, std::vector<std::pair<int, float> > > >& ad,
        std::vector<float>& score) const = 0;
    
    virtual float score( 
        const std::string& text,
        const std::vector<std::pair<int, float> >& context, 
        const std::pair<docid_t, std::vector<std::pair<int, float> > >& ad,
        const float score) const = 0;

    virtual bool context(const std::string& text, std::vector<std::pair<int, float> >& context) const = 0;
    virtual void dispatch(const std::string& method, msgpack::rpc::request& req) = 0;
};
} }
#endif
