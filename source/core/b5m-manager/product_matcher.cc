#include "product_matcher.h"
#include <util/hashFunction.h>
#include <common/ScdParser.h>
#include <common/ScdWriter.h>
#include <mining-manager/util/split_ustr.h>
#include <product-manager/product_price.h>
#include <boost/unordered_set.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/serialization/hash_map.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <algorithm>
#include <cmath>
#include <idmlib/util/svm.h>
#include <util/functional.h>
#include <util/ClockTimer.h>
#include <3rdparty/json/json.h>
using namespace sf1r;
using namespace idmlib::sim;
using namespace idmlib::kpe;
using namespace idmlib::util;
namespace bfs = boost::filesystem;


//#define B5M_DEBUG

const std::string ProductMatcher::AVERSION("20130222");

ProductMatcher::KeywordTag::KeywordTag():type_app(0), kweight(0.0), ngram(1)
{
}

void ProductMatcher::KeywordTag::Flush()
{
    std::sort(category_name_apps.begin(), category_name_apps.end());
    std::vector<CategoryNameApp> new_category_name_apps;
    new_category_name_apps.reserve(category_name_apps.size());
    for(uint32_t i=0;i<category_name_apps.size();i++)
    {
        const CategoryNameApp& app = category_name_apps[i];
        if(new_category_name_apps.empty())
        {
            new_category_name_apps.push_back(app);
            continue;
        }
        CategoryNameApp& last_app = new_category_name_apps.back();
        if(app.cid==last_app.cid)
        {
            if(app.is_complete&&!last_app.is_complete)
            {
                last_app = app;
            }
            else if(app.is_complete==last_app.is_complete&&app.depth<last_app.depth)
            {
                last_app = app;
            }
        }
        else
        {
            new_category_name_apps.push_back(app);
        }
    }
    std::swap(new_category_name_apps, category_name_apps);
    //SortAndUnique(category_name_apps);
    SortAndUnique(attribute_apps);
    //SortAndUnique(spu_title_apps);
}
void ProductMatcher::KeywordTag::Append(const KeywordTag& another, bool is_complete)
{
    if(is_complete)
    {
        for(uint32_t i=0;i<another.category_name_apps.size();i++)
        {
            CategoryNameApp aapp = another.category_name_apps[i];
            if(!is_complete) aapp.is_complete = false;
            category_name_apps.push_back(aapp);
        }
    }
    if(is_complete)
    {
        attribute_apps.insert(attribute_apps.end(), another.attribute_apps.begin(), another.attribute_apps.end());
    }
    //spu_title_apps.insert(spu_title_apps.end(), another.spu_title_apps.begin(), another.spu_title_apps.end());
}
bool ProductMatcher::KeywordTag::Combine(const KeywordTag& another)
{
    //check if positions were overlapped
    for(uint32_t i=0;i<another.positions.size();i++)
    {
        const Position& ap = another.positions[i];
        //std::cerr<<"ap "<<ap.first<<","<<ap.second<<std::endl;
        for(uint32_t j=0;j<positions.size();j++)
        {
            const Position& p = positions[j];
            //std::cerr<<"p "<<p.first<<","<<p.second<<std::endl;
            bool _overlapped = true;
            if( ap.first>=p.second || p.first>=ap.second) _overlapped = false;
            if(_overlapped)
            {
                return false;
            }
        }
    }
    category_name_apps.clear();
    std::vector<AttributeApp> new_attribute_apps;
    new_attribute_apps.reserve(std::max(attribute_apps.size(), another.attribute_apps.size()));

    uint32_t i=0;
    uint32_t j=0;
    while(i<attribute_apps.size()&&j<another.attribute_apps.size())
    {
        AttributeApp& app = attribute_apps[i];
        const AttributeApp& aapp = another.attribute_apps[j];
        if(app.spu_id==aapp.spu_id)
        {
            if(app.attribute_name!=aapp.attribute_name)
            {
                new_attribute_apps.push_back(app);
                new_attribute_apps.push_back(aapp);
                //app.attribute_name += "+"+aapp.attribute_name;
                //app.is_optional = app.is_optional | aapp.is_optional;
            }
            else
            {
                new_attribute_apps.push_back(app);
            }
            //else
            //{
                //app.spu_id = 0;
            //}
            ++i;
            ++j;
        }
        else if(app.spu_id<aapp.spu_id)
        {
            //app.spu_id = 0;
            ++i;
        }
        else
        {
            ++j;
        }
    }
    std::swap(new_attribute_apps, attribute_apps);
    //i = 0;
    //j = 0;
    //while(i<spu_title_apps.size()&&j<another.spu_title_apps.size())
    //{
        //SpuTitleApp& app = spu_title_apps[i];
        //const SpuTitleApp& aapp = another.spu_title_apps[j];
        //if(app.spu_id==aapp.spu_id)
        //{
            //app.pstart = std::min(app.pstart, aapp.pstart);
            //app.pend = std::max(app.pend, aapp.pend);
            //++i;
            //++j;
        //}
        //else if(app<aapp)
        //{
            //app.spu_id = 0;
            //++i;
        //}
        //else
        //{
            //++j;
        //}
    //}
    //while(i<spu_title_apps.size())
    //{
        //SpuTitleApp& app = spu_title_apps[i];
        //app.spu_id = 0;
        //++i;
    //}
    ++ngram;
    positions.insert(positions.end(), another.positions.begin(), another.positions.end());
    return true;
}

ProductMatcher::ProductMatcher()
:is_open_(false), 
 use_price_sim_(true), matcher_only_(false), category_max_depth_(0), use_ngram_(false),
 aid_manager_(NULL), analyzer_(NULL), char_analyzer_(NULL), chars_analyzer_(NULL),
 test_docid_("7bc999f5d10830d0c59487bd48a73cae"),
 left_bracket_("("), right_bracket_(")"), place_holder_("__PLACE_HOLDER__"), blank_(" "),
 left_bracket_term_(0), right_bracket_term_(0), place_holder_term_(0)
{
}

ProductMatcher::~ProductMatcher()
{
    if(aid_manager_!=NULL)
    {
        aid_manager_->close();
        delete aid_manager_;
    }
    if(analyzer_!=NULL)
    {
        delete analyzer_;
    }
    if(char_analyzer_!=NULL)
    {
        delete char_analyzer_;
    }
    if(chars_analyzer_!=NULL)
    {
        delete chars_analyzer_;
    }
    //if(cr_result_!=NULL)
    //{
        //cr_result_->flush();
        //delete cr_result_;
    //}
}

bool ProductMatcher::IsOpen() const
{
    return is_open_;
}

bool ProductMatcher::Open(const std::string& kpath)
{
    if(!is_open_)
    {
        path_ = kpath;
        try
        {
            Init_();
            std::string path = path_+"/products";
            izenelib::am::ssf::Util<>::Load(path, products_);
            LOG(INFO)<<"products size "<<products_.size()<<std::endl;
            path = path_+"/category_list";
            izenelib::am::ssf::Util<>::Load(path, category_list_);
            LOG(INFO)<<"category list size "<<category_list_.size()<<std::endl;
            //path = path_+"/keywords";
            //izenelib::am::ssf::Util<>::Load(path, keywords_thirdparty_);
            path = path_+"/category_index";
            izenelib::am::ssf::Util<>::Load(path, category_index_);
            path = path_+"/cid_to_pids";
            izenelib::am::ssf::Util<>::Load(path, cid_to_pids_);
            path = path_+"/product_index";
            izenelib::am::ssf::Util<>::Load(path, product_index_);
            path = path_+"/keyword_trie";
            izenelib::am::ssf::Util<>::Load(path, trie_);
            LOG(INFO)<<"trie size "<<trie_.size()<<std::endl;
            path = path_+"/back2front";
            std::map<std::string, std::string> b2f_map;
            izenelib::am::ssf::Util<>::Load(path, b2f_map);
            back2front_.insert(b2f_map.begin(), b2f_map.end());
            LOG(INFO)<<"back2front size "<<back2front_.size()<<std::endl;
            //path = path_+"/nf";
            //std::map<uint64_t, uint32_t> nf_map;
            //izenelib::am::ssf::Util<>::Load(path, nf_map);
            //nf_.insert(nf_map.begin(), nf_map.end());
            //LOG(INFO)<<"nf size "<<nf_.size()<<std::endl;

        }
        catch(std::exception& ex)
        {
            LOG(ERROR)<<"product matcher open failed"<<std::endl;
            return false;
        }
        is_open_ = true;
    }
    return true;
}

//void ProductMatcher::Clear(const std::string& path, int omode)
//{
    //int mode = omode;
    //std::string version = GetVersion(path);
    //if(version!=VERSION)
    //{
        //mode = 3;
    //}
    //if(mode>=2)
    //{
        //B5MHelper::PrepareEmptyDir(path);
    //}
    //else if(mode>0)
    //{
        //std::string bdb_path = path+"/bdb";
        //B5MHelper::PrepareEmptyDir(bdb_path);
        ////if(mode>=2)
        ////{
            ////std::string odb_path = path+"/odb";
            ////B5MHelper::PrepareEmptyDir(odb_path);
        ////}
    //}
//}
std::string ProductMatcher::GetVersion(const std::string& path)
{
    std::string version_file = path+"/VERSION";
    std::string version;
    std::ifstream ifs(version_file.c_str());
    getline(ifs, version);
    boost::algorithm::trim(version);
    return version;
}
std::string ProductMatcher::GetAVersion(const std::string& path)
{
    std::string version_file = path+"/AVERSION";
    std::string version;
    std::ifstream ifs(version_file.c_str());
    getline(ifs, version);
    boost::algorithm::trim(version);
    return version;
}
std::string ProductMatcher::GetRVersion(const std::string& path)
{
    std::string version_file = path+"/RVERSION";
    std::string version;
    std::ifstream ifs(version_file.c_str());
    getline(ifs, version);
    boost::algorithm::trim(version);
    return version;
}

bool ProductMatcher::GetProduct(const std::string& pid, Product& product)
{
    ProductIndex::const_iterator it = product_index_.find(pid);
    if(it==product_index_.end()) return false;
    uint32_t index = it->second;
    if(index>=products_.size()) return false;
    product = products_[index];
    return true;
}
void ProductMatcher::CategoryDepthCut(std::string& category, uint16_t d)
{
    if(d==0) return;
    if(category.empty()) return;
    std::vector<std::string> vec;
    boost::algorithm::split(vec, category, boost::algorithm::is_any_of(">"));
    if(vec.size()>d)
    {
        category="";
        for(uint16_t i=0;i<d;i++)
        {
            if(!category.empty()) category+=">";
            category+=vec[i];
        }
    }
}

bool ProductMatcher::GetFrontendCategory(UString& backend, UString& frontend) const
{
    if(back2front_.empty()) return false;
    std::string b;
    backend.convertString(b, UString::UTF_8);
    std::string ob = b;//original
    while(!b.empty())
    {
        Back2Front::const_iterator it = back2front_.find(b);
        if(it!=back2front_.end())
        {
            std::string sfront = it->second;
            CategoryDepthCut(sfront, category_max_depth_);
            frontend = UString(sfront, UString::UTF_8);
            break;
        }
        std::size_t pos = b.find_last_of('>');
        if(pos==std::string::npos) b.clear();
        else
        {
            b = b.substr(0, pos);
        }
    }
    if(frontend.length()>0 && b!=ob)
    {
        backend = UString(b, UString::UTF_8);
    }
    if(b==ob) return true;
    return false;
}

