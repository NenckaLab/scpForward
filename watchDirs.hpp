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
#include <ctime>
#include <map>
#include "mapping.hpp"
#include <mutex>

class WatchDirs
{
private:
    typedef std::vector<std::string> stringvec;
    typedef std::vector<boost::filesystem::directory_entry> dirvec;
    typedef std::map<std::string, std::time_t> sendmap;
    std::string path;
    std::string finishedPath;
    std::string adminEmail;
    int dcmProcThreshold;
    // Map to track directories and their corresponding mutexes
    // static so there's only one instance across all threads
    static sendmap lastsent;
    static std::map<std::string, std::mutex> directoryLocks;
    static std::mutex mapMutex;  // Mutex for accessing the directoryLocks map
    
    void read_directory(const std::string& name, stringvec& v);
    void read_directory(const std::string& name, dirvec& v);
    void process_aetitle_dir(boost::filesystem::directory_entry d);
    bool process_aetitle_dir(boost::filesystem::directory_entry d, bool checkOnly);
    bool process_aetitle_dir(boost::filesystem::directory_entry d, bool checkOnly, bool sort);
    bool process_aetitle_dir_sort(const dirvec& v);
    bool process_aetitle_dir_process(const dirvec& v, boost::filesystem::directory_entry d);
    bool processSessionDir(mappings myMappings, boost::filesystem::directory_entry sd, bool cfgPresent);
    int sendmail(const char *to, const char *from, const char *subject, const char *message);
    void fireEmails(const std::string &to, const std::string &cc, int hours, boost::filesystem::directory_entry d, const std::string &message);
    
    bool lockDirectory(const boost::filesystem::directory_entry& dir);
    void unlockDirectory(const boost::filesystem::directory_entry& dir);
    
public:
    WatchDirs(const std::string &workingDir, const std::string &finalDir, const std::string &adminEmail);
    
    WatchDirs(const std::string &workingDir, const std::string &finalDir, const std::string &adminEmail, int procThreshold);
    void setProcessingThreshold(int procThreshold);
    bool sortChecks();
    bool runChecks();
};


#endif /* watchDirs_hpp */
