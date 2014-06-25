#ifndef SF1R_SLIM_RECOMMEND_PARAM_H
#define SF1R_SLIM_RECOMMEND_PARAM_H

#include <string>

namespace sf1r { namespace slim {

class SlimRecommendParam {
public:
    std::string title_;
    int id_;
    int topn_;
};

}}

#endif
