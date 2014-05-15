/**
* @file  AdBidStrategy.cpp
* @brief Implementation to class AdBidStrategy.
*/

#include <functional>
#include <limits>
#include <algorithm>
#include <utility>
#include <set>
#include <queue>
#include <cmath>
#include <time.h>
#include <cstdlib>
#include <boost/unordered_map.hpp>
#include "AdBidStrategy.h"

namespace sf1r
{

    namespace sponsored
    {

        static const double E = 2.71828;

        AdBidStrategy::AdBidStrategy()
        {

        }

        AdBidStrategy::~AdBidStrategy()
        {

        }

        static bool isZero(double f)
        {
            return std::abs(f) < std::numeric_limits<double>::epsilon();
        }

        struct Point
        {
            double x;
            double y;

            Point(double X = 0.0, double Y = 0.0):x(X), y(Y){}
        };

        static bool operator<(const Point& p1, const Point& p2)
        {
            return (p1.x < p2.x) || (isZero(p1.x - p2.x) && p1.y < p2.y);
        }

        static bool xsmall(const Point& p1, const Point& p2)
        {
            return (p1.x < p2.x) ;
        }

        //judge the turning direction for line p0->p1->p2 at point p1.
        //turn left return 1, turn right return -1, in the same line return 0.
        static int turnDirection(const Point& p0, const Point& p1, const Point& p2)
        {
            double tmp = (p2.x - p0.x)*(p1.y - p0.y) - (p2.y - p0.y)*(p1.x - p0.x);
            if (tmp < 0) return 1;
            else if (tmp > 0) return -1;
            else return 0;
        }

        class AnglePredicate{
        public:
            AnglePredicate(const Point& p): base_(p){}

            bool operator()(const Point& p1, const Point& p2)
            {
                int tmp = turnDirection(base_, p1, p2);
                return (tmp > 0) || (tmp == 0 && 
                    (p1.x - base_.x)*(p1.x - base_.x) + (p1.y - base_.y)*(p1.y - base_.y) < (p2.x - base_.x)*(p2.x - base_.x) + (p2.y - base_.y)*(p2.y - base_.y)
                    );
            }

        private:
            Point base_;
        };

