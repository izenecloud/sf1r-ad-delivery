#ifndef AD_RESULT_TYPE_H
#define AD_RESULT_TYPE_H

#include <common/ResultType.h>

namespace sf1r
{

class AdKeywordSearchResult : public KeywordSearchResult
{
public:
    AdKeywordSearchResult()
        :KeywordSearchResult()
    {
    }

    std::vector<double> topKAdCost_;
    std::vector<std::string> topKAdBidStrList_;

    void swap(AdKeywordSearchResult& other)
    {
        KeywordSearchResult::swap(other);
        topKAdCost_.swap(other.topKAdCost_);
        topKAdBidStrList_.swap(other.topKAdBidStrList_);
    }
};

}
#endif


