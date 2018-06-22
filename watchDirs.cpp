//
//  watchDirs.cpp
//  dtest.exe
//
//  Created by Brad Swearingen on 6/19/18.
//

#include "watchDirs.hpp"
#include "mapping.hpp"
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

WatchDirs::WatchDirs(const std::string &dName)
{
    this->path = dName;
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
    dirvec v;
    printf("%s\n", d.path().c_str());
    read_directory(d.path().c_str(), v);
    //first we need to find the template file in the i directory
    //then apply that to all .dcm files that are complete
    //printf("%s\n", i->c_str());
    for (auto i = v.begin(); i != v.end(); ++i)
    {
//        printf("  %s\n", i->path().c_str());
        //so, here we look for my homebrewed config file, whatever we call it
        //load all of them
        //how to prioritize
        //how to format
//        if(i->path().extension().string() == ".dcm")
//        {
//            printf("   matches .dcm\n");
//        }
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
    
    //only if the config file is found, if not, we have other processing to do
    //maybe the if goes in this loop? if no config and there are .dcm
    //otherwise, I don't really care what is in the folder.
    for (auto i = v.begin(); i != v.end(); ++i)
    {
//        printf("  %s\n", i->path().c_str());
        //then, run a second loop to apply all processing to the .dcm files
        //we'll need to verify completeness
        //apply the rules found in the config file
        if(i->path().extension().string() == ".dcm")
        {
            //printf("   matches .dcm\n");
        }
    }
}

bool WatchDirs::runChecks()
{
    //printf("in watchdirs\n");
    
    dirvec v;
    //watch all subfolders
    //((cfg->getValueOfKey<std::string>("keep_running")) == "True")
    read_directory(path , v);
    //std::copy(v.begin(), v.end(), std::ostream_iterator<std::string>(std::cout, "\n"));
    for (auto i = v.begin(); i != v.end(); ++i)
    {
        //boost::filesystem::path temp(*i);
        if(exists(*i) && is_directory(*i))
            process_aetitle_dir(*i);
    }
    
//    std::ostream_iterator<string> out_it(std::cout,", ");
//    std::copy(text.begin(), text.end(), out_it);
//
//    for (std::vector<char>::const_iterator i = path.begin(); i != path.end(); ++i)
//        std::cout << *i << ' ';
//    //c++11
//    for (auto i = path.begin(); i != path.end(); ++i)
//        std::cout << *i << ' ';
    return true;
}
