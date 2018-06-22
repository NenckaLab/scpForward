//
//  mapping.cpp
//  dtest.exe
//
//  Created by Brad Swearingen on 6/20/18.
//

#include <stdio.h>
#include "mapping.hpp"
#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmdata/dctk.h"

Qualifier::Qualifier(long group, long element, std::string symbol, std::string value)
{
    //TODO - need to add some checks to this
    this->group = group;
    this->element = element;
    this->symbol = symbol;
    this->value = value;
}
long Qualifier::getGroup()
{
    return group;
}
long Qualifier::getElement()
{
    return element;
}
bool Qualifier::passesTest(std::string value)
{
    if(symbol == "=")
    {
        return this->value.compare(value);
    }
    else if(symbol == "in")
    {
        return (this->value.find(value) != std::string::npos);
    }
    else if(symbol == "<")
    {
        try {
            float tmp1 = std::stof(value.c_str());
            float tmp2 = std::stof(this->value.c_str());
            return tmp1<tmp2;
        } catch (std::exception e) {
            return false;
        }
    }
    else if(symbol == ">")
    {
        try {
            float tmp1 = std::stof(value.c_str());
            float tmp2 = std::stof(this->value.c_str());
            return tmp1>tmp2;
        } catch (std::exception e) {
            return false;
        }
    }
    else
    {
        return false;
    }
}


Modify::Modify(long group, long element, std::string value)
{
    this->group = group;
    this->element = element;
    this->value = value;
}
bool Modify::updateDCM(std::string fPath)
{
    
    DcmFileFormat fileformat;
    if (fileformat.loadFile(fPath.c_str()).good())
    {
        DcmItem *item = NULL;
        DcmDataset *dataset = fileformat.getDataset();
        
        if(dataset->findOrCreateSequenceItem(DcmTagKey(group, element), item).good())
        //if (dataset->findOrCreateSequenceItem(DCM_ReferencedImageSequence, item, -2 /* append */).good())
        {
            item->putAndInsertString(DcmTagKey(group,element), value.c_str());
//            item->putAndInsertString(DCM_ReferencedSOPClassUID, UID_SecondaryCaptureImageStorage);
//            item->putAndInsertString(DCM_ReferencedSOPInstanceUID, "1.2.276.0.7230010.3.1.4.1787205428.14833.1256718852.966062");
            fileformat.saveFile(fPath.c_str());
            return true;
        }
    }
    return false;
}


KeyMap::KeyMap(long group, long element, std::string fMapPath)
{
    
}
bool KeyMap::updateDCM(std::string fPath)
{
    return false;
}


Anon::Anon(long group, long element)
{
    
}
bool Anon::updateDCM(std::string fPath)
{
    return false;
}


Forward::Forward(std::string AETitle, std::string Dest, std::string DestPort)
{
    
}


    //may be beneficial to add a mapping() and mapping(std::vector<Qualifier>, ...) along with a public mapping::readfile(std::string filepath)
    //but I'll do it when/if I need to.
mapping::mapping(std::string filepath)
{
    
}