void ProductMatcher::SetIndexDone_(const std::string& path, bool b)
{
    static const std::string file(path+"/index.done");
    if(b)
    {
        std::ofstream ofs(file.c_str());
        ofs<<"a"<<std::endl;
        ofs.close();
    }
    else
    {
        boost::filesystem::remove_all(file);
    }
}
bool ProductMatcher::IsIndexDone_(const std::string& path)
{
    static const std::string file(path+"/index.done");
    return boost::filesystem::exists(file);
}
void ProductMatcher::Init_()
{
    boost::filesystem::create_directories(path_);
    if(analyzer_==NULL)
    {
        idmlib::util::IDMAnalyzerConfig aconfig = idmlib::util::IDMAnalyzerConfig::GetCommonConfig("",cma_path_, "");
        aconfig.symbol = true;
        analyzer_ = new idmlib::util::IDMAnalyzer(aconfig);
    }
    if(char_analyzer_==NULL)
    {
        idmlib::util::IDMAnalyzerConfig cconfig = idmlib::util::IDMAnalyzerConfig::GetCommonConfig("","", "");
        cconfig.symbol = true;
        char_analyzer_ = new idmlib::util::IDMAnalyzer(cconfig);
    }
    if(chars_analyzer_==NULL)
    {
        idmlib::util::IDMAnalyzerConfig csconfig = idmlib::util::IDMAnalyzerConfig::GetCommonConfig("","", "");
        csconfig.symbol = true;
        chars_analyzer_ = new idmlib::util::IDMAnalyzer(csconfig);
    }
    left_bracket_term_ = GetTerm_(left_bracket_);
    right_bracket_term_ = GetTerm_(right_bracket_);
    place_holder_term_ = GetTerm_(place_holder_);
    blank_term_ = GetTerm_(blank_);
    symbol_terms_.insert(blank_term_);
    symbol_terms_.insert(left_bracket_term_);
    symbol_terms_.insert(right_bracket_term_);

    std::vector<std::string> connect_symbol_strs;
    connect_symbol_strs.push_back("-");
    connect_symbol_strs.push_back("/");
    //connect_symbol_strs.push_back("|");
    connect_symbol_strs.push_back(blank_);

    for(uint32_t i=0;i<connect_symbol_strs.size();i++)
    {
        connect_symbols_.insert(GetTerm_(connect_symbol_strs[i]));
    }
    std::vector<std::string> text_symbol_strs;
    text_symbol_strs.push_back(".");
    text_symbol_strs.push_back("+");
    for(uint32_t i=0;i<text_symbol_strs.size();i++)
    {
        text_symbols_.insert(GetTerm_(text_symbol_strs[i]));
    }
    products_.clear();
    keywords_thirdparty_.clear();
    not_keywords_.clear();
    category_list_.clear();
    cid_to_pids_.clear();
    category_index_.clear();
    product_index_.clear();
    keyword_set_.clear();
    trie_.clear();
    back2front_.clear();
    //nf_.clear();

}
bool ProductMatcher::Index(const std::string& kpath, const std::string& scd_path, int omode)
{
    path_ = kpath;
    if(!boost::filesystem::exists(kpath))
    {
        boost::filesystem::create_directories(kpath);
    }
    std::string rversion = GetVersion(scd_path);
    LOG(INFO)<<"rversion "<<rversion<<std::endl;
    LOG(INFO)<<"aversion "<<AVERSION<<std::endl;
    std::string erversion = GetRVersion(kpath);
    std::string eaversion = GetAVersion(kpath);
    LOG(INFO)<<"erversion "<<erversion<<std::endl;
    LOG(INFO)<<"eaversion "<<eaversion<<std::endl;
    int mode = omode;
    if(AVERSION>eaversion)
    {
        if(mode<2) mode = 2;
    }
    if(mode<2) //not re-training, try open
    {
        if(!Open(path_))
        {
            mode = 2;
        }
        Init_();
    }
    LOG(INFO)<<"mode "<<mode<<std::endl;
    if(mode==3)
    {
        B5MHelper::PrepareEmptyDir(kpath);
    }
    else if(mode==2)
    {
        SetIndexDone_(kpath, false);
        std::string bdb_path = kpath+"/bdb";
        B5MHelper::PrepareEmptyDir(bdb_path);
    }
    if(IsIndexDone_(path_))
    {
        std::cout<<"product trained at "<<path_<<std::endl;
        return true;
    }
    SetIndexDone_(kpath, false);
    Init_();
    std::string keywords_file = scd_path+"/keywords.txt";
    if(boost::filesystem::exists(keywords_file))
    {
        std::ifstream ifs(keywords_file.c_str());
        std::string line;
        while( getline(ifs, line))
        {
            boost::algorithm::trim(line);
            keywords_thirdparty_.push_back(line);
        }
        ifs.close();
    }
    std::string not_keywords_file = scd_path+"/not_keywords.txt";
    if(boost::filesystem::exists(not_keywords_file))
    {
        std::ifstream ifs(not_keywords_file.c_str());
        std::string line;
        while( getline(ifs, line))
        {
            boost::algorithm::trim(line);
            TermList tl;
            GetTerms_(line, tl);
            not_keywords_.insert(tl);
        }
        ifs.close();
    }
    std::string back2front_file= scd_path+"/back2front";
    std::map<std::string, std::string> b2f_map;
    if(boost::filesystem::exists(back2front_file))
    {
        std::ifstream ifs(back2front_file.c_str());
        std::string line;
        while( getline(ifs, line))
        {
            boost::algorithm::trim(line);
            std::vector<std::string> vec;
            boost::algorithm::split(vec, line, boost::algorithm::is_any_of(","));
            if(vec.size()!=2) continue;
            std::pair<std::string, std::string> value(vec[0], vec[1]);
            b2f_map.insert(value);
            back2front_.insert(value);
        }
        ifs.close();
    }
    std::string spu_scd = scd_path+"/SPU.SCD";
    if(!boost::filesystem::exists(spu_scd))
    {
        std::cerr<<"SPU.SCD not exists"<<std::endl;
        return false;
    }
    products_.resize(1);
    category_list_.resize(1);
    {
        std::string category_path = scd_path+"/category";
        std::string line;
        std::ifstream ifs(category_path.c_str());
        while(getline(ifs, line))
        {
            boost::algorithm::trim(line);
            std::vector<std::string> vec;
            boost::algorithm::split(vec, line, boost::algorithm::is_any_of(","));
            if(vec.size()<2) continue;
            uint32_t cid = category_list_.size();
            const std::string& scategory = vec[0];
            if(scategory.empty()) continue;
            //ignore book category
            if(boost::algorithm::starts_with(scategory, B5MHelper::BookCategoryName()))
            {
                continue;
            }
            //const std::string& tag = vec[1];
            Category c;
            c.name = scategory;
            c.cid = cid;
            c.parent_cid = 0;
            c.is_parent = false;
            c.has_spu = false;
            std::set<std::string> akeywords;
            std::set<std::string> rkeywords;
            for(uint32_t i=2;i<vec.size();i++)
            {
                std::string keyword = vec[i];
                bool remove = false;
                if(keyword.empty()) continue;
                if(keyword[0]=='+')
                {
                    keyword = keyword.substr(1);
                }
                else if(keyword[0]=='-')
                {
                    keyword = keyword.substr(1);
                    remove = true;
                }
                if(keyword.empty()) continue;
                if(!remove)
                {
                    akeywords.insert(keyword);
                }
                else
                {
                    rkeywords.insert(keyword);
                }
            }
            std::vector<std::string> cs_list;
            boost::algorithm::split( cs_list, c.name, boost::algorithm::is_any_of(">") );
            c.depth=cs_list.size();
            if(cs_list.empty()) continue;
            std::vector<std::string> keywords_vec;
            boost::algorithm::split( keywords_vec, cs_list.back(), boost::algorithm::is_any_of("/") );
            for(uint32_t i=0;i<keywords_vec.size();i++)
            {
                akeywords.insert(keywords_vec[i]);
            }
            for(std::set<std::string>::const_iterator it = rkeywords.begin();it!=rkeywords.end();it++)
            {
                akeywords.erase(*it);
            }
            for(std::set<std::string>::const_iterator it = akeywords.begin();it!=akeywords.end();it++)
            {
                c.keywords.push_back(*it);
            }
            if(c.depth>1)
            {
                std::string parent_name;
                for(uint32_t i=0;i<c.depth-1;i++)
                {
                    if(!parent_name.empty())
                    {
                        parent_name+=">";
                    }
                    parent_name+=cs_list[i];
                }
                c.parent_cid = category_index_[parent_name];
                category_list_[c.parent_cid].is_parent = true;
                //std::cerr<<"cid "<<c.cid<<std::endl;
                //std::cerr<<"parent cid "<<c.parent_cid<<std::endl;
            }
            category_list_.push_back(c);
            category_index_[c.name] = cid;
        }
        ifs.close();
    }
    std::string bdb_path = path_+"/bdb";
    BrandDb bdb(bdb_path);
    bdb.open();
    ScdParser parser(izenelib::util::UString::UTF_8);
    parser.load(spu_scd);
    uint32_t n=0;
    for( ScdParser::iterator doc_iter = parser.begin();
      doc_iter!= parser.end(); ++doc_iter, ++n)
    {
        //if(n>=15000) break;
        if(n%100000==0)
        {
            LOG(INFO)<<"Find Product Documents "<<n<<std::endl;
        }
        Document doc;
        izenelib::util::UString pid;
        izenelib::util::UString title;
        izenelib::util::UString category;
        izenelib::util::UString attrib_ustr;
        SCDDoc& scddoc = *(*doc_iter);
        SCDDoc::iterator p = scddoc.begin();
        for(; p!=scddoc.end(); ++p)
        {
            const std::string& property_name = p->first;
            doc.property(property_name) = p->second;
            if(property_name=="DOCID")
            {
                pid = p->second;
            }
            else if(property_name=="Title")
            {
                title = p->second;
            }
            else if(property_name=="Category")
            {
                category = p->second;
            }
            else if(property_name=="Attribute")
            {
                attrib_ustr = p->second;
            }
        }
        if(category.length()==0 || attrib_ustr.length()==0 || title.length()==0)
        {
            continue;
        }
        //convert category
        uint32_t cid = 0;
        std::string scategory;
        category.convertString(scategory, UString::UTF_8);
        CategoryIndex::iterator cit = category_index_.find(scategory);
        if(cit==category_index_.end())
        {
            continue;
        }
        else
        {
            cid = cit->second;
            Category& category = category_list_[cid];
            //if(category.is_parent) continue;
            category.has_spu = true;
        }
        double price = 0.0;
        UString uprice;
        if(doc.getProperty("Price", uprice))
        {
            ProductPrice pp;
            pp.Parse(uprice);
            pp.GetMid(price);
        }
        std::string stitle;
        title.convertString(stitle, izenelib::util::UString::UTF_8);
        std::string spid;
        pid.convertString(spid, UString::UTF_8);
        uint128_t ipid = B5MHelper::StringToUint128(spid);
        std::string sattribute;
        attrib_ustr.convertString(sattribute, UString::UTF_8);
        Product product;
        product.spid = spid;
        product.stitle = stitle;
        product.scategory = scategory;
        product.cid = cid;
        product.price = price;
        ParseAttributes(attrib_ustr, product.attributes);
        if(product.attributes.size()<2) continue;
        UString dattribute_ustr;
        doc.getProperty("DAttribute", dattribute_ustr);
        if(!dattribute_ustr.empty())
        {
            ParseAttributes(dattribute_ustr, product.dattributes);
        }
        product.tweight = 0.0;
        product.aweight = 0.0;
        //std::cerr<<"[SPU][Title]"<<stitle<<std::endl;
        boost::unordered_set<std::string> attribute_value_app;
        bool invalid_attribute = false;
        for(uint32_t i=0;i<product.attributes.size();i++)
        {
            const Attribute& attrib = product.attributes[i];
            for(uint32_t a=0;a<attrib.values.size();a++)
            {
                std::string v = attrib.values[a];
                boost::algorithm::to_lower(v);
                if(attribute_value_app.find(v)!=attribute_value_app.end())
                {
                    invalid_attribute = true;
                    break;
                }
                attribute_value_app.insert(v);
            }
            if(invalid_attribute) break;
        }
        if(invalid_attribute)
        {
#ifdef B5M_DEBUG
            //LOG(INFO)<<"invalid SPU attribute "<<spid<<","<<sattribute<<std::endl;
#endif
            continue;
        }
        for(uint32_t i=0;i<product.attributes.size();i++)
        {
            if(product.attributes[i].is_optional)
            {
                //product.aweight+=0.5;
            }
            else if(product.attributes[i].name=="型号")
            {
                product.aweight+=1.5;
            }
            else
            {
                product.aweight+=1.0;
            }

        }
        UString brand;
        std::string sbrand;
        for(uint32_t i=0;i<product.attributes.size();i++)
        {
            if(product.attributes[i].name=="品牌")
            {
                sbrand = product.attributes[i].GetValue();
                brand = UString(sbrand, UString::UTF_8);
                break;
            }
        }
        if(!brand.empty())
        {
            BrandDb::BidType bid = bdb.set(ipid, brand);
            bdb.set_source(brand, bid);
        }
        product.sbrand = sbrand;
        uint32_t spu_id = products_.size();
        products_.push_back(product);
        product_index_[spid] = spu_id;
    }
    bdb.flush();
    //add virtual spus
    for(uint32_t i=1;i<category_list_.size();i++)
    {
        const Category& c = category_list_[i];
        if(!c.has_spu)
        {
            Product p;
            p.scategory = c.name;
            p.cid = i;
            products_.push_back(p);
        }
    }
    cid_to_pids_.resize(category_list_.size());
    for(uint32_t spu_id=1;spu_id<products_.size();spu_id++)
    {
        Product& p = products_[spu_id];
        string_similarity_.Convert(p.stitle, p.title_obj);
        cid_to_pids_[p.cid].push_back(spu_id);
    }
    if(!products_.empty())
    {
        IndexFuzzy_();
        TrieType suffix_trie;
        ConstructSuffixTrie_(suffix_trie);
        ConstructKeywords_();
        LOG(INFO)<<"find "<<keyword_set_.size()<<" keywords"<<std::endl;
        ConstructKeywordTrie_(suffix_trie);
    }
    std::string offer_scd = scd_path+"/OFFER.SCD";
    if(boost::filesystem::exists(offer_scd))
    {
        IndexOffer_(offer_scd);
    }
    std::string path = path_+"/products";
    izenelib::am::ssf::Util<>::Save(path, products_);
    path = path_+"/category_list";
    LOG(INFO)<<"category list size "<<category_list_.size()<<std::endl;
    izenelib::am::ssf::Util<>::Save(path, category_list_);
    //path = path_+"/keywords";
    //izenelib::am::ssf::Util<>::Save(path, keywords_thirdparty_);
    path = path_+"/category_index";
    izenelib::am::ssf::Util<>::Save(path, category_index_);
    path = path_+"/cid_to_pids";
    izenelib::am::ssf::Util<>::Save(path, cid_to_pids_);
    path = path_+"/product_index";
    izenelib::am::ssf::Util<>::Save(path, product_index_);
    path = path_+"/keyword_trie";
    izenelib::am::ssf::Util<>::Save(path, trie_);
    path = path_+"/back2front";
    izenelib::am::ssf::Util<>::Save(path, b2f_map);
    //path = path_+"/nf";
    //std::map<uint64_t, uint32_t> nf_map(nf_.begin(), nf_.end());
    //izenelib::am::ssf::Util<>::Save(path, nf_map);
    {
        std::string aversion_file = path_+"/AVERSION";
        std::ofstream ofs(aversion_file.c_str());
        ofs<<AVERSION<<std::endl;
        ofs.close();
    }
    {
        std::string rversion_file = path_+"/RVERSION";
        std::ofstream ofs(rversion_file.c_str());
        ofs<<rversion<<std::endl;
        ofs.close();
    }
    SetIndexDone_(path_, true);
    
    return true;
}

