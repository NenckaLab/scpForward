//
//  watchDirs.cpp
//  dtest.exe
//
//  Created by Brad Swearingen on 6/19/18.
//

#include "watchDirs.hpp"
#include <boost/algorithm/string.hpp>

// Defined as static in the header, chat says I also need to define here
std::map<std::string, std::mutex> WatchDirs::directoryLocks;
std::mutex WatchDirs::mapMutex;
WatchDirs::sendmap WatchDirs::lastsent;


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

WatchDirs::WatchDirs(const std::string &workingDir, const std::string &finalDir, const std::string &adminEmail)
{
    this->path = workingDir;
    this->finishedPath = finalDir;
    this->adminEmail = adminEmail;
    this->dcmProcThreshold = 90;
}

void WatchDirs::setProcessingThreshold(int procThreshold){
    this->dcmProcThreshold = procThreshold;
}

WatchDirs::WatchDirs(const std::string &workingDir, const std::string &finalDir, const std::string &adminEmail, int procThreshold)
{
    this->path = workingDir;
    this->finishedPath = finalDir;
    this->adminEmail = adminEmail;
    this->dcmProcThreshold = procThreshold;
}

int WatchDirs::sendmail(const char *to, const char *from, const char *subject, const char *message)
{
    int retval = 01;
    FILE *mailpipe = popen("/usr/lib/sendmail -t", "w");
    if(mailpipe != NULL)
    {
        fprintf(mailpipe, "To: %s\n", to);
        fprintf(mailpipe, "From: %s\n", from);
        fprintf(mailpipe, "Subject: %s\n\n", subject);
        fwrite(message, 1, strlen(message), mailpipe);
        fwrite(".\n", 1, 2, mailpipe);
        pclose(mailpipe);
        retval = 0;
    }
    else
    {
        perror("Failed to invoke sendmail");
    }
    return retval;
}
void WatchDirs::fireEmails(const std::string &to, const std::string &cc, int hours, boost::filesystem::directory_entry d, const std::string &message)
{
    std::string key = d.path().string();
    sendmap::iterator it = lastsent.find(key);
    if(it != lastsent.end())
    {
        //get the key, compare time and see if more than 24 hours ago
        //pseudo magic numbers going on here. Their use is obvious, but they are still just numbers.
        double secs = difftime( time(0), it->second);
        double hours = secs/60/60;
        if(hours >= 24)
        {
            //send the email
            //printf("Send an email because it has been 24 hours: %s\n", key.c_str());
            sendmail(to.c_str(), adminEmail.c_str(), "Issue with SCP Forwarding folder.", message.c_str());
            it->second = time(0);
        }
    }
    else
    {
        //never been sent, so send it
        //printf("Send an email because it has never been sent: %s\n", key.c_str());
        sendmail(to.c_str(), adminEmail.c_str(), "Issue with SCP Forwarding folder.", message.c_str());
        lastsent.insert(std::pair<std::string, std::time_t>(key, time(0)));
    }
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
}

void WatchDirs::process_aetitle_dir(boost::filesystem::directory_entry d)
{
    WatchDirs::process_aetitle_dir(d, false);
}


bool WatchDirs::process_aetitle_dir(boost::filesystem::directory_entry d, bool checkOnly){
    return WatchDirs::process_aetitle_dir(d, false, false);
}

/*
 process_aetitle_dir(d, checkOnly, sort)
 processes directory d
 if checkOnly is True, it won't do anything, it'll just check
 if sort is True, it'll just sort the directory, but won't process the files
 Returns True if the request completed successfully and processed the directory and False otherwise.
 Return doesn't appear to matter as I never use it...
 */
