//
//  loadConfig.h
//  dtest.exe
//
//  Created by Brad Swearingen on 6/13/18.
//  https://www.dreamincode.net/forums/topic/183191-create-a-simple-configuration-file-parser/
//

#ifndef loadConfig_h
#define loadConfig_h

#include <stdio.h>
#include <string>
#include <map>
#include <iostream>
#include <sstream>
#include <fstream>
#include <typeinfo>


//void exitWithError(const std::string &error);

class Convert
{
public:
    //template functions may not have the implementation separated from the declaration
    //doing so causes several issues that must be dealt with.
    template <typename T>
    static std::string T_to_string(T const &val)
    {
        std::ostringstream ostr;
        ostr << val;
        
        return ostr.str();
    }
    
    template <typename T>
    static T string_to_T(std::string const &val)
    {
        std::istringstream istr(val);
        T returnVal;
        if (!(istr >> returnVal))
            //exitWithError("CFG: Not a valid " + (std::string)typeid(T).name() + " received!\n");
            throw std::runtime_error("CFG: Not a valid " + (std::string)typeid(T).name() + " received!\n");
        return returnVal;
    }
};

class ConfigFile
{
private:
    std::map<std::string, std::string> contents;
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
    ConfigFile(const std::string &fName);
    
    bool keyExists(const std::string &key) const;
    bool valueExists(const std::string &val) const;
    
    void updateKey(std::string key, std::string value);
    
    template <typename ValueType>
    ValueType getValueOfKey(const std::string &key, const ValueType  &defaultValue = ValueType()) const
    {
        if (!keyExists(key))
            return defaultValue;
        
        return Convert::string_to_T<ValueType>(contents.find(key)->second);
        //    return defaultValue;
    }
};


#endif /* loadConfig_h */
