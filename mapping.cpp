//
//  mapping.cpp
//  dtest.exe
//
//  Created by Brad Swearingen on 6/20/18.
//

#include <stdio.h>
#include <boost/filesystem.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/generator_iterator.hpp>
#include <boost/random.hpp>
#include <boost/random/random_device.hpp>
#include <boost/algorithm/string.hpp>
#include <random>
#include "mapping.hpp"
#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmnet/scu.h"
#include "loadConfig.h"
#include "md5.h"

//helper functions used throughout these data types
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

/* returns the Meta Info if the group is in the Meta Info,
 * and the rest of the Dataset if the group is not in the Meta Info
 */
DcmItem* findItem(long group, DcmMetaInfo &mi, DcmDataset &ds)
{
    DcmItem *di = &ds;
    if(group == 0x0002)
        di = &mi;
    return di;
}

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
    //note: splitOnChar will handle the special case of clearing a dcmkey by passing in no value
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
        printf("load failed on: %s\n", toString().c_str());
        printf("    %s\n", status.text());
        return false;
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
        //calculate a new uid
        //massive modify of dcmGenerateUniqueIdentifier
        char buf[128]; /* be very safe */
        char uid[100];
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
//printf("orig:%s   new:%s\n", key.c_str(), uid);
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
        printf("orig:%s   new:%s\n", key.c_str(), uid);
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