bool WatchDirs::process_aetitle_dir(boost::filesystem::directory_entry d, bool checkOnly, bool sort)
{
    if (!lockDirectory(d)){
        //if we're unable to lock the directory, that means something else is already processing it
        return false;
    }
    
    
    bool result = false;
    
    try {
        printf("Processing %s.\n",d.path().c_str());
        
        dirvec v;
        read_directory(d.path().c_str(), v);
        
        if(sort){
            result = process_aetitle_dir_sort(v);
        }else{
            result = process_aetitle_dir_process(v, d);
        }
        
    } catch (const std::exception& e) {
        printf("Error in directory: %s\n    %s\n", d.path().c_str(), e.what());
    } catch (...) {
        printf("Undefined exception caught in directory watch.\n");
    }
    
    unlockDirectory(d);
    return result;
}

bool WatchDirs::process_aetitle_dir_sort(const dirvec& v){
    
    for (auto i = v.begin(); i != v.end(); ++i)
    {
        
        //if this is a dcm, move it.
        if(exists(*i) && is_regular_file(*i) && (i->path().extension().string() == ".dcm"))
        {
            //            std::cout<<"found a file: "<<i->path().filename().string()<<std::endl;
            //            std::cout<<"   "<<i->path().string()<<std::endl;
            //get the correct UID for the session
            long group = std::strtol("0x0020", NULL, 16);
            long element = std::strtol("0x000D", NULL, 16);
            DcmFileFormat fileformat;
            OFCondition status = fileformat.loadFile(i->path().string().c_str());
            if (!status.good())
            {
                return false;
            }
            DcmDataset *dataset = fileformat.getDataset();
            OFString value = "Default String";
            dataset->findAndGetOFString(DcmTagKey(group, element), value);
            //            std::cout<<"    "<<value.c_str()<<std::endl;
            
            //create the folder if it doesn't exist
            std::string destPath(i->path().parent_path().string() + "/" + value.c_str() ); // + i->path().filename().c_str())
            if(!boost::filesystem::exists(destPath))
            {
                boost::filesystem::create_directories(destPath);
            }
            //move the file
            std::rename(i->path().string().c_str(), (destPath + "/" + i->path().filename().string()).c_str() );
        }
    }
    return true;
}

bool WatchDirs::process_aetitle_dir_process(const dirvec& v, boost::filesystem::directory_entry d){
    bool filesPresent, dcmPresent1hour = false;
    //first, find the config file
    bool cfgPresent = false;
    std::string cfgPath;
    mappings myMappings("");
    for (auto i = v.begin(); i != v.end(); ++i)
    {
        if(i->path().filename().string() == "myname.cfg")
        {
            cfgPresent = true;
            myMappings.setPath(i->path().c_str());
            break;
        }
    }
    
    for (auto i = v.begin(); i != v.end(); ++i)
    {
        filesPresent = true;
        //only process directories
        if(is_directory(*i)){
            //process the session directory, returns true if it has dicoms and all are over 1 hour old.
            if(WatchDirs::processSessionDir(myMappings, *i, cfgPresent)){
                dcmPresent1hour = true;
            }
        }
    }
    
    if(!cfgPresent && filesPresent)
    {
       //send email to admin that this AETitle is receiving files
        //but has no config
        //something like:
        //fireEmail(toAddress,ccAddress,timeframe,directory,reason);
        fireEmails(adminEmail,"",24,d,"AETitle folder exists with dicoms, but has no config file.\nPath is: " + d.path().string() + "\n");
        return false;
    }
   else if(cfgPresent && dcmPresent1hour)
    {
        //get local email
        std::string projEmail = myMappings.getProjEmail();
        if(projEmail == "")
        {
            projEmail = adminEmail;
        }
        //get when email last sent - configs gets reloaded, so we'll need to store in this object
        //      some sort of mapping...
        //send email status to admin and user email - ~ daily?
        fireEmails(projEmail,adminEmail,24,d,"AETitle folder exists with dicoms. Config file exists, but was not applied.\nPath is: " + d.path().string() + "\n");
        return false;
    }
    else{}
    return true;
}










