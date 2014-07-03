#ifndef LASER_OFFLINE_MODEL_H
#define LASER_OFFLINE_MODEL_H

#include "LaserModel.h"
namespace sf1r { namespace laser {
class LaserOfflineModel : public LaserModel 
{
public:
    LaserOfflineModel(const std::string& filename, 
        const std::size_t adDimension);
    ~LaserOfflineModel();
public:
    virtual bool candidate(
        const std::string& text,
        const std::size_t ncandidate,
        const std::vector<std::pair<int, float> >& context, 
        std::vector<std::pair<docid_t, std::vector<std::pair<int, float> > > >& ad,
        std::vector<float>& score) const;
    virtual float score( 
        const std::string& text,
        const std::vector<std::pair<int, float> >& user, 
        const std::pair<docid_t, std::vector<std::pair<int, float> > >& ad,
        const float score) const;
    
    virtual void updateAdDimension(const std::size_t adDimension)
    {
        adDimension_ = adDimension;
        betaStable_->resize(adDimension_);
        quadraticStable_->resize(adDimension_);
    }
    
    
    virtual void dispatch(const std::string& method, msgpack::rpc::request& req)
    {
    }

    virtual bool context(const std::string& text, std::vector<std::pair<int, float> >& context) const
    {
        return true;
    }
    
    void setAlpha(std::vector<float>& alpha)
    {
        alpha_->swap(alpha);
    }

    void setBeta(std::vector<float>& beta)
    {
        beta_->swap(beta);
    }
    
    void setQuadratic(std::vector<std::vector<float> >& quadratic)
    {
        quadratic_->swap(quadratic);
    }
    
    void save();
    void load();
private:
    const std::string filename_;
    std::size_t adDimension_;
    std::vector<float>* alpha_;
    std::vector<float>* beta_;
    std::vector<float>* betaStable_;
    std::vector<std::vector<float> >* quadratic_;
    std::vector<std::vector<float> >* quadraticStable_;
};
} }
#endif