void ProductMatcher::IndexFuzzy_()
{
}

void ProductMatcher::IndexOffer_(const std::string& offer_scd)
{
    LOG(INFO)<<"index offer begin"<<std::endl;
    ScdParser parser(izenelib::util::UString::UTF_8);
    parser.load(offer_scd);
    uint32_t n=0;
    NgramFrequent nf;
    for( ScdParser::iterator doc_iter = parser.begin();
      doc_iter!= parser.end(); ++doc_iter, ++n)
    {
        //if(n>=15000) break;
        if(n%100000==0)
        {
            LOG(INFO)<<"Find Offer Documents "<<n<<std::endl;
        }
        //Document doc;
        SCDDoc& scddoc = *(*doc_iter);
        SCDDoc::iterator p = scddoc.begin();
        UString title;
        UString category;
        for(; p!=scddoc.end(); ++p)
        {
            const std::string& property_name = p->first;
            if(property_name=="Title")
            {
                title = p->second;
            }
            else if(property_name=="Category")
            {
                category = p->second;
            }
        }
        if(category.empty()||title.empty()) continue;
        std::string scategory;
        category.convertString(scategory, UString::UTF_8);
        CategoryIndex::const_iterator cit = category_index_.find(scategory);;
        if(cit==category_index_.end()) continue;
        uint32_t cid = cit->second;
        OfferCategoryApp app;
        app.cid = cid;
        app.count = 1;
        std::vector<Term> term_list;
        Analyze_(title, term_list);
        KeywordVector keyword_vector;
        GetKeywordVector_(term_list, keyword_vector);
        for(uint32_t i=0;i<keyword_vector.size();i++)
        {
            const TermList& tl = keyword_vector[i].term_list;
            trie_[tl].offer_category_apps.push_back(app);
        }

        if(use_ngram_)
        {
            for(uint32_t i=0;i<keyword_vector.size();i++)
            {
                const KeywordTag& ki = keyword_vector[i];
                for(uint32_t j=i+1;j<keyword_vector.size();j++)
                {
                    const KeywordTag& kj = keyword_vector[j];
                    std::vector<uint32_t> ids(2);
                    ids[0] = ki.id;
                    ids[1] = kj.id;
                    if(ids[1]<ids[0]) std::swap(ids[0], ids[1]);
                    nf[ids].push_back(std::make_pair(cid, 1));
                }
            }
        }
        
    }
    for(TrieType::iterator it = trie_.begin();it!=trie_.end();it++)
    {
        KeywordTag& tag = it->second;
        std::sort(tag.offer_category_apps.begin(), tag.offer_category_apps.end());
        std::vector<OfferCategoryApp> new_apps;
        new_apps.reserve(tag.offer_category_apps.size());
        OfferCategoryApp last;
        last.cid = 0;
        last.count = 0;
        for(uint32_t i=0;i<tag.offer_category_apps.size();i++)
        {
            const OfferCategoryApp& app = tag.offer_category_apps[i];
            if(app.cid!=last.cid)
            {
                if(last.count>0)
                {
                    new_apps.push_back(last);
                }
                last.cid = app.cid;
                last.count = 0;
            }
            last.count+=app.count;
        }
        if(last.count>0)
        {
            new_apps.push_back(last);
        }
        tag.offer_category_apps.swap(new_apps);
    }

    if(use_ngram_)
    {
        uint32_t new_kid = all_keywords_.size();
        for(NgramFrequent::iterator it = nf.begin();it!=nf.end();it++)
        {
            const std::vector<uint32_t>& ids = it->first;
            FrequentValue& value = it->second;
            std::sort(value.begin(), value.end());
            FrequentValue new_value;
            new_value.reserve(value.size());
            for(uint32_t i=0;i<value.size();i++)
            {
                const std::pair<uint32_t, uint32_t>& v = value[i];
                if(new_value.empty())
                {
                    new_value.push_back(v);
                }
                else
                {
                    if(new_value.back().first==v.first)
                    {
                        new_value.back().second+=v.second;
                    }
                    else
                    {
                        new_value.push_back(v);
                    }
                }
            }
            uint32_t freq = 0;
            for(uint32_t i=0;i<new_value.size();i++)
            {
                const std::pair<uint32_t, uint32_t>& v = new_value[i];
                freq += v.second;
            }
            double entropy = 0.0;
            for(uint32_t i=0;i<new_value.size();i++)
            {
                const std::pair<uint32_t, uint32_t>& v = new_value[i];
                double p = (double)v.second/freq;
                entropy += p*std::log(p);
            }
            entropy*=-1.0;
            
            if(freq>=20 && entropy<1.0)
            {
#ifdef B5M_DEBUG
                std::cerr<<"NGRAM";
                for(uint32_t i=0;i<ids.size();i++)
                {
                    uint32_t id = ids[i];
                    const KeywordTag& k = all_keywords_[id];
                    std::cerr<<","<<GetText_(k.term_list);
                }
                std::cerr<<" - "<<freq<<","<<entropy<<std::endl;
#endif

                double rev_entropy = 10.0;
                if(entropy>0.0)
                {
                    rev_entropy = 1.0/entropy;
                }
                if(rev_entropy>10.0) rev_entropy = 10.0;
                double weight = std::log((double)freq)*rev_entropy;
                KeywordTag ngram_tag;
                ngram_tag.id = new_kid++;
                ngram_tag.kweight = weight;
                for(uint32_t i=0;i<ids.size();i++)
                {
                    uint32_t id = ids[i];
                    const KeywordTag& k = all_keywords_[id];
                    ngram_tag.term_list.push_back(0);
                    ngram_tag.term_list.insert(ngram_tag.term_list.end(), k.term_list.begin(), k.term_list.end());

                }
                for(uint32_t i=0;i<new_value.size();i++)
                {
                    const std::pair<uint32_t, uint32_t>& v = new_value[i];
                    OfferCategoryApp app;
                    app.cid = v.first;
                    app.count = v.second;
                    ngram_tag.offer_category_apps.push_back(app);
                }
                trie_[ngram_tag.term_list] = ngram_tag;

            }
        }
        LOG(INFO)<<"find ngram keyword "<<new_kid-all_keywords_.size()<<std::endl;
    }
    LOG(INFO)<<"index offer end"<<std::endl;
}


bool ProductMatcher::DoMatch(const std::string& scd_path, const std::string& output_file)
{
    izenelib::util::ClockTimer clocker;
    std::vector<std::string> scd_list;
    B5MHelper::GetIUScdList(scd_path, scd_list);
    if(scd_list.empty()) return false;
    std::string match_file = output_file;
    if(match_file.empty())
    {
        match_file = path_+"/match";
    }
    std::ofstream ofs(match_file.c_str());
    std::size_t doc_count=0;
    std::size_t spu_matched_count=0;
    for(uint32_t i=0;i<scd_list.size();i++)
    {
        std::string scd_file = scd_list[i];
        LOG(INFO)<<"Processing "<<scd_file<<std::endl;
        ScdParser parser(izenelib::util::UString::UTF_8);
        parser.load(scd_file);
        uint32_t n=0;
        for( ScdParser::iterator doc_iter = parser.begin();
          doc_iter!= parser.end(); ++doc_iter, ++n)
        {
            if(n%1000==0)
            {
                LOG(INFO)<<"Find Offer Documents "<<n<<std::endl;
            }
            if(n==150000) break;
            SCDDoc& scddoc = *(*doc_iter);
            SCDDoc::iterator p = scddoc.begin();
            Document doc;
            for(; p!=scddoc.end(); ++p)
            {
                const std::string& property_name = p->first;
                doc.property(property_name) = p->second;
            }
            Product result_product;
            Process(doc, result_product);
            doc_count++;
            std::string spid = result_product.spid;
            std::string sptitle = result_product.stitle;
            std::string scategory = result_product.scategory;
            std::string soid;
            std::string stitle;
            doc.getString("DOCID", soid);
            doc.getString("Title", stitle);
            if(spid.length()>0)
            {
                spu_matched_count++;
                ofs<<soid<<","<<spid<<","<<stitle<<","<<sptitle<<","<<scategory<<std::endl;
            }
            else
            {
                ofs<<soid<<","<<stitle<<","<<scategory<<std::endl;
            }
            //Product product;
            //if(GetMatched(doc, product))
            //{
                //UString oid;
                //doc.getProperty("DOCID", oid);
                //std::string soid;
                //oid.convertString(soid, izenelib::util::UString::UTF_8);
                //ofs<<soid<<","<<product.spid;
                //boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
                //ofs<<","<<boost::posix_time::to_iso_string(now);
                //std::string stitle;
                //title.convertString(stitle, UString::UTF_8);
                //ofs<<","<<stitle<<","<<product.stitle<<std::endl;
            //}
            //else
            //{
                ////std::cerr<<"not got pid"<<std::endl;
            //}
        }
    }
    ofs.close();
    LOG(INFO)<<"clocker used "<<clocker.elapsed()<<std::endl;
    LOG(INFO)<<"stat: doc_count:"<<doc_count<<", spu_matched_count:"<<spu_matched_count<<std::endl;
    return true;
}