KeyMap::KeyMap(long p_group, long p_element, std::string p_fMapName):group(p_group),element(p_element),fMapName(std::move(p_fMapName)){}
KeyMap::KeyMap(std::string csvList)
{
    long group = std::strtol(splitOnChar(csvList, ',').c_str(), NULL, 16);
    long element = std::strtol(splitOnChar(csvList, ',').c_str(), NULL, 16);
    std::string value = csvList;
    *this=KeyMap(group, element, value);
}
std::string KeyMap::getMapPath(std::string fPath)
{
    //returns the full path to the keymap associated with a file (will be the same for all files in a directory)
    //creates the keymap if needed
    boost::filesystem::path p(fPath);
    std::string somepath = p.parent_path().string() + "/" + fMapName;
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
    OFString key;
    DcmItem *di = findItem(group, mi, ds);
    static Uint8 UID_SIZE = 25;
    if(di->findAndGetOFString(DcmTagKey(group, element), key).good())
    {
        std::string temp = key.c_str();
        boost::trim(temp);
        key = temp.c_str();
        char uid[UID_SIZE + 1];
        uid[UID_SIZE] = '\0';
        //determine the new value -
        //if fmappath has an entry, use that
        if (myKeyMapping.keyExists(key.c_str()))
        {
            std::strncpy(uid, myKeyMapping.getValueOfKey<std::string>(key.c_str()).c_str(), UID_SIZE);
        }
        else
        {
            //otherwise, generate a string and map the pair in fmappath, save fmappath
            //dcmGenerateUniqueIdentifier(uid, SITE_SERIES_UID_ROOT);
            std::strncpy(uid, randomString(UID_SIZE).c_str(), UID_SIZE);
            //boost::filesystem::ofstream outfile(mapPath, std::ios_base::app);
            std::ofstream outfile(mapPath, std::ios_base::app);
            outfile<<key << "=" << uid <<std::endl;
            outfile.close();
        }
        //update dicom and save
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
std::string KeyMap::toString(){return ("Group: "+std::to_string(group)+"   Element: "+std::to_string(element)+"   Value: "+fMapName);}



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



Forward::Forward(std::string p_AETitle, std::string p_Dest, unsigned short p_DestPort)
:AETitle(std::move(p_AETitle)), Dest(std::move(p_Dest)),DestPort(p_DestPort){}
Forward::Forward(std::string csvList)
{
    std::string addr = splitOnChar(csvList, ',');
    long port = std::strtol(splitOnChar(csvList, ',').c_str(), NULL, 10);
    std::string aetitle = csvList;
    *this=Forward(aetitle, addr, port);
}
//TODO - should probably clean this up a bit and make it a private function of Forward
static Uint8 findUncompressedPC(const OFString& sopClass, DcmSCU& scu)
{
    Uint8 pc;
    pc = scu.findPresentationContextID(sopClass, UID_LittleEndianExplicitTransferSyntax);
    if(pc ==0)
        pc = scu.findPresentationContextID(sopClass, UID_BigEndianExplicitTransferSyntax);
    if(pc == 0)
        pc = scu.findPresentationContextID(sopClass, UID_LittleEndianImplicitTransferSyntax);
    return pc;
}
bool Forward::Send(std::string fPath)
{
    DcmSCU scu;
    OFFilename fn = OFFilename(fPath.c_str());
    OFList<OFString> ts;
    Uint16 response;
    
    scu.setAETitle(AETitle.c_str());
    scu.setPeerAETitle(AETitle.c_str());
    scu.setPeerHostName(Dest.c_str());
    scu.setPeerPort(DestPort);
    /* Presentation context */
    ts.push_back(UID_LittleEndianExplicitTransferSyntax);
    ts.push_back(UID_BigEndianExplicitTransferSyntax);
    ts.push_back(UID_LittleEndianImplicitTransferSyntax);
    scu.addPresentationContext(UID_MRImageStorage, ts);
    scu.addPresentationContext(UID_FINDStudyRootQueryRetrieveInformationModel, ts);
    scu.addPresentationContext(UID_MOVEStudyRootQueryRetrieveInformationModel, ts);
    scu.addPresentationContext(UID_VerificationSOPClass, ts);
    /* Negotiate Association */
    
    /* Initialize network */
    OFCondition result = scu.initNetwork();
    if (result.bad())
    {
        printf("init: %s\n",result.text());
        return 1;
    }
    
    result = scu.negotiateAssociation();
    if (result.bad())
    {
        printf("neg: %s", result.text());
        return 1;
    }
    
    /* Let's look whether the server is listening:
     Assemble and send C-ECHO request
     */
    result = scu.sendECHORequest(0);
    if (result.bad())
    {
        printf("echo: %s", result.text());
        return 1;
    }
    
    T_ASC_PresentationContextID presID = findUncompressedPC(UID_MRImageStorage, scu);
    if(presID == 0)
    {
        printf("There is no uncompressed presentation context for Study Root FIND\n");
        printf("%s\n", reinterpret_cast<char *>(presID));
        return 1;
    }
    
    scu.sendSTORERequest(presID, fn, 0, response);
    //scu.closeAssociation(DCMSCU_RELEASE_ASSOCIATION);
    scu.releaseAssociation();
    return false;
}
std::string Forward::toString(){return ("IP: "+Dest+"   Port: "+std::to_string(DestPort)+"   Value: "+AETitle);}


mapping::mapping(myMaps m)
{
    //create my vectors
    updateVec("qualifiers", qset, m);
    updateVec("modify", mset, m);
    updateVec("anon", aset, m);
    updateVec("key", kset, m);
    updateVec("forward", fset, m);
    updateVec("hash", hset, m);
    updateVec("delete", dset, m);
    updateVec("sequence", sset, m);
    std::string k = "removePrivateTags";
    if ((m.find(k) != m.end()) && ((m.find(k)->second).begin()->compare("true") == 0))
    {
        rmvPrivateData=true;
    }
    k = "removeCurveData";
    if ((m.find(k) != m.end()) && ((m.find(k)->second).begin()->compare("true") == 0))
    {
        rmvCurveData=true;
    }
    k = "cleanOverlays";
    if ((m.find(k) != m.end()) && ((m.find(k)->second).begin()->compare("true") == 0))
    {
        cleanOverlays=true;
    }
}
void mapping::removeAllPrivateTags(DcmDataset &ds)
{
    //these 3 special functions are basically the same (just a different condition).
    //I should abstract them at some point
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
            stack.pop();
            delete ((DcmItem *)(stack.top()))->remove(dobj);
        }
        status = ds.nextObject(stack, OFTrue);
    }
}
void mapping::removeCurveData(DcmDataset &ds)
{
    DcmStack stack;
    DcmObject *dobj = NULL;
    DcmTagKey tag;
    OFCondition status = ds.nextObject(stack, OFTrue);
    while(status.good())
    {
        dobj = stack.top();
        tag = dobj->getTag();
        
        //if group is 0x50xx
        if( (tag.getGroup() & 0xFF00) == 0x5000)
        {
            stack.pop();
            delete ((DcmItem *)(stack.top()))->remove(dobj);
        }
        status = ds.nextObject(stack, OFTrue);
    }
}
void mapping::cleanAllOverlays(DcmDataset &ds)
{
    DcmStack stack;
    DcmObject *dobj = NULL;
    DcmTagKey tag;
    OFCondition status = ds.nextObject(stack, OFTrue);
    while(status.good())
    {
        dobj = stack.top();
        tag = dobj->getTag();
        
        //remove overlay comments (0x60xx,0x4000)
        if( ((tag.getGroup()&0xFF00)==0x6000) && (tag.getElement()==0x4000) )
        {
            stack.pop();
            delete ((DcmItem *)(stack.top()))->remove(dobj);
        }
        //remove overlay data(0x60xx,0x3000)
        else if( ((tag.getGroup()&0xFF00)==0x6000) && (tag.getElement()==0x3000) )
        {
            stack.pop();
            delete ((DcmItem *)(stack.top()))->remove(dobj);
        }
        else {/*do nothing*/}
        status = ds.nextObject(stack, OFTrue);
    }
}
bool mapping::apply(std::string targetFile)
{
    DcmFileFormat fileformat;
    OFCondition status = fileformat.loadFile(targetFile.c_str());
    if (!status.good())
    {
        return false;
    }
    DcmDataset *dataset = fileformat.getDataset();
    DcmMetaInfo *metainfo = fileformat.getMetaInfo();
    //if there are no qualifiers, it should pass this step
    //TODO - I may need a way to specify no qualifier on the initial setup!
    //it looks like, if it isn't there, it'll just not make and then therefore skip this for loop
    for(Qualifier q:qset)
    {
        if(!q.passesTest(*metainfo, *dataset))
            return false;
    }
    //passed all the qualifiers, apply all mappings
    if(rmvPrivateData)
    {
        //printf("trying to remove private tags\n");
        removeAllPrivateTags(*dataset);
    }
    if(rmvCurveData)
    {
        removeCurveData(*dataset);
    }
    if(cleanOverlays)
    {
        cleanAllOverlays(*dataset);
    }
    
    // ?? for(auto m:mset) m.updateDCM(targetFile);
    for(auto d = dset.begin(); d != dset.end(); ++d)
    {
        d->updateDCM(*metainfo, *dataset);
    }
    for(auto m = mset.begin(); m != mset.end(); ++m)
    {
        m->updateDCM(*metainfo, *dataset);
    }
    for(auto k = kset.begin(); k != kset.end(); ++k)
    {
        k->updateDCM(*metainfo, *dataset, k->getMapPath(targetFile));
    }
    for(auto a = aset.begin(); a != aset.end(); ++a)
    {
        a->updateDCM(*metainfo, *dataset);
    }
    for(auto s:sset)
    {
        s.updateDCM(*metainfo, *dataset);
    }
    for(auto h:hset)
    {
        //printf("hash: %s\n", h.toString().c_str());
        h.updateDCM(*metainfo, *dataset);
    }
    fileformat.saveFile("test.dcm");
    for(auto f = fset.begin(); f != fset.end(); ++f)
    {
        f->Send("test.dcm");
    }
    return true;
}

