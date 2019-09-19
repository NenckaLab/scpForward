//
//  loadConfig.cpp
//  dtest.exe
//
//  Created by Brad Swearingen on 6/13/18.
//

#include "loadConfig.h"
#include <boost/algorithm/string.hpp>


template <>
std::string Convert::string_to_T(std::string const &val)
{
    return val;
}

//class ConfigFile
//private:
void ConfigFile::removeComment(std::string &line) const
{
    //find first equals sign
    line = line.substr(line.find_first_not_of(" \t"));
    std::size_t commentPos = line.find_first_of(";#");
    std::size_t equalPos = line.find_first_of("=");
    
    if(commentPos != 0){
        if(commentPos < equalPos){
            //comment part of front of equation, find one after the = sign
            commentPos = line.find_first_of(";#", equalPos);
        }
    }
    if(commentPos != line.npos){
        line = line.erase(commentPos);
    }
//    //TODO - if it isn't at the start, make sure that an = comes before it,
//    //otherwise, if an = comes after, keep it.
//    if (line.find(';') != line.npos)
//        line.erase(line.find(';'));
//    if (line.find('#') != line.npos)
//        line.erase(line.find('#'));
}

bool ConfigFile::onlyWhitespace(const std::string &line) const
{
    return (line.find_first_not_of(' ') == line.npos);
}
bool ConfigFile::validLine(const std::string &line) const
{
    std::string temp = line;
    temp.erase(0, temp.find_first_not_of("\t "));
    if (temp[0] == '=')
        return false;
    
    for (size_t i = temp.find('=') + 1; i < temp.length(); i++)
        if (temp[i] != ' ')
            return true;
    
    return false;
}

void ConfigFile::extractKey(std::string &key, size_t const &sepPos, const std::string &line) const
{
    key = line.substr(0, sepPos);
    std::string temp = key;
    boost::trim(temp);
    key = temp;
    if (key.find('\t') != line.npos)
        key.erase(key.find_first_of("\t"));
}
void ConfigFile::extractValue(std::string &value, size_t const &sepPos, const std::string &line) const
{
    value = line.substr(sepPos + 1);
    value.erase(0, value.find_first_not_of("\t "));
    value.erase(value.find_last_not_of("\t ") + 1);
}

void ConfigFile::extractContents(const std::string &line)
{
    std::string temp = line;
    temp.erase(0, temp.find_first_not_of("\t "));
    size_t sepPos = temp.find_last_of('=');
    
    std::string key, value;
    extractKey(key, sepPos, temp);
    extractValue(value, sepPos, temp);
    
    if (!keyExists(key))
        contents.insert(std::pair<std::string, std::string>(key, value));
    else
        //exitWithError("CFG: Can only have unique key names! " + key + " is a duplicate value " + value + "\n");
        throw std::runtime_error("CFG: Can only have unique key names! " + key + " is a duplicate value " + value);
}

void ConfigFile::parseLine(const std::string &line, size_t const lineNo)
{
    if (line.find('=') == line.npos)
        //exitWithError("CFG: Couldn't find separator on line: " + Convert::T_to_string(lineNo) + "   Echo Line: " + line.c_str() + "\n");
        throw std::runtime_error("CFG: No separator on line " + Convert::T_to_string(lineNo) + ": " + line);
    
    if (!validLine(line))
        //exitWithError("CFG: Bad format for line: " + Convert::T_to_string(lineNo) + "\n");
        throw std::runtime_error("CFG: Bad format for line " + Convert::T_to_string(lineNo));
    
    extractContents(line);
}

void ConfigFile::ExtractKeys()
{
    std::ifstream file;
    file.open(fName.c_str());
    if (!file)
        //exitWithError("CFG: File " + fName + " couldn't be found!\n");
        throw std::runtime_error("CFG: File " + fName + " couldn't be found!");
    
    std::string line;
    size_t lineNo = 0;
    while (std::getline(file, line))
    {
        lineNo++;
        std::string temp = line;
        
        if (temp.empty())
            continue;
        
        removeComment(temp);
        if (onlyWhitespace(temp))
            continue;
        
        parseLine(temp, lineNo);
    }
    
    file.close();
}
//public:
ConfigFile::ConfigFile(const std::string &fName)
{
    this->fName = fName;
    ExtractKeys();
}

bool ConfigFile::keyExists(const std::string &key) const
{
    return contents.find(key) != contents.end();
}

bool ConfigFile::valueExists(const std::string &val) const
{
    //std::cout<<"in sub"<<std::endl;
    auto it = contents.begin();
    while(it != contents.end())
    {
        if(it->second == val)
        {
            return true;
        }
        it++;
    }
    return false;
}

void ConfigFile::updateKey(std::string key,std::string value)
{
    std::map<std::string, std::string>::iterator it = contents.find(key);
    if (it != contents.end())
        it->second = value;
    else
        contents.insert(std::pair<std::string, std::string>(key, value));
}
