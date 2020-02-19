//
//  mapping.cpp
//  dtest.exe
//  Generic map tokens
//
//  Created by Brad Swearingen on 6/20/18.
//

#include <stdio.h>
#define BOOST_NO_CXX11_SCOPED_ENUMS
//need BOOST_NO_CXX11_SCOPED_ENUMS to prevent bug on boost::filesystem::copy_file from showing up in Linux compiler
#include <boost/filesystem.hpp>
#undef BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/archive/text_oarchive.hpp>
#include <boost/generator_iterator.hpp>
#include <boost/random.hpp>
#include <boost/random/random_device.hpp>
#include <boost/algorithm/string.hpp>
#include <chrono>
#include <thread>
#include <random>
#include <ctime>
#include "mapping.hpp"
#include "dcmtk/config/osconfig.h"
#include "loadConfig.h"
#include "md5.h"

//HELPER FUNCTIONS used throughout these data types
std::string splitOnChar(std::string &stToParse, char splitter)
{
    size_t sepPos = stToParse.find(splitter);
    std::string returnval;
    if(sepPos != std::string::npos)
    {
        returnval = stToParse.substr(0,sepPos);
        stToParse = stToParse.substr(sepPos + 1);
    } else {
        returnval = stToParse;
        //returnval.assign(stToParse);
        stToParse = "";
    }
    return returnval;
}

std::string randomString(int length)
{
    char myChars[length + 1];
    constexpr char chset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    typedef boost::mt19937 rngtype;
    //preferable, but having issues
//    boost::random_device rd;
    std::random_device rd;
    rngtype r_gen(rd());
    // -1 for the null character at the end, -1 for the 0 offset
    boost::uniform_int<> myRange(0, sizeof(chset) - 2);
    boost::variate_generator<rngtype, boost::uniform_int<>> r_char(r_gen, myRange);
    
    //set null terminator
    myChars[length] = '\0';
    
    for (auto i = 0; i < length; i++)
        myChars[i] = chset[r_char()];
    return std::string(myChars);
}

static char* stripTrailing(char* s, char c)
{
    int i, n;
    if (s == NULL) return s;
    n = strlen(s);
    for (i = n - 1; (i >= 0) && (s[i] == c); i--)
        s[i] = '\0';
    return s;
}

static void addUIDComponent(char* uid, const char* s)
{
    unsigned int maxUIDLen = 64;
    /* copy into UID as much of the contents of s as possible */
    // maxUIDLen+1 because strlcat() wants the size of the buffer, not the permitted number of characters.
    if (OFStandard::strlcat(uid, s, maxUIDLen + 1) >= maxUIDLen + 1)
    {
        DCMDATA_WARN("Truncated UID in dcmGenerateUniqueIdentifier(), SITE_UID_ROOT too long?");
    }
    stripTrailing(uid, '.');
}

//CRC-32C
#define POLY 0x82f63b78
//CRC-32
//#define POLY 0xedb88320
uint32_t crc32c(uint32_t crc, const char *buf, size_t len)
//uint32_t crc32c(uint32_t crc, const unsigned char *buf, size_t len)
{
    int k;
    crc = ~crc;
    while(len--)
    {
        crc ^= *buf++;
        for (k=0; k<8; k++)
            crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
    }
    return ~crc;
}

static void hashUID(char* uid, OFString key)
{
    //calculate a new uid
    //massive modify of dcmGenerateUniqueIdentifier
    char buf[128]; /* be very safe */
    //char uid[100];
    uint32_t crc = 0;
    uid[0] = '\0'; /* initialise */
    /* On 64-bit Linux, the "32-bit identifier" returned by gethostid() is
     sign-extended to a 64-bit long, so we need to blank the upper 32 bits */
    long hostIdentifier = OFstatic_cast(unsigned long, gethostid() & 0xffffffff);
    addUIDComponent(uid, SITE_INSTANCE_UID_ROOT);
    sprintf(buf, ".%lu", hostIdentifier);
    addUIDComponent(uid, buf);
    //use the initial UID to seed the rest of the new one
    //crc = crc32c(crc, const unsigned char *buf, size_t len);
    crc = crc32c(crc, key.c_str(), key.length());
    sprintf(buf, ".%u", crc);
    addUIDComponent(uid, buf);
}

