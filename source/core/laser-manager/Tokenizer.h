#ifndef SF1R_LASER_TOKENIZER_H
#define SF1R_LASER_TOKENIZER_H
#include <string>
#include <knlp/title_pca.h>
#include "laser-manager/clusteringmanager/type/TermDictionary.h"

namespace sf1r { namespace laser { 

class Tokenizer
{
public:
    Tokenizer(const std::string& pcaDict, const std::string& termDict);
    Tokenizer(const std::string& termDict);
    ~Tokenizer();

public:    
    void tokenize(const std::string& title, boost::unordered_map<std::string, float>& vec) const;
    void tokenize(const std::string& title, boost::unordered_map<int, float>& vec) const;
    void tokenize(const std::string& title, std::map<int, float>& vec) const;
    void tokenize(const std::string& title, std::vector<std::pair<int, float> >& vec) const;

    void numeric(const boost::unordered_map<std::string, float>& vec, boost::unordered_map<int, float>& res) const;
    void numeric(const boost::unordered_map<std::string, float>& vec, std::map<int, float>& res) const;
    void numeric(const boost::unordered_map<std::string, float>& vec, std::vector<std::pair<int, float> >& res) const;
    void numeric(const boost::unordered_map<std::string, float>& vec, std::vector<float>& res) const;

    bool stringstr(const int index, std::string& str) const;
private:
    ilplib::knlp::TitlePCA* tok; //the pca
    clustering::type::TermDictionary* termDict_;
    std::vector<std::string>* dict_;
};
} }

#endif