void ProductMatcher::Test(const std::string& scd_path)
{
    //{
        //std::ifstream ifs(scd_path.c_str());
        //ScdWriter writer(".", INSERT_SCD);
        //std::string line;
        //while(getline(ifs, line))
        //{
            //boost::algorithm::trim(line);
            //Document doc;
            //doc.property("DOCID") = UString("00000000000000000000000000000000", UString::UTF_8);
            //doc.property("Title") = UString(line, UString::UTF_8);
            //SetUsePriceSim(false);
            //Product result_product;
            //Process(doc, result_product);
            //doc.property("Category") = UString(result_product.scategory, UString::UTF_8);
            //writer.Append(doc);
            //std::cerr<<result_product.scategory<<std::endl;
            
        //}
        //writer.Close();
        //return;
    //}
    std::vector<std::pair<std::string, std::string> > process_list;
    bfs::path d(scd_path);
    if(!bfs::is_directory(d)) return;
    std::vector<std::string> scd_list;
    B5MHelper::GetIUScdList(scd_path, scd_list);
    if(scd_list.size()==1)
    {
        process_list.push_back(std::make_pair(d.filename().string(), scd_list.front()));
    }
    bfs::path p(scd_path);
    bfs::directory_iterator end;
    for(bfs::directory_iterator it(p);it!=end;it++)
    {
        bfs::path d = it->path();
        if(bfs::is_directory(d))
        {
            std::string test_dir = d.string();
            std::string test_name = d.filename().string();
            scd_list.clear();
            B5MHelper::GetIUScdList(test_dir, scd_list);
            if(scd_list.size()==1)
            {
                process_list.push_back(std::make_pair(test_name, scd_list.front()));
            }
        }
    }
    if(process_list.empty()) return;
    boost::posix_time::ptime now(boost::posix_time::microsec_clock::local_time());
    std::string ios_str = boost::posix_time::to_iso_string(now);
    std::string ts;
    ts += ios_str.substr(0,8);
    ts += ios_str.substr(9,6);
    std::string output_dir = "./matcher_test_result";
    boost::filesystem::create_directories(output_dir);
    std::string output_file = output_dir+"/"+ts+".json";
    std::ofstream ofs(output_file.c_str());
    for(uint32_t i=0;i<process_list.size();i++)
    {
        izenelib::util::ClockTimer clocker;
        uint32_t allc = 0;
        uint32_t correctc = 0;
        uint32_t allp = 0;
        uint32_t correctp = 0;
        //std::ofstream ofs("./brand.txt");
        //for(uint32_t i=0;i<products_.size();i++)
        //{
            //Product& p = products_[i];
            //std::vector<Attribute>& attributes = p.attributes;
            //for(uint32_t a=0;a<attributes.size();a++)
            //{
                //Attribute& attribute = attributes[a];
                //if(attribute.name=="品牌")
                //{
                    //std::string value;
                    //for(uint32_t v=0;v<attribute.values.size();v++)
                    //{
                        //if(!value.empty()) value+="/";
                        //value+=attribute.values[v];
                    //}
                    //ofs<<value<<std::endl;
                //}
            //}
        //}
        //ofs.close();
        std::string test_name = process_list[i].first;
        std::string scd_file = process_list[i].second;
        LOG(INFO)<<"Processing "<<scd_file<<std::endl;
        ScdParser parser(izenelib::util::UString::UTF_8);
        parser.load(scd_file);
        uint32_t n=0;
        for( ScdParser::iterator doc_iter = parser.begin();
          doc_iter!= parser.end(); ++doc_iter, ++n)
        {
            if(n%1000==0)
            {
                LOG(INFO)<<"Find Offer Documents "<<n<<std::endl;
            }
            SCDDoc& scddoc = *(*doc_iter);
            SCDDoc::iterator p = scddoc.begin();
            Document doc;
            for(; p!=scddoc.end(); ++p)
            {
                const std::string& property_name = p->first;
                doc.property(property_name) = p->second;
            }
            if(doc.hasProperty("Price"))
            {
                SetUsePriceSim(true);
            }
            else
            {
                SetUsePriceSim(false);
            }
            bool has_category = false;
            bool has_pid = false;
            if(doc.hasProperty("Category"))
            {
                has_category = true;
            }
            if(doc.hasProperty("uuid"))
            {
                has_pid = true;
            }
            std::string sdocid;
            doc.getString("DOCID", sdocid);
            std::string ecategory;
            doc.getString("Category", ecategory);
            std::string stitle;
            doc.getString("Title", stitle);
            std::string epid;
            doc.getString("uuid", epid);
            Product result_product;
            doc.eraseProperty("Category");
            if(!has_category&&!has_pid)
            {
                uint32_t limit = 3;
                std::vector<UString> frontends;
                UString title(stitle, UString::UTF_8);
                GetFrontendCategory(title, limit, frontends);
                std::cerr<<"frontend results of "<<stitle<<": ";
                for(uint32_t i=0;i<frontends.size();i++)
                {
                    std::string str;
                    frontends[i].convertString(str, UString::UTF_8);
                    std::cerr<<str<<",";
                }
                std::cerr<<std::endl;
            }
            else
            {
                Process(doc, result_product);
                //LOG(INFO)<<"categorized "<<stitle<<","<<result_product.scategory<<std::endl;
                if(has_category)
                {
                    allc++;
                    if(boost::algorithm::starts_with(result_product.scategory,ecategory))
                    {
                        correctc++;
                    }
                    else
                    {
                        LOG(ERROR)<<"category error : "<<sdocid<<","<<stitle<<","<<ecategory<<","<<result_product.scategory<<std::endl;
                    }
                }
                if(has_pid)
                {
                    allp++;
                    if(result_product.spid==epid) 
                    {
                        correctp++;
                    }
                    else
                    {
                        LOG(ERROR)<<"spu error : "<<sdocid<<","<<stitle<<","<<epid<<","<<result_product.spid<<std::endl;
                    }
                }
            }
        }
        double ratioc = (double)correctc/allc;
        double ratiop = (double)correctp/allp;
        LOG(INFO)<<"stat : "<<allc<<","<<correctc<<","<<allp<<","<<correctp<<std::endl;
        double time_used = clocker.elapsed();
        LOG(INFO)<<"clocker used "<<clocker.elapsed()<<std::endl;
        Json::Value json_value;
        json_value["name"] = test_name;
        json_value["allc"] = Json::Value::UInt(allc);
        json_value["correctc"] = Json::Value::UInt(correctc);
        json_value["allp"] = Json::Value::UInt(allp);
        json_value["correctp"] = Json::Value::UInt(correctp);
        json_value["ratioc"] = ratioc;
        json_value["ratiop"] = ratiop;
        json_value["time"] = time_used;
        Json::FastWriter writer;
        std::string str_value = writer.write(json_value);
        boost::algorithm::trim(str_value);
        ofs<<str_value<<std::endl;
    }
    ofs.close();
}
bool ProductMatcher::GetIsbnAttribute(const Document& doc, std::string& isbn_value)
{
    const static std::string isbn_name = "isbn";
    UString attrib_ustr;
    doc.getProperty("Attribute", attrib_ustr);
    std::vector<Attribute> attributes;
    ParseAttributes(attrib_ustr, attributes);
    for(uint32_t i=0;i<attributes.size();i++)
    {
        const Attribute& a = attributes[i];
        std::string aname = a.name;
        boost::algorithm::trim(aname);
        boost::to_lower(aname);
        if(aname==isbn_name)
        {
            if(!a.values.empty())
            {
                isbn_value = a.values[0];
                boost::algorithm::replace_all(isbn_value, "-", "");
            }
            break;
        }
    }
    if(!isbn_value.empty()) return true;
    return false;

}

bool ProductMatcher::ProcessBook(const Document& doc, Product& result_product)
{
    std::string scategory;
    doc.getString("Category", scategory);
    if(boost::algorithm::starts_with(scategory, B5MHelper::BookCategoryName()))
    {
        std::string isbn_value;
        GetIsbnAttribute(doc, isbn_value);
        if(!isbn_value.empty())
        {
            result_product.spid = B5MHelper::GetPidByIsbn(isbn_value);
        }
        return true;
    }
    else if(scategory.empty())
    {
        std::string isbn_value;
        GetIsbnAttribute(doc, isbn_value);
        if(!isbn_value.empty())
        {
            result_product.spid = B5MHelper::GetPidByIsbn(isbn_value);
            result_product.scategory = B5MHelper::BookCategoryName();
            return true;
        }
    }
    return false;
}

bool ProductMatcher::Process(const Document& doc, Product& result_product)
{
    static const uint32_t limit = 1;
    std::vector<Product> products;
    if(Process(doc, limit, products) && !products.empty())
    {
        result_product = products.front();
        return true;
    }
    return false;
}

bool ProductMatcher::Process(const Document& doc, uint32_t limit, std::vector<Product>& result_products)
{
    if(!IsOpen()) return false;
    if(limit==0) return false;
    Product pbook;
    if(ProcessBook(doc, pbook))
    {
        result_products.resize(1, pbook);
        return true;
    }
    izenelib::util::UString title;
    izenelib::util::UString category;
    doc.getProperty("Category", category);
    doc.getProperty("Title", title);
    
    if(title.length()==0)
    {
        return false;
    }
    //keyword_vector_.resize(0);
    //std::cout<<"[TITLE]"<<stitle<<std::endl;
    std::vector<Term> term_list;
    Analyze_(title, term_list);
    KeywordVector keyword_vector;
    GetKeywordVector_(term_list, keyword_vector);
    Compute_(doc, term_list, keyword_vector, limit, result_products);
    for(uint32_t i=0;i<result_products.size();i++)
    {
        //gen frontend category
        Product& p = result_products[i];
        UString frontend;
        UString backend(p.scategory, UString::UTF_8);
        GetFrontendCategory(backend, frontend);
        backend.convertString(p.scategory, UString::UTF_8);
        frontend.convertString(p.fcategory, UString::UTF_8);
    }
    return true;
}