static void hashItem(char* buf, OFString key, std::string method, OFString seed)
{
    //OFString key;
    OFCondition status;
    uint32_t crc = 0;
    const time_t ONE_DAY = 24 * 60 * 60;
    const int OFFSET = 1;
    
    if(std::strcmp(method.c_str(), "date") == 0)
    {
        //first, get subjid - 10,10 in the ds
        OFString seed;
        MD5 md5;
        struct tm tm;
        //lots of funky stuff going on, strdup converts const char* to a char*, * dereferences so I can do binary ops
        //only keep bits 0x0000 1010 (0 to 10), which we'll make into +1 to 11 days
        crc = (*md5.digestString(strdup(seed.c_str())) & 0x0A);
        //adjust date - convert key to a date
        //need to clear this or you get underfined results
        memset(&tm, 0, sizeof(struct tm));
        strptime(key.c_str(), "%Y%m%d", &tm);
        time_t newSecs = mktime(&tm) + (((int)crc + OFFSET) * ONE_DAY);
        //        const struct tm *newDay = localtime(&newSecs);
        //        strftime(buf, 9, "%Y%m%d", newDay);
        strftime(buf, 9, "%Y%m%d", localtime(&newSecs));
        //TODO - special case, birthdate?, if over 90
    }
    else if(std::strcmp(method.c_str(), "uid") == 0)
    {
        hashUID(buf, key);
    }
    else if(std::strcmp(method.c_str(), "time") == 0)
    {
        //make it noon
        buf = strdup("120000.000000");
    }
    else if(std::strcmp(method.c_str(), "datetime") == 0)
    {
        //see "date" for general notes
        OFString seed;
        MD5 md5;
        struct tm tm;
        crc = (*md5.digestString(strdup(seed.c_str())) & 0x0A);
        memset(&tm, 0, sizeof(struct tm));
        std::string s(key.c_str());
        std::string temp = splitOnChar(s, '.');
        strptime(temp.c_str(), "%Y%m%d%H%M%S", &tm);
        tm.tm_hour = 12;
        tm.tm_min = 0;
        tm.tm_sec = 0;
        time_t newSecs = mktime(&tm) + (((int)crc + OFFSET) * ONE_DAY);
        strftime(buf, 13, "%Y%m%d%H%M%S", localtime(&newSecs));
    }
    else if(std::strcmp(method.c_str(), "other") == 0)
    {
        //just a md5 on the input string and then write back
        MD5 md5;
        buf = md5.digestString(strdup(key.c_str()));
    }
    else
    {
        //TODO
        //not sure yet, might be the same as none
        //hmmm, maybe, it does a crc with the method as the start value?
        //crc = crc32c(crc, key.c_str(), key.length());
        strcpy(buf, key.c_str());
    }
}

/* returns a pointer to the Meta Info if the group is in the Meta Info,
 * and the rest of the Dataset if the group is not in the Meta Info
 */
DcmItem* findItem(long group, DcmMetaInfo &mi, DcmDataset &ds)
{
    DcmItem *di = &ds;
    if(group == 0x0002)
        di = &mi;
    return di;
}




//ACTUAL MAP TOKENS

//Qualifier datatype - I still think there's more I can abstract from these types
Qualifier::Qualifier(long p_group, long p_element, std::string p_symbol, std::string p_value)
: group(p_group), element(p_element),symbol(std::move(p_symbol)),value(std::move(p_value)){}

Qualifier::Qualifier(std::string csvList)
{
    long group = std::strtol(splitOnChar(csvList, ',').c_str(), NULL, 16);
    long element = std::strtol(splitOnChar(csvList, ',').c_str(), NULL, 16);
    std::string symbol = splitOnChar(csvList, ',').c_str();
    std::string value = csvList;
    *this=Qualifier(group, element, symbol, value);
    //printf("just assigned: %s\n",toString().c_str());
}
long Qualifier::getGroup(){return group;}
long Qualifier::getElement(){return element;}
bool Qualifier::passesTest(DcmMetaInfo &mi, DcmDataset &ds)
{
    OFString value = "Default String";
    DcmItem *di = findItem(group, mi, ds);
    di->findAndGetOFString(DcmTagKey(group, element), value);
    //compare value
    if(symbol == "=")
    {
        return this->value.compare(value.c_str());
    }
    else if(symbol == "in")
    {
        return (this->value.find(value.c_str()) != std::string::npos);
    }
    else if(symbol == "<")
    {
        try {
            float tmp1 = std::stof(value.c_str());
            float tmp2 = std::stof(this->value.c_str());
            return tmp1<tmp2;
        } catch (std::exception e) {return false;}
    }
    else if(symbol == ">")
    {
        try {
            float tmp1 = std::stof(value.c_str());
            float tmp2 = std::stof(this->value.c_str());
            return tmp1>tmp2;
        } catch (std::exception e) {return false;}
    }
    else
    {
        return false;
    }
}
//TODO - I'm really debating removing this whole "fPath" set of functions,
//things I dislike: not used, not abstracted well, clumsy, saves to different file out of necessity (confusing)(except this one, as it's only testing(even more confusing)),
//is this difficult to maintain, poorly documented, redundant code worth saving for any reason?
bool Qualifier::passesTest(std::string fPath)
{
    DcmFileFormat fileformat;
    if (fileformat.loadFile(fPath.c_str()).good())
    {
        //get value from file
        DcmDataset *dataset = fileformat.getDataset();
        DcmMetaInfo *metainfo = fileformat.getMetaInfo();
        return passesTest(*metainfo, *dataset);
    }
    return false;
}
std::string Qualifier::toString()
{
    return ("Group: " + std::to_string(group) + "   Element: " + std::to_string(element) + "   Compare: "  + symbol + "   Value: " + value);
}




