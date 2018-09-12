//
//  loadConfig.cpp
//  dtest.exe
//
//  Created by Brad Swearingen on 6/13/18.
//

#include "loadProcessingConfig.h"
#include "loadConfig.h"

/*
 priority handled by where it is in the file, top has highest
 qualifiers[]   - must meet all for it to run this configuration
 modify[]       - dcmtag to a value
 key[]          - dcmtag to key, file to store the map in, do I need to let them name the file?
 anon[]         - dcmtag to anonymize
 forward[]      - ip, port, aetitle to forward to
 complete       - end of configuration
 */

//class ProcConfigFile
//private:
void ProcConfigFile::removeComment(std::string &line) const
{
    if (line.find(';') != line.npos)
        line.erase(line.find(';'));
    if (line.find('#') != line.npos)
        line.erase(line.find('#'));
}

bool ProcConfigFile::onlyWhitespace(const std::string &line) const
{
    return (line.find_first_not_of(' ') == line.npos);
}
bool ProcConfigFile::validLine(const std::string &line) const
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

void ProcConfigFile::extractKey(std::string &key, size_t const &sepPos, const std::string &line) const
{
    key = line.substr(0, sepPos);
    if (key.find('\t') != line.npos || key.find(' ') != line.npos)
        key.erase(key.find_first_of("\t "));
}
void ProcConfigFile::extractValue(std::string &value, size_t const &sepPos, const std::string &line) const
{
    value = line.substr(sepPos + 1);
    value.erase(0, value.find_first_not_of("\t "));
    value.erase(value.find_last_not_of("\t ") + 1);
}

void ProcConfigFile::extractContents(const std::string &line)
{
    std::string temp = line;
    temp.erase(0, temp.find_first_not_of("\t "));
    size_t sepPos = temp.find('=');
    
    std::string key, value;
    extractKey(key, sepPos, temp);
    extractValue(value, sepPos, temp);
    
    //printf(" %s   %s   %s\n", line.c_str(), key.c_str(), value.c_str());
    if (!keyExists(key))
    {
        std::vector<std::string> v;
        v.push_back(value);
    
        contents.insert(std::pair<std::string, std::vector<std::string>>(key, v));
    }
    else
    {
        updateKey(key, value);
        //exitWithError("CFG: Can only have unique key names!\n");
        //add to array or vector,
    }
    
}

void ProcConfigFile::parseLine(const std::string &line, size_t const lineNo)
{
//    printf(" %s", line.c_str());
    if (line.compare("complete") == 0)
    {
        cs.push_back(contents);
        contents.clear();
    }
    else if (line.find('=') == line.npos)
        exitWithError("CFG: Couldn't find separator on line: " + Convert::T_to_string(lineNo) + "\nEcho Line: " + line.c_str() + "\n");
    else if (!validLine(line))
        exitWithError("CFG: Bad format for line: " + Convert::T_to_string(lineNo) + "\n");
    else
        extractContents(line);
}

void ProcConfigFile::ExtractKeys()
{
    std::ifstream file;
    file.open(fName.c_str());
    if (!file)
        exitWithError("CFG: File " + fName + " couldn't be found!\n");
    
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
    
    //so, I need to push each set. Also, need to return the vector.
    //cs.push_back(contents);
    
    
    file.close();
}
//public:
ProcConfigFile::ProcConfigFile(const std::string &fName)
{
    this->fName = fName;
    ExtractKeys();
}

bool ProcConfigFile::keyExists(const std::string &key) const
{
    return contents.find(key) != contents.end();
}

void ProcConfigFile::updateKey(std::string key,std::string value)
{
    //std::map<std::string, std::string>::iterator it = contents.find(key);
    myMaps::iterator it = contents.find(key);
    if (it != contents.end())
    {
        it->second.push_back(value);
        //it->second = value;
    }
    else
    {
        std::vector<std::string> v;
        v.push_back(value);
        contents.insert(std::pair<std::string, std::vector<std::string>>(key, v));
        //contents.insert(std::pair<std::string, std::string>(key, value));
    }
}

std::vector<std::string> ProcConfigFile::getValueOfKey(const std::string &key)
{
    if (!keyExists(key))
    {
        printf(" test\n");
        std::vector<std::string> a;
        a.push_back("");
        return a;
    }
        
    return (contents.find(key)->second);
}

contentsSets ProcConfigFile::getConfigs()
{
    return cs;
}
