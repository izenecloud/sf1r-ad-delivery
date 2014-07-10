#ifndef SF1R_LASER_GENERIC_MODEL_H
#define SF1R_LASER_GENERIC_MODEL_H
#include "LaserModel.h"
#include "LaserModelDB.h"
namespace sf1r { namespace laser {
class AdIndexManager;
class LaserOnlineModel;
class LaserOfflineModel;

template <typename V> class LaserModelContainer;

namespace context {
class KVClient;
class MQClient;
}

class LaserGenericModel : public LaserModel
{
typedef LaserModelDB<std::string, LaserOnlineModel> OrigOnlineDB;
public:
    LaserGenericModel(const AdIndexManager& adIndexer,
        const std::string& kvaddr,
        const int kvport,
        const std::string& mqaddr,
        const int mqport,
        const std::string& workdir,
        const std::string& sysdir,
        const std::size_t maxDocid);
    ~LaserGenericModel();

public:
    virtual bool candidate(
        const std::string& text,
        const std::size_t ncandidate,
        const std::vector<std::pair<int, float> >& context, 
        std::vector<std::pair<docid_t, std::vector<std::pair<int, float> > > >& ad,
        std::vector<float>& score) const;
    
    virtual bool candidate(
        const std::string& text,
        const std::size_t ncandidate,
        const std::vector<float>& context, 
        std::vector<std::pair<docid_t, std::vector<std::pair<int, float> > > >& ad,
        std::vector<float>& score) const;
    
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
    
    virtual bool context(const std::string& text, 
        std::vector<std::pair<int, float> >& context) const;

    virtual bool context(const std::string& text, 
        std::vector<float>& context) const;

    virtual void dispatch(const std::string& method, msgpack::rpc::request& req);

    virtual void updateAdDimension(const std::size_t adDimension);

private:
    void updatepAdDb(msgpack::rpc::request& req);
    void load();
    void save();
    void localizeFromOrigModel();
protected:
    const AdIndexManager& adIndexer_;
    const std::string workdir_;
    const std::string sysdir_;
    std::size_t adDimension_;
    std::vector<LaserOnlineModel>* pAdDb_;
    LaserOfflineModel* offlineModel_;
    context::KVClient* kvclient_;
    context::MQClient* mqclient_;
    long adIndex_;
    // for support rebuild operation
    OrigOnlineDB* origLaserModel_;
};
} }
#endif
