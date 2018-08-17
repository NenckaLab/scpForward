//
//  loadConfig.h
//  dtest.exe
//
//  Created by Brad Swearingen on 6/13/18.
//  https://www.dreamincode.net/forums/topic/183191-create-a-simple-configuration-file-parser/
//

#ifndef loadProcessingConfig_h
#define loadProcessingConfig_h

#include <stdio.h>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <fstream>

typedef std::map<std::string, std::vector<std::string>> myMaps;
typedef std::vector<myMaps> contentsSets;

class ProcConfigFile
{
private:
    //std::vector<std::map<std::string, std::string>> contentsSets;
    std::vector<myMaps> cs;
    //std::map<std::string, std::string> contents;
    myMaps contents;
    std::string fName;
    
    void removeComment(std::string &line) const;
    
    bool onlyWhitespace(const std::string &line) const;
    
    bool validLine(const std::string &line) const;
    
    void extractKey(std::string &key, size_t const &sepPos, const std::string &line) const;
    void extractValue(std::string &value, size_t const &sepPos, const std::string &line) const;
    
    void extractContents(const std::string &line);
    
    void parseLine(const std::string &line, size_t const lineNo);
    
    void ExtractKeys();
public:
    ProcConfigFile(const std::string &fName);
    
    bool keyExists(const std::string &key) const;
    
    void updateKey(std::string key, std::string value);
    
    contentsSets getConfigs();
    
    std::vector<std::string> getValueOfKey(const std::string &key);
};


#endif /* loadConfig_h */
