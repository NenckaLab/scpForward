//
//  mapFwd.cpp
//  scpFwd.exe
//
//  Created by Brad Swearingen on 7/23/19.
//

//#include "mapFwd.hpp"
//#include <stdio.h>
//#define BOOST_NO_CXX11_SCOPED_ENUMS
////need BOOST_NO_CXX11_SCOPED_ENUMS to prevent bug on boost::filesystem::copy_file from showing up in Linux compiler
//#include <boost/filesystem.hpp>
//#undef BOOST_NO_CXX11_SCOPED_ENUMS
//#include <boost/archive/text_oarchive.hpp>
//#include <boost/generator_iterator.hpp>
//#include <boost/random.hpp>
//#include <boost/random/random_device.hpp>
//#include <boost/algorithm/string.hpp>
//#include <chrono>
//#include <thread>
//#include <random>
//#include <ctime>
#include "mapping.hpp"
//#include "dcmtk/config/osconfig.h"
//#include "loadConfig.h"
//#include "md5.h"


////helper function
//std::string splitOnChar2(std::string &stToParse, char splitter)
//{
//    size_t sepPos = stToParse.find(splitter);
//    std::string returnval;
//    if(sepPos != std::string::npos)
//    {
//        returnval = stToParse.substr(0,sepPos);
//        stToParse = stToParse.substr(sepPos + 1);
//    } else {
//        returnval = stToParse;
//        //returnval.assign(stToParse);
//        stToParse = "";
//    }
//    return returnval;
//}


Forward::Forward(std::string p_AETitle, std::string p_Dest, unsigned short p_DestPort)
:AETitle(std::move(p_AETitle)), Dest(std::move(p_Dest)),DestPort(p_DestPort){}
Forward::Forward(std::string csvList)
{
    //TODO - can I reuse this structure for forwarding to another AETITLE?, add a method - r(aw), f(ormatted)(default)
    //need to give this some thought.
    std::string addr = splitOnChar(csvList, ',');
    long port = std::strtol(splitOnChar(csvList, ',').c_str(), NULL, 10);
    std::string aetitle = csvList;
    //*this=Forward(aetitle, addr, port);
    AETitle = aetitle;
    DestPort = port;
    Dest = addr;
    //*this=Forward::Clone(Forward(aetitle, addr, port));
}
//override copy constructor because it is implicitly deleted due to DcmSCU being global.
Forward::Forward(const Forward &other)
{
    AETitle = *new std::string;
    AETitle = other.AETitle;
    Dest = *new std::string;
    Dest = other.Dest;
    DestPort = *new unsigned short;
    DestPort = other.DestPort;
    //    scu = new DcmSCU; - we really don't want this to be populated on a new instance
//    presID = *new T_ASC_PresentationContextID;
}

