//
//  mapping.hpp
//  dtest.exe
//
//  Created by Brad Swearingen on 6/20/18.
//

#ifndef mapping_h
#define mapping_h

#include <string>

class Qualifier
{
private:
    long group;
    long element;
    std::string symbol;
    std::string value;
public:
    Qualifier(long group, long element, std::string symbol, std::string value);
    long getGroup();
    long getElement();
    bool passesTest(std::string value);
};

class Modify
{
private:
    long group;
    long element;
    std::string value;
public:
    Modify(long group, long element, std::string value);
    bool updateDCM(std::string fPath);
};

class KeyMap
{
private:
    long group;
    long element;
    std::string fMapPath;
public:
    KeyMap(long group, long element, std::string fMapPath);
    bool updateDCM(std::string fPath);
};

class Anon
{
private:
    long group;
    long element;
public:
    Anon(long group, long element);
    bool updateDCM(std::string fPath);
};

class Forward
{
private:
    std::string AETitle;
    std::string Dest;
    std::string DestPort;
public:
    Forward(std::string AETitle, std::string Dest, std::string DestPort);
};

class mapping
{
private:
    typedef std::vector<Qualifier> qualifiers;
    typedef std::vector<Modify> modifiers;
    typedef std::vector<KeyMap> keymaps;
    typedef std::vector<Anon> anons;
    typedef std::vector<Forward> forwards;
    
    void readfile(std::string filepath);
public:
    //may be beneficial to add a mapping() and mapping(std::vector<Qualifier>, ...) along with a public mapping::readfile(std::string filepath)
    //but I'll do it when/if I need to.
    mapping(std::string filepath);
};

#endif /* mapping_h */