void ProductMatcher::GetFrontendCategory(const UString& text, uint32_t limit, std::vector<UString>& results)
{
    if(!IsOpen()) return;
    if(limit==0) return;
    if(text.length()==0) return;
    Document doc;
    doc.property("Title") = text;
    const UString& title = text;
    
    std::vector<Term> term_list;
    Analyze_(title, term_list);
    KeywordVector keyword_vector;
    GetKeywordVector_(term_list, keyword_vector);
    uint32_t flimit = limit*2;
    std::vector<Product> result_products;
    Compute_(doc, term_list, keyword_vector, flimit, result_products);
    for(uint32_t i=0;i<result_products.size();i++)
    {
        Product& p = result_products[i];
        UString frontend;
        UString backend(p.scategory, UString::UTF_8);
        GetFrontendCategory(backend, frontend);
        //std::cerr<<"find backend "<<p.scategory<<std::endl;
        if(frontend.length()==0)
        {
            p.score = 0.0;
            continue;
        }
        frontend.convertString(p.fcategory, UString::UTF_8);
        //std::cerr<<"find frontend "<<p.fcategory<<std::endl;
    }
    std::stable_sort(result_products.begin(), result_products.end(), Product::FCategoryCompare);

    for(uint32_t i=0;i<result_products.size();i++)
    {
        Product& pi = result_products[i];
        if(pi.score==0.0) continue;
        for(uint32_t j=i+1;j<result_products.size();j++)
        {
            Product& pj = result_products[j];
            if(pj.score==0.0) continue;
            if(pi.fcategory==pj.fcategory)
            {
                pj.score = 0.0;
            }
            else if(boost::algorithm::starts_with(pj.fcategory, pi.fcategory))
            {
                if(pi.score>=pj.score*0.85)
                {
                    pj.score = 0.0;
                }
                else
                {
                    pi.score = 0.0;
                }
            }
        }
    }
    std::sort(result_products.begin(), result_products.end(), Product::ScoreCompare);
    for(uint32_t i=0;i<result_products.size();i++)
    {
        Product& p = result_products[i];
        if(p.score>0.0)
        {
            UString frontend(p.fcategory, UString::UTF_8);
            results.push_back(frontend);
            if(results.size()>=limit) break;
        }
    }
    //typedef std::map<std::string, double> FrontApp;
    //boost::unordered_set<std::string> front_app;
    //FrontApp front_app;
    //for(uint32_t i=0;i<result_products.size();i++)
    //{
        //Product& p = result_products[i];
        //UString frontend;
        //UString backend(p.scategory, UString::UTF_8);
        //GetFrontendCategory(backend, frontend);
        //if(frontend.length()==0) continue;
        ////backend.convertString(p.scategory, UString::UTF_8);
        //frontend.convertString(p.fcategory, UString::UTF_8);
        //CategoryDepthCut(p.fcategory, category_max_depth_);
        //FrontApp::iterator it = front_app.lower_bound(p.fcategory);
        //if(it==front_app.end())
        //{
            //front_app.insert(std::make_pair(p.fcategory, p.score));
        //}
        //else
        //{
            //if(it->first==p.fcategory) //duplicate
            //{
                ////ignore
            //}
            //else
            //{
                ////is child
            //}
        //}
        //if(front_app.find(p.fcategory)!=front_app.end()) continue;
        //results.push_back(frontend);
        //front_app.insert(p.fcategory);
        //if(results.size()==limit) break;
    //}
}

void ProductMatcher::GetKeywordVector_(const std::vector<Term>& term_list, KeywordVector& keyword_vector)
{
    //std::string stitle;
    //text.convertString(stitle, UString::UTF_8);
    uint32_t begin = 0;
    uint8_t bracket_depth = 0;
    keyword_vector.reserve(10);
    //typedef boost::unordered_map<TermList, uint32_t> KeywordIndex;
    //typedef KeywordVector::value_type ItemType;
    //typedef std::pair<double, std::string> ItemScore;
    //KeywordIndex keyword_index;
    //KeywordAppMap app_map;
    while(begin<term_list.size())
    {
        //if(bracket_depth>0)
        //{
            //begin++;
            //continue;
        //}
        //bool found_keyword = false;
        //typedef std::vector<ItemType> Buffer;
        //Buffer buffer;
        //buffer.reserve(5);
        //std::pair<TrieType::key_type, TrieType::mapped_type> value;
        KeywordTag found_keyword;
        uint32_t keyword_fol_position = 0;
        std::vector<term_t> candidate_keyword;
        std::vector<Term> candidate_terms;
        candidate_keyword.reserve(10);
        candidate_terms.reserve(10);
        uint32_t next_pos = begin;
        Term last_term;
        //std::cerr<<"[BEGIN]"<<begin<<std::endl;
        while(true)
        {
            if(next_pos>=term_list.size()) break;
            const Term& term = term_list[next_pos++];
            //std::string sterm = GetText_(term);
            //std::cerr<<"[A]"<<sterm<<std::endl;
            if(IsSymbol_(term.id))
            {
                if(IsConnectSymbol_(term.id))
                {
                    if(candidate_keyword.empty())
                    {
                        //std::cerr<<"[B0]"<<sterm<<","<<term<<std::endl;
                        break;
                    }
                    else
                    {
                        last_term = term;
                        //std::cerr<<"[B]"<<sterm<<","<<term<<std::endl;
                        continue;
                    }
                }
                else
                {
                    if(term.id==left_bracket_term_||term.id==right_bracket_term_)
                    {
                        if(next_pos-begin==1)
                        {
                            if(term.id==left_bracket_term_)
                            {
                                bracket_depth++;
                            }
                            else if(term.id==right_bracket_term_)
                            {
                                if(bracket_depth>0)
                                {
                                    bracket_depth--;
                                }
                            }
                        }
                    }
                    //std::cerr<<"[C]"<<sterm<<","<<term<<std::endl;
                    break;
                }
            }
            else
            {
                if(last_term.id==blank_term_&&!candidate_keyword.empty())
                {
                    const Term& last_candidate_term = candidate_terms.back();
                    //UString last_candidate = UString(GetText_(last_candidate_term), UString::UTF_8);
                    //UString this_text = UString(GetText_(term), UString::UTF_8);
                    if(IsBlankSplit_(last_candidate_term.text, term.text))
                    {
                        break;
                    }
                    //std::cerr<<"[D]"<<sterm<<","<<term<<std::endl;
                }
            }
            //std::cerr<<"[E]"<<sterm<<","<<term<<std::endl;
            candidate_keyword.push_back(term.id);
            candidate_terms.push_back(term);
            //std::string skeyword = GetText_(candidate_keyword);
            //std::cerr<<"[SK]"<<skeyword<<std::endl;
            TrieType::const_iterator it = trie_.lower_bound(candidate_keyword);
            if(it!=trie_.end())
            {
                if(candidate_keyword==it->first)
                {
                    //ItemType item;
                    //item.first = it->first;
                    //item.second = it->second;
                    KeywordTag tag = it->second;
                    tag.ngram = 1;
                    tag.kweight = 1.0;
                    tag.positions.push_back(std::make_pair(begin, next_pos));
                    if(bracket_depth>0) tag.kweight=0.1;
                    bool need_append = true;
                    for(uint32_t i=0;i<keyword_vector.size();i++)
                    {
                        KeywordTag& ek = keyword_vector[i];
                        if(tag.id==ek.id) //duplidate
                        {
                            if(tag.kweight>ek.kweight)
                            {
                                ek.kweight = tag.kweight;
                            }
                            need_append = false;
                            break;
                        }
                    }
                    if(need_append)
                    {
                        Term pre_term;
                        Term fol_term;
                        if(begin>0)
                        {
                            pre_term = term_list[begin-1];
                        }
                        if(next_pos<term_list.size())
                        {
                            fol_term = term_list[next_pos];
                        }
                        if(pre_term.TextString()=="." || fol_term.TextString()==".")
                        {
                            //std::cerr<<"[F]"<<pre_str<<","<<fol_str<<std::endl;
                            need_append = false;
                        }
                    }
                    //for(uint32_t i=0;i<keyword_vector.size();i++)
                    //{
                        //int select = SelectKeyword_(tag, keyword_vector[i]);
                        //if(select==1)
                        //{
                            //keyword_vector[i] = tag;
                            //need_append = false;
                            //break;
                        //}
                        //else if(select==2)
                        //{
                            //need_append = false;
                            //break;
                        //}
                        //else if(select==3)
                        //{
                            ////keyword_vector.push_back(tag);
                        //}
                    //}
                    if(need_append)
                    {
                        //keyword_vector.push_back(tag);
                        //std::cerr<<"[K]"<<sterm<<","<<term<<std::endl;
                        found_keyword = tag;
                        keyword_fol_position = next_pos;
                    }
                }
                else if(StartsWith_(it->first, candidate_keyword))
                {
                    //continue trying
                }
                else
                {
                    break;
                }
            }
            else
            {
                break;
            }
            last_term = term;
        }
        if(found_keyword.term_list.size()>0)
        {
            keyword_vector.push_back(found_keyword);
            begin = keyword_fol_position;
        }
        else
        {
            //begin = next_pos;
            ++begin;
        }
        //begin = next_pos;
    }

    if(use_ngram_)
    {
        uint32_t keyword_count = keyword_vector.size();
        std::vector<KeywordTag> ngram_keywords;
        for(uint32_t i=0;i<keyword_count;i++)
        {
            const KeywordTag& ki = keyword_vector[i];
            for(uint32_t j=i+1;j<keyword_count;j++)
            {
                const KeywordTag& kj = keyword_vector[j];
                std::vector<uint32_t> ngram_term_list;
                ngram_term_list.reserve(ki.term_list.size()+kj.term_list.size()+2);
                ngram_term_list.push_back(0);
                const KeywordTag* ks = &ki;
                const KeywordTag* kl = &kj;
                if(ki.id>kj.id)
                {
                    ks = &kj;
                    kl = &ki;
                }
                ngram_term_list.insert(ngram_term_list.end(), ks->term_list.begin(), ks->term_list.end());
                ngram_term_list.push_back(0);
                ngram_term_list.insert(ngram_term_list.end(), kl->term_list.begin(), kl->term_list.end());
                TrieType::const_iterator it = trie_.find(ngram_term_list);
                if(it!=trie_.end())
                {
                    KeywordTag ngramk = it->second;
                    ngramk.ngram = 2;
                    ngram_keywords.push_back(ngramk);
                }

            }
        }
        std::sort(ngram_keywords.begin(), ngram_keywords.end(), KeywordTag::WeightCompare);
        std::size_t ngram_count = std::min(2ul, ngram_keywords.size());
        for(std::size_t i=0;i<ngram_count;i++)
        {
            KeywordTag& k = ngram_keywords[i];
            k.kweight = 1.0;
            keyword_vector.push_back(k);
        }
    }
    
}

uint32_t ProductMatcher::GetCidBySpuId_(uint32_t spu_id)
{
    //LOG(INFO)<<"spu_id:"<<spu_id<<std::endl;
    const Product& product = products_[spu_id];
    const std::string& scategory = product.scategory;
    return category_index_[scategory];
}

uint32_t ProductMatcher::GetCidByMaxDepth_(uint32_t cid)
{
    while(true)
    {
        const Category& c = category_list_[cid];
        if(c.depth<=category_max_depth_)
        {
            return cid;
        }
        cid = c.parent_cid;
    }
    return 0;
}

bool ProductMatcher::EqualOrIsParent_(uint32_t parent, uint32_t child) const
{
    uint32_t cid = child;
    while(true)
    {
        if(cid==0) return false;
        if(cid==parent) return true;
        const Category& c = category_list_[cid];
        cid = c.parent_cid;
    }
    return false;
}

