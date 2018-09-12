//
//  watchDirs.cpp
//  dtest.exe
//
//  Created by Brad Swearingen on 6/19/18.
//

#include "watchDirs.hpp"
#include "mapping.hpp"
#include <boost/algorithm/string.hpp>

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
//        else
//        {
//            printf(" %f                     \n", hours);
//        }
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

bool WatchDirs::process_aetitle_dir(boost::filesystem::directory_entry d, bool checkOnly)
{
    dirvec v;
    bool cfgPresent, dcmPresent30sec, dcmPresent1hour = false;
    std::string cfgPath;
    mappings myMappings("");
//    printf("%s\n", d.path().c_str());
    read_directory(d.path().c_str(), v);
    
    //first, find the config file
    for (auto i = v.begin(); i != v.end(); ++i)
    {
        if(i->path().filename().string() == "myname.cfg")
        {
            cfgPresent = true;
            myMappings.setPath(i->path().c_str());
        }
    }
    
    //process dicoms
    for (auto i = v.begin(); i != v.end(); ++i)
    {
        //apply the rules found in the config file
        if(i->path().extension().string() == ".dcm")
        {
            //std::time_t t = boost::filesystem::last_write_time(i);
            std::time_t s = boost::filesystem::last_write_time(*i);
            std::time_t n = std::time(0);
            double d = difftime(n,s);
            //if i has been here for more than 30 seconds
            dcmPresent30sec = (d > 30);
            //if i has been here for more than 1 hour
            dcmPresent1hour = (d > 3600);
            if(!checkOnly && dcmPresent30sec && cfgPresent)
            {
                //apply the config file
                if(myMappings.apply(i->path().c_str()))
                {
                    printf("Mappings applied\n");
                    //move the original file to the completed directory
                    boost::filesystem::path p(*i);
                    std::string test = p.c_str();
                    //boost::replace_all(test, "Output", "Finished");
                    boost::replace_all(test, path, finishedPath);
                    boost::filesystem::path dest(test);
                    std::string destPath = dest.parent_path().string();
                    if(!boost::filesystem::exists(destPath))
                    {
                        boost::filesystem::create_directories(destPath);
                    }
//                        printf("   %s\n", p.c_str());
//                        printf("   %s\n", test.c_str());
                    std::rename(p.c_str(),  test.c_str());
                    dcmPresent30sec = false;
                    dcmPresent1hour = false;
                }
            }
        }
    }
    
    if(!cfgPresent && dcmPresent30sec)
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
