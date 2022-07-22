//
//  mapFwd.cpp
//  scpFwd.exe
//
//  Created by Brad Swearingen on 7/23/19.
//
#include "mapping.hpp"

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
    //TODO - This needs to be rewritten. I'm using it in a much different fashion than the original example.
    //I need to gather all PCs from the header of the files I will be passing (maybe pass the whole dataset to here?)
    //Then create the connection with all needed PCs.
    //Probably two functions: getAllPCs(dataset), createConnection(PCs)
    //Send then becomes trivial as it no longer needs a PC ID
    //Need to figure out how to send a "finished" message as well
    
//    DcmSCU scu;
//    T_ASC_PresentationContextID presID;
    OFList<OFString> ts;
    OFList<OFString> ts2;
    scu.setAETitle(AETitle.c_str());
    scu.setPeerAETitle(AETitle.c_str());
    scu.setPeerHostName(Dest.c_str());
    scu.setPeerPort(DestPort);
    /* Presentation context */
    //Squash verbose output - see if this helps speed.
    scu.setVerbosePCMode(OFFalse);
    scu.setProgressNotificationMode(OFFalse);
    
    //add the presentation context per the dicom header
    //should update to better names
    OFString seed;
    OFString seed2;
    mi.findAndGetOFString(DCM_TransferSyntaxUID, seed);
    mi.findAndGetOFString(DCM_MediaStorageSOPClassUID, seed2);
    ts2.push_back(seed);
    scu.addPresentationContext(seed2, ts2);
    //scu.addPresentationContext(UID_MRImageStorage, ts2);
    
    //add some general case presentation contexts
    ts.push_back(UID_LittleEndianExplicitTransferSyntax);
    ts.push_back(UID_BigEndianExplicitTransferSyntax);
    ts.push_back(UID_LittleEndianImplicitTransferSyntax);
    scu.addPresentationContext(UID_MRImageStorage, ts);
    scu.addPresentationContext(UID_CTImageStorage, ts);
    scu.addPresentationContext(UID_FINDStudyRootQueryRetrieveInformationModel, ts);
    scu.addPresentationContext(UID_MOVEStudyRootQueryRetrieveInformationModel, ts);
    scu.addPresentationContext(UID_VerificationSOPClass, ts);
    
    //add some special case pcs we use often
    OFList<OFString> ts_leiONLY;
    ts_leiONLY.push_back(UID_LittleEndianImplicitTransferSyntax);
    scu.addPresentationContext(UID_RTStructureSetStorage, ts_leiONLY);
    scu.addPresentationContext(UID_RTDoseStorage, ts_leiONLY);
    scu.addPresentationContext(UID_RTPlanStorage, ts_leiONLY);
    
    //no longer using
    //scu.setDatasetConversionMode(OFTrue);
    
    
    //printf("storage mode: %i\n", scu.getStorageMode());
    scu.setStorageMode(DCMSCU_STORAGE_BIT_PRESERVING);
    //printf("storage mode post: %i\n", scu.getStorageMode());
    
    
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
        //not a show stopper, as some won't respond to echo, but definitely log this.
        printf("echo: %s\n", result.text());
        //scu.releaseAssociation();
        //return 0;
    }
    Uint8 pc;
    pc = scu.findPresentationContextID(UID_RTStructureSetStorage,UID_LittleEndianImplicitTransferSyntax );
    result = scu.sendECHORequest(pc);
    printf("echo test: %i %s\n", pc, result.text());
    
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
    OFCondition result;
    //OFFilename fn = OFFilename(fPath.c_str());
    if(presID != 0)
    {
        //if we've found any successful presID - redundant, we could streamline this.
        //use the one set by the file
        result = scu.sendSTORERequest(0, 0, dataset, response);
        //printf("result: %s     response: %u\n",result.text(), response);
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
    
    //add some standard presentation contexts
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


