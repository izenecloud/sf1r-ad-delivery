#include <ad-manager/AdStreamSubscriber.h>
#include <ad-manager/AdFeedbackMgr.h>
#include <ad-manager/sponsored-ad-search/AdAuctionLogMgr.h>
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>
#include <fstream>
#include <glog/logging.h>
#include <la-manager/TitlePCAWrapper.h>

#define MAX_TEST_KID 1000
#define MAX_AD_CREATIVES_NUM 1000
namespace bfs=boost::filesystem;
using namespace sf1r;
namespace sp=sf1r::sponsored;

std::string gen_rand_str()
{
    size_t rand_len = rand() % 30 + 3;
    std::string ret(rand_len, '\0');
    for(size_t i = 0; i < rand_len; ++i)
    {
        int r = rand();
        ret[i] = ((r%2 == 0) ?'a':'A') + r%26;
    }
    return ret;
}


int main()
{
    srand(time(NULL));

    std::string test_base_path = "/opt/mine/sponsored_ad_test";
    if (!bfs::exists(test_base_path.c_str()))
        return 0;

    LOG(INFO) << "begin generating the test data.";
    
    using sp::AdAuctionLogMgr;
    AdAuctionLogMgr ad_auctionlog_mgr_;

    std::vector<sp::BidKeywordId>  heavy_searched_keyword;
    std::vector<sp::BidKeywordId>  normal_searched_keyword;
    std::vector<sp::BidKeywordId>  less_searched_keyword;
    for(sp::BidKeywordId kid = 1; kid < MAX_TEST_KID; ++kid)
    {
        if (kid % 7 <= 2)
        {
            heavy_searched_keyword.push_back(kid);
        }
        else if (kid % 7 <=5)
        {
            normal_searched_keyword.push_back(kid);
        }
        else
            less_searched_keyword.push_back(kid);
    }
    std::map<std::string, sp::BidPhraseT>  test_ad_bid_list;
    std::vector<std::string>  slot1_adstr_list;
    std::vector<std::string>  slot2_adstr_list;
    std::vector<std::string>  slot3_adstr_list;
    for (std::size_t i = 0; i < MAX_AD_CREATIVES_NUM; ++i)
    {
        {
            int k = rand();
            std::string adstr = gen_rand_str();
            slot1_adstr_list.push_back(adstr);
            test_ad_bid_list[adstr].push_back(heavy_searched_keyword[k % heavy_searched_keyword.size()]);
            test_ad_bid_list[adstr].push_back(normal_searched_keyword[k % normal_searched_keyword.size()]);
            test_ad_bid_list[adstr].push_back(less_searched_keyword[k % less_searched_keyword.size()]);
        }
        if (i < MAX_AD_CREATIVES_NUM/3*2 && i > MAX_AD_CREATIVES_NUM/3)
        {
            int k = rand();
            std::string adstr = slot1_adstr_list[k % slot1_adstr_list.size()];
            slot2_adstr_list.push_back(adstr);
        }
        else
        {
            int k = rand();
            std::string adstr = slot1_adstr_list[k % slot1_adstr_list.size()];
            slot3_adstr_list.push_back(adstr);
        }
    }

    std::size_t MAX_SEARCH_NUM = 10000000;
    std::vector<std::string> searched_list;
    std::set<sp::BidKeywordId> keyword_list;
    std::vector<std::string>  searched_clicked_list;
    std::vector<sp::BidPhraseT> clicked_bid_phrase_list;
    for(std::size_t i = 0; i < MAX_SEARCH_NUM; ++i)
    {
        searched_list.clear();
        keyword_list.clear();
        int k = rand();
        std::string adstr = slot1_adstr_list[k % slot1_adstr_list.size()];
        const sp::BidPhraseT& bidphrase = test_ad_bid_list[adstr];
        keyword_list.insert(bidphrase.begin(), bidphrase.end());
        searched_list.push_back(adstr);

        adstr = slot2_adstr_list[k % slot2_adstr_list.size()];
        const sp::BidPhraseT& bidphrase2 = test_ad_bid_list[adstr];
        keyword_list.insert(bidphrase2.begin(), bidphrase2.end());
        searched_list.push_back(adstr);

        adstr = slot3_adstr_list[k % slot3_adstr_list.size()];
        const sp::BidPhraseT& bidphrase3 = test_ad_bid_list[adstr];
        keyword_list.insert(bidphrase3.begin(), bidphrase3.end());
        searched_list.push_back(adstr);

        ad_auctionlog_mgr_.updateAdSearchStat(keyword_list, searched_list);

        if (i % 500 == 0)
        {
            int click_slot_rand = rand() % 10000;
            std::string clicked_adstr;
            sp::BidPhraseT clicked_keyword;
            int click_cost = 10;
            uint32_t click_slot = 0;
            if (click_slot_rand < 7000)
            {
                clicked_adstr = searched_list[0];
                clicked_keyword = test_ad_bid_list[clicked_adstr];
            }
            else if (click_slot_rand < 9900)
            {
                clicked_adstr = searched_list[1];
                clicked_keyword = test_ad_bid_list[clicked_adstr];
                click_slot = 1;
                click_cost *= 0.8;
            }
            else
            {
                clicked_adstr = searched_list[2];
                clicked_keyword = test_ad_bid_list[clicked_adstr];
                click_slot = 2;
                click_cost *= 0.6;
            }
            searched_clicked_list.push_back(clicked_adstr);
            clicked_bid_phrase_list.push_back(clicked_keyword);
            ad_auctionlog_mgr_.updateAuctionLogData(clicked_adstr, clicked_keyword, click_cost, click_slot);
        }
    }

    std::ofstream ofs(std::string(test_base_path + "/test.out").c_str());

    ofs << "Total clicked : " << ad_auctionlog_mgr_.getAvgTotalClickedNum() << std::endl;

    ofs << "Ad statistical data for clicked ad: " << std::endl;
    for(std::size_t i = 0; i < searched_clicked_list.size(); ++i)
    {
        std::string adstr = searched_clicked_list[i];
        ofs << adstr << ":" << ad_auctionlog_mgr_.getAdCTR(adstr) << ","
            << ad_auctionlog_mgr_.getAdAvgCost(adstr) << std::endl;
    }

    ofs << "Ad statistical data for keyword: " << std::endl;
    ofs << "clicked bid keyword: " << std::endl;
    for(std::size_t i = 0; i < clicked_bid_phrase_list.size(); ++i)
    {
        for(std::size_t j = 0; j < clicked_bid_phrase_list[i].size(); ++j)
        {
            sp::BidKeywordId bid = clicked_bid_phrase_list[i][j];
            ofs << bid << ":" << ad_auctionlog_mgr_.getAvgKeywordClickedNum(bid) << ", cost: "
                << ad_auctionlog_mgr_.getKeywordAvgCost(bid, 0) << "-"
                << ad_auctionlog_mgr_.getKeywordAvgCost(bid, 1) << "-"
                << ad_auctionlog_mgr_.getKeywordAvgCost(bid, 2) << ", ctr: "
                << ad_auctionlog_mgr_.getKeywordCTR(bid, 0) << std::endl;
        }
    }


    std::vector<std::vector<std::pair<int, double> > > cost_click_list;
    std::vector<sp::BidKeywordId> all_keys;
    ad_auctionlog_mgr_.getKeywordBidLandscape(all_keys, cost_click_list);
    ofs << "keyword landscape: " << std::endl;
    for(std::size_t i = 0; i < all_keys.size(); ++i)
    {
        ofs << "key: " << all_keys[i] << ", ";
        for (std::size_t j = 0; j < cost_click_list[i].size(); ++j)
        {
            ofs << "(cost:" << cost_click_list[i][j].first << ", ctr:" << cost_click_list[i][j].second << "),";
        }
        ofs << std::endl;
    }

    std::vector<std::string> all_ads;
    ad_auctionlog_mgr_.getAdLandscape(all_ads, cost_click_list);
    ofs << "ad creative landscape: " << std::endl;
    for(std::size_t i = 0; i < all_ads.size(); ++i)
    {
        ofs << "key: " << all_ads[i] << ", ";
        for (std::size_t j = 0; j < cost_click_list[i].size(); ++j)
        {
            ofs << "(cost:" << cost_click_list[i][j].first << ", ctr:" << cost_click_list[i][j].second << "),";
        }
        ofs << std::endl;
    }

    ofs.close();

    LOG(INFO) << "begin test sponsored ad manager.";

}
