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
    bool passesTest(DcmDataset &ds);
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
    bool updateDCM(DcmDataset &ds);
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
    bool updateDCM(DcmDataset &ds, std::string mapPath);
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
    bool updateDCM(DcmDataset &ds);
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
    typedef std::vector<Qualifier> qualifiers;
    typedef std::vector<Modify> modifiers;
    typedef std::vector<KeyMap> keymaps;
    typedef std::vector<Anon> anons;
    typedef std::vector<Forward> forwards;
    
    qualifiers qset;
    modifiers mset;
    keymaps kset;
    anons aset;
    forwards fset;
    
    //void readfile(std::string filepath);
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
//    typedef std::list<mapping> myMappings;
    typedef std::vector<mapping> myMappings;
    myMappings mapSet;
    std::string filepath;
public:
    mappings();
    mappings(std::string filepath);
    bool setPath(std::string filepath);
    std::string getPath();
    bool initialize();
    bool initialize(std::string filepath);
    bool apply(std::string targetFile);
    myMappings getMaps();
};

#endif /* mapping_h */
