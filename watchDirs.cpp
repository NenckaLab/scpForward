//
//  watchDirs.cpp
//  dtest.exe
//
//  Created by Brad Swearingen on 6/19/18.
//

#include "watchDirs.hpp"
#include "mapping.hpp"
#include <boost/algorithm/string.hpp>
//#include "loadProcessingConfig.h"
//#include <boost/filesystem.hpp>
// lots of helpful boost filesystem stuff - https://theboostcpplibraries.com/boost.filesystem-files-and-directories

struct path_leaf_string
{
    std::string operator()(const boost::filesystem::directory_entry& entry) const
    {
        return entry.path().leaf().string();
    }
};

struct path_string
{
    std::string operator()(const boost::filesystem::directory_entry& entry) const
    {
        return entry.path().string();
    }
};

WatchDirs::WatchDirs(const std::string &workingDir, const std::string &finalDir)
{
    this->path = workingDir;
    this->finishedPath = finalDir;
}

void WatchDirs::read_directory(const std::string& name, stringvec& v)
{
    boost::filesystem::path p(name);
    boost::filesystem::directory_iterator start(p);
    boost::filesystem::directory_iterator end;
    std::transform(start, end, std::back_inserter(v), path_string());
    
    
//    copy(directory_iterator(p), directory_iterator(), // directory_iterator::value_type
//         ostream_iterator<directory_entry>(cout, "\n"));
}

void WatchDirs::read_directory(const std::string& name, dirvec& v)
{
    boost::filesystem::path p(name);
    boost::filesystem::directory_iterator start(p);
    boost::filesystem::directory_iterator end;
    //std::transform(start, end, std::back_inserter(v), path_string());
    copy(start, end, std::back_inserter(v));
    
    //    copy(directory_iterator(p), directory_iterator(), // directory_iterator::value_type
    //         ostream_iterator<directory_entry>(cout, "\n"));
}

void WatchDirs::process_aetitle_dir(boost::filesystem::directory_entry d)
{
    WatchDirs::process_aetitle_dir(d, false);
}

bool WatchDirs::process_aetitle_dir(boost::filesystem::directory_entry d, bool checkOnly)
{
    dirvec v;
    bool cfgPresent, dcmPresent30sec, dcmPresent1hour = false;
    bool cfgLoaded = false;
    std::string cfgPath;
    mappings myMappings("/dummy/path");
//    printf("%s\n", d.path().c_str());
    read_directory(d.path().c_str(), v);
    //first we need to find the template file in the i directory
    for (auto i = v.begin(); i != v.end(); ++i)
    {
        //so, here we look for my homebrewed config file, whatever we call it
        //load it, fixed name, forces one per directory
        //printf("  %s\n", i-> path().filename().c_str());
        if(i->path().filename().string() == "myname.cfg")
        {
            printf("   matches config file\n");
            cfgPresent = true;
            myMappings.setPath(i->path().c_str());
        }
//        boost::filesystem::path p(*i);
//        printf("\ndecomposition:\n");
//        printf("  root_name()----------: %s\n",p.root_name().c_str());
//        printf("  root_directory()-----: %s\n",p.root_directory().c_str());
//        printf("  root_path()----------: %s\n",p.root_path().c_str());
//        printf("  relative_path()------: %s\n",p.relative_path().c_str());
//        printf("  parent_path()--------: %s\n",p.parent_path().c_str());
//        printf("  filename()-----------: %s\n",p.filename().c_str());
//        printf("  stem()---------------: %s\n",p.stem().c_str());
//        printf("  extension()----------: %s\n",p.extension().c_str());
    }
    
    //check for files to process
    for (auto i = v.begin(); i != v.end(); ++i)
    {
//        printf("  %s\n", i->path().c_str());
        //apply the rules found in the config file
        if(i->path().extension().string() == ".dcm")
        {
            //std::time_t t = boost::filesystem::last_write_time(i);
            std::time_t s = boost::filesystem::last_write_time(*i);
            std::time_t n = std::time(0);
            double d = difftime(n,s);
            //if i has been here for more than 5 minutes
            dcmPresent30sec = (d > 30);
            //if i has been here for more than 1 hour
            dcmPresent1hour = (d > 3600);
            if(!checkOnly && dcmPresent30sec && cfgPresent)
            {
//                printf("   %d\n", cfgLoaded);
                if(!cfgLoaded)
                {
                    if(myMappings.getPath() != "/dummy/path")
                    {
                        myMappings.initialize();
                        cfgLoaded = true;
                    }
                }
                
                if(cfgLoaded)
                {
                    //apply the config file
                    if(myMappings.apply(i->path().c_str()))
                    {
                        printf("Mappings applied\n");
                        //TODO - move the original file to some sort of completed directory
                        boost::filesystem::path p(*i);
                        std::string test = p.c_str();
                        //boost::replace_all(test, "Output", "Finished");
                        boost::replace_all(test, path, finishedPath);
//                        printf("   %s\n", p.c_str());
//                        printf("   %s\n", test.c_str());
                        std::rename(p.c_str(),  test.c_str());
                    }
                }
            }
        }
    }
    
    if( (!cfgPresent && dcmPresent30sec) ||
        (cfgPresent && dcmPresent1hour))
    {
        return false;
    }
    return true;
}

bool WatchDirs::runChecks()
{
    dirvec v;
    //watch all subfolders
    read_directory(path , v);
    for (auto i = v.begin(); i != v.end(); ++i)
    {
        //boost::filesystem::path temp(*i);
        if(exists(*i) && is_directory(*i)){
            //we should have some exclusions here
            process_aetitle_dir(*i);
        }
    }
    return true;
}