mappings::mappings(){filepath = "";}
mappings::mappings(std::string p_filepath):filepath(std::move(p_filepath)){}
bool mappings::setPath(std::string filepath)
{
    this->filepath=filepath;
    return true;
}
std::string mappings::getPath(){return this->filepath;}
std::string mappings::getProjEmail(){return this->projEmail;}
mappings::myMappings mappings::getMaps(){return mapSet;}
bool mappings::initialize()
{
    if(!boost::filesystem::is_regular_file(filepath))
    {
        return false;
    }
    //load config file
    ProcConfigFile cf(filepath);
    
    //I need to return cs and iterate through that
    contentsSets cs = cf.getConfigs();
    for (std::vector<myMaps>::iterator i = cs.begin(); i != cs.end(); ++i)
    {
        std::string key = "email";
        if (i->find(key) != i->end())
        {
            //should be only 1 per file
            for(std::string j:i->find(key)->second){projEmail = j;}
        }
        mapSet.emplace_back(*i);
    }
    return true;
}
bool mappings::initialize(std::string filepath)
{
    this->filepath = filepath;
    return initialize();
}
bool mappings::apply(std::string targetFile)
{
    if(!bInit)
    {
        if(initialize())
        {
            bInit = true;
        }
        else
        {
            return false;
        }
    }
    //will apply the first mapping that meets the appropriate criteria
    //returns true is a mapping has been applied
    for(auto m = mapSet.begin();m != mapSet.end(); ++m )
    {
        if(m->apply(targetFile))
        {
            //printf("mapping applied\n");
            return true;
        }
    }
    return false;
}

