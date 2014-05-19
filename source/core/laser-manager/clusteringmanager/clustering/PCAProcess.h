#ifndef SF1R_LASER_PCA_PROCESS_H
#define SF1R_LASER_PCA_PROCESS_H
#include <string>
#include <knlp/title_pca.h>
#include <am/sequence_file/ssfr.h>
#include <boost/unordered_map.hpp>
#include "laser-manager/clusteringmanager/type/TermDictionary.h"
#include "PCARunner.h"

namespace sf1r { namespace laser { namespace clustering {
class PCAProcess
{
public:
    PCAProcess(const std::string& workdir, 
        const std::string& scddir, 
        const std::string& pcaDictPath,
        float threhold,  
        const std::size_t minDocPerClustering = 10, 
        const std::size_t maxDocPerClustering = 1000, 
        const std::size_t dictLimit = 10000, 
        const std::size_t threadNum = 3);
    ~PCAProcess();

public:
    void run();
    
private:
    bool next(std::string& title, std::string& category);
    void initFstream();
    void save();
private:
    const std::string workdir_;
    const std::string scddir_;
    const std::size_t dictLimit_;
    type::TermDictionary* termDict_;
    PCARunner* runner_;
    std::queue<std::fstream*> fstream_;
};
} } }
#endif