void ProductMatcher::Compute_(const Document& doc, const std::vector<Term>& term_list, KeywordVector& keyword_vector, uint32_t limit, std::vector<Product>& result_products)
{
    UString title;
    doc.getProperty("Title", title);
    std::string stitle;
    title.convertString(stitle, UString::UTF_8);
#ifdef B5M_DEBUG
    std::cout<<"[TITLE]"<<stitle<<std::endl;
#endif
    double price = 0.0;
    UString uprice;
    if(doc.getProperty("Price", uprice))
    {
        ProductPrice pp;
        pp.Parse(uprice);
        pp.GetMid(price);
    }
    SimObject title_obj;
    string_similarity_.Convert(title, title_obj);
    bool matcher_only = matcher_only_;
    uint32_t given_cid = 0;
    UString given_category;
    doc.getProperty("Category", given_category);
    std::string sgiven_category;
    given_category.convertString(sgiven_category, UString::UTF_8);
    if(!given_category.empty())
    {
        CategoryIndex::const_iterator it = category_index_.find(sgiven_category);
        if(it!=category_index_.end())
        {
            given_cid = it->second;
        }
    }
    else
    {
        matcher_only = false;
    }
    if(matcher_only && given_cid==0) return;
    //std::string soid;
    //doc.getString("DOCID", soid);
    //std::cerr<<soid<<",matcher only:"<<matcher_only<<std::endl;
    //firstly do cid;
    typedef dmap<uint32_t, double> IdToScore;
    typedef dmap<uint32_t, WeightType> IdToWeight;
    //typedef std::vector<double> ScoreList;
    //typedef dmap<uint32_t, ScoreList> IdToScoreList;
    //typedef std::map<uint32_t, ProductCandidate> PidCandidates;
    typedef dmap<uint32_t, std::pair<WeightType, uint32_t> > CidSpu;
    //ScoreList max_share_point(3,0.0);
    //max_share_point[0] = 2.0;
    //max_share_point[1] = 1.0;
    //max_share_point[2] = 1.0;
    //IdToScore cid_candidates(0.0);
    IdToWeight pid_weight;
    IdToScore cid_score(0.0);
    CidSpu cid_spu(std::make_pair(WeightType(), 0));
    //PidCandidates pid_candidates;
    //ScoreList default_score_list(3, 0.0);
    //IdToScoreList cid_share_list(default_score_list);
    //uint32_t keyword_count = keyword_vector.size();
    typedef boost::unordered_set<std::pair<uint32_t, std::string> > SpuAttributeApp;
    SpuAttributeApp sa_app;
    std::size_t text_term_len = 0;
    for(uint32_t i=0;i<term_list.size();i++)
    {
        if(!IsSymbol_(term_list[i].id))
        {
            ++text_term_len;
        }
    }
    for(KeywordVector::const_iterator it = keyword_vector.begin();it!=keyword_vector.end();++it)
    {
        const TermList& tl = it->term_list;
        const KeywordTag& tag = *it;
        double kweight = tag.kweight;
        //double aweight = tag.aweight;
        double terms_length = tl.size()-(tag.ngram-1);
        double len_weight = terms_length/text_term_len;
        //double count_weight = 1.0/keyword_count;
#ifdef B5M_DEBUG
        std::string text = GetText_(tl);
        std::cout<<"[KEYWORD]"<<text<<","<<kweight<<","<<std::endl;
#endif
        
        for(uint32_t i=0;i<tag.category_name_apps.size();i++)
        {
            const CategoryNameApp& app = tag.category_name_apps[i];
            if(matcher_only)
            {
                if(!EqualOrIsParent_(given_cid, app.cid)) continue;
            }
            double depth_ratio = 1.0-0.2*(app.depth-1);
            double times = 1.0;
            //if(count_weight>=0.8) times = 2.5;
            //else if(count_weight>=0.5) times = 2.0;
            double share_point = times*depth_ratio;
            if(!app.is_complete) share_point*=0.6;
            double add_score = share_point*kweight;
            cid_score[app.cid] += add_score;
#ifdef B5M_DEBUG
            //std::cerr<<"[CNA]"<<category_list_[app.cid].name<<","<<app.depth<<","<<app.is_complete<<","<<add_score<<std::endl;
#endif
        }
        for(uint32_t i=0;i<tag.offer_category_apps.size();i++)
        {
            const OfferCategoryApp& app = tag.offer_category_apps[i];
            if(matcher_only)
            {
                if(!EqualOrIsParent_(given_cid, app.cid)) continue;
            }
            uint32_t app_count = app.count;
            if(app_count>20) app_count = 20;
            double depth_ratio = 1.0;
            double times = app_count*0.04;
            //if(count_weight>=0.8) times *= 2.5;
            //else if(count_weight>=0.5) times *= 2.0;
            double share_point = times*depth_ratio;
            double add_score = share_point*kweight;
            cid_score[app.cid] += add_score;
#ifdef B5M_DEBUG
            //std::cerr<<"[CNO]"<<category_list_[app.cid].name<<","<<add_score<<std::endl;
#endif
        }
        for(uint32_t i=0;i<tag.attribute_apps.size();i++)
        {
            const AttributeApp& app = tag.attribute_apps[i];
            if(app.spu_id==0) continue;
            //if(app.is_optional) continue;
            const Product& p = products_[app.spu_id];
            if(matcher_only)
            {
                if(!EqualOrIsParent_(given_cid, p.cid)) continue;
            }
            double psim = PriceSim_(price, p.price);
            if(psim<0.25) continue;
            std::pair<uint32_t, std::string> sa_app_value(app.spu_id, app.attribute_name);
            if(sa_app.find(sa_app_value)!=sa_app.end()) continue;
            double share_point = 0.0;
            double p_point = 0.0;
            if(app.is_optional) 
            {
                share_point = 0.1;
                p_point = 0.1;
            }
            else if(app.attribute_name=="型号") 
            {
                share_point = 0.3;
                p_point = 1.5;
            }
            else 
            {
                share_point = 0.2;
                p_point = 1.0;
            }
            WeightType& wt = pid_weight[app.spu_id];
            wt.aweight+=share_point*kweight;
            double paweight = p_point;
            wt.paweight+=paweight;
            wt.paratio+=len_weight;
            if(app.attribute_name=="型号") wt.type_match = true;
            if(app.attribute_name=="品牌") wt.brand_match = true;
            if(p.price==0.0)
            {
                wt.price_diff = 99999999.0; //set to a huge value
            }
            else
            {
                wt.price_diff = std::fabs(price-p.price);
            }
            sa_app.insert(sa_app_value);
#ifdef B5M_DEBUG
            //std::cerr<<"[AN]"<<category_list_[p.cid].name<<","<<share_point*kweight<<std::endl;
#endif
        }
        //for(uint32_t i=0;i<tag.spu_title_apps.size();i++)
        //{
            //const SpuTitleApp& app = tag.spu_title_apps[i];
            //if(app.spu_id==0) continue;
//#ifdef B5M_DEBUG
            ////std::cout<<"[TS]"<<products_[app.spu_id].stitle<<std::endl;
//#endif
            //const Product& p = products_[app.spu_id];
            //if(matcher_only)
            //{
                //if(!EqualOrIsParent_(given_cid, p.cid)) continue;
            //}
            //double psim = PriceSim_(price, p.price);
            //pid_weight[app.spu_id].tweight+=0.2*weight*psim;
        //}
        
    }
    //return;
    for(IdToWeight::const_iterator it = pid_weight.begin();it!=pid_weight.end();it++)
    {
        uint32_t spu_id = it->first;
        const Product& p = products_[spu_id];
        const WeightType& weight = it->second;
        uint32_t cid = p.cid;
        std::pair<WeightType, uint32_t>& e_weight_spuid = cid_spu[cid];
        WeightType& eweight = e_weight_spuid.first;
        uint32_t e_spu_id = e_weight_spuid.second;
        const Product& ep = products_[e_spu_id];
        bool ematched = SpuMatched_(eweight, ep);
        
        bool matched = SpuMatched_(weight, p);
#ifdef B5M_DEBUG
        //std::cerr<<category_list_[cid].name<<","<<p.stitle<<","<<ematched<<","<<matched<<","<<eweight.sum()<<","<<weight.sum()<<std::endl;
#endif
        if(ematched&&matched)
        {
            bool replace = false;
            if(weight.paweight>eweight.paweight) replace = true;
            else if(weight.paweight==eweight.paweight)
            {
                if(eweight.price_diff!=weight.price_diff)
                {
                    if(weight.price_diff<eweight.price_diff) replace = true;
                }
                else
                {
                    if(weight.sum()>eweight.sum())
                    {
                        replace = true;
                    }
                }
            }
            if(replace)
            {
                e_weight_spuid.first = weight;
                e_weight_spuid.second = spu_id;
            }
        }
        else if(!ematched&&matched)
        {
            if(weight.sum()>eweight.sum()*0.7)
            {
                e_weight_spuid.first = weight;
                e_weight_spuid.second = spu_id;
            }
        }
        else if(!ematched&&!matched)
        {
            if(weight.sum()>=eweight.sum())
            {
                e_weight_spuid.second = spu_id;
                e_weight_spuid.first = weight;
            }
        }
    }
    for(CidSpu::iterator it = cid_spu.begin();it!=cid_spu.end();it++)
    {
        uint32_t cid = it->first;
        std::pair<WeightType, uint32_t>& e = it->second;
        IdToScore::const_iterator sit = cid_score.find(cid);
        if(sit!=cid_score.end())
        {
            e.first.cweight+=sit->second;
            cid_score.erase(sit);
        }

    }
    for(IdToScore::const_iterator it = cid_score.begin();it!=cid_score.end();it++)
    {
        uint32_t cid = it->first;
        double score = it->second;
        const IdList& pid_list = cid_to_pids_[cid];
        if(!pid_list.empty())
        {
            uint32_t spu_id = pid_list.front();
            std::pair<WeightType, uint32_t>& e = cid_spu[cid];
            e.first.cweight+=score;
            e.second = spu_id;
        }
    }
    std::vector<ResultVectorItem> result_vector;
    result_vector.reserve(cid_spu.size());
    double max_score = 0.0;
    for(CidSpu::iterator it = cid_spu.begin();it!=cid_spu.end();++it)
    {
        uint32_t cid = it->first;
        uint32_t spu_id = it->second.second;
        //const Product& p = products_[spu_id];
        //const Category& c = category_list_[cid];
        WeightType& weight = it->second.first;
        //post-process on weight

        //weight.tweight*=string_similarity_.Sim(title_obj, p.title_obj);
        //if(weight.tweight>0.0&&p.tweight>0.0)
        //{
            //weight.tweight/=p.tweight;
        //}
        //if(weight.cweight>0.0)
        //{
            //weight.cweight*=std::log(10.0-c.depth);
        //}
        //else
        //{
            ////weight.kweight*=std::log(8.0+keyword_count-c.depth);
        //}
        //weight.kweight*=std::log(28.0+keyword_count-c.depth);
        ResultVectorItem item;
        item.cid = cid;
        item.spu_id = spu_id;
        item.score = weight.sum();
        item.weight = weight;
        item.is_given_category = false;
        if(item.score>max_score) max_score = item.score;
        result_vector.push_back(item);
#ifdef B5M_DEBUG
        //std::cerr<<"[CSC]"<<category_list_[cid].name<<","<<weight<<std::endl;
#endif
    }
    for(uint32_t i=0;i<result_vector.size();i++)
    {
        if(result_vector[i].cid==given_cid)
        {
            result_vector[i].score = max_score;
            result_vector[i].is_given_category = true;
        }
    }
    std::sort(result_vector.begin(), result_vector.end());
    static const uint32_t MAX_SPU_CANDIDATE = 3;
    
    double score_limit = 0.9;
    if(limit>1)
    {
        score_limit = 0.7;
    }
    score_limit = max_score*score_limit;
    for(uint32_t i=0;i<result_vector.size();i++)
    {
        uint32_t spu_id = result_vector[i].spu_id;
        double score = result_vector[i].score;
        if(score<0.4) break;
        if(score<score_limit) break;
        const WeightType& weight = result_vector[i].weight;
        const Product& p = products_[spu_id];
#ifdef B5M_DEBUG
        //uint32_t cid = result_vector[i].cid;
        //std::cerr<<"[CC]"<<category_list_[cid].name<<","<<products_[spu_id].stitle<<","<<score<<std::endl;
#endif
        if(i<MAX_SPU_CANDIDATE)
        {
            if(SpuMatched_(weight, p))
            {
#ifdef B5M_DEBUG
                std::cerr<<"[MATCHED]"<<p.stitle<<std::endl;
#endif
                //if(category_max_depth_>0)
                //{
                    //matched_p.cid = GetCidByMaxDepth_(matched_p.cid);
                    //if(matched_p.cid==0) continue;
                    //matched_p.scategory = category_list_[matched_p.cid].name;
                //}
                result_products.resize(1);
                result_products[0] = p;
                result_products[0].score = score;
                return;
            }
        }
        Product cp;
        cp.score = score;
        cp.scategory = p.scategory;
        if(weight.brand_match)
        {
            UString brand(p.sbrand, UString::UTF_8);
            if(brand.length()>=7)
            {
                cp.sbrand = p.sbrand;
            }
        }
        result_products.push_back(cp); 
        if(result_products.size()>=limit) break;
    }
    //result_vector.resize(i);
    //if(result_vector.empty()) return;
    //if(!match_found)
    //{
        //boost::unordered_set<uint32_t> cid_set;
        //for(uint32_t r=0;r<result_vector.size();r++)
        //{
            //if(result_products.size()>=limit) break;
            //Product result_product;
            //const ResultVectorItem& item = result_vector[r];
            //uint32_t spu_id = item.spu_id;
            //const Product& p = products_[spu_id];
            //uint32_t cid = item.cid;
            //if(category_max_depth_>0)
            //{
                //cid = GetCidByMaxDepth_(cid);
                //if(cid==0) continue;
                //if(cid_set.find(cid)!=cid_set.end()) continue;
                //cid_set.insert(cid);
            //}
            //const WeightType& weight = item.weight;
            //if(weight.brand_match)
            //{
                //UString brand(p.sbrand, UString::UTF_8);
                //if(brand.length()>=7)
                //{
                    //result_product.sbrand = p.sbrand;
                //}
            //}
            //result_product.scategory = category_list_[cid].name;
            //result_products.push_back(result_product);
        //}
    //}
}

