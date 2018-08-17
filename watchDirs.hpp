//
//  watchDirs.hpp
//  dtest.exe
//
//  Created by Brad Swearingen on 6/19/18.
//

#ifndef watchDirs_hpp
#define watchDirs_hpp

#include <stdio.h>
#include <string>
#include <boost/filesystem.hpp>

class WatchDirs
{
private:
    typedef std::vector<std::string> stringvec;
    typedef std::vector<boost::filesystem::directory_entry> dirvec;
    std::string path;
    std::string finishedPath;
    
    void read_directory(const std::string& name, stringvec& v);
    void read_directory(const std::string& name, dirvec& v);
    void process_aetitle_dir(boost::filesystem::directory_entry d);
    bool process_aetitle_dir(boost::filesystem::directory_entry d, bool checkOnly);
    
public:
    WatchDirs(const std::string &workingDir, const std::string &finalDir);
    
    bool runChecks();
};


#endif /* watchDirs_hpp */
