#ifndef SF1R_LASER_MODEL_H
#define SF1R_LASER_MODEL_H
#include <vector>
#include <string>
#include <vector>
#include <common/inttypes.h>
#include <3rdparty/msgpack/msgpack.hpp>
#include <3rdparty/msgpack/rpc/server.h>
#include <smmintrin.h>

namespace sf1r { namespace laser {
class LaserModel
{
public:
    virtual ~LaserModel()
    {
    }
public:
    inline float dot(const std::vector<float>& model, 
        const std::vector<std::pair<int, float> >& args) const
    {
        std::vector<std::pair<int, float> >::const_iterator it = args.begin();
        float ret = 0.0;
        for (; it != args.end(); ++it)
        {
            ret += model[it->first] * it->second;
        }
        return ret;
    }
    
    inline float dot(const std::vector<float>& model, 
        const std::vector<float>& args) const
    {
        const float* mdata = model.data();
        const float* end = mdata + model.size();
        const float* adata = args.data();
        __m128 score = {0};
        for (; mdata < end - 4; mdata+=4, adata +=4)
        {
            __m128 mul = _mm_mul_ps(_mm_loadu_ps(mdata), _mm_loadu_ps(adata));
            score = _mm_add_ps(score, mul);
        }
        static float res[4];
        _mm_storeu_ps(res, score);
        float ret = res[0] + res[1] + res[2] + res[3];
        if (mdata < end) for (; mdata < end; ++mdata)
        {
            ret += (*mdata) * (*adata);
        }
        return ret;
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
    
    
    virtual bool candidate(
        const std::string& text,
        const std::size_t ncandidate,
        const std::vector<float>& context, 
        std::vector<std::pair<docid_t, std::vector<std::pair<int, float> > > >& ad,
        std::vector<float>& score) const = 0;
    
    virtual float score( 
        const std::string& text,
        const std::vector<float>& context, 
        const std::pair<docid_t, std::vector<std::pair<int, float> > >& ad,
        const float score) const = 0;

    virtual bool context(const std::string& text, std::vector<float>& context) const = 0;
    
    virtual void dispatch(const std::string& method, msgpack::rpc::request& req) = 0;

    virtual void updateAdDimension(const std::size_t adDimension)
    {
    }
};
} }
#endif