double ProductMatcher::PriceSim_(double offerp, double spup)
{
    if(!use_price_sim_) return 0.25;
    if(spup==0.0) return 0.25;
    if(offerp==0.0) return 0.0;
    if(offerp>spup) return spup/offerp;
    else return offerp/spup;
}

bool ProductMatcher::PriceMatch_(double p1, double p2)
{
    static double min_ratio = 0.25;
    static double max_ratio = 3;
    double ratio = p1/p2;
    return (ratio>=min_ratio)&&(ratio<=max_ratio);
}


void ProductMatcher::Analyze_(const izenelib::util::UString& btext, std::vector<Term>& result)
{
    izenelib::util::UString text(btext);
    text.toLowerString();
    std::string stext;
    text.convertString(stext, UString::UTF_8);
    std::vector<idmlib::util::IDMTerm> term_list;
    chars_analyzer_->GetTermList(text, term_list);
    result.reserve(term_list.size());
    std::size_t spos = 0;
    bool last_match = true;
    for(std::size_t i=0;i<term_list.size();i++)
    {
        std::string str;
        term_list[i].text.convertString(str, izenelib::util::UString::UTF_8);
        term_t id = GetTerm_(str);
        //std::cerr<<"[A]"<<str<<","<<term_list[i].tag<<","<<term_list[i].position<<","<<spos<<","<<last_match<<std::endl;
        std::size_t pos = stext.find(str, spos);
        bool has_blank = false;
        if(pos!=std::string::npos)
        {
            if(pos>spos && last_match)
            {
                has_blank = true;
            }
            spos = pos+str.length();
            last_match = true;
        }
        else
        {
            last_match = false;
#ifdef B5M_DEBUG
            //std::cerr<<"[TE]"<<str<<std::endl;
            //std::cerr<<"[TITLE]"<<stext<<std::endl;
#endif
        }
        if(has_blank)
        {
            std::string append = blank_;
            term_t app_id = GetTerm_(append);
            Term term;
            term.id = app_id;
            term.text = UString(append, UString::UTF_8);
            result.push_back(term);
        }

        char tag = term_list[i].tag;
        if(tag == idmlib::util::IDMTermTag::SYMBOL)
        {
            if(str=="("||str=="（")
            {
                str = left_bracket_;
            }
            else if(str==")"||str=="）")
            {
                str = right_bracket_;
            }
            else
            {
                if(!IsTextSymbol_(id))
                {
                    symbol_terms_.insert(id);
                }
            }
        }
        id = GetTerm_(str);
        term_list[i].id = id;
        result.push_back(term_list[i]);
    }
}

void ProductMatcher::AnalyzeA_(const izenelib::util::UString& btext, std::vector<Term>& result)
{
    izenelib::util::UString text(btext);
    text.toLowerString();
    std::vector<idmlib::util::IDMTerm> term_list;
    char_analyzer_->GetTermList(text, term_list);
    result.reserve(term_list.size());
    for(std::size_t i=0;i<term_list.size();i++)
    {
        std::string str;
        term_list[i].text.convertString(str, izenelib::util::UString::UTF_8);
        term_list[i].id = GetTerm_(str);
        //logger_<<"[A]"<<str<<","<<term_list[i].tag<<std::endl;
        char tag = term_list[i].tag;
        //std::cerr<<"ARB,"<<str<<std::endl;
        if(tag == idmlib::util::IDMTermTag::SYMBOL)
        {
            if(!IsTextSymbol_(term_list[i].id))
            {
                continue;
            }
        }
        result.push_back(term_list[i]);

    }
}


void ProductMatcher::ParseAttributes(const UString& ustr, std::vector<Attribute>& attributes)
{
    std::vector<AttrPair> attrib_list;
    std::vector<std::pair<UString, std::vector<UString> > > my_attrib_list;
    split_attr_pair(ustr, attrib_list);
    for(std::size_t i=0;i<attrib_list.size();i++)
    {
        Attribute attribute;
        attribute.is_optional = false;
        const std::vector<izenelib::util::UString>& attrib_value_list = attrib_list[i].second;
        if(attrib_value_list.size()!=1) continue; //ignore empty value attrib and multi value attribs
        izenelib::util::UString attrib_value = attrib_value_list[0];
        izenelib::util::UString attrib_name = attrib_list[i].first;
        if(attrib_value.length()==0 || attrib_value.length()>30) continue;
        attrib_name.convertString(attribute.name, UString::UTF_8);
        boost::algorithm::trim(attribute.name);
        if(boost::algorithm::ends_with(attribute.name, "_optional"))
        {
            attribute.is_optional = true;
            attribute.name = attribute.name.substr(0, attribute.name.find("_optional"));
        }
        std::vector<std::string> value_list;
        std::string svalue;
        attrib_value.convertString(svalue, izenelib::util::UString::UTF_8);
        boost::algorithm::split(attribute.values, svalue, boost::algorithm::is_any_of("/"));
        for(uint32_t v=0;v<attribute.values.size();v++)
        {
            boost::algorithm::trim(attribute.values[v]);
        }
        if(attribute.name=="容量" && attribute.values.size()==1 && boost::algorithm::ends_with(attribute.values[0], "G"))
        {
            std::string gb_value = attribute.values[0]+"B";
            attribute.values.push_back(gb_value);
        }
        attributes.push_back(attribute);
    }
}

void ProductMatcher::MergeAttributes(std::vector<Attribute>& eattributes, const std::vector<Attribute>& attributes)
{
    if(attributes.empty()) return;
    boost::unordered_set<std::string> to_append_name;
    for(uint32_t i=0;i<attributes.size();i++)
    {
        to_append_name.insert(attributes[i].name);
    }
    for(uint32_t i=0;i<eattributes.size();i++)
    {
        to_append_name.erase(eattributes[i].name);
    }
    for(uint32_t i=0;i<attributes.size();i++)
    {
        const ProductMatcher::Attribute& a = attributes[i];
        if(to_append_name.find(a.name)!=to_append_name.end())
        {
            eattributes.push_back(a);
        }
    }
}

UString ProductMatcher::AttributesText(const std::vector<Attribute>& attributes)
{
    std::string str;
    for(uint32_t i=0;i<attributes.size();i++)
    {
        const ProductMatcher::Attribute& a = attributes[i];
        if(!str.empty()) str+=",";
        str+=a.GetText();
    }
    return UString(str, UString::UTF_8);
}

ProductMatcher::term_t ProductMatcher::GetTerm_(const UString& text)
{
    term_t term = izenelib::util::HashFunction<izenelib::util::UString>::generateHash32(text);
    //term_t term = izenelib::util::HashFunction<izenelib::util::UString>::generateHash64(text);
#ifdef B5M_DEBUG
    id_manager_[term] = text;
#endif
    return term;
}
ProductMatcher::term_t ProductMatcher::GetTerm_(const std::string& text)
{
    UString utext(text, UString::UTF_8);
    return GetTerm_(utext);
}

std::string ProductMatcher::GetText_(const TermList& tl) const
{
    std::string result;
#ifdef B5M_DEBUG
    for(uint32_t i=0;i<tl.size();i++)
    {
        std::string str;
        term_t term = tl[i];
        IdManager::const_iterator it = id_manager_.find(term);
        if(it!=id_manager_.end())
        { 
            const UString& ustr = it->second;
            ustr.convertString(str, UString::UTF_8);
        }
        else
        {
            str = "__UNKNOW__";
        }
        result+=str;
    }
#endif
    return result;
}
std::string ProductMatcher::GetText_(const term_t& term) const
{
#ifdef B5M_DEBUG
    IdManager::const_iterator it = id_manager_.find(term);
    if(it!=id_manager_.end())
    {
        const UString& ustr = it->second;
        std::string str;
        ustr.convertString(str, UString::UTF_8);
        return str;
    }
#endif
    return "";
}

void ProductMatcher::GetTerms_(const UString& text, std::vector<term_t>& term_list)
{
    std::vector<Term> t_list;
    AnalyzeA_(text, t_list);
    term_list.resize(t_list.size());
    for(uint32_t i=0;i<t_list.size();i++)
    {
        term_list[i] = t_list[i].id;
    }
}
//void ProductMatcher::GetCRTerms_(const UString& text, std::vector<term_t>& term_list)
//{
    //std::vector<UString> text_list;
    //AnalyzeCR_(text, text_list);
    //term_list.resize(text_list.size());
    //for(uint32_t i=0;i<term_list.size();i++)
    //{
        //term_list[i] = GetTerm_(text_list[i]);
    //}
//}
void ProductMatcher::GetTerms_(const std::string& text, std::vector<term_t>& term_list)
{
    UString utext(text, UString::UTF_8);
    GetTerms_(utext, term_list);
}
void ProductMatcher::GenSuffixes_(const std::vector<term_t>& term_list, Suffixes& suffixes)
{
    suffixes.resize(term_list.size());
    for(uint32_t i=0;i<term_list.size();i++)
    {
        suffixes[i].assign(term_list.begin()+i, term_list.end());
    }
}
void ProductMatcher::GenSuffixes_(const std::string& text, Suffixes& suffixes)
{
    std::vector<term_t> terms;
    GetTerms_(text, terms);
    GenSuffixes_(terms, suffixes);
}
void ProductMatcher::ConstructSuffixTrie_(TrieType& trie)
{
    for(uint32_t i=1;i<category_list_.size();i++)
    {
        if(i%100==0)
        {
            LOG(INFO)<<"scanning category "<<i<<std::endl;
            //LOG(INFO)<<"max tid "<<aid_manager_->getMaxDocId()<<std::endl;
        }
        Category category = category_list_[i];
        if(category.name.empty()) continue;
        uint32_t cid = category.cid;
        uint32_t depth = 1;
        uint32_t ccid = cid;
        Category ccategory = category;
        std::set<std::string> app;
        while(ccid>0)
        {
            const std::vector<std::string>& keywords = ccategory.keywords;
            for(uint32_t k=0;k<keywords.size();k++)
            {
                const std::string& w = keywords[k];
                if(app.find(w)!=app.end()) continue;
                std::vector<term_t> term_list;
                GetTerms_(w, term_list);
                Suffixes suffixes;
                GenSuffixes_(term_list, suffixes);
                for(uint32_t s=0;s<suffixes.size();s++)
                {
                    CategoryNameApp cn_app;
                    cn_app.cid = cid;
                    cn_app.depth = depth;
                    cn_app.is_complete = false;
                    if(s==0) cn_app.is_complete = true;
                    trie[suffixes[s]].category_name_apps.push_back(cn_app);
                }
                app.insert(w);
            }
            ccid = ccategory.parent_cid;
            ccategory = category_list_[ccid];
            depth++;
        }
    }
    for(uint32_t i=0;i<products_.size();i++)
    {
        if(i%100000==0)
        {
            LOG(INFO)<<"scanning product "<<i<<std::endl;
            //LOG(INFO)<<"max tid "<<aid_manager_->getMaxDocId()<<std::endl;
        }
        uint32_t pid = i;
        const Product& product = products_[i];
        //std::vector<term_t> title_terms;
        //GetTerms_(product.stitle, title_terms);
        //Suffixes title_suffixes;
        //GenSuffixes_(title_terms, title_suffixes);
        //for(uint32_t s=0;s<title_suffixes.size();s++)
        //{
            //SpuTitleApp st_app;
            //st_app.spu_id = pid;
            //st_app.pstart = s;
            //st_app.pend = title_terms.size();
            //trie[title_suffixes[s]].spu_title_apps.push_back(st_app);
        //}
        const std::vector<Attribute>& attributes = product.attributes;
        for(uint32_t a=0;a<attributes.size();a++)
        {
            const Attribute& attribute = attributes[a];
            for(uint32_t v=0;v<attribute.values.size();v++)
            {
                std::vector<term_t> terms;
                GetTerms_(attribute.values[v], terms);
                AttributeApp a_app;
                a_app.spu_id = pid;
                a_app.attribute_name = attribute.name;
                a_app.is_optional = attribute.is_optional;
                trie[terms].attribute_apps.push_back(a_app);
                //Suffixes suffixes;
                //GenSuffixes_(attribute.values[v], suffixes);
                //for(uint32_t s=0;s<suffixes.size();s++)
                //{
                    //AttributeApp a_app;
                    //a_app.spu_id = pid;
                    //a_app.attribute_name = attribute.name;
                    //a_app.is_optional = attribute.is_optional;
                    //trie[suffixes[s]].attribute_apps.push_back(a_app);
                //}

            }
        }
    }
}

