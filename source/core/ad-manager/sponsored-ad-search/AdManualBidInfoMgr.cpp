#include "AdManualBidInfoMgr.h"

#include <util/izene_serialization.h>
#include <glog/logging.h>
#include <fstream>
#include <boost/serialization/map.hpp>
#include <boost/serialization/unordered_map.hpp>

#include <boost/filesystem.hpp>
#define MAX_CAMPAIGN_NUM 1000000

namespace bfs = boost::filesystem;

namespace sf1r
{

namespace sponsored
{

AdManualBidInfoMgr::AdManualBidInfoMgr()
{
}

AdManualBidInfoMgr::~AdManualBidInfoMgr()
{
    save();
}

void AdManualBidInfoMgr::init(const std::string& data_path)
{
    manual_bidprice_list_.rehash(MAX_CAMPAIGN_NUM);
    ad_campaign_budget_list_.rehash(MAX_CAMPAIGN_NUM);
    data_path_ = data_path;

    if (!bfs::exists(data_path_))
    {
        bfs::create_directories(data_path_);
    }
    load();
}

void AdManualBidInfoMgr::load()
{
    std::ifstream ifs(std::string(data_path_ + "/ad_manual_bidinfo.data").c_str());
    std::string data;

    if (ifs.good())
    {
        std::size_t len = 0;
        {
            ifs.read((char*)&len, sizeof(len));
            data.resize(len);
            ifs.read((char*)&data[0], len);
            izenelib::util::izene_deserialization<ManualBidInfoT> izd(data.data(), data.size());
            izd.read_image(manual_bidprice_list_);
        }

        {
            len = 0;
            ifs.read((char*)&len, sizeof(len));
            data.resize(len);
            ifs.read((char*)&data[0], len);
            izenelib::util::izene_deserialization<boost::unordered_map<std::string, int> > izd(data.data(), data.size());
            izd.read_image(ad_campaign_budget_list_);
        }
        LOG(INFO) << "manual bidinfo loaded: " << manual_bidprice_list_.size() << ", " << ad_campaign_budget_list_.size();
        LOG(INFO) << "manual bucket info : " << manual_bidprice_list_.bucket_count() << ", " << ad_campaign_budget_list_.bucket_count();
    }

    ifs.close();
}

void AdManualBidInfoMgr::save()
{
    std::ofstream ofs(std::string(data_path_ + "/ad_manual_bidinfo.data").c_str());
    std::size_t len = 0;
    char* buf = NULL;
    {
        izenelib::util::izene_serialization<ManualBidInfoT> izs(manual_bidprice_list_);
        izs.write_image(buf, len);
        ofs.write((const char*)&len, sizeof(len));
        ofs.write(buf, len);
        ofs.flush();
    }
    {
        len = 0;
        izenelib::util::izene_serialization<boost::unordered_map<std::string, int> > izs(ad_campaign_budget_list_);
        izs.write_image(buf, len);
        ofs.write((const char*)&len, sizeof(len));
        ofs.write(buf, len);
        ofs.flush();
    }
}

void AdManualBidInfoMgr::setManualBidPrice(const std::string& campaign_name, const std::vector<std::string>& key_list,
    const std::vector<int>& price_list)
{
    std::map<std::string, int>& v = manual_bidprice_list_[campaign_name];
    for(std::size_t i = 0; i < key_list.size(); ++i)
    {
        v[key_list[i]] = price_list[i];
    }
}

void AdManualBidInfoMgr::getManualBidPriceList(const std::string& campaign_name, std::map<std::string, int>& price_list)
{
    price_list.clear();
    ManualBidInfoT::const_iterator cit = manual_bidprice_list_.find(campaign_name);
    if (cit == manual_bidprice_list_.end())
        return;
    price_list = cit->second;
}

void AdManualBidInfoMgr::setBidBudget(const std::string& campaign_name, int budget)
{
    ad_campaign_budget_list_[campaign_name] = budget;
}

}

}
