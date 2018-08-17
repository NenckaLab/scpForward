//
//  mapping.cpp
//  dtest.exe
//
//  Created by Brad Swearingen on 6/20/18.
//

#include <stdio.h>
#include <boost/filesystem.hpp>
#include <boost/archive/text_oarchive.hpp>

//#include <boost/random/uniform_int.hpp>
//#include <boost/random/linear_congruential.hpp>
//#include <boost/random/uniform_real.hpp>
//#include <boost/random/variate_generator.hpp>

#include <boost/generator_iterator.hpp>

#include <boost/random.hpp>
#include <boost/random/random_device.hpp>
#include <random>

#include "mapping.hpp"
#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmnet/scu.h"
#include "loadConfig.h"

std::string splitOnChar(std::string &stToParse, char splitter)
{
    size_t sepPos = stToParse.find(splitter);
    std::string returnval = stToParse.substr(0,sepPos);
    stToParse = stToParse.substr(sepPos + 1);
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
    
    //set std results
    myChars[length] = '\0';
    
    for (auto i = 0; i < length; i++)
        myChars[i] = chset[r_char()];
    return std::string(myChars);
}

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
bool Qualifier::passesTest(DcmDataset &ds)
{
    OFString value = "Default String";
    ds.findAndGetOFString(DcmTagKey(group, element), value);
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
bool Qualifier::passesTest(std::string fPath)
{
    DcmFileFormat fileformat;
    if (fileformat.loadFile(fPath.c_str()).good())
    {
        //get value from file
        DcmDataset *dataset = fileformat.getDataset();
        return passesTest(*dataset);
    }
    //printf("Value retrieved from file: %s\n", value.c_str());
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
        bool status2 = updateDCM(*dataset);
        fileformat.saveFile("test.dcm");
        return status2;
    }
    return false;
}
bool Modify::updateDCM(DcmDataset &ds)
{
    OFCondition status;
    status = ds.putAndInsertString(DcmTagKey(group,element), value.c_str());
    return status.good();
}
std::string Modify::toString()
{
    return ("Group: " + std::to_string(group) + "   Element: " + std::to_string(element) + "   Value: " + value);
}



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
        printf("create file here\n");
        //not sure what I'm going to do if it fails is_regular_file()
        //may just skip this oddball case for now
        boost::filesystem::ofstream outfile(somepath);
        //outfile<<""<<std::endl;
        outfile.close();
    }
    return somepath;
}
bool KeyMap::updateDCM(DcmDataset &ds, std::string mapPath)
{
    //load the keymapping
    ConfigFile myKeyMapping(mapPath);
    OFCondition status;
    OFString key;
    static Uint8 UID_SIZE = 25;
    if(ds.findAndGetOFString(DcmTagKey(group, element), key).good())
    
    //maybe load the fMapPath first and use this if to load the tag
    //if(dataset->findOrCreateSequenceItem(DcmTagKey(group, element), item).good())
    {
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
            boost::filesystem::ofstream outfile(mapPath, std::ios_base::app);
            outfile<<key << "=" << uid <<std::endl;
            outfile.close();
        }
        //update dicom and save
        status = ds.putAndInsertString(DcmTagKey(group,element), uid);
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
        bool status = updateDCM(*dataset, getMapPath(fPath));
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
bool Anon::updateDCM(DcmDataset &ds)
{
    OFCondition status;
    //TODO - can I just delete it?
    char uid[100];
    dcmGenerateUniqueIdentifier(uid, SITE_SERIES_UID_ROOT);
    status = ds.putAndInsertString(DcmTagKey(group, element), uid);
    return status.good();
}
bool Anon::updateDCM(std::string fPath)
{
    DcmFileFormat fileformat;
    if (fileformat.loadFile(fPath.c_str()).good())
    {
        DcmDataset *dataset = fileformat.getDataset();
        bool status = updateDCM(*dataset);
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
    scu.setPeerHostName(Dest.c_str());
    scu.setPeerPort(DestPort);
    /* Presentation context */
    ts.push_back(UID_LittleEndianExplicitTransferSyntax);
    ts.push_back(UID_BigEndianExplicitTransferSyntax);
    ts.push_back(UID_LittleEndianImplicitTransferSyntax);
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
    
    T_ASC_PresentationContextID presID = findUncompressedPC(UID_FINDStudyRootQueryRetrieveInformationModel, scu);
    if(presID == 0)
    {
        printf("There is no uncompressed presentation context for Study Root FIND");
        return 1;
    }
    
    scu.sendSTORERequest(presID, fn, 0, response);
    scu.closeAssociation(DCMSCU_RELEASE_ASSOCIATION);
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
    //if there are no qualifiers, it should pass this step
    //TODO - I may need a way to specify no qualifier on the initial setup!
    //it looks like, if it isn't there, it'll just not make and then therefore skip this for loop
    for(Qualifier q:qset)
    {
        if(!q.passesTest(*dataset))
            return false;
    }
    //passed all the qualifiers, apply all mappings
    //printf("passed: starting apply on %s\n", targetFile.c_str());
    
    // ?? for(auto m:mset) m.updateDCM(targetFile);
    for(auto m = mset.begin(); m != mset.end(); ++m)
    {
        //printf("modify: %s\n", m->toString().c_str());
        //m->updateDCM(targetFile);
        m->updateDCM(*dataset);
    }
    for(auto k = kset.begin(); k != kset.end(); ++k)
    {
        //printf("key: %s\n", k->toString().c_str());
        //k->updateDCM(targetFile);
        k->updateDCM(*dataset, k->getMapPath(targetFile));
    }
    for(auto a = aset.begin(); a != aset.end(); ++a)
    {
        //printf("anon: %s\n", a->toString().c_str());
        //a->updateDCM(targetFile);
        a->updateDCM(*dataset);
    }
    fileformat.saveFile("test.dcm");
    for(auto f = fset.begin(); f != fset.end(); ++f)
    {
        //printf("fwd: %s\n", f->toString().c_str());
        //f->Send(targetFile);
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
mappings::myMappings mappings::getMaps(){return mapSet;}
bool mappings::initialize()
{
    //load config file
    ProcConfigFile cf(filepath);
    
    //I need to return cs and iterate through that
    contentsSets cs = cf.getConfigs();
    for (std::vector<myMaps>::iterator i = cs.begin(); i != cs.end(); ++i)
    {
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
    //will apply the first mapping that meets the appropriate criteria
    //returns true is a mapping has been applied
    for(auto m = mapSet.begin();m != mapSet.end(); ++m )
    {
        if(m->apply(targetFile))
        {
            printf("mapping applied\n");
            //TODO move the file to the finished directory!
            return true;
        }
    }
    return false;
}

