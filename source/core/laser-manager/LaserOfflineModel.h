#ifndef LASER_OFFLINE_MODEL_H
#define LASER_OFFLINE_MODEL_H
#include "LaserModel.h"
#include "LaserModelDB.h"

namespace sf1r { namespace laser {
class AdIndexManager;
class LaserOfflineModel : public LaserModel 
{
typedef LaserModelDB<std::string, float> OrigBetaStableDB;
typedef LaserModelDB<std::string, std::vector<float> > OrigConjunctionStableDB;
public:
    LaserOfflineModel(const AdIndexManager& adIndexer,
        const std::string& filename, 
        const std::string& sysdir,
        const std::size_t adDimension,
        const std::size_t AD_FD,
        const std::size_t USER_FD);
    ~LaserOfflineModel();
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
    
    virtual void updateAdDimension(const std::size_t adDimension);
    
    virtual void dispatch(const std::string& method, msgpack::rpc::request& req);

    virtual bool context(const std::string& text, std::vector<std::pair<int, float> >& context) const
    {
        return false;
    }
    
    virtual bool context(const std::string& text, std::vector<float>& context) const
    {
        return false;
    }
    
    
    void setAlpha(std::vector<float>& alpha)
    {
        alpha_->swap(alpha);
    }

    void setBeta(std::vector<float>& beta)
    {
        beta_->swap(beta);
    }
    
    void setConjunction(std::vector<std::vector<float> >& quadratic)
    {
        conjunction_->swap(quadratic);
    }
    
    void save();
    void load();

    void precompute(std::size_t startId, std::size_t endId, int threadId);

    const std::vector<float>* alpha() const
    {
        return alpha_;
    }

    const std::vector<float>* betaStable() const
    {
        return betaStable_;
    }

    const std::vector<std::vector<float> >* conjunctionStable() const
    {
        return conjunctionStable_;
    }
private:
    void saveOrigModel();
    void localizeFromOrigModel();
private:
    const AdIndexManager& adIndexer_;
    const std::string filename_;
    const std::string sysdir_;
    std::size_t adDimension_;
    const std::size_t AD_FD_;
    const std::size_t USER_FD_;
    std::vector<float>* alpha_;
    std::vector<float>* beta_;
    std::vector<float>* betaStable_;
    std::vector<std::vector<float> >* conjunction_;
    std::vector<std::vector<float> >* conjunctionStable_;
    boost::thread_group* threadGroup_;
    OrigBetaStableDB* origBetaStable_;
    OrigConjunctionStableDB* origConjunctionStable_;
    const static int THREAD_NUM = 4;
};
} }
#endif