static Uint8 findAnyPc(const OFString& sopClass, DcmSCU& scu, OFString& ts)
{
    Uint8 pc;
    pc = scu.findPresentationContextID(sopClass, ts);
    return pc;
}
T_ASC_PresentationContextID Forward::getPresentationID(){
    return presID;
}
T_ASC_PresentationContextID Forward::CreateConnection(DcmMetaInfo &mi)
{
//    DcmSCU scu;
//    T_ASC_PresentationContextID presID;
    OFList<OFString> ts;
    OFList<OFString> ts2;
    scu.setAETitle(AETitle.c_str());
    scu.setPeerAETitle(AETitle.c_str());
    scu.setPeerHostName(Dest.c_str());
    scu.setPeerPort(DestPort);
    /* Presentation context */
    
    
    //    ts.push_back(UID_JPEGLSLosslessTransferSyntax);
    //    ts.push_back(UID_JPEG2000LosslessOnlyTransferSyntax);
    ts.push_back(UID_LittleEndianExplicitTransferSyntax);
    ts.push_back(UID_BigEndianExplicitTransferSyntax);
    ts.push_back(UID_LittleEndianImplicitTransferSyntax);
    //    ts.push_back(UID_DeflatedExplicitVRLittleEndianTransferSyntax );
    //    ts.push_back(UID_JPEGProcess1TransferSyntax    );
    //    ts.push_back(UID_JPEGProcess2_4TransferSyntax );
    //    ts.push_back(UID_JPEGProcess3_5TransferSyntax  );
    //    ts.push_back(UID_JPEGProcess6_8TransferSyntax       );
    //    ts.push_back(UID_JPEGProcess7_9TransferSyntax );
    //    ts.push_back(UID_JPEGProcess10_12TransferSyntax  );
    //    ts.push_back(UID_JPEGProcess11_13TransferSyntax );
    //    ts.push_back(UID_JPEGProcess14TransferSyntax    );
    //    ts.push_back(UID_JPEGProcess15TransferSyntax  );
    //    ts.push_back(UID_JPEGProcess16_18TransferSyntax  );
    //    ts.push_back(UID_JPEGProcess17_19TransferSyntax   );
    //    ts.push_back(UID_JPEGProcess20_22TransferSyntax  );
    //    ts.push_back(UID_JPEGProcess21_23TransferSyntax   );
    //    ts.push_back(UID_JPEGProcess24_26TransferSyntax  );
    //    ts.push_back(UID_JPEGProcess25_27TransferSyntax   );
    //    ts.push_back(UID_JPEGProcess28TransferSyntax        );
    //    ts.push_back(UID_JPEGProcess29TransferSyntax         );
    //    ts.push_back(UID_JPEGLSLossyTransferSyntax    );
    //    ts.push_back(UID_JPEG2000LosslessOnlyTransferSyntax );
    //    ts.push_back(UID_JPEG2000TransferSyntax        );
    //    ts.push_back(UID_JPEG2000Part2MulticomponentImageCompressionLosslessOnlyTransferSyntax);
    //    ts.push_back(UID_JPEG2000Part2MulticomponentImageCompressionTransferSyntax);
    //    ts.push_back(UID_JPIPReferencedTransferSyntax  );
    //    ts.push_back(UID_JPIPReferencedDeflateTransferSyntax);
    //    ts.push_back(UID_MPEG2MainProfileAtMainLevelTransferSyntax );
    //    ts.push_back(UID_MPEG2MainProfileAtHighLevelTransferSyntax );
    //    ts.push_back(UID_MPEG4HighProfileLevel4_1TransferSyntax);
    //    ts.push_back(UID_MPEG4BDcompatibleHighProfileLevel4_1TransferSyntax);
    //    ts.push_back(UID_MPEG4HighProfileLevel4_2_For2DVideoTransferSyntax);
    //    ts.push_back(UID_MPEG4HighProfileLevel4_2_For3DVideoTransferSyntax);
    //    ts.push_back(UID_MPEG4StereoHighProfileLevel4_2TransferSyntax);
    //    ts.push_back(UID_HEVCMainProfileLevel5_1TransferSyntax);
    //    ts.push_back(UID_HEVCMain10ProfileLevel5_1TransferSyntax);
    //    ts.push_back(UID_RLELosslessTransferSyntax );
    //    ts.push_back(UID_RFC2557MIMEEncapsulationTransferSyntax);
    //    ts.push_back(UID_XMLEncodingTransferSyntax );
    //    ts.push_back(UID_PrivateGE_LEI_WithBigEndianPixelDataTransferSyntax);
    //scu.addPresentationContext(UID_MRImageStorage, ts);
    
    //add the presentation context per the dicom header
    //should update to better names
    OFString seed;
    OFString seed2;
    mi.findAndGetOFString(DCM_TransferSyntaxUID, seed);
    mi.findAndGetOFString(DCM_MediaStorageSOPClassUID, seed2);
    ts2.push_back(seed);
    scu.addPresentationContext(seed2, ts2);
    //scu.addPresentationContext(UID_MRImageStorage, ts2);
    
    scu.addPresentationContext(UID_FINDStudyRootQueryRetrieveInformationModel, ts);
    scu.addPresentationContext(UID_MOVEStudyRootQueryRetrieveInformationModel, ts);
    scu.addPresentationContext(UID_VerificationSOPClass, ts);
    scu.setDatasetConversionMode(OFTrue);
    /* Negotiate Association */
    
    /* Initialize network */
    time_t now = time(0);
    printf("Sending: %s\n", ctime(&now) );
    OFCondition result = scu.initNetwork();
    if (result.bad())
    {
        printf("init: %s\n",result.text());
        scu.releaseAssociation();
        return 0;
    }
    
    result = scu.negotiateAssociation();
    if (result.bad())
    {
        printf("neg: %s\n", result.text());
        scu.releaseAssociation();
        return 0;
    }
    
    /* Let's look whether the server is listening:
     Assemble and send C-ECHO request
     */
    result = scu.sendECHORequest(0);
    if (result.bad())
    {
        printf("echo: %s\n", result.text());
        scu.releaseAssociation();
        return 0;
    }
    
    presID = findAnyPc(seed2, scu, seed);
    
    if(presID == 0)
    {
        printf("Unknown presentation context. %s : %s\n",seed.c_str(),seed2.c_str());
        scu.releaseAssociation();
        return 0;
    }
    return presID;
}
bool Forward::Send(DcmDataset *dataset, T_ASC_PresentationContextID presID)
{
    Uint16 response;
    //OFFilename fn = OFFilename(fPath.c_str());
    if(presID != 0)
    {
        //TODO - also make sure scu is initialized. Not really a huge deal as it fails if it isn't.
        //scu.sendSTORERequest(presID, fn, 0, response);
        scu.sendSTORERequest(presID, 0, dataset, response);
        //virtual OFCondition sendSTORERequest(const T_ASC_PresentationContextID presID,
//                                             const OFFilename &dicomFile,
//                                             DcmDataset *dataset,
//                                             Uint16 &rspStatusCode,
//                                             const OFString &moveOriginatorAETitle = "",
//                                             const Uint16 moveOriginatorMsgID = 0);
        return true;
    }
    return false;
}
bool Forward::Release()
{
    //TODO - figure out a way to make sure we always release the association. I think we can also send a finalize message.
    scu.releaseAssociation();
    return false;
}