Modify::Modify(long p_group, long p_element, std::string p_value): group(p_group), element(p_element), value(std::move(p_value)){}
Modify::Modify(std::string csvList)
{
    long group = std::strtol(splitOnChar(csvList, ',').c_str(), NULL, 16);
    //note: splitOnChar will handle the special case of clearing a dcmkey by passing in no value
    long element = std::strtol(splitOnChar(csvList, ',').c_str(), NULL, 16);
    std::string value = csvList;
    *this=Modify(group, element, value);
}
bool Modify::updateDCM(std::string fPath)
{
    DcmFileFormat fileformat;
    OFCondition status = fileformat.loadFile(fPath.c_str());
    if (status.good())
    {
        DcmDataset *dataset = fileformat.getDataset();
        DcmMetaInfo *metainfo = fileformat.getMetaInfo();
        bool status2 = updateDCM(*metainfo, *dataset);
        fileformat.saveFile("test.dcm");
        return status2;
    }
    return false;
}
bool Modify::updateDCM(DcmMetaInfo &mi, DcmDataset &ds)
{
    OFCondition status;
    DcmItem *di = findItem(group, mi, ds);
    status = di->putAndInsertString(DcmTagKey(group,element), value.c_str());
    return status.good();
}
std::string Modify::toString()
{
    return ("Modify- Group: " + std::to_string(group) + "   Element: " + std::to_string(element) + "   Value: " + value);
}




Sequence::Sequence(long p_group, long p_element, long p_subgroup, long p_subelement, int p_loc, std::string p_value)
:group(p_group), element(p_element), subgroup(p_subgroup), subelement(p_subelement), location(p_loc), value(std::move(p_value)){}
Sequence::Sequence(std::string csvList)
{
    long group = std::strtol(splitOnChar(csvList, ',').c_str(), NULL, 16);
    //note: splitOnChar will handle the special case of clearing a dcmkey by passing in no value
    long element = std::strtol(splitOnChar(csvList, ',').c_str(), NULL, 16);
    long subgroup = std::strtol(splitOnChar(csvList, ',').c_str(), NULL, 16);
    long subelement = std::strtol(splitOnChar(csvList, ',').c_str(), NULL, 16);
    int location = std::strtol(splitOnChar(csvList, ',').c_str(), NULL, 10);
    std::string value = csvList;
    *this=Sequence(group, element, subgroup, subelement, location, value);
}
bool Sequence::updateDCM(DcmMetaInfo &mi, DcmDataset &ds)
{
    OFCondition status;
    DcmItem *di = findItem(group, mi, ds);
    DcmItem *item = NULL;
    if(di->findOrCreateSequenceItem(DcmTagKey(group,element), item, location).good())
    {
        status = item->putAndInsertString(DcmTagKey(subgroup,subelement), value.c_str());
    }
    return status.good();
}
std::string Sequence::toString()
{
    return ("Sequence- Group: " + std::to_string(group) + "   Element: " + std::to_string(element) +
            "   Subgroup: " + std::to_string(subgroup) + "   Subelement: " + std::to_string(subelement) +
            "   Location: " + std::to_string(location) + "   Value: " + value);
}




Delete::Delete(long p_group, long p_element): group(p_group), element(p_element){}
Delete::Delete(std::string csvList)
{
    long group = std::strtol(splitOnChar(csvList, ',').c_str(), NULL, 16);
    long element = std::strtol(csvList.c_str(), NULL, 16);
    *this=Delete(group, element);
}
bool Delete::updateDCM(DcmMetaInfo &mi, DcmDataset &ds)
{
    OFCondition status;
    DcmItem *di = findItem(group, mi, ds);
    status = di->findAndDeleteElement(DcmTagKey(group,element));
    return status.good();
}
std::string Delete::toString()
{
    return ("Delete-  Group: " + std::to_string(group) + "   Element: " + std::to_string(element));
}




