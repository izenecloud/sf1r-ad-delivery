/*
 * ClusteringData.h
 *
 *  Created on: Apr 10, 2014
 *      Author: alex
 */

#ifndef SF1R_LASER_CLUSTERINGDATA_H_
#define SF1R_LASER_CLUSTERINGDATA_H_
#include "Document.h"
#include <vector>
#include <3rdparty/msgpack/msgpack.hpp>
#include <iostream>
#include <string>
#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>
namespace sf1r
{
namespace laser
{
namespace clustering
{
namespace type
{

struct ClusteringData
{
    std::vector<Document> clusteringData;
    hash_t clusteringHash;
    void output()
    {
        cout<<"clusteringHash:"<<clusteringHash<<std::endl;
        cout<<"output clusteringData"<<std::endl;
        for(vector<Document>::iterator iter = clusteringData.begin(); iter != clusteringData.end(); iter++)
        {
            cout<<"document id"<<iter->doc_id<<" "<<endl;
            for(map<clustering::hash_t, float>::iterator mapiter = iter->terms.begin(); mapiter != iter->terms.end(); mapiter++)
            {
                cout<<"term id"<< mapiter->first<<" value:"<<mapiter->second<<endl;
            }
        }
        cout<<"output clusteringData over"<<endl;
    }
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
//        ar & clusteringHash;
        ar & clusteringData;
    }
    MSGPACK_DEFINE(clusteringData);
};

} /* namespace type */
} /* namespace clustering */
}
}
#endif /* CLUSTERINGDATA_H_ */
