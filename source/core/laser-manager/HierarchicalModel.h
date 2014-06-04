#ifndef SF1R_LASER_HIERARCHICAL_MODEL_H
#define SF1R_LASER_HIERARCHICAL_MODEL_H
#include "LaserGenericModel.h"
namespace sf1r { namespace laser {
class HierarchicalModel : public LaserGenericModel
{
public:
    HierarchicalModel(const AdIndexManager& adIndexer, 
        const std::string& workdir,
        const std::size_t ncandidate);
    ~HierarchicalModel();
public:
    virtual bool candidate(
        const std::string& text,
        const std::vector<std::pair<int, float> >& context, 
        std::vector<std::pair<docid_t, std::vector<std::pair<int, float> > > >& ad,
        std::vector<float>& score) const;
private:
    const std::string workdir_;
    const std::size_t ncandidate_;
    LaserModelDB<std::size_t, LaserOnlineModel>* pClusteringDb_;
};
} }
#endif
