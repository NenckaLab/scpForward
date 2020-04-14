#include "dcmtk/config/osconfig.h"
#include "dcmtk/ofstd/ofstring.h"
#include "dstorscp2.h"
#include <thread>
#include <time.h>
#include "loadConfig.h"
#include "watchDirs.hpp"

#define VERSION_MAJOR 1
#define VERSION_MINOR 3
#define VERSION_REVISION 6

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
            printf("The SCP listener stopped or failed to start.\n%d stop(s) within the window.\n", x);
        }
        //reset the timer
        start = time(0);
    }
    printf("SCP listener has started");
}

void configureSCPListener(DcmStorageSCP *myListener, ConfigFile *cfg)
{
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
    myListener->setOutputDirectory(OFString((cfg->getValueOfKey<std::string>("output_dir")).c_str()));
    myListener->setDirectoryGenerationMode(DcmStorageSCP::DGM_AEName);
    myListener->setConnectionBlockingMode(DUL_NOBLOCK);
    //not really a timeout how I'm using it. More of a check for shutdown command frequency.
    myListener->setConnectionTimeout(5);
    
    OFCondition status;
    status = myListener->loadAssociationConfiguration(OFString((cfg->getValueOfKey<std::string>("assoc_config")).c_str()), "Default");
    printf("SCP config load status: %s\n", status.text());
}

ConfigFile getConfig()
{
    ConfigFile temp("config.cfg");
    return temp;
}

void startDirectoryWatch(ConfigFile *cfg)
{
    //TODO - need a way to capture and restart a fail.
    WatchDirs w(cfg->getValueOfKey<std::string>("output_dir"), cfg->getValueOfKey<std::string>("finished_dir"), cfg->getValueOfKey<std::string>("admin_email"));
    while(cfg->getValueOfKey<std::string>("keep_running") == "True")
    {
        w.runChecks();
        sleep(2);
    }
}

void startDirectorySort(ConfigFile *cfg)
{
    //TODO - need a way to capture and restart a fail.
    WatchDirs w(cfg->getValueOfKey<std::string>("output_dir"), cfg->getValueOfKey<std::string>("finished_dir"), cfg->getValueOfKey<std::string>("admin_email"));
    while(cfg->getValueOfKey<std::string>("keep_running") == "True")
    {
        w.sortChecks();
        sleep(2);
    }
}

int main(int /*argc*/, char * /*argv*/ [])
{
    
    printf("Version %d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION);
    
    ConfigFile cfg = getConfig();
    
    //Thread 1
    DcmStorageSCP myListener; // = DcmSCP();
    configureSCPListener(&myListener, &cfg);
    std::thread scp(startSCPListener, &myListener, &cfg);
    printf("SCP thread has launched.\n");
    
    //Thread 2
    std::thread dwatch(startDirectoryWatch, &cfg);
    printf("directory processor has launched.\n");
    
    //Thread 3
    std::thread dsort(startDirectorySort, &cfg);
    printf("directory sorter has launched.\n");
    
    
    //TODO - The future logic that watches cfg for updates will need to map back to this location in myListener.
    //Do I really need to turn off both?
//    sleep(12);
//    cfg.updateKey("keep_running", "False");
//    myListener.setStopRunning(OFTrue);
//    printf("stop running should now be active\n");
    
//    cfgThread.join();
    scp.join();
    dwatch.join();
    dsort.join();
    printf("stopped\n");
}