Hash::Hash(long p_group, long p_element, std::string p_method):group(p_group),element(p_element)
{
    if (std::find(std::begin(myValidEntries), std::end(myValidEntries), p_method) != std::end(myValidEntries))
        method = std::move(p_method);
    else
        method = "none";
}
Hash::Hash(std::string csvList)
{
    long group = std::strtol(splitOnChar(csvList, ',').c_str(), NULL, 16);
    long element = std::strtol(splitOnChar(csvList, ',').c_str(), NULL, 16);
    std::string method = csvList;
    *this=Hash(group, element, method);
}
bool Hash::updateDCM(std::string fPath)
{
    DcmFileFormat fileformat;
    if (fileformat.loadFile(fPath.c_str()).good())
    {
        DcmDataset *dataset = fileformat.getDataset();
        DcmMetaInfo *metainfo = fileformat.getMetaInfo();
        bool status = updateDCM(*metainfo, *dataset);
        fileformat.saveFile("temp.dcm");
        return status;
    }
    return false;
}
bool Hash::updateDCM(DcmMetaInfo &mi, DcmDataset &ds)
{
    OFString key;
    OFCondition status;
    uint32_t crc = 0;
    const time_t ONE_DAY = 24 * 60 * 60;
    const int OFFSET = 1;
    //set up a pointer to the appropriate area
    DcmItem *di = findItem(group, mi, ds);
    
    //get the original value of whatever key we're updating
    status =di->findAndGetOFString(DcmTagKey(group, element), key);
    if(!status.good())
    {
        //printf("load failed on: %s\n", toString().c_str());
        //printf("    %s\n", status.text());
        return false;
    }
    if(key.length() == 0)
    {
        //make no change to blank keys
        return true;
    }
    if(std::strcmp(method.c_str(), "date") == 0)
    {
        //first, get subjid - 10,10 in the ds
        OFString seed;
        MD5 md5;
        struct tm tm;
        char buf[9];
        ds.findAndGetOFString(DCM_PatientName, seed);
        //lots of funky stuff going on, strdup converts const char* to a char*, * dereferences so I can do binary ops
        //only keep bits 0x0000 1010 (0 to 10), which we'll make into +1 to 11 days
        crc = (*md5.digestString(strdup(seed.c_str())) & 0x0A);
        //adjust date - convert key to a date
        //need to clear this or you get underfined results
        memset(&tm, 0, sizeof(struct tm));
        strptime(key.c_str(), "%Y%m%d", &tm);
        time_t newSecs = mktime(&tm) + (((int)crc + OFFSET) * ONE_DAY);
//        const struct tm *newDay = localtime(&newSecs);
//        strftime(buf, 9, "%Y%m%d", newDay);
        strftime(buf, 9, "%Y%m%d", localtime(&newSecs));
        status = di->putAndInsertString(DcmTagKey(group,element), buf);
//TODO - special case, birthdate?, if over 90
    }
    else if(std::strcmp(method.c_str(), "uid") == 0)
    {
        char uid[100];
        hashUID(uid, key);
//        //calculate a new uid
//        //massive modify of dcmGenerateUniqueIdentifier
//        char buf[128]; /* be very safe */
//        char uid[100];
//        uid[0] = '\0'; /* initialise */
//        /* On 64-bit Linux, the "32-bit identifier" returned by gethostid() is
//         sign-extended to a 64-bit long, so we need to blank the upper 32 bits */
//        long hostIdentifier = OFstatic_cast(unsigned long, gethostid() & 0xffffffff);
//        addUIDComponent(uid, SITE_INSTANCE_UID_ROOT);
//        sprintf(buf, ".%lu", hostIdentifier);
//        addUIDComponent(uid, buf);
//        //use the initial UID to seed the rest of the new one
//        //crc = crc32c(crc, const unsigned char *buf, size_t len);
//        crc = crc32c(crc, key.c_str(), key.length());
//        sprintf(buf, ".%u", crc);
//        addUIDComponent(uid, buf);
        //write back the new UID
        status = di->putAndInsertString(DcmTagKey(group,element), uid);
    }
    
    else if(std::strcmp(method.c_str(), "time") == 0)
    {
        char *uid = strdup("120000.000000");
        //printf("orig:%s   new:%s\n", key.c_str(), uid);
        status = di->putAndInsertString(DcmTagKey(group,element), uid);
    }
    else if(std::strcmp(method.c_str(), "datetime") == 0)
    {
        //see "date" for general notes
        OFString seed;
        MD5 md5;
        struct tm tm;
        char buf[13];
        ds.findAndGetOFString(DCM_PatientID, seed);
        crc = (*md5.digestString(strdup(seed.c_str())) & 0x0A);
        memset(&tm, 0, sizeof(struct tm));
        std::string s(key.c_str());
        std::string temp = splitOnChar(s, '.');
        strptime(temp.c_str(), "%Y%m%d%H%M%S", &tm);
        tm.tm_hour = 12;
        tm.tm_min = 0;
        tm.tm_sec = 0;
        time_t newSecs = mktime(&tm) + (((int)crc + OFFSET) * ONE_DAY);
        strftime(buf, 13, "%Y%m%d%H%M%S", localtime(&newSecs));
        status = di->putAndInsertString(DcmTagKey(group,element), buf);
    }
    else if(std::strcmp(method.c_str(), "other") == 0)
    {
        //just a md5 on the input string and then write back
        MD5 md5;
        char *uid = md5.digestString(strdup(key.c_str()));
        //printf("orig:%s   new:%s\n", key.c_str(), uid);
        status = di->putAndInsertString(DcmTagKey(group,element), uid);
    }
    else
    {
//TODO
        //not sure yet, might be the same as none
        //hmmm, maybe, it does a crc with the method as the start value?
        //crc = crc32c(crc, key.c_str(), key.length());
    }
    
    //should be able to return status.good();
    return status.good();
}
std::string Hash::toString(){return ("Hash- Group: "+std::to_string(group)+"   Element: "+std::to_string(element)+"   Method: "+method);}




