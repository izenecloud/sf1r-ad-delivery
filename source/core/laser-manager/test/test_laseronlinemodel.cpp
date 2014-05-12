#include <iostream>
#include <string>
#include "LaserOnlineModel.h"
#include "PerUserOnlineModel.h"
#include <vector>
#include <sstream>
#include <boost/filesystem.hpp>
namespace bfs = boost::filesystem;

using namespace std;
using namespace predict;
using namespace clustering;
string TESTHOME="./testlaseronlinemodel";
void test()
{
    bfs::remove_all(TESTHOME);
    LaserOnlineModel* lom_ptr = LaserOnlineModel::get();
    lom_ptr->init(TESTHOME);
    for(int i = 0; i < 10; i++)
    {
        std::vector<float> tmp(10, 0.1*(i+1));
        PerUserOnlineModel puom;
        stringstream ss;
        ss<<"test"<<i;
        puom.setUserName(ss.str());
        puom.setArgs(tmp);
        lom_ptr->update(puom);
    }
    lom_ptr->release();
    lom_ptr->init(TESTHOME);
    for(int i = 0; i < 10; i++)
    {
        PerUserOnlineModel puom;
        stringstream ss;
        ss<<"test"<<i;
        bool res = lom_ptr->get(ss.str(),puom);
        cout<<ss.str()<<" :"<<endl;

        for(int j = 0; j < puom.getArgs().size(); j++)
        {
            cout<< puom.getArgs()[j]<<" ";
        }
        cout<<endl;
    }
    lom_ptr->release();
}
int main()
{
    test();

}