void ProductMatcher::AddKeyword_(const UString& otext)
{
    std::string str;
    otext.convertString(str, UString::UTF_8);
    boost::algorithm::trim(str);
    boost::algorithm::to_lower(str);
    //std::cerr<<"[AK]"<<str<<std::endl;
    UString text(str, UString::UTF_8);
    if(text.length()<2) return;
    int value_type = 0;//0=various, 1=all digits, 2=all alphabets
    for(uint32_t u=0;u<text.length();u++)
    {
        int ctype = 0;
        if(text.isDigitChar(u))
        {
            ctype = 1;
        }
        else if(text.isAlphaChar(u))
        {
            ctype = 2;
        }
        if(u==0)
        {
            value_type = ctype;
        }
        else {
            if(value_type==1&&ctype==1)
            {
            }
            else if(value_type==2&&ctype==2)
            {
            }
            else
            {
                value_type = 0;
            }
        }
        if(value_type==0) break;
    }
    if(value_type==2 && text.length()<2) return;
    if(value_type==1 && text.length()<3) return;
    std::vector<Term> term_list;
    //std::vector<term_t> term_list;
    AnalyzeA_(text, term_list);
    if(term_list.empty()) return;
    std::vector<term_t> id_list(term_list.size());
    for(uint32_t i=0;i<term_list.size();i++)
    {
        id_list[i] = term_list[i].id;
    }
    if(not_keywords_.find(id_list)!=not_keywords_.end()) return;
    if(term_list.size()==1)
    {
        if(term_list[0].text.length()<2) return;
    }
    keyword_set_.insert(id_list);
    keyword_text_[id_list] = str;
    //std::cout<<"[AKT]"<<GetText_(term_list);
    //for(uint32_t i=0;i<term_list.size();i++)
    //{
        //std::cout<<","<<term_list[i];
    //}
    //std::cout<<std::endl;
}

void ProductMatcher::ConstructKeywords_()
{
    for(uint32_t i=1;i<category_list_.size();i++)
    {
        if(i%100==0)
        {
            LOG(INFO)<<"keywords scanning category "<<i<<std::endl;
        }
        const Category& category = category_list_[i];
        //if(category.is_parent) continue;
        if(category.name.empty()) continue;
        std::vector<std::string> cs_list;
        boost::algorithm::split( cs_list, category.name, boost::algorithm::is_any_of(">") );
        
        for(uint32_t c=0;c<cs_list.size();c++)
        {
            std::string cs = cs_list[c];
            std::vector<std::string> words;
            boost::algorithm::split( words, cs, boost::algorithm::is_any_of("/") );
            for(uint32_t i=0;i<words.size();i++)
            {
                std::string w = words[i];
                UString text(w, UString::UTF_8);
                AddKeyword_(text);
            }
        }
        for(uint32_t k=0;k<category.keywords.size();k++)
        {
            AddKeyword_(UString(category.keywords[k], UString::UTF_8));
        }
    }
    LOG(INFO)<<"keywords count "<<keyword_set_.size()<<std::endl;
    for(uint32_t i=1;i<products_.size();i++)
    {
        if(i%100000==0)
        {
            LOG(INFO)<<"keywords scanning product "<<i<<std::endl;
        }
        const Product& product = products_[i];
        //std::cerr<<product.spid<<std::endl;
        const std::vector<Attribute>& attributes = product.attributes;
        for(uint32_t a=0;a<attributes.size();a++)
        {
            const Attribute& attribute = attributes[a];
            for(uint32_t v=0;v<attribute.values.size();v++)
            {
                const std::string& svalue = attribute.values[v];
                UString uvalue(svalue, UString::UTF_8);
                AddKeyword_(uvalue);
            }
        }
    }
    LOG(INFO)<<"keywords count "<<keyword_set_.size()<<std::endl;
    for(uint32_t i=0;i<keywords_thirdparty_.size();i++)
    {
        UString uvalue(keywords_thirdparty_[i], UString::UTF_8);
        AddKeyword_(uvalue);
    }
    LOG(INFO)<<"keywords count "<<keyword_set_.size()<<std::endl;
    //for(KeywordSet::const_iterator it = keyword_set_.begin(); it!=keyword_set_.end();it++)
    //{
        //const TermList& tl = *it;
        //std::string text = GetText_(tl);
        //std::cout<<"[FK]"<<text<<std::endl;
    //}
}

void ProductMatcher::ConstructKeywordTrie_(const TrieType& suffix_trie)
{
    std::vector<TermList> keywords;
    for(KeywordSet::const_iterator it = keyword_set_.begin(); it!=keyword_set_.end();it++)
    {
        keywords.push_back(*it);
    }
    std::sort(keywords.begin(), keywords.end());
    //typedef TrieType::const_node_iterator node_iterator;
    //std::stack<node_iterator> stack;
    //stack.push(suffix_trie.node_begin());
    all_keywords_.resize(keywords.size()+1);
    for(uint32_t k=0;k<keywords.size();k++)
    {
        if(k%100000==0)
        {
            LOG(INFO)<<"build keyword "<<k<<std::endl;
        }
        uint32_t keyword_id = k+1;
        const TermList& keyword = keywords[k];
        //std::string keyword_str = GetText_(keyword);
        //LOG(INFO)<<"keyword "<<keyword_str<<std::endl;
        KeywordTag tag;
        tag.id = keyword_id;
        tag.term_list = keyword;
        //find keyword in suffix_trie
        for(TrieType::const_iterator it = suffix_trie.lower_bound(keyword);it!=suffix_trie.end();it++)
        {
            const TermList& key = it->first;
            //std::string key_str = GetText_(key);
            const KeywordTag& value = it->second;
            if(StartsWith_(key, keyword))
            {
                bool is_complete = false;
                if(key==keyword) is_complete = true;
                //LOG(INFO)<<"key found "<<key_str<<std::endl;
                tag.Append(value, is_complete);
                //tag+=value;
            }
            else
            {
                //LOG(INFO)<<"key break "<<key_str<<std::endl;
                break;
            }
        }
        tag.Flush();
        for(uint32_t i=0;i<tag.category_name_apps.size();i++)
        {
            const CategoryNameApp& app = tag.category_name_apps[i];
            if(app.is_complete) 
            {
                tag.type_app["c|"+boost::lexical_cast<std::string>(app.cid)] = 1;
                //break;
            }
        }
        if(tag.type_app.empty())
        {
            for(uint32_t i=0;i<tag.attribute_apps.size();i++)
            {
                const AttributeApp& app = tag.attribute_apps[i];
                if(app.spu_id==0) continue;
                //if(app.is_optional) continue;
                const Product& p = products_[app.spu_id];
                uint32_t cid = p.cid;
                std::string key = boost::lexical_cast<std::string>(cid)+"|"+app.attribute_name;
                tag.type_app[key]+=1;
                //double share_point = 0.0;
                //if(app.is_optional) share_point = 0.5;
                //else if(app.attribute_name=="型号") share_point = 1.5;
                //else share_point = 1.0;
                //sts["a|"+app.attribute_name]+=share_point;
            }
        }
        trie_[keyword] = tag;
        all_keywords_[keyword_id] = tag;
    }
    //post-process
#ifdef B5M_DEBUG
    for(TrieType::iterator it = trie_.begin();it!=trie_.end();it++)
    {
        KeywordTag& tag = it->second;
        std::string text = keyword_text_[tag.term_list];
        if(!tag.category_name_apps.empty())
        {
            std::cerr<<"XXKTC,"<<text<<std::endl;
        }
        bool has_brand = false;
        bool has_type = false;
        bool has_other = false;
        for(uint32_t i=0;i<tag.attribute_apps.size();i++)
        {
            const AttributeApp& app = tag.attribute_apps[i];
            if(app.spu_id==0) continue;
            if(app.attribute_name=="品牌")
            {
                has_brand = true;
            }
            else if(app.attribute_name=="型号")
            {
                has_type = true;
            }
            else
            {
                has_other = true;
            }
        }
        if(has_brand)
        {
            std::cerr<<"XXKTB,"<<text<<std::endl;
        }
        if(has_type)
        {
            std::cerr<<"XXKTT,"<<text<<std::endl;
        }
        if(has_other)
        {
            std::cerr<<"XXKTO,"<<text<<std::endl;
        }
    }
#endif
}
bool ProductMatcher::SpuMatched_(const WeightType& weight, const Product& p) const
{
    if(p.spid.empty()) return false; //detect empty product
    double paweight = weight.paweight;
    if(weight.paratio>=0.8&&weight.type_match) paweight*=2;
    if(paweight>=1.5&&paweight>=p.aweight) return true;
    return false;
}

int ProductMatcher::SelectKeyword_(const KeywordTag& tag1, const KeywordTag& tag2) const
{
    if(tag1.term_list==tag2.term_list)
    {
        if(tag1.kweight>tag2.kweight)
        {
            return 1;
        }
        return 2;
    }
    const Position& position1 = tag1.positions.front();
    const Position& position2 = tag2.positions.front();
    if( (position1.first>=position2.first&&position1.second<=position2.second) 
      || (position2.first>=position1.first&&position2.second<=position1.second))
    {
        //overlapped
        const KeywordTag* container = NULL;
        const KeywordTag* be_contained = NULL;
        int pcontainer = 1;
        
        if( (position1.first>=position2.first&&position1.second<=position2.second) )
        {
            container = &tag2;
            pcontainer = 2;
            be_contained = &tag1;
        }
        else
        {
            container = &tag1;
            pcontainer = 1;
            be_contained = &tag2;
        }
        bool has_same = false;
        for(KeywordTypeApp::const_iterator tit = be_contained->type_app.begin();tit!=be_contained->type_app.end();tit++)
        {
            const std::string& key = tit->first;
            if(container->type_app.find(key)!=container->type_app.end())
            {
                has_same = true;
                break;
            }
        }
        if(has_same) return pcontainer;
        return 3;
    }
    return 3;
    //if(!tag1.type_app.empty()&&!tag2.type_app.empty())
    //{
        //if(last_type_app.find("c")!=last_type_app.end())
        //{
            //mode_one = true;
        //}
        //else if(type_app.find("c")!=type_app.end())
        //{
            //mode_one = false;
        //}
        //else
        //{
            //bool has_same = false;
            //for(TypeApp::const_iterator tit = type_app.begin();tit!=type_app.end();tit++)
            //{
                //const std::string& key = tit->first;
                //if(last_type_app.find(key)!=last_type_app.end())
                //{
                    //has_same = true;
                    //break;
                //}
            //}
            //if(has_same) mode_one = true;
            //else mode_one = false;
        //}
    //}
}
bool ProductMatcher::IsBlankSplit_(const UString& t1, const UString& t2) const
{
    if(t1.length()==0||t2.length()==0) return true;
    if(t1.isDigitChar(t1.length()-1) && t2.isDigitChar(0)) return true;
    return false;
}