SeqHash::SeqHash(long p_group, long p_element, std::string p_method):group(p_group),element(p_element)
{
    if (std::find(std::begin(myValidEntries), std::end(myValidEntries), p_method) != std::end(myValidEntries))
        method = std::move(p_method);
    else
        method = "none";
//    std::cout<<toString()<<std::endl;
}
SeqHash::SeqHash(std::string csvList)
{
    long group = std::strtol(splitOnChar(csvList, ',').c_str(), NULL, 16);
    long element = std::strtol(splitOnChar(csvList, ',').c_str(), NULL, 16);
    std::string method = csvList;
    *this=SeqHash(group, element, method);
}
bool SeqHash::updateDCM(std::string fPath)
{
    DcmFileFormat fileformat;
    if (fileformat.loadFile(fPath.c_str()).good())
    {
        DcmDataset *dataset = fileformat.getDataset();
        DcmMetaInfo *metainfo = fileformat.getMetaInfo();
        bool status = updateDCM(*metainfo, *dataset);
        fileformat.saveFile("temp.dcm");
        return status;
    }
    return false;
}
bool SeqHash::updateDCM(DcmMetaInfo &mi, DcmDataset &ds)
{
    DcmStack stack;
    DcmObject *dobj = NULL;
    DcmTagKey tag;
    OFCondition status = ds.nextObject(stack, OFTrue);
    
    char buf[100];
    OFString seed;
    OFString key;
    ds.findAndGetOFString(DCM_PatientID, seed);
    
    while(status.good())
    {
        dobj = stack.top();
        tag = dobj->getTag();
        if( (tag.getGroup()==group) && (tag.getElement()==element) )
        {
//            std::cout<<"in modification"<<std::endl;
//            dobj->print(std::cout);
//            DcmItem *di = (DcmItem*)dobj; //findItem(group, mi, ds);
//            std::cout<<"as DcmItem"<<std::endl;
//            di->print(std::cout);
//            //load the new value into the buffer - finding the group/element here is
//            //superficial, we should only have one tag in di
//            di->findAndGetOFString(DcmTagKey(group, element), key);
//            std::cout<<key<<std::endl;
//            hashItem(buf, key, method, seed);
//            //stack.pop();
//            //delete ((DcmItem *)(stack.top()))->remove(dobj);
//            //put the returned buffer into the item
//            std::cout<<"attempting to write   "<<buf<<std::endl;
//            status = di->putAndInsertString(DcmTagKey(group,element), buf);
            
            DcmElement *di = (DcmElement*)dobj; //findItem(group, mi, ds);
//            std::cout<<"as DcmElement"<<std::endl;
//            di->print(std::cout);
            //load the new value into the buffer - finding the group/element here is
            //superficial, we should only have one tag in di
            di->getOFString(key,0);
//            std::cout<<key<<std::endl;
            hashItem(buf, key, method, seed);
            //stack.pop();
            //delete ((DcmItem *)(stack.top()))->remove(dobj);
            //put the returned buffer into the item
//            std::cout<<"attempting to write   "<<buf<<std::endl;
            status = di->putOFStringArray(buf);
        }
        status = ds.nextObject(stack, OFTrue);
    }
    //should be able to return status.good();
    return status.good();
}
std::string SeqHash::toString(){return ("Hash- Group: "+std::to_string(group)+"   Element: "+std::to_string(element)+"   Method: "+method);}