bool Forward::Send(std::string fPath, DcmMetaInfo &mi )
{
    //composite option - break this out once I write the subroutines
    //TODO - if I reuse as mentioned above, this will need to handle both methods
//    DcmSCU scu;
//    T_ASC_PresentationContextID presID;
    
    OFFilename fn = OFFilename(fPath.c_str());
    OFList<OFString> ts;
    OFList<OFString> ts2;
    Uint16 response;
    
    scu.setAETitle(AETitle.c_str());
    scu.setPeerAETitle(AETitle.c_str());
    scu.setPeerHostName(Dest.c_str());
    scu.setPeerPort(DestPort);
    /* Presentation context */
    
    
    //    ts.push_back(UID_JPEGLSLosslessTransferSyntax);
    //    ts.push_back(UID_JPEG2000LosslessOnlyTransferSyntax);
    ts.push_back(UID_LittleEndianExplicitTransferSyntax);
    ts.push_back(UID_BigEndianExplicitTransferSyntax);
    ts.push_back(UID_LittleEndianImplicitTransferSyntax);
    //    ts.push_back(UID_DeflatedExplicitVRLittleEndianTransferSyntax );
    //    ts.push_back(UID_JPEGProcess1TransferSyntax    );
    //    ts.push_back(UID_JPEGProcess2_4TransferSyntax );
    //    ts.push_back(UID_JPEGProcess3_5TransferSyntax  );
    //    ts.push_back(UID_JPEGProcess6_8TransferSyntax       );
    //    ts.push_back(UID_JPEGProcess7_9TransferSyntax );
    //    ts.push_back(UID_JPEGProcess10_12TransferSyntax  );
    //    ts.push_back(UID_JPEGProcess11_13TransferSyntax );
    //    ts.push_back(UID_JPEGProcess14TransferSyntax    );
    //    ts.push_back(UID_JPEGProcess15TransferSyntax  );
    //    ts.push_back(UID_JPEGProcess16_18TransferSyntax  );
    //    ts.push_back(UID_JPEGProcess17_19TransferSyntax   );
    //    ts.push_back(UID_JPEGProcess20_22TransferSyntax  );
    //    ts.push_back(UID_JPEGProcess21_23TransferSyntax   );
    //    ts.push_back(UID_JPEGProcess24_26TransferSyntax  );
    //    ts.push_back(UID_JPEGProcess25_27TransferSyntax   );
    //    ts.push_back(UID_JPEGProcess28TransferSyntax        );
    //    ts.push_back(UID_JPEGProcess29TransferSyntax         );
    //    ts.push_back(UID_JPEGLSLossyTransferSyntax    );
    //    ts.push_back(UID_JPEG2000LosslessOnlyTransferSyntax );
    //    ts.push_back(UID_JPEG2000TransferSyntax        );
    //    ts.push_back(UID_JPEG2000Part2MulticomponentImageCompressionLosslessOnlyTransferSyntax);
    //    ts.push_back(UID_JPEG2000Part2MulticomponentImageCompressionTransferSyntax);
    //    ts.push_back(UID_JPIPReferencedTransferSyntax  );
    //    ts.push_back(UID_JPIPReferencedDeflateTransferSyntax);
    //    ts.push_back(UID_MPEG2MainProfileAtMainLevelTransferSyntax );
    //    ts.push_back(UID_MPEG2MainProfileAtHighLevelTransferSyntax );
    //    ts.push_back(UID_MPEG4HighProfileLevel4_1TransferSyntax);
    //    ts.push_back(UID_MPEG4BDcompatibleHighProfileLevel4_1TransferSyntax);
    //    ts.push_back(UID_MPEG4HighProfileLevel4_2_For2DVideoTransferSyntax);
    //    ts.push_back(UID_MPEG4HighProfileLevel4_2_For3DVideoTransferSyntax);
    //    ts.push_back(UID_MPEG4StereoHighProfileLevel4_2TransferSyntax);
    //    ts.push_back(UID_HEVCMainProfileLevel5_1TransferSyntax);
    //    ts.push_back(UID_HEVCMain10ProfileLevel5_1TransferSyntax);
    //    ts.push_back(UID_RLELosslessTransferSyntax );
    //    ts.push_back(UID_RFC2557MIMEEncapsulationTransferSyntax);
    //    ts.push_back(UID_XMLEncodingTransferSyntax );
    //    ts.push_back(UID_PrivateGE_LEI_WithBigEndianPixelDataTransferSyntax);
    //scu.addPresentationContext(UID_MRImageStorage, ts);
    
    //add the presentation context per the dicom header
    //should update to better names
    OFString seed;
    OFString seed2;
    mi.findAndGetOFString(DCM_TransferSyntaxUID, seed);
    mi.findAndGetOFString(DCM_MediaStorageSOPClassUID, seed2);
    ts2.push_back(seed);
    scu.addPresentationContext(seed2, ts2);
    //scu.addPresentationContext(UID_MRImageStorage, ts2);
    
    scu.addPresentationContext(UID_FINDStudyRootQueryRetrieveInformationModel, ts);
    scu.addPresentationContext(UID_MOVEStudyRootQueryRetrieveInformationModel, ts);
    scu.addPresentationContext(UID_VerificationSOPClass, ts);
    scu.setDatasetConversionMode(OFTrue);
    /* Negotiate Association */
    
    /* Initialize network */
    time_t now = time(0);
    printf("Sending: %s\n", ctime(&now) );
    OFCondition result = scu.initNetwork();
    if (result.bad())
    {
        printf("init: %s\n",result.text());
        return 1;
    }
    
    result = scu.negotiateAssociation();
    if (result.bad())
    {
        printf("neg: %s\n", result.text());
        return 1;
    }
    
    /* Let's look whether the server is listening:
     Assemble and send C-ECHO request
     */
    result = scu.sendECHORequest(0);
    if (result.bad())
    {
        printf("echo: %s\n", result.text());
        return 1;
    }
    
    //verify presentation context, mostly trivial at this point
    //T_ASC_PresentationContextID presID = findUncompressedPC(UID_MRImageStorage, scu);
    presID = findAnyPc(seed2, scu, seed);
    
    if(presID == 0)
    {
        printf("Unknown presentation context. %s : %s\n",seed.c_str(),seed2.c_str());
        return 1;
    }
    
    scu.sendSTORERequest(presID, fn, 0, response);
    //scu.closeAssociation(DCMSCU_RELEASE_ASSOCIATION);
    scu.releaseAssociation();
    return false;
}
std::string Forward::toString(){return ("IP: "+Dest+"   Port: "+std::to_string(DestPort)+"   Value: "+AETitle);}

