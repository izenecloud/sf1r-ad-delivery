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
public:
    LaserGenericModel(const AdIndexManager& adIndexer,
        const std::string& kvaddr,
        const int kvport,
        const std::string& mqaddr,
        const int mqport,
        const std::string& workdir,
        const std::size_t maxDocid);
    ~LaserGenericModel();

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
    
    virtual bool context(const std::string& text, 
        std::vector<std::pair<int, float> >& context) const;

    virtual void dispatch(const std::string& method, msgpack::rpc::request& req);

    virtual void updateAdDimension(const std::size_t adDimension);
private:
    void updatepAdDb(msgpack::rpc::request& req);
    void updateOfflineModel(msgpack::rpc::request& req);
    void load();
    void save();
protected:
    const AdIndexManager& adIndexer_;
    const std::string workdir_;
    std::size_t adDimension_;
    std::vector<LaserOnlineModel>* pAdDb_;
    LaserOfflineModel* offlineModel_;
    context::KVClient* kvclient_;
    context::MQClient* mqclient_;
    long adIndex_;
};
} }
#endif