KeyMap::KeyMap(long p_group, long p_element, std::string p_fMapName, std::string p_token, long p_groupSource, long p_elementSource):group(p_group), element(p_element),fMapName(std::move(p_fMapName)),token(std::move(p_token)),groupSource(p_groupSource),elementSource(p_elementSource){}
KeyMap::KeyMap(long group, long element, std::string fMapName, std::string token)
//:group(p_group),element(p_element),fMapName(std::move(p_fMapName)),token(std::move(p_token))
{
    *this=KeyMap(group, element, fMapName, token, group, element);
//    groupSource = group;
//    elementSource = element;
}
KeyMap::KeyMap(std::string csvList)
{
    long group = std::strtol(splitOnChar(csvList, ',').c_str(), NULL, 16);
    long element = std::strtol(splitOnChar(csvList, ',').c_str(), NULL, 16);
    //for general case where token isn't provided, this will consume the rest of the string and token will be ''
    std::string value = splitOnChar(csvList, ',');
    //std::string value = csvList;
    std::string token = splitOnChar(csvList, ',');
    long sourceGroup = std::strtol(splitOnChar(csvList,',').c_str(), NULL, 16);
    long sourceElement = std::strtol(splitOnChar(csvList,',').c_str(),NULL,16);
    if(sourceElement == 0){
        sourceGroup = group;
        sourceElement = element;
    }
    *this=KeyMap(group, element, value, token, sourceGroup, sourceElement);
}
std::string KeyMap::getMapPath(std::string fPath)
{
    //returns the full path to the keymap associated with a file (will be the same for all files in a directory)
    //creates the keymap if needed
    boost::filesystem::path p(fPath);
    //kind of hacky. Really should force it to the same level as the config file.
    //this is effectively the same, doing it this way to save a lot of pain now at the risk of future pain.
    std::string somepath = p.parent_path().parent_path().string() + "/" + fMapName;
    
    if(!boost::filesystem::exists(somepath))
    {
        //printf("create file here\n");
        //not sure what I'm going to do if it fails is_regular_file()
        //boost::filesystem::ofstream outfile(somepath);
        std::ofstream outfile(somepath);
        outfile.close();
    }
    return somepath;
}
bool KeyMap::updateDCM(DcmMetaInfo &mi, DcmDataset &ds, std::string mapPath)
{
    //load the keymapping
    ConfigFile myKeyMapping(mapPath);
    OFCondition status;
    OFString key, keySource;
    DcmItem *di = findItem(group, mi, ds);
    static Uint8 UID_SIZE = 25;
    if(di->findAndGetOFString(DcmTagKey(group, element), key).good() &&
       di->findAndGetOFString(DcmTagKey(groupSource, elementSource), keySource).good())
    {
        std::string temp = keySource.c_str();
        boost::trim(temp);
        //potential solution to stupid problem of someone putting extra spaces in 0010,0010, which come through as ^
        //boost::trim_if(temp, =="^")
        boost::trim_if(temp, boost::is_any_of("^"));  //-- Probably this one...
        //boost::trim_all_if(temp, boost::is_any_of("^"));
        keySource = temp.c_str();
        char uid[UID_SIZE + 1];
        uid[UID_SIZE] = '\0';
        //determine the new value -
        if(keySource.length() == 0)
        {
            //make no change to blank keys and indicate success
            //TODO - this needs some cleaning up. Users will probably expect the
            //destination key to change even if the source key is missing.
            return true;
        }
        else if (myKeyMapping.keyExists(keySource.c_str()))
        {
            //if the mapping has an entry, use that
            std::strncpy(uid, myKeyMapping.getValueOfKey<std::string>(keySource.c_str()).c_str(), UID_SIZE);
        }
        else
        {
            //if leading chars are fixed
//            std::cout<<"token: "<<token<<"   "<<token.length()<<std::endl;
            if(token.length() != 0)
            {
                int a = 0;
                while(a < 9999)
                {
//                    std::cout<<"testing: " <<a<<"\n"<<std::endl;
                    std::stringstream test;
                    test<<std::setw(4) << std::setfill('0')<<a;
                    std::string test1 = token + test.str();
//                    std::cout<<test1<<std::endl;
                    if(myKeyMapping.valueExists(test1))
                    {
                        a++;
                    }
                    else
                    {
                        std::strcpy(uid, test1.c_str()); //, UID_SIZE);
//                        std::cout<<"copied to uid: "<<uid<<std::endl;
                        break;
                    }
                }
            }
            else
            {
                //otherwise, generate a string and map the pair in fmappath, save fmappath
                //dcmGenerateUniqueIdentifier(uid, SITE_SERIES_UID_ROOT);
                std::strncpy(uid, randomString(UID_SIZE).c_str(), UID_SIZE);
                //boost::filesystem::ofstream outfile(mapPath, std::ios_base::app);
            }
//            std::cout<<"here"<<std::endl;
            std::ofstream outfile(mapPath, std::ios_base::app);
            outfile<<keySource << "=" << uid <<std::endl;
            outfile.close();
        }
        //update target in the dicom and save
        status = di->putAndInsertString(DcmTagKey(group,element), uid);
        return status.good();
    }
    return false;
}
bool KeyMap::updateDCM(std::string fPath)
{
    DcmFileFormat fileformat;
    if (fileformat.loadFile(fPath.c_str()).good())
    {
        DcmDataset *dataset = fileformat.getDataset();
        DcmMetaInfo *metainfo = fileformat.getMetaInfo();
        bool status = updateDCM(*metainfo, *dataset, getMapPath(fPath));
        fileformat.saveFile("temp.dcm");
        return status;
    }
    return false;
}
std::string KeyMap::toString(){return ("Group: "+std::to_string(group)+"   Element: "+std::to_string(element)+"   Value: "+fMapName + "    Token: " + token);}




