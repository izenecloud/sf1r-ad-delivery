#ifndef LASER_ONLINE_MODEL_H
#define LASER_ONLINE_MODEL_H
#include "LaserModel.h"
#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>
#include <3rdparty/msgpack/msgpack.hpp>
namespace sf1r { namespace laser {

class LaserOnlineModel : public LaserModel 
{
public:
    LaserOnlineModel()
    {
    }

    LaserOnlineModel(float delta, const std::vector<float>& eta)
        : delta_(delta)
        , eta_(eta)
    {
    }
    MSGPACK_DEFINE(delta_, eta_);

public:
    virtual bool candidate(
        const std::string& text,
        const std::size_t ncandidate,
        const std::vector<std::pair<int, float> >& context, 
        std::vector<std::pair<docid_t, std::vector<std::pair<int, float> > > >& ad,
        std::vector<float>& score) const
    {
        return false;
    }

    virtual bool candidate(
        const std::string& text,
        const std::size_t ncandidate,
        const std::vector<float>& context, 
        std::vector<std::pair<docid_t, std::vector<std::pair<int, float> > > >& ad,
        std::vector<float>& score) const
    {
        return false;
    }

    virtual float score( 
        const std::string& text,
        const std::vector<std::pair<int, float> >& user, 
        const std::pair<docid_t, std::vector<std::pair<int, float> > >& ad,
        const float score) const;
    
    virtual float score( 
        const std::string& text,
        const std::vector<float>& user, 
        const std::pair<docid_t, std::vector<std::pair<int, float> > >& ad,
        const float score) const;
    
    virtual void dispatch(const std::string& method, msgpack::rpc::request& req)
    {
    }
    
    virtual bool context(const std::string& text, std::vector<std::pair<int, float> >& context) const
    {
        return false;
    }
        
    virtual bool context(const std::string& text, std::vector<float>& context) const
    {
        return false;
    }
        
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & delta_;
        ar & eta_;
    }
    
    const std::vector<float>& eta() const
    {
        return eta_;
    }

private:
    float delta_;
    std::vector<float> eta_;
};

} }

#endif
