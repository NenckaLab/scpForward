//
//  mapFwd.cpp
//  scpFwd.exe
//
// mappingCustom.cpp
//  This is a place for all those custom mapping functions we wind up needing to write because this person or that person needs something special
//  and unique.
//
//  Created by Brad Swearingen on 9/18/19.
//


#include "mapping.hpp"

//class ReferenceTime
//{
//private:
//    long group;
//    long element;
//    long refGroup;
//    long refElement;
//public:
//    ReferenceTime(long group, long element, long refGroup, long refElement);
//    ReferenceTime(std::string csvList);
//    bool updateDCM(std::string fPath);
//    bool updateDCM(DcmMetaInfo &mi, DcmDataset &ds);
//    std::string toString();
//};


ReferenceTime::ReferenceTime(long p_group, long p_element, long p_refGroup, long p_refElement)
: group(p_group), element(p_element), refGroup(p_refGroup), refElement(p_refElement){}

ReferenceTime::ReferenceTime(std::string csvList){
    //probably unnecessary in this context, but consistent with other initializers.
    long group = std::strtol(splitOnChar(csvList,',').c_str(), NULL, 16);
    long element = std::strtol(splitOnChar(csvList, ',').c_str(), NULL, 16);
    long refGroup = std::strtol(splitOnChar(csvList,',').c_str(), NULL, 16);
    long refElement = std::strtol(splitOnChar(csvList, ',').c_str(), NULL, 16);
    *this=ReferenceTime(group, element, refGroup, refElement);
}
long ReferenceTime::getGroup(){return group;}
long ReferenceTime::getElement(){return element;}
long ReferenceTime::getRefGroup(){return refGroup;}
long ReferenceTime::getRefElement(){return refElement;}


bool ReferenceTime::updateDCM(DcmMetaInfo &mi, DcmDataset &ds)
{
    OFCondition status, status2;
    DcmItem *di = findItem(group, mi, ds);
    DcmItem *di2 = findItem(refGroup, mi, ds);
    OFString key, master;
    
    //get refGroup,refElement
    //calc group,element - refGroup,refElement + 120000.0000
    status =di->findAndGetOFString(DcmTagKey(group, element), key);
    status2 =di2->findAndGetOFString(DcmTagKey(refGroup, refElement), master);
    if(!status.good() || !status2.good() )
    {
        //printf("load failed on: %s\n", toString().c_str());
        //printf("    %s\n", status.text());
        return false;
    }
    double value1 = atof(key.c_str());
    double value2 = atof(master.c_str());
    double value = value1 - value2 + 120000.000;
    
//    std::cout<<"Time: "<<key<<"   Base: "<<master<<std::endl;
//    std::cout << std::fixed;
//    std::cout << std::setprecision(5);
//    std::cout<<"lTime: "<<value1<<"    lBase: "<<value2<<std::endl;
//    std::cout<<"result: "<<value<<std::endl;
    
    status = di->putAndInsertString(DcmTagKey(group,element), std::to_string(value).c_str());
    return status.good();
}
std::string ReferenceTime::toString(){return ("ReferenceTime- Group: "+std::to_string(group)+"   Element: "+std::to_string(element) + "   RefGroup: "+std::to_string(refGroup)+"   Element: "+std::to_string(refElement));}