Anon::Anon(long p_group, long p_element):group(p_group), element(p_element){}
Anon::Anon(std::string csvList)
{
    //trivial, was trying to be consistent with other adjusters, need to decide if I'm going to reconcile or diverge
    long group = std::strtol(splitOnChar(csvList, ',').c_str(), NULL, 16);
    long element = std::strtol(csvList.c_str(), NULL, 16);
    *this=Anon(group, element);
}
bool Anon::updateDCM(DcmMetaInfo &mi, DcmDataset &ds)
{
    OFCondition status;
    //TODO - remove anon and replace with delete and other options?
    static Uint8 UID_SIZE = 10;
    OFString key;
    DcmItem *di = findItem(group, mi, ds);
    char uid[UID_SIZE + 1];
    uid[UID_SIZE] = '\0';
    std::strncpy(uid, randomString(UID_SIZE).c_str(), UID_SIZE);
//    char uid[100];
//    dcmGenerateUniqueIdentifier(uid, SITE_SERIES_UID_ROOT);
    status = di->putAndInsertString(DcmTagKey(group, element), uid);
    return status.good();
}
bool Anon::updateDCM(std::string fPath)
{
    DcmFileFormat fileformat;
    if (fileformat.loadFile(fPath.c_str()).good())
    {
        DcmDataset *dataset = fileformat.getDataset();
        DcmMetaInfo *metainfo = fileformat.getMetaInfo();
        bool status = updateDCM(*metainfo, *dataset);
        fileformat.saveFile("temp.dcm");
        return status;
    }
    return false;
}
std::string Anon::toString(){return ("Group: "+std::to_string(group)+"   Element: "+std::to_string(element));
}


RemovePrivateTagsWithExceptions::RemovePrivateTagsWithExceptions(std::string csvList)
{
    //std::vector<std::pair<long,long>> exceptions;
    long group = 0;
    long element = 0;
    while(csvList.length() > 0)
    {
        group = std::strtol(splitOnChar(csvList, ',').c_str(), NULL, 16);
        element = std::strtol(splitOnChar(csvList, ',').c_str(), NULL, 16);
        exceptions.push_back(std::make_pair(group,element));
    }
}
bool RemovePrivateTagsWithExceptions::updateDCM(DcmMetaInfo &mi, DcmDataset &ds)
{
    DcmStack stack;
    DcmObject *dobj = NULL;
    DcmTagKey tag;
    OFCondition status = ds.nextObject(stack, OFTrue);
    while(status.good())
    {
        dobj = stack.top();
        tag = dobj->getTag();
        if(tag.getGroup() & 1) //private tag
        {
            std::pair<long,long> test = std::make_pair(tag.getGroup(), tag.getElement());
            if(std::find(exceptions.begin(), exceptions.end(), test) != exceptions.end()) {
                /* v contains x */
            } else {
                /* v does not contain x */
                stack.pop();
                delete ((DcmItem *)(stack.top()))->remove(dobj);
            }
        }
        status = ds.nextObject(stack, OFTrue);
    }
    return true;
}
std::string RemovePrivateTagsWithExceptions::toString()
{
    std::string test = "Remove all private tags except:\n";
    for (std::vector<std::pair<long,long>>::const_iterator iter = exceptions.begin();
         iter != exceptions.end();
         ++iter)
    {
        test = test + "  Group:" + std::to_string(iter->first) + " Element:" + std::to_string(iter->second) + "\n";
    }
    return test;
}



