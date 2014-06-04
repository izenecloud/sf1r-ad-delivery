#ifndef SF1R_LASER_GENERIC_MODEL_H
#define SF1R_LASER_GENERIC_MODEL_H
#include "LaserModel.h"
#include "LaserModelDB.h"
namespace sf1r { namespace laser {
class AdIndexManager;
class LaserOnlineModel;
class LaserOfflineModel;

class LaserGenericModel : public LaserModel
{
public:
    LaserGenericModel(const AdIndexManager& adIndexer,
        const std::string& workdir);
    ~LaserGenericModel();

public:
    virtual bool candidate(
        const std::string& text,
        const std::vector<std::pair<int, float> >& context, 
        std::vector<std::pair<docid_t, std::vector<std::pair<int, float> > > >& ad,
        std::vector<float>& score) const;
    virtual float score( 
        const std::string& text,
        const std::vector<std::pair<int, float> >& user, 
        const std::pair<docid_t, std::vector<std::pair<int, float> > >& ad,
        const float score) const;

    virtual void dispatch(const std::string& method, msgpack::rpc::request& req);

private:
    void updatepAdDb(msgpack::rpc::request& req);
    void updateOfflineModel(msgpack::rpc::request& req);
protected:
    const AdIndexManager& adIndexer_;
    const std::string workdir_;
    LaserModelDB<docid_t, LaserOnlineModel>* pAdDb_;
    LaserOfflineModel* offlineModel_;
};
} }
#endif
