//
//  mapping.cpp
//  dtest.exe
//
//  mapping structures - the structures that hold the individual map tokens and apply them
//
//  Created by Brad Swearingen on 9/18/19.
//

//#include <stdio.h>
#define BOOST_NO_CXX11_SCOPED_ENUMS
////need BOOST_NO_CXX11_SCOPED_ENUMS to prevent bug on boost::filesystem::copy_file from showing up in Linux compiler
#include <boost/filesystem.hpp>
#undef BOOST_NO_CXX11_SCOPED_ENUMS
//#include <boost/archive/text_oarchive.hpp>
//#include <boost/generator_iterator.hpp>
//#include <boost/random.hpp>
//#include <boost/random/random_device.hpp>
//#include <boost/algorithm/string.hpp>
#include <chrono>
#include <thread>
//#include <random>
//#include <ctime>
#include "mapping.hpp"
//#include "dcmtk/config/osconfig.h"
//#include "loadConfig.h"
//#include "md5.h"

mapping::mapping(myMaps m)
{
    //create my vectors
    updateVec("referenceTime", rTimeset, m);
    updateVec("qualifiers", qset, m);
    updateVec("modify", mset, m);
    updateVec("anon", aset, m);
    updateVec("key", kset, m);
    updateVec("forward", fset, m);
    updateVec("projects", pset, m);
    updateVec("hash", hset, m);
    updateVec("delete", dset, m);
    updateVec("sequence", sset, m);
    updateVec("seqhash",shset, m);
    updateVec("replaceChars",rCharset, m);
    updateVec("removePrivateTagsExcept",rPriv, m);
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
    //don't like this. I'm doing this two ways in two differentp places now
    //TODO: I should break out the actual apply and return the updated datasets
    
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
    //TODO: abstract apply from here
    //it looks like, if it isn't there, it'll just not make and then therefore skip this for loop
    for(Qualifier q:qset)
    {
        if(!q.passesTest(*metainfo, *dataset))
            return false;
    }
    //copy into any other projects it should be included in
    for(auto p = pset.begin(); p != pset.end(); ++p)
    {
        p->Send(targetFile.c_str(), *metainfo, *dataset);
    }
    // reference times first, who knows what we'll remove where later
//for(auto r = rTimeset.begin(); r != rTimeset.end(); ++r)
    for(auto r:rTimeset)
    {
        r.updateDCM(*metainfo, *dataset);
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
        h.updateDCM(*metainfo, *dataset);
    }
    for(auto h:shset)
    {
        h.updateDCM(*metainfo, *dataset);
    }
    for(auto r:rCharset)
    {
        r.updateDCM(*metainfo, *dataset);
    }
    for(auto p:rPriv)
    {
        p.updateDCM(*metainfo, *dataset);
    }
    //TODO: abstract apply to here
    fileformat.saveFile("test.dcm");
    for(auto f = fset.begin(); f != fset.end(); ++f)
    {
        f->Send("test.dcm", *metainfo);
        //give a small delay, we can overwhelm XNAT if we send to quickly
        //a small delay will have neglible impact on one session, maybe add a few seconds
        //but, will give significant time for XNAT to sort through things when sending
        //1000s of files.
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    return true;
}
bool mapping::apply(std::vector<std::string> targetFiles)
{
    DcmFileFormat fileformat;
    OFCondition status = fileformat.loadFile(targetFiles.front().c_str());
    if (!status.good())
    {
        return false;
    }
    DcmMetaInfo *metainfo = fileformat.getMetaInfo();
    DcmDataset *dataset = fileformat.getDataset();
    
    //create the connection for every location we're sending this to.
    for(auto f = fset.begin(); f != fset.end(); ++f)
    {
        //f->Send(*dataset, *metainfo);
        //give a small delay, we can overwhelm XNAT if we send too quickly
        //a small delay will have neglible impact on one session, maybe add a few seconds
        //but, will give significant time for XNAT to sort through things when sending
        //1000s of files.
        //std::this_thread::sleep_for(std::chrono::milliseconds(200));
        f->CreateConnection(*metainfo);
    }
    
    for(auto targetFile = targetFiles.begin(); targetFile != targetFiles.end(); ++targetFile)
    {
        OFCondition status = fileformat.loadFile(targetFile->c_str(), EXS_Unknown, EGL_noChange, 16384, ERM_autoDetect);
        
        if (!status.good())
        {
            return false;
        }
        dataset = fileformat.getDataset();
        metainfo = fileformat.getMetaInfo();
        
        
        //fileformat.saveFile("/home/bswearingen/samples/before.dcm");
        //TODO: abstract actual apply - from here
        //update the dataset
        for(Qualifier q:qset)
        {
            if(!q.passesTest(*metainfo, *dataset))
                return false;
        }
        
        //fileformat.saveFile("/home/bswearingen/samples/qualifier.dcm");
        //copy into any other projects it should be included in
        for(auto p = pset.begin(); p != pset.end(); ++p)
        {
            p->Send(targetFile->c_str(), *metainfo, *dataset);
        }
        //fileformat.saveFile("/home/bswearingen/samples/otherProjects.dcm");
        // reference times first, who knows what we'll remove where later
        for(auto r:rTimeset)
        {
            r.updateDCM(*metainfo, *dataset);
        }
        //fileformat.saveFile("/home/bswearingen/samples/references.dcm");
        //passed all the qualifiers, apply all mappings
        if(rmvPrivateData)
        {
            //printf("trying to remove private tags\n");
            removeAllPrivateTags(*dataset);
        }
        
        //fileformat.saveFile("/home/bswearingen/samples/private.dcm");
        if(rmvCurveData)
        {
            removeCurveData(*dataset);
        }
        
        //fileformat.saveFile("/home/bswearingen/samples/curve.dcm");
        if(cleanOverlays)
        {
            cleanAllOverlays(*dataset);
        }
        
        //fileformat.saveFile("/home/bswearingen/samples/overlays.dcm");
        // ?? for(auto m:mset) m.updateDCM(targetFile);
        for(auto d = dset.begin(); d != dset.end(); ++d)
        {
            d->updateDCM(*metainfo, *dataset);
        }
        
        //fileformat.saveFile("/home/bswearingen/samples/dset.dcm");
        for(auto m = mset.begin(); m != mset.end(); ++m)
        {
            m->updateDCM(*metainfo, *dataset);
        }
        
        //fileformat.saveFile("/home/bswearingen/samples/mset.dcm");
        for(auto k = kset.begin(); k != kset.end(); ++k)
        {
            k->updateDCM(*metainfo, *dataset, k->getMapPath(*targetFile));
        }
        
        //fileformat.saveFile("/home/bswearingen/samples/kset.dcm");
        for(auto a = aset.begin(); a != aset.end(); ++a)
        {
            a->updateDCM(*metainfo, *dataset);
        }
        
        //fileformat.saveFile("/home/bswearingen/samples/aset.dcm");
        for(auto s:sset)
        {
            s.updateDCM(*metainfo, *dataset);
        }
        
        //fileformat.saveFile("/home/bswearingen/samples/sset.dcm");
        for(auto h:hset)
        {
            h.updateDCM(*metainfo, *dataset);
        }
        
        //fileformat.saveFile("/home/bswearingen/samples/hset.dcm");
        for(auto h:shset)
        {
            h.updateDCM(*metainfo, *dataset);
        }
        
        //fileformat.saveFile("/home/bswearingen/samples/shset.dcm");
        for(auto r:rCharset)
        {
            r.updateDCM(*metainfo, *dataset);
        }
        
        //fileformat.saveFile("/home/bswearingen/samples/rCharset.dcm");
        for(auto p:rPriv)
        {
            p.updateDCM(*metainfo, *dataset);
        }
        
        //fileformat.saveFile("/home/bswearingen/samples/rPriv.dcm");
        //TODO: abstract apply to here
        //send file to each connection
        for(auto f = fset.begin(); f != fset.end(); ++f)
        {
            f->Send(dataset, f->getPresentationID());
            //give a small delay, we can overwhelm XNAT if we send to quickly
            //a small delay will have neglible impact on one session, maybe add a few seconds
            //but, will give significant time for XNAT to sort through things when sending
            //1000s of files.
            
            //TODO - test if this is still necessary.
            //std::this_thread::sleep_for(std::chrono::milliseconds(100));
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        
    }
    
    //close all connections
    for(auto f = fset.begin(); f != fset.end(); ++f)
    {
        f->Release();
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

bool mappings::applySession(std::vector<std::string> targetFiles){
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
        if(m->apply(targetFiles))
        {
            return true;
        }
    }
    return false;
}