//forward moved to mapFwd.cpp



AddtlProject::AddtlProject(std::string csvList)
{
    std::string method = splitOnChar(csvList, ',');
    if(method == "tag"){
        useTag=true;
        group = std::strtol(splitOnChar(csvList, ',').c_str(), NULL, 16);
        element = std::strtol(csvList.c_str(), NULL, 16);
        AETitle="";
    }else{
        useTag=false;
        group=0;
        element=0;
        AETitle = splitOnChar(csvList, ',');
    }
}
bool AddtlProject::Send(std::string fPath, DcmMetaInfo &mi, DcmDataset &ds )
{
    if(useTag){
        //move the value from the tag into AETitle
        OFCondition status;
        OFString key;
        DcmItem *di = findItem(group, mi, ds);
        if(!di->findAndGetOFString(DcmTagKey(group, element), key).good()){
            return false;
        }
        AETitle = key.c_str();
    }
    
    //copy the raw file into the new AETitle folder
    //build our new path
    boost::filesystem::path source(fPath);
    std::string destFile = source.filename().string();
    //path_to_output/project/uid/file - so, we want the great-grandparent
    std::string destPath = source.parent_path().parent_path().parent_path().string();
    destPath = destPath + "/" + AETitle;
    if(!boost::filesystem::exists(destPath))
    {
        boost::filesystem::create_directories(destPath);
    }
    destPath = destPath + "/" + destFile;
    boost::filesystem::path dest(destPath);
    boost::filesystem::copy_file(source, dest); //, boost::filesystem::copy_option::overwrite_if_exists);
    
    //TODO - one small problem here. If the move fails, we repeatedly try to move forever.
    return true;
}
std::string AddtlProject::toString(){return ("Group: "+std::to_string(group)+"   Element: "+std::to_string(element)+"   or AETitle: "+AETitle);}



ReplaceChars::ReplaceChars(long p_group, long p_element, std::string p_oldSymbol, std::string p_newSymbol)
:group(p_group),element(p_element),oldSymbol(p_oldSymbol),newSymbol(p_newSymbol){}
ReplaceChars::ReplaceChars(std::string csvList)
{
    long group = std::strtol(splitOnChar(csvList, ',').c_str(), NULL, 16);
    long element = std::strtol(splitOnChar(csvList, ',').c_str(), NULL, 16);
    //for general case where token isn't provided, this will consume the rest of the string and token will be ''
    std::string oldsymbol = splitOnChar(csvList, ',');
    //std::string value = csvList;
    std::string newsymbol = csvList;
    *this=ReplaceChars(group, element, oldsymbol, newsymbol);
}
bool ReplaceChars::updateDCM(DcmMetaInfo &mi, DcmDataset &ds)
{
    //load the keymapping
    OFCondition status;
    OFString key;
    DcmItem *di = findItem(group, mi, ds);
    if(di->findAndGetOFString(DcmTagKey(group, element), key).good())
    {
        std::string temp = key.c_str();
        boost::trim(temp);
        
//        replace( temp.begin(), temp.end(), oldSymbol.c_str(), newSymbol.c_str() );
        size_t safetyCheck = newSymbol.find_first_of(oldSymbol);
        if(safetyCheck != std::string::npos)
        {
            return false;
        }
        size_t found = temp.find_first_of(oldSymbol);

        while (found != std::string::npos) { // If we found the old symbol.
            temp.replace(found, oldSymbol.length(), newSymbol);
            found = temp.find_first_of(oldSymbol, found+1); // Find again.
        }
        
        
        //update target in the dicom and save
        status = di->putAndInsertString(DcmTagKey(group,element), temp.c_str());
        return status.good();
    }
    return false;
}
std::string ReplaceChars::toString(){return ("Group: "+std::to_string(group)+"   Element: "+std::to_string(element)+"   old chars: "+oldSymbol+"    new chars: "+newSymbol);}

//mapping and mappings moved to mappingStructures.cpp

