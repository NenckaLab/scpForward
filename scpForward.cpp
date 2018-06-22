#include "dcmtk/config/osconfig.h"
#include "dcmtk/ofstd/ofstring.h"
//#include "dcmtk/dcmsr/dsrdoc.h"
//#include "dcmtk/dcmdata/dcfilefo.h"
//#include "dcmtk/dcmnet/dstorscp.h"
#include "dstorscp2.h"
#include <thread>
#include <time.h>
#include "loadConfig.h"
#include "watchDirs.hpp"

//function to launch the SCP listener on a separate thread.
void startSCPListener(DcmStorageSCP *b, ConfigFile *cfg)
{
    int x = 0;
    time_t start = time(0);
    
    //while( (!b->getStopRunning()) && (x < 5))
    while( ((cfg->getValueOfKey<std::string>("keep_running")) == "True") && (x<5) )
    {
        printf("Starting the SCP listener.\n");
        //it'll automatically "hang" here and listen forever
        //the only reasons it should stop are:
        //  a failed start
        //  a crash of the scp listener
        //  if we tell it to
        OFCondition status = b->listen();
        if( status.bad())
        {
            printf("SCP listener failed to start\n");
        }
        
        //Fail Stuff
        //will fail if we get 5 fails, each in under 2 minutes
        //if we ever run for over 2 minutes, the entire counter resets
        if(difftime( time(0), start) > 120)
        {
            x = 0;
        }
        x++;
        if(!b->getStopRunning())
        {
            printf("The SCP listener stopped or failed to start.\nThis is the %d stop within the window.\n", x);
        }
        //reset the timer
        start = time(0);
    }
}

void configureSCPListener(DcmStorageSCP *myListener, ConfigFile *cfg)
{
    //TODO - Port should be configurable
    //myListener->setPort(11112);
//    printf("%s\n", cfg->getValueOfKey<std::string>("port").c_str());
    myListener->setPort(stoi(cfg->getValueOfKey<std::string>("port")));
//    printf("port set\n");
    myListener->setAETitle("ANY-SCP");
    myListener->setMaxReceivePDULength(16384);
    myListener->setACSETimeout(30);
    myListener->setDIMSETimeout(0);
    myListener->setDIMSEBlockingMode(DIMSE_BLOCKING);
    myListener->setVerbosePCMode(OFFalse);
    myListener->setRespondWithCalledAETitle(OFTrue);
    myListener->setHostLookupEnabled(OFTrue);
    myListener->setFilenameGenerationMode(DcmStorageSCP::FGM_SOPInstanceUID);
    myListener->setFilenameExtension(".dcm");
    myListener->setDatasetStorageMode(DcmStorageSCP::DGM_StoreToFile);
    //myListener->setOutputDirectory("/Users/bswearingen/scpLT/Output/");
    myListener->setOutputDirectory(OFString((cfg->getValueOfKey<std::string>("output_dir")).c_str()));
//    printf("outdir set\n");
    myListener->setDirectoryGenerationMode(DcmStorageSCP::DGM_AEName);
    myListener->setConnectionBlockingMode(DUL_NOBLOCK);
    //not really a timeout how I'm using it. More of a check for shutdown command frequency.
    myListener->setConnectionTimeout(5);
    
    OFCondition status;
    //status = myListener->loadAssociationConfiguration("/Users/bswearingen/scpLT/storescp2.cfg", "Default");
    status = myListener->loadAssociationConfiguration(OFString((cfg->getValueOfKey<std::string>("assoc_config")).c_str()), "Default");
//    printf("assoc set\n");
//    printf("status\n");
    printf("%s\n", status.text());
}

ConfigFile getConfig()
{
    //TODO - change this to the directory this is launched from.
    ConfigFile temp("/Users/bswearingen/scpLT/config.cfg");
    return temp;
}

void startDirectoryWatch(ConfigFile *cfg)
{
    //TODO - would be better if the while loop captured the SCP listener running.
    //TODO - need a way to capture and restart a fail.
    WatchDirs w(cfg->getValueOfKey<std::string>("output_dir"));
    while(cfg->getValueOfKey<std::string>("keep_running") == "True")
    {
        w.runChecks();
        sleep(2);
    }
}

//void watchConfig(ConfigFile *config)
//{
//    printf("Start\n");
//    ConfigFile temp("/Users/bswearingen/scpLT/config.cfg");
//    config = &temp;
////    while( (config->getValueOfKey<std::string>("keep_running")) == "True")
////    {
////        //I'd prefer to watch the file and then update only if there's a change.
////        ConfigFile temp("/Users/bswearingen/scpLT/config.cfg");
////        //ConfigFile temp = loadConfig();
////        //I don't like this, I need to lock config here and anywhere it is accessed.
////        config = &temp;
//////        if( (config.getValueOfKey<std::string>("keep_running")) != (temp.getValueOfKey<std::string>("keep_running")))
//////        {
//////            (config.getValueOfKey<std::string>("keep_running")) = (temp.getValueOfKey<std::string>("keep_running"));
//////        }
////        sleep(5);
////        printf("%s\n", (config->getValueOfKey<std::string>("keep_running")).c_str() );
////    }
//    printf("Stop\n");
//}

int main(int /*argc*/, char * /*argv*/ [])
{
    //I'll need to set keep running to true here
    
    //Thread 0
    //Need an external way to change part of this.
    //who to email, whether or not to keep running, etc
    ConfigFile cfg = getConfig();
//    ConfigFile cfg = ConfigFile(); //loadConfig();
//    std::thread cfgThread(watchConfig, &cfg);
//    printf("Config thread started\n");
//    sleep(2);
    
    //Thread 1
    DcmStorageSCP myListener; // = DcmSCP();
    configureSCPListener(&myListener, &cfg);
    std::thread scp(startSCPListener, &myListener, &cfg);
    printf("SCP thread has launched.\n");
    
    //Thread 2
    std::thread dwatch(startDirectoryWatch, &cfg);
    printf("directory watch has launched.\n");
    
    //Thread 3
    //watch all subfolders
    //email on a regular basis about unhandled files
    //email on a regular basis about folders without an appropriate .[some appropriate name here] files
    //email any data keys on a regular basis

    
    //testing purposes for now
    sleep(12);
    //need a way to update this while running
    //printf("%s\n", ((cfg.getValueOfKey<std::string>("keep_running")) == "True")?"True":"False");
    cfg.updateKey("keep_running", "False");
    //TODO - The future logic that watches cfg for updates will need to map back to this location in myListener.
    myListener.setStopRunning(OFTrue);
    printf("stop running should now be active\n");
    //printf("%s\n", ((cfg.getValueOfKey<std::string>("keep_running")) == "True")?"True":"False");
    
//    cfgThread.join();
    scp.join();
    dwatch.join();
    printf("stopped\n");
}



