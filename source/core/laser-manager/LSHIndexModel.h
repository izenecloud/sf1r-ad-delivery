#ifndef SF1R_LASER_LSH_INDEX_MODEL_H
#define SF1R_LASER_LSH_INDEX_MODEL_H
#include "LaserGenericModel.h"
#include <idmlib/lshkit.h>

namespace sf1r { namespace laser {
class LSHIndexModel: public LaserGenericModel
{
typedef lshkit::Tail<lshkit::RepeatHash<lshkit::GaussianLsh> > LshHash;
typedef lshkit::LshIndex<LshHash, unsigned> LshIndex;
class SCANNER
{
public:
    SCANNER(const std::size_t adDimension, 
        std::vector<std::pair<docid_t, std::vector<std::pair<int, float> > > >& ad)
        : mask_(NULL)
        , ad_(ad)
    {
        mask_ = new std::vector<bool>(adDimension);
    }

    ~SCANNER()
    {
        if (NULL != mask_)
        {
            delete mask_;
            mask_ = NULL;
        }
    }
public:

    void operator()(std::size_t key) 
    {
        if (!(*mask_)[key])
        {
            const static std::vector<std::pair<int, float> > empty;
            ad_.push_back(std::make_pair(key, empty));
        }
    }

private:
    std::vector<bool>* mask_;
    std::vector<std::pair<docid_t, std::vector<std::pair<int, float> > > >& ad_;
};

public:
    LSHIndexModel(const AdIndexManager& adIndexer, 
        const std::string& kvaddr,
        const int kvport,
        const std::string& mqaddr,
        const int mqport,
        const std::string& workdir,
        const std::string& sysdir,
        const std::size_t adDimension,
        const std::size_t AD_FD,
        const std::size_t USER_FD);
    ~LSHIndexModel();
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
    LshIndex* createLshIndex();
    void buildLshIndex(LshIndex* lsh);
    void save();
    void load();
    void saveOrigModel();
    void localizeFromOrigModel();
private:
    const std::string workdir_;
    LshIndex* lsh_;
    int etaD_;
    int conjunctionD_;
    int LSH_DIM_;
    int ALSH_DIM_;
    mutable boost::shared_mutex mtx_;
};
} }
#endif
