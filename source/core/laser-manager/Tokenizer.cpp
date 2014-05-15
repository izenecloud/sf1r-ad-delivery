#include "Tokenizer.h"
using namespace sf1r::laser::clustering::type;
using namespace ilplib::knlp;

namespace sf1r { namespace laser {

Tokenizer::Tokenizer(const std::string& pcaDict, const std::string& termDict)
    : termDict_(NULL)
    , tok(NULL)
{
    termDict_ = new TermDictionary(termDict);
    tok = new TitlePCA(pcaDict);
}

Tokenizer::~Tokenizer()
{
    if (NULL != termDict_)
    {
        delete termDict_;
        termDict_ = NULL;
    }

    if(tok != NULL)
    {
        delete tok;
        tok = NULL;
    }
}

void Tokenizer::tokenize(const std::string& title, boost::unordered_map<std::string, float>& vec) const
{
    typedef std::pair<std::string, float> TermPair;
    std::vector<TermPair> tks;
    std::vector<TermPair> subtks;
    std::string brand, mdt;
    tok->pca(title, tks, brand, mdt, subtks, false);
    
    float total = 0;
    for (size_t i = 0; i < tks.size(); ++i)
    {
        boost::unordered_map<std::string, std::pair<int, int> >::const_iterator it = termDict_->getTerms().find(tks[i].first);
        if(it != termDict_->getTerms().end())
        {
            vec[tks[i].first] = tks[i].second;
            total+=tks[i].second;
        }
    }

    if(total > 0)
    {
        for (boost::unordered_map<std::string, float>::iterator it = vec.begin(); it != vec.end(); it++)
        {
            it->second /= total;
        }
    }
    else
    {
        vec.clear();
    }
}
    
void Tokenizer::tokenize(const std::string& title, boost::unordered_map<int, float>& vec) const
{
    typedef std::pair<std::string, float> TermPair;
    std::vector<TermPair> tks;
    std::vector<TermPair> subtks;
    std::string brand, mdt;
    tok->pca(title, tks, brand, mdt, subtks, false);
    
    float total = 0;
    for (size_t i = 0; i < tks.size(); ++i)
    {
        boost::unordered_map<std::string, std::pair<int, int> >::const_iterator it = termDict_->getTerms().find(tks[i].first);
        if(it != termDict_->getTerms().end())
        {
            vec[it->second.second] = tks[i].second;
            total+=tks[i].second;
        }
    }

    if(total > 0)
    {
        for (boost::unordered_map<int, float>::iterator it = vec.begin(); it != vec.end(); it++)
        {
            it->second /= total;
        }
    }
    else
    {
        vec.clear();
    }
}

void Tokenizer::tokenize(const std::string& title, std::map<int, float>& vec) const
{
    typedef std::pair<std::string, float> TermPair;
    std::vector<TermPair> tks;
    std::vector<TermPair> subtks;
    std::string brand, mdt;
    tok->pca(title, tks, brand, mdt, subtks, false);
    
    float total = 0;
    for (size_t i = 0; i < tks.size(); ++i)
    {
        boost::unordered_map<std::string, std::pair<int, int> >::const_iterator it = termDict_->getTerms().find(tks[i].first);
        if(it != termDict_->getTerms().end())
        {
            vec[it->second.second] = tks[i].second;
            total+=tks[i].second;
        }
    }

    if(total > 0)
    {
        for (std::map<int, float>::iterator it = vec.begin(); it != vec.end(); it++)
        {
            it->second /= total;
        }
    }
    else
    {
        vec.clear();
    }
}


void Tokenizer::numeric(const boost::unordered_map<std::string, float>& pow, std::map<int, float>& res) const
{
    boost::unordered_map<std::string, float>::const_iterator it = pow.begin();
    for (; it != pow.end(); ++it)
    {
        boost::unordered_map<std::string, std::pair<int, int> >::const_iterator thisIt = termDict_->getTerms().find(it->first);
        if (termDict_->getTerms().end() != thisIt)
        {
            res[thisIt->second.second] = it->second; 
        }
    }
}
    
void Tokenizer::numeric(const boost::unordered_map<std::string, float>& pow, boost::unordered_map<int, float>& res) const
{
    boost::unordered_map<std::string, float>::const_iterator it = pow.begin();
    for (; it != pow.end(); ++it)
    {
        boost::unordered_map<std::string, std::pair<int, int> >::const_iterator thisIt = termDict_->getTerms().find(it->first);
        if (termDict_->getTerms().end() != thisIt)
        {
            res[thisIt->second.second] = it->second; 
        }
    }
}
    
void Tokenizer::numeric(const boost::unordered_map<std::string, float>& vec, std::vector<std::pair<int, float> >& res) const
{
    boost::unordered_map<std::string, float>::const_iterator it = vec.begin();
    for (; it != vec.end(); ++it)
    {
        boost::unordered_map<std::string, std::pair<int, int> >::const_iterator thisIt = termDict_->getTerms().find(it->first);
        if (termDict_->getTerms().end() != thisIt)
        {
            res.push_back(std::make_pair(thisIt->second.second, it->second)); 
        }
    }
}


} }
