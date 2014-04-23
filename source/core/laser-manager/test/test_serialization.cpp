#include "common/utils.h"
#include "type/ClusteringData.h"
#include "type/Document.h"
#include "predict/TopNClusterContainer.h"
using namespace clustering::type;
using namespace clustering;
using namespace std;
using namespace predict;
void test()
{
    ClusteringData oridata;
    Document doc("doctest");
    doc.add(1,1.1);
    doc.add(2,2.2);
    Document doc2("doctest2");
    doc2.add(1,2.1);
    doc2.add(2,3.2);
    oridata.clusteringData.push_back(doc);
    oridata.clusteringData.push_back(doc2);
    std::string serial_str;
    serialization<ClusteringData>(serial_str, oridata);
    ClusteringData newdata;
    deserialize<ClusteringData>(serial_str, newdata);
    newdata.output();
}
void test2()
{

//{
    TopNClusterContainer* ptncc = TopNClusterContainer::get();
    ptncc->init("./test");

    for(int i = 0; i < 10; i++)
    {
        TopNCluster tnc;
        ClusterContainer cc;
        cc[1] = 1*i;
        cc[2] = 2*i;
        stringstream ss;
        ss<<"name"<<i;
        tnc.setUserName(ss.str());
        tnc.setClusterContainer(cc);
        ptncc->update(tnc);
    }
    ptncc->release();
    ptncc = TopNClusterContainer::get();
    ptncc->init("./test");
    //ptncc->output();
    delete ptncc;
}
int main()
{
   // test();
    test2();
}


