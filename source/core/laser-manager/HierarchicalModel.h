#ifndef SF1R_LASER_HIERARCHICAL_MODEL_H
#define SF1R_LASER_HIERARCHICAL_MODEL_H
#include "LaserGenericModel.h"
namespace sf1r { namespace laser {
class HierarchicalModel : public LaserGenericModel
{
public:
    HierarchicalModel(const AdIndexManager& adIndexer, 
        const std::string& kvaddr,
        const int kvport,
        const std::string& mqaddr,
        const int mqport,
        const std::string& workdir,
        const std::string& sysdir,
        const std::size_t adDimension,
        const std::size_t clusteringDimension,
        const std::size_t AD_FD,
        const std::size_t USER_FD);
    ~HierarchicalModel();
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
    
    virtual void dispatch(const std::string& method, msgpack::rpc::request& req);

private:
    void updatepClusteringDb(msgpack::rpc::request& req);
    void save();
    void load();
    void saveOrigModel();
    void localizeFromOrigModel();
private:
    const std::string workdir_;
    const std::string sysdir_;
    const std::size_t clusteringDimension_;
    std::vector<LaserOnlineModel>* pClusteringDb_;
};
} }
#endif
