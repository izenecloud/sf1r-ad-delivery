/**
* @file AdBidStrategy.h
* @author Cheng Zhi
* @date Created <2014-05-06 15:55:20>
* @brief Bid Strategies for sponsored advertising.
*/

#ifndef AD_BIDSTRATEGY_H_
#define AD_BIDSTRATEGY_H_

#include <map>
#include <string>
#include <utility>
#include <vector>
#include <list>


namespace sf1r
{

namespace sponsored
{

/**
* class AdQueryStatisticInfo
* @brief A class assembles statistic info for each user query.
*/

struct AdQueryStatisticInfo
{
    /** cost per click for each ad position. in unit fen*/
    std::vector<int> cpc_;

    /** click through rate for each ad position. from top to bottom.*/
    std::vector<double> ctr_;

    /** impression for this query. */
    int impression_;

    /** min bid for this query. */
    int minBid_;

    /** predefined bid for this query, if having not, just set to -1. if a non-negative predefined bid is given, this keyword's bid will just be this bid.*/
    int bid_;

    AdQueryStatisticInfo():impression_(0), minBid_(1), bid_(-1)
    {

    }
};


/**
* class AdBidStrategy
* @brief A class providing various bid strategies for bid optimization problem under constrained budget.
*/

class AdBidStrategy
{
public:
    AdBidStrategy();
    virtual ~AdBidStrategy();

public:
    /**
    * @brief generate two bids with probabilities for randomly selecting, this bid is independent of keywords.
    * @param qsInfos query statistic infos related to current bid.
    * @param budget total Budget in budget period.
    * @return Returning a list of two bids, <bid, probability> for each.
    */
    static std::vector<std::pair<int, double> > convexUniformBid(const std::list<AdQueryStatisticInfo>& qsInfos, int budget);

    /**
    * @brief provide one bid for each interesting keyword, to maximize revenue(clicks traffic). This strategy take the budget left in current charging period and the time remaining into consideration. So it is a real time bid method.
    * @qsInfo this keyword's statistic info including cost per click and click through rate for each ad position, impression, minbid etc. NOTE: impression is the expected impression in the remaining time of budget period.
    * @param budgetUsed total Budget already used today.
    * @param budgetLeft total Budget left today.
    * @param vpc value per click for current ad, such as 8.0, 10.0, 12.0.
    * @return Returning bid value.
    */
    static int realtimeBidWithRevenueMax(const AdQueryStatisticInfo& qsInfo, int budgetUsed, int budgetLeft, int vpc = 1000);

    /**
    * @brief provide one bid for each interesting keyword, to maximize profit(revenue - cost). This strategy take the budget left in current charging period and the time remaining into consideration. So it is a real time bid method.
    * @qsInfo this keyword's statistic info including cost per click and click through rate for each ad position, impressions, minbid etc.NOTE: impression is the expected impression in the remaining time of budget period.
    * @param budgetUsed total Budget already used today.
    * @param budgetLeft total Budget left today.
    * @param vpc value per click for current ad, such as 8.0, 10.0, 12.0.
    * @return Returning bid value for current query.
    */
    static int realtimeBidWithProfitMax(const AdQueryStatisticInfo& qsInfo, int budgetUsed, int budgetLeft, int vpc = 1000);


    /**
    * @brief generate a list of bids for interesting keywords, one different bid for each keyword.
    * @param qsInfos query statistic infos related to current bid.
    * @param budget total Budget in budget period.
    * @return Returning a list of bids, whose length is equal to qsInfos.
    */
    static std::vector<int> geneticBid(const std::list<AdQueryStatisticInfo>& qsInfos, int budget);
};


}
}

#endif