bool WatchDirs::processSessionDir(mappings myMappings, boost::filesystem::directory_entry sd, bool cfgPresent){
    
    // TODO - block this when dicoms are found so additional threads don't process at the same time
    
    dirvec v;
    //std::vector<boost::filesystem::path> toProcess;
    std::vector<std::string> toProcess;
    read_directory(sd.path().c_str(), v);
    bool allDcmPresentProcThreshold = true;
    bool allDcmPresent1hour = true;
    
    //TODO - move this to a subprocess on subdirectories
    //pass path and myMappings
    //process dicoms
    int count = 0;
    for (auto i = v.begin(); i != v.end(); ++i)
    {
        //apply the rules found in the config file
        if(i->path().extension().string() == ".dcm")
        {
            count++;
            //std::time_t t = boost::filesystem::last_write_time(i);
            std::time_t s = boost::filesystem::last_write_time(*i);
            std::time_t n = std::time(0);
            double d = difftime(n,s);
            //TODO - magic numbers, maybe move to the config file?
            if(d < dcmProcThreshold){
                allDcmPresent1hour=false;
                allDcmPresentProcThreshold=false;
                break;
            }
            if(d < 3600){
                allDcmPresent1hour=false;
            }
            toProcess.push_back(i->path().string());
        }
    }
    if(count == 0){
        return false;
    }
    
    if(allDcmPresentProcThreshold && cfgPresent){
        //apply the config file
        if(myMappings.applySession(toProcess))
        {
            //sd is the directory we're processing
            std::string test = sd.path().c_str();
            //change the name to the finished directory
            boost::replace_all(test, path, finishedPath);
            boost::filesystem::path dest(test);
            //make sure the parent destination exist
            std::string destParent = dest.parent_path().string();
            if(!boost::filesystem::exists(destParent))
            {
                //TODO - figure out why this isn't working
                boost::filesystem::create_directories(destParent);
            }
            //if the destination already exists
            if(boost::filesystem::exists(dest.string()))
            {
                //destination exists, copy each file
                for (auto i = v.begin(); i != v.end(); ++i)
                {
                    boost::filesystem::path p(dest);
                    p /= i->path().filename();
                    std::rename(i->path().string().c_str(), p.string().c_str() );
                }
                //then remove the directory
                boost::filesystem::remove_all(sd.path());
            }else{
                //destination doesn't exist. Simple move
                std::rename(sd.path().string().c_str(), dest.string().c_str());
            }
            //if it was successful, switch message back to false.
            allDcmPresent1hour=false;
        }
    }
    return allDcmPresent1hour;
}

bool WatchDirs::sortChecks()
{
    dirvec v;
    //watch all subfolders
    read_directory(path , v);
    for (auto i = v.begin(); i != v.end(); ++i)
    {
        if(exists(*i) && is_directory(*i)){
            process_aetitle_dir(*i, false, true);
        }
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
        if(exists(*i) && is_directory(*i)){
            process_aetitle_dir(*i);
        }
    }
    return true;
}

bool WatchDirs::lockDirectory(const boost::filesystem::directory_entry& dir) {
    // Resolve the symlink to get the target directory
    std::string dirPath = boost::filesystem::canonical(dir).string();
    // Lock the mapMutex to safely check/update directoryLocks
    std::lock_guard<std::mutex> guard(mapMutex);
    auto it = directoryLocks.find(dirPath);
    if (it == directoryLocks.end()) {
        // If directory is not in the map, create a new mutex for it
        directoryLocks[dirPath];
        it = directoryLocks.find(dirPath);
    }

    // Try to lock the directory mutex without blocking
    if (it->second.try_lock()) {
        //printf("Lock %s\n",dirPath.c_str());
        return true;  // Successfully locked
    } else {
        return false;  // Directory is already locked by another thread
    }
}

void WatchDirs::unlockDirectory(const boost::filesystem::directory_entry& dir) {
    // Resolve the symlink to get the target directory
    std::string dirPath = boost::filesystem::canonical(dir).string();
    // Lock the mapMutex to safely access directoryLocks
    std::lock_guard<std::mutex> guard(mapMutex);
    auto it = directoryLocks.find(dirPath);
    if (it != directoryLocks.end()) {
        //printf("Unlock %s\n",dirPath.c_str());
        // Unlock the mutex if it exists for the directory
        it->second.unlock();
    }
}


