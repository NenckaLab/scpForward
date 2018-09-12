//
//  mapping.hpp
//  dtest.exe
//
//  Created by Brad Swearingen on 6/20/18.
//

#ifndef mapping_h
#define mapping_h

#include <string>
#include "loadProcessingConfig.h"
#include "dcmtk/dcmdata/dctk.h"

//TODO - base class for the alterations
//class Alteration
//{
//public:
//    virtual bool updateDCM(std::string fPath) = 0;
//    virtual bool updateDCM(DcmMetaInfo &mi, DcmDataset &ds) = 0;
//protected:
//    long group;
//    long element;
//    std::string value;
//};

class Qualifier
{
private:
    long group;
    long element;
    std::string symbol;
    std::string value;
public:
    Qualifier(long group, long element, std::string symbol, std::string value);
    Qualifier(std::string csvList);
    long getGroup();
    long getElement();
    bool passesTest(DcmMetaInfo &mi, DcmDataset &ds);
    bool passesTest(std::string fpath);
    std::string toString();
};

class Modify
{
private:
    long group;
    long element;
    std::string value;
public:
    Modify(long group, long element, std::string value);
    Modify(std::string csvList);
    bool updateDCM(std::string fPath);
    bool updateDCM(DcmMetaInfo &mi, DcmDataset &ds);
    std::string toString();
};

class Sequence
{
private:
    long group;
    long element;
    long subgroup;
    long subelement;
    int location;
    std::string value;
public:
    Sequence(long p_group, long p_element, long p_subgroup, long p_subelement, int p_loc, std::string p_value);
    Sequence(std::string csvList);
    bool updateDCM(DcmMetaInfo &mi, DcmDataset &ds);
    std::string toString();
};

class Delete
{
private:
    long group;
    long element;
    std::string value;
public:
    Delete(long group, long element);
    Delete(std::string csvList);
    //bool updateDCM(std::string fPath);
    bool updateDCM(DcmMetaInfo &mi, DcmDataset &ds);
    std::string toString();
};

class Hash
{
private:
    long group;
    long element;
    std::string method;
    std::vector<std::string> myValidEntries{"date", "uid", "time", "datetime", "other","none"};
public:
    Hash(long p_group, long p_element, std::string p_method);
    Hash(std::string csvList);
    bool updateDCM(std::string fPath);
    bool updateDCM(DcmMetaInfo &mi, DcmDataset &ds);
    std::string toString();
};

class KeyMap
{
private:
    long group;
    long element;
    std::string fMapName;
public:
    KeyMap(long group, long element, std::string fMapName);
    KeyMap(std::string csvList);
    bool updateDCM(DcmMetaInfo &mi, DcmDataset &ds, std::string mapPath);
    bool updateDCM(std::string fPath);
    std::string getMapPath(std::string fPath);
    std::string toString();
};

class Anon
{
private:
    long group;
    long element;
public:
    Anon(long group, long element);
    Anon(std::string csvList);
    bool updateDCM(DcmMetaInfo &mi, DcmDataset &ds);
    bool updateDCM(std::string fPath);
    std::string toString();
};

class Forward
{
private:
    std::string AETitle;
    std::string Dest;
    unsigned short DestPort;
public:
    Forward(std::string AETitle, std::string Dest, unsigned short DestPort);
    Forward(std::string csvList);
    bool Send(std::string fPath);
    std::string toString();
};

class mapping
{
private:
    //TODO - if I add Alterations, we only need one vector with several instantiations
    typedef std::vector<Qualifier> qualifiers;
    typedef std::vector<Modify> modifiers;
    typedef std::vector<KeyMap> keymaps;
    typedef std::vector<Anon> anons;
    typedef std::vector<Forward> forwards;
    typedef std::vector<Hash> hashes;
    typedef std::vector<Delete> deletes;
    typedef std::vector<Sequence> sequences;
    
    //Or, we could reduce, but, that would significantly alter my order of operations
    qualifiers qset;
    modifiers mset;
    keymaps kset;
    anons aset;
    forwards fset;
    hashes hset;
    deletes dset;
    sequences sset;
    
    bool rmvPrivateData = false;
    bool rmvCurveData = false;
    bool cleanOverlays = false;
    
    void removeAllPrivateTags(DcmDataset &ds);
    void removeCurveData(DcmDataset &ds);
    void cleanAllOverlays(DcmDataset &ds);
public:
    mapping(myMaps m);
    bool apply(std::string targetFile);
    
    template <typename T>
    void updateVec(std::string key, T &vec, myMaps m)
    {
        if (m.find(key) != m.end())
        {
            for(std::string j:m.find(key)->second)
            {
                try
                {
                    vec.emplace_back(j.c_str());
                }catch(std::exception e){}
            }
        }
    }
};

class mappings
{
private:
    typedef std::vector<mapping> myMappings;
    bool bInit = false;
    myMappings mapSet;
    std::string filepath;
    std::string projEmail = "";
public:
    mappings();
    mappings(std::string filepath);
    bool setPath(std::string filepath);
    std::string getPath();
    std::string getProjEmail();
    bool initialize();
    bool initialize(std::string filepath);
    bool apply(std::string targetFile);
    myMappings getMaps();
};

#endif /* mapping_h */