        std::vector<std::pair<double, double> > AdBidStrategy::convexUniformBid( const std::list<AdQueryStatisticInfo>& qsInfos, double budget)
        {
            static const std::vector<std::pair<double, double> > NULLBID_(2, std::make_pair(0.0, 0.0));

            //aggregate landscape
            boost::unordered_map<double, Point> landscape; //(cpc, <cost, clicks>)
            for (std::list<AdQueryStatisticInfo>::const_iterator cit = qsInfos.begin(); cit != qsInfos.end(); ++cit)
            {
                std::vector<double>::const_iterator cpcit = cit->cpc_.begin(), ctrit = cit->ctr_.begin();
                for (; cpcit != cit->cpc_.end() && ctrit != cit->ctr_.end(); ++cpcit, ++ctrit)
                {
                    Point& curP = landscape[*cpcit];
                    curP.x += (*cpcit) * (*ctrit);
                    curP.y += (*ctrit);
                }
            }

            std::vector<struct Point> allPoints;
            allPoints.reserve(landscape.size());
            Point minP(std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
            size_t minI = -1;
            for (boost::unordered_map<double, Point>::const_iterator cit = landscape.begin(); cit != landscape.end(); ++cit)
            {
                if (cit->second.y > 0 && cit->second.x > 0)
                {
                    allPoints.push_back(cit->second);

                    if (cit->second < minP)
                    {
                        minP = cit->second;
                        minI = allPoints.size() - 1;
                    }
                }
            }
            //   landscape.clear();

            if (allPoints.empty())
            {
                return NULLBID_;
            }

            std::swap(allPoints[0], allPoints[minI]);

            AnglePredicate ap(minP);
            std::sort(++allPoints.begin(), allPoints.end(), ap);

            //convex hull, Graham's Scan Algorithm.
            std::vector<struct Point> ch;
            int i = 0;
            while (i < 2 && i < allPoints.size())
            {//init
                ch.push_back(allPoints[i++]);   
            }

            for (; i < allPoints.size(); ++i)
            {
                while(ch.size() >= 2)
                {
                    int tmp = turnDirection(ch[ch.size() -2], ch[ch.size() - 1], allPoints[i]);
                    if (tmp < 0)
                    {
                        ch.pop_back();
                    }
                    else
                    {
                        break;
                    }
                }
                ch.push_back(allPoints[i]);
            }
            allPoints.clear();

            //candidate convex hull points for bid
            std::vector<struct Point> canch;
            int j = 0;
            for (; j < ch.size(); ++j)
            {
                if (j < ch.size() -1 && ch[j] < ch[j+1])
                {
                    continue;
                }
                else
                {
                    break;
                }
            }

            //upper bipart of convex hull.
            canch.reserve(ch.size() - j + 2);
            canch.push_back(Point()); //zero point.
            if (ch.size()>= 2 && !isZero(ch.front().x - ch.back().x))
            {
                canch.push_back(ch[0]);
            }

            for (int i = ch.size() -1; i >= j; --i)
            {
                canch.push_back(ch[i]);
            }

            ch.clear();

            //convex combination
            std::vector<std::pair<double, double> > bid;
            Point budgetPoint(budget, 0.0);
            std::vector<struct Point>::const_iterator uit = std::upper_bound(canch.begin(), canch.end(), budgetPoint, xsmall);
            if (uit == canch.end())
            {
                double mybid = canch.back().x / canch.back().y;
                bid.push_back(std::make_pair(mybid, 0.5));
                bid.push_back(std::make_pair(mybid, 0.5));
            }
            else 
            {
                std::vector<struct Point>::const_iterator preit = uit - 1;
                if (isZero(preit->x - budget))
                {
                    double mybid = preit->x / preit->y;
                    bid.push_back(std::make_pair(mybid, 0.5));
                    bid.push_back(std::make_pair(mybid, 0.5));
                }
                else
                {
                    double p = (budget - preit->x) / (uit->x - preit->x);
                    bid.push_back(std::make_pair(preit->x / preit->y, p));
                    bid.push_back(std::make_pair(uit->x / uit->y, 1.0 - p));
                }
            }

            return bid;
        }

        double AdBidStrategy::realtimeBidWithRevenueMax(const AdQueryStatisticInfo& qsInfo,  double budgetUsed, double budgetLeft, double vpc /*= 10.0*/)
        {
            double budget = budgetLeft + budgetUsed;
            if (isZero(budget) || qsInfo.cpc_.empty())
            {
                return 0.0;
            }

            double U = std::numeric_limits<double>::max();
            double minBid = qsInfo.minBid_;
            if (minBid < 0.0)
            {
                minBid = 0.0;
            }
            if (!isZero(minBid))
            {
                U = vpc / minBid;
            }

            double z = budgetUsed / budget;

            //efficiency
            double eff = pow(U * E, z) / E;

            for (int i = 0; i < qsInfo.cpc_.size(); ++i)
            {
                double avaiableB = budgetLeft / (qsInfo.ctr_[i] * qsInfo.impression_);
                double myeff = vpc / qsInfo.cpc_[i];
                if (eff > myeff && qsInfo.cpc_[i] <= avaiableB)
                {
                    eff = myeff;
                }
            }

            for (int i = 0; i < qsInfo.cpc_.size(); ++i)
            {
                double myeff = vpc / qsInfo.cpc_[i];
                if (myeff >= eff)
                {
                    return qsInfo.cpc_[i];
                }
            }

            return vpc / (1 + eff);
        }

        double AdBidStrategy::realtimeBidWithProfitMax( const AdQueryStatisticInfo& qsInfo, double budgetUsed, double budgetLeft, double vpc /*= 10.0*/)
        {
            static const double MINEFF = 0.1;

            double budget = budgetLeft + budgetUsed;
            if (isZero(budget) || qsInfo.cpc_.empty())
            {
                return 0.0;
            }

            double U = std::numeric_limits<double>::max();
            double minBid = qsInfo.minBid_;
            if (minBid < 0.0)
            {
                minBid = 0.0;
            }
            if (!isZero(minBid))
            {
                U = vpc / minBid - 1;
            }

            double z = budgetUsed / budget;

            //efficiency
            double eff = pow(U * E / MINEFF, z) * MINEFF / E;

            for (int i = 0; i < qsInfo.cpc_.size(); ++i)
            {
                double avaiableB = budgetLeft / (qsInfo.ctr_[i] * qsInfo.impression_);
                double myeff = vpc / qsInfo.cpc_[i] - 1;
                if (eff > myeff && qsInfo.cpc_[i] <= avaiableB)
                {
                    eff = myeff;
                }
            }

            int maxI = -1;
            double maxV = -1.0;
            for (int i = 0; i < qsInfo.cpc_.size(); ++i)
            {
                double myeff = vpc / qsInfo.cpc_[i] - 1;
                if (myeff >= eff)
                {
                    double myV = (vpc - qsInfo.cpc_[i]) * qsInfo.ctr_[i];
                    if (myV > maxV)
                    {
                        maxV = myV;
                        maxI = i;
                    }
                }
            }
            if (maxI >= 0)
            {
                return qsInfo.cpc_[maxI];
            }
            else
                return vpc / (1 + eff);
        }


        class LargerFit{
        public:
            LargerFit(const std::vector<double>& Fit): _fit(Fit){}
            bool operator()(int l, int r)
            {
                return _fit[l] < _fit[r];
            }

        private:
            const std::vector<double>& _fit;
        };

        std::vector<double> AdBidStrategy::geneticBid( const std::list<AdQueryStatisticInfo>& qsInfos, double budget )
        {
            static const int MaxAllowedEvolutions = 3000;
            static const double EndPopulationRate = 0.90;  //when 90% of the population has same fitness value, stop evolute.
            static const int PopulationSize = 40; //must be even
            static const int ElitismSize = 2;

            std::vector<double> bid(qsInfos.size(), 0.0);
            if (qsInfos.empty() || budget < 0.0 || isZero(budget))
            {
                return bid;
            }

            typedef std::vector<std::vector<double> > TQSDataType;
            TQSDataType W(qsInfos.size()), V(qsInfos.size());
            std::vector<int> adP(qsInfos.size());  //ad position num for each keyword.
            int kNum = 0;
            for (std::list<AdQueryStatisticInfo>::const_iterator cit = qsInfos.begin(); cit != qsInfos.end(); ++cit, ++kNum)
            {
                const std::vector<double>& cpc = cit->cpc_;
                const std::vector<double>& ctr = cit->ctr_;

                W[kNum].reserve(cpc.size());
                V[kNum].reserve(cpc.size());
                adP[kNum] = cpc.size();

                for (int j = 0; j < cpc.size(); ++j)
                {
                    W[kNum].push_back(cpc[j] * cit->impression_ * ctr[j]);
                    V[kNum].push_back(cit->impression_ * ctr[j]);  //max traffics. value is defined as click traffics.
                }
            }

            typedef std::vector<std::vector<int> > TPopType; //every individual is a vector of index of ad position for each keyword, 0-based.
            TPopType P(PopulationSize);
            TPopType newP(PopulationSize);

            srand(time(NULL));
            for (int i = 0; i < PopulationSize; ++i)
            {
                P[i].reserve(qsInfos.size());
                newP[i].reserve(qsInfos.size());
                for (size_t j = 0; j < qsInfos.size(); ++j)
                {
                    int N = adP[j] + 1;
                    P[i].push_back(rand() % N - 1); //ad position is 0-based, -1 means do not bid for that keyword.
                    newP[i].push_back(0);
                }
            }

            int iterNum = MaxAllowedEvolutions;
            while(iterNum--)
            {
                //clear population for budget requirement.
                for (int i = 0; i < PopulationSize; ++i)
                {
                    std::vector<std::pair<int, int> > kw; //(keyword index, selected ad position index)
                    for (int j = 0; j < P[i].size(); ++j)
                    {//for each keyword
                        if (P[i][j] != -1)
                        {
                            kw.push_back(std::make_pair(j, P[i][j]));
                        }
                    }

                    double totalW = 0.0;
                    for (std::vector<std::pair<int, int> >::const_iterator cit = kw.begin(); cit != kw.end(); ++cit)
                    {
                        totalW += W[cit->first][cit->second];
                    }

                    int aN = kw.size();
                    while(totalW > budget)
                    {
                        int r = rand() % aN;
                        totalW -= W[kw[r].first][kw[r].second];
                        P[i][kw[r].first] = -1;
                        std::swap(kw[r], kw[aN - 1]);
                        --aN;
                    }
                }

                //selection, 
                std::vector<int> SP(PopulationSize); //selected individual's index in P
                int GASize = PopulationSize - ElitismSize;

                {
                    std::vector<double> fitness(PopulationSize); //total fitness.
                    std::vector<double> fit(PopulationSize);    //each individual fitness.
                    double fn = 0.0;
                    for (int i = 0; i < PopulationSize; ++i)
                    {
                        double curfn = 0.0;
                        for (int j = 0; j < P[i].size(); ++j)
                        {//for each keyword
                            if (P[i][j] != -1)
                            {
                                curfn += V[j][P[i][j]];
                            }
                        }
                        fn += curfn;
                        fit[i] = curfn;
                        fitness[i] = fn;
                    }

                    //check condition of stop evolution.
                    {
                        boost::unordered_map<double, int> fitNum;
                        for (int i = 0; i < PopulationSize; ++i)
                        {
                            fitNum[fit[i]]++;
                        }
                        int maxNum = -1;
                        for (boost::unordered_map<double, int>::const_iterator cit = fitNum.begin(); cit != fitNum.end(); ++cit)
                        {
                            if (cit->second > maxNum)
                            {
                                maxNum = cit->second;
                            }
                        }

                        if ((double)maxNum / PopulationSize >= EndPopulationRate)
                        {
                            break;
                        }
                    }

                    //stochastic universal sampling.
                    double fstep = fn / GASize;
                    double fstart = ((double)rand()) / RAND_MAX * fstep;

                    if (!isZero(fstep))
                    {
                        for (int i = 0, j = 0; i < GASize; ++i)
                        {
                            double fcur = fstart + i * fstep;
                            for (int k = j; k < PopulationSize; ++k)
                            {
                                if (fitness[k] > fcur)
                                {
                                    SP[i] = k;
                                    j = k;
                                    break;
                                }
                            }
                        }
                    }
                    else
                    {
                        for (int i = 0; i < PopulationSize; ++i)
                        {
                            SP[i] = i;
                        }
                    }

                    //random to select crossover pair
                    for (int i = GASize; i >= 1; --i)
                    {
                        int j = rand() % i;
                        std::swap(SP[j], SP[i - 1]);
                    }

                    //elitism
                    LargerFit mylf(fit);
                    std::priority_queue<int, std::vector<int>, LargerFit> pq(mylf);
                    for (int i = 0; i < PopulationSize; ++i)
                    {
                        pq.push(i);
                    }
                    for (int i = GASize; i < PopulationSize; ++i)
                    {
                        SP[i] = pq.top();
                        pq.pop();
                    }


                }

                //crossover
                int crossoverRate = 85;
                for (int i = 0; i < GASize; i += 2)
                {
                    int myRate = rand() % 100;
                    if (myRate < crossoverRate)
                    {
                        const std::vector<int>& lp = P[SP[i]];
                        const std::vector<int>& rp = P[SP[i+1]];

                        for (int j = 0; j < qsInfos.size(); ++j)
                        {
                            double p = (double)rand() / RAND_MAX * 1.50 - 0.25;

                            if (lp[j] != -1 && rp[j] != -1)
                            {
                                //newP[i][j]
                                int v = lp[j] * p + rp[j] * (1.0 - p) + 0.5;
                                v = std::max(v, 0);
                                v = std::min(v, adP[j] - 1);
                                newP[i][j] = v;

                                //newP[i+1]
                                //newP[i]
                                v = lp[j] * ( 1.0 - p ) + rp[j] * p + 0.5;
                                v = std::max(v, 0);
                                v = std::min(v, adP[j] - 1);
                                newP[i+1][j] = v;
                            }
                            else if (lp[j] == -1 && rp[j] == -1)
                            {
                                newP[i][j] = -1;
                                newP[i+1][j] = -1;
                            }
                            else
                            {
                                //discrete
                                int ip = rand() % 2;
                                if (ip)
                                {
                                    newP[i][j] = -1;
                                    newP[i+1][j] = -1;
                                }
                                else
                                {
                                    int tmp = std::max(lp[j], rp[j]);
                                    newP[i][j] = tmp;
                                    newP[i+1][j] = tmp;
                                }
                            }

                        }
                    }
                    else
                    {
                        newP[i] = P[SP[i]];
                        newP[i+1] = P[SP[i+1]];
                    }
                }
                for (int i = GASize; i < PopulationSize; ++i)
                {
                    newP[i] = P[SP[i]];
                }

                //mutation, mutation rate, 1/countof(var)
                int mRate = qsInfos.size();
                for (int i = 0; i < GASize; ++i)
                {
                    for (int j = 0; j < qsInfos.size(); ++j)
                    {
                        if (rand() % mRate == 0)
                        {
                            //do mutation
                            newP[i][j] = rand() % (adP[j] + 1) - 1;
                        }
                    }
                }

                newP.swap(P);
            }

            //max fitness in population
            int maxI = -1;
            double maxfit = -1.0;
            for (int i = 0; i < PopulationSize; ++i)
            {
                double ft = 0.0;
                for (int j = 0; j < qsInfos.size(); ++j)
                {
                    if (P[i][j] != -1)
                    {
                        ft += V[j][P[i][j]];
                    }
                }
                if (ft > maxfit)
                {
                    maxfit = ft;
                    maxI = i;
                }
            }


            if (maxI != -1)
            {
                int j = 0;
                std::list<AdQueryStatisticInfo>::const_iterator cit = qsInfos.begin();
                for (; cit != qsInfos.end(); ++cit, ++j)
                {
                    if (P[maxI][j] != -1)
                    {
                        bid[j] = cit->cpc_[P[maxI][j]];
                    }
                }
            }

            return bid;
        }

    }
}

