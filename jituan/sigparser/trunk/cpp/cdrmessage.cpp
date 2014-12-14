/***************************************************************************
 fileName    : cdrmessage.cpp  
 begin       : 2011-06-16
 copyright   : (C) 2011 by wyt
 ***************************************************************************/
#include "cdrmessage.h"
#include "message.h"
#include "zxcomm.h"
#include "zxclient.h"
#include "g.h"
#include "hwsshare.h"
#include "sugwserver.h"


CDRMessage::CDRMessage() {
    m_msgSeqID = 0;
    m_pZxClient = 0;
    m_zxSocket = -1;  
    m_zxPort = -1;
    m_zxHostIP = "";
    
    m_CDRBuf[0] = 0;
	m_CDRLen = 0;
	m_CDRVersion = 0;
	m_VarIENumber = 0;
	m_protocolType = 0;
	m_CDRID = 0;
	
	m_startTime[0] = 0;
	m_startTime[1] = 0;
	m_endTime[0] = 0;
	m_endTime[1] = 0;
	m_caller[0] = 0;
	m_called[0] = 0;
    m_status = 0;
    m_CIC = 0;
    m_opc[0] = 0;
    m_dpc[0] = 0;
    m_relReason = 0;
    m_srcIP[0] = 0;
    m_dstIP[0] = 0;
    m_callSeqNum[0] = 0;
    m_packID = 0;
    m_callID = 0;
    
    m_bMatched = false;
    m_bSend = false;
}

CDRMessage::~CDRMessage() {
}


void CDRMessage::setZxClient(ZxClient* pClient){
    m_pZxClient = pClient;
    if(pClient){
        m_zxPort = pClient->getPort();
        m_zxHostIP = pClient->getHostIP();
        m_zxSocket = pClient->getSocket();
    }
}


void CDRMessage::setMessageBuf(PUCHAR pData, UINT dataLen){
    memset(m_CDRBuf,0,MAX_DATA_MSG_BODY_LEN);
    memcpy(m_CDRBuf, pData, dataLen);
    m_CDRBuf[dataLen] = 0;
    m_CDRLen = dataLen;
}

/*
notify CDR/TDR data: 
IAM time(8B) + callingPartyNumber + calledpartyNumber + redirect number + trigger tag(1B)

0: success
-1:failed, invalid message
*/
int CDRMessage::parseMessage(){
    PUCHAR ptr = m_CDRBuf;
    int ret = 0;
    static UINT nValid = 0;
	UCHAR redirectCalled[PHONE_NUMBER_LEN+1];	
    memset(redirectCalled,0,PHONE_NUMBER_LEN+1);

    //get IAM time
    readTime(ptr,m_startTime);
    ptr +=8;
    
    //get callingPartyNumber
    UINT len = (UINT)*ptr;
    ptr++;
    if(len>PHONE_NUMBER_LEN || len<MIN_PHONE_NUMBER_LEN){
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_ERROR(rootLogger, "CDRMessage: wrong callerNumber len="<<len);
        G::displayHex("CDRMessage: wrong callerNumber len",m_CDRBuf,m_CDRLen);
        return -1;
    }else{
        memcpy(m_caller,ptr,len);
        m_caller[len] = 0;
        ptr += len;
    }

    //get calledPartyNumber
    len = (UINT)*ptr;
    ptr++;
    if(len>PHONE_NUMBER_LEN || len<MIN_PHONE_NUMBER_LEN){
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_ERROR(rootLogger, "CDRMessage: wrong calledNumber len="<<len);
        G::displayHex("CDRMessage: wrong calledNumber len",m_CDRBuf,m_CDRLen);
        return -1;
    }else{
        memcpy(m_called,ptr,len);
        m_called[len] = 0;
        ptr += len;
    }

    //get redirect Number
    len = (UINT)*ptr;
    ptr++;
    if(0 != len){
        if(len>PHONE_NUMBER_LEN || len<MIN_PHONE_NUMBER_LEN){
            LoggerPtr rootLogger = Logger::getRootLogger();  
            LOG4CXX_ERROR(rootLogger, "CDRMessage: wrong redirect len="<<len);
            G::displayHex("CDRMessage: wrong redirect len",m_CDRBuf,m_CDRLen);
            return -1;
        }else{
            memcpy(redirectCalled,ptr,len);
            redirectCalled[len] = 0;
            ptr += len;
        }
    }

    //get trigger tag
    m_status = (UINT)*ptr;


    ret = checkDelay();
    if(0 != ret){
        return -1;
    }

    //check caller
    ret = checkCallNumber(m_caller);
    if(-1 == ret){
        if(G::isTestPhone((char*)m_caller) || G::isTestPhone((char*)m_called)){
            LoggerPtr rootLogger = Logger::getRootLogger();  
            LOG4CXX_DEBUG(rootLogger, "invalid call: "<<m_caller<<"->"<<m_called<<", status="<<m_status);
            G::displayHex("invalid call",m_CDRBuf,m_CDRLen);
        }
        
        if(++nValid%10000 == 0){
            LoggerPtr rootLogger = Logger::getRootLogger(); 
            LOG4CXX_INFO(rootLogger, "invalid caller="<<m_caller<<", called="<<m_called);

            if(strlen((char*)m_caller)<11 || strlen((char*)m_called)<11){
                G::displayHex("invalid call",m_CDRBuf,m_CDRLen);
            }
        }
        return -1;
    }else if(1 == ret){
        ZxComm::m_nVpnDRs++;
    }else if(2 == ret){
        ZxComm::m_nFmlDRs++;
    }

    //check called
    ret = checkCallNumber(m_called);
    if(-1 == ret){
        if(G::isTestPhone((char*)m_caller) || G::isTestPhone((char*)m_called)){
            LoggerPtr rootLogger = Logger::getRootLogger();  
            LOG4CXX_DEBUG(rootLogger, "invalid call: "<<m_caller<<"->"<<m_called<<", status="<<m_status);
            G::displayHex("invalid call",m_CDRBuf,m_CDRLen);
        }
        
        if(++nValid%10000 == 0){
            LoggerPtr rootLogger = Logger::getRootLogger(); 
            LOG4CXX_INFO(rootLogger, "BICC: invalid caller="<<m_caller<<", called="<<m_called);

            if(strlen((char*)m_caller)<11 || strlen((char*)m_called)<11){
                G::displayHex("invalid call",m_CDRBuf,m_CDRLen);
            }
        }
        return -1;
    }else if(1==ret || 2==ret){
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_ERROR(rootLogger, "wrong call: "<<m_caller<<"->"<<m_called<<", status="<<m_status);
        G::displayHex("wrong call",m_CDRBuf,m_CDRLen);
    }
    
    
    //set sequence number
    ret = setSeqNum();
    if(-1 == ret){
        return -1;
    }
    

    char* ss = (char*)G::m_testZone.c_str();
    if((0 == strncmp((char*)m_caller, ss, 7)) || (0 == strncmp((char*)m_called, ss, 7))){
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_DEBUG(rootLogger, "<== "<<m_caller<<"->"<<m_called<<", redirectCalled="<<redirectCalled
            <<", status="<<m_status<<", procID="<<m_procID<<", seqID="<<m_msgSeqID
            <<", start="<<m_startTime[0]<<"s"<<m_startTime[1]<<"ns");
        //G::displayHex("",m_CDRBuf,m_CDRLen);
     }

     if(G::isTestPhone((char*)m_caller) || G::isTestPhone((char*)m_called) || G::isTestPhone((char*)redirectCalled)){
         LoggerPtr rootLogger = Logger::getRootLogger();  
         LOG4CXX_DEBUG(rootLogger, "<== "<<m_caller<<"->"<<m_called<<", redirectCalled="<<redirectCalled
             <<", status="<<m_status<<", procID="<<m_procID<<", seqID="<<m_msgSeqID
             <<", start="<<m_startTime[0]<<"s"<<m_startTime[1]<<"ns");
         G::displayHex("",m_CDRBuf,m_CDRLen);
     }

    return 0;    
}


int CDRMessage::parseBICC_Message(PUCHAR pData){
    PUCHAR ptr = pData;
    int ret = 0;
    static int nValid = 0;
    
    //parse Fixed Load
    ret = parseBICC_FixLoad(ptr);
    if(0 != ret){
        return -1;
    }
    ptr += 74;

    //parse Optional Load
    if(m_VarIENumber>0){
        ret = parseOptLoad(ptr);
        if(0 != ret){
            return -1;
        }
    }

    ret = checkDelay();
    if(0 != ret){
        return -1;
    }

    //check caller
    ret = checkCallNumber(m_caller);
    if(-1 == ret){
        if(G::isTestPhone((char*)m_caller) || G::isTestPhone((char*)m_called)){
            LoggerPtr rootLogger = Logger::getRootLogger();  
            LOG4CXX_DEBUG(rootLogger, "invalid call: "<<m_caller<<"->"<<m_called<<", type="<<m_protocolType<<", status="<<m_status);
            G::displayHex("invalid call",m_CDRBuf,m_CDRLen);
        }
        
        if(++nValid%10000 == 0){
            LoggerPtr rootLogger = Logger::getRootLogger(); 
            LOG4CXX_INFO(rootLogger, "BICC: invalid caller="<<m_caller<<", called="<<m_called);

            if(strlen((char*)m_caller)<11 || strlen((char*)m_called)<11){
                G::displayHex("invalid call",m_CDRBuf,m_CDRLen);
            }
        }
        return -1;
    }else if(1 == ret){
        ZxComm::m_nVpnDRs++;
    }else if(2 == ret){
        ZxComm::m_nFmlDRs++;
    }

    //check called
    ret = checkCallNumber(m_called);
    if(-1 == ret){
        if(G::isTestPhone((char*)m_caller) || G::isTestPhone((char*)m_called)){
            LoggerPtr rootLogger = Logger::getRootLogger();  
            LOG4CXX_DEBUG(rootLogger, "invalid call: "<<m_caller<<"->"<<m_called<<", type="<<m_protocolType<<", status="<<m_status);
            G::displayHex("invalid call",m_CDRBuf,m_CDRLen);
        }
        
        if(++nValid%10000 == 0){
            LoggerPtr rootLogger = Logger::getRootLogger(); 
            LOG4CXX_INFO(rootLogger, "BICC: invalid caller="<<m_caller<<", called="<<m_called);

            if(strlen((char*)m_caller)<11 || strlen((char*)m_called)<11){
                G::displayHex("invalid call",m_CDRBuf,m_CDRLen);
            }
        }
        return -1;
    }else if(1==ret || 2==ret){
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_ERROR(rootLogger, "wrong call: "<<m_caller<<"->"<<m_called<<", type="<<m_protocolType<<", status="<<m_status);
        G::displayHex("wrong call",m_CDRBuf,m_CDRLen);
    }
    
    //set sequence number
    ret = setSeqNum();
    if(-1 == ret){
        return -1;
    }

    return 0;
}


int CDRMessage::parseISUP_Message(PUCHAR pData){
    PUCHAR ptr = pData;
    int ret = 0;
    static int nValid = 0;
    
    if(m_VarIENumber<1){
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_ERROR(rootLogger, "ISUP: wrong VarIENumber="<<m_VarIENumber);
        G::displayHex("ISUP: wrong VarIENumber",m_CDRBuf,m_CDRLen);
        return -1;
    }
    
    ret = parseISUP_FixLoad(ptr);
    if(0 != ret){
        return -1;
    }
    ptr += 16;

    //parse Optional Load
    ret = parseOptLoad(ptr);
    if(0 != ret){
        return -1;
    }

    ret = checkDelay();
    if(0 != ret){
        return -1;
    }

    //check caller
    ret = checkCallNumber(m_caller);
    if(-1 == ret){
        if(G::isTestPhone((char*)m_caller) || G::isTestPhone((char*)m_called)){
            LoggerPtr rootLogger = Logger::getRootLogger();  
            LOG4CXX_DEBUG(rootLogger, "invalid call: "<<m_caller<<"->"<<m_called<<", type="<<m_protocolType<<", status="<<m_status);
            G::displayHex("invalid call",m_CDRBuf,m_CDRLen);
        }
        
        if(++nValid%10000 == 0){
            LoggerPtr rootLogger = Logger::getRootLogger(); 
            LOG4CXX_INFO(rootLogger, "ISUP: invalid caller="<<m_caller<<", called="<<m_called);

            if(strlen((char*)m_caller)<11 || strlen((char*)m_called)<11){
                G::displayHex("invalid call",m_CDRBuf,m_CDRLen);
            }
        }
        return -1;
    }else if(1 == ret){
        ZxComm::m_nVpnDRs++;
    }else if(2 == ret){
        ZxComm::m_nFmlDRs++;
    }
    
    //check called
    ret = checkCallNumber(m_called);
    if(-1 == ret){
        if(G::isTestPhone((char*)m_caller) || G::isTestPhone((char*)m_called)){
            LoggerPtr rootLogger = Logger::getRootLogger();  
            LOG4CXX_DEBUG(rootLogger, "invalid call: "<<m_caller<<"->"<<m_called<<", type="<<m_protocolType<<", status="<<m_status);
            G::displayHex("invalid call",m_CDRBuf,m_CDRLen);
        }
        
        if(++nValid%10000 == 0){
            LoggerPtr rootLogger = Logger::getRootLogger(); 
            LOG4CXX_INFO(rootLogger, "ISUP: invalid caller="<<m_caller<<", called="<<m_called);

            if(strlen((char*)m_caller)<11 || strlen((char*)m_called)<11){
                G::displayHex("invalid call",m_CDRBuf,m_CDRLen);
            }
        }
        return -1;
    }else if(1 == ret){
        ZxComm::m_nVpnDRs++;
    }else if(2 == ret){
        ZxComm::m_nFmlDRs++;
    }

    //set sequence number
    ret = setSeqNum();
    if(-1 == ret){
        return -1;
    }

    return 0;
}

/*
ISUP Fix Load(16): 
Start Time(8B) + End Time(8B) 

0: success
-1:failed, invalid message
*/
int CDRMessage::parseISUP_FixLoad(PUCHAR pData){
    PUCHAR ptr = pData;
    
    //get startTime(8B): sec(4B) + nSec(4B)
    readTime(ptr,m_startTime);
    ptr +=8;
    
    //get endTime(8B): sec(4B) + nSec(4B)
    readTime(ptr,m_endTime);
    ptr +=8;
    
    return 0;
}


/*
BICC Fix Load(74): 
Start Time(8B) + End Time(8B) + CIC (Call Instance Code)(4B) 
+ OPC (Originating Point Code)(4B) + DPC (Destination Point Code)(4B)
+ Release Reason(1B) + Calling Party Number(16B) + Called Party Number(16B) 
+ Status(1B)  + TimeoutBits(4B) + SourceIP(4B) + DestIP(4B)

0: success
-1:failed, invalid message
*/
int CDRMessage::parseBICC_FixLoad(PUCHAR pData){
    PUCHAR ptr = pData;
    
    //get startTime(8B): sec(4B) + nSec(4B)
    readTime(ptr,m_startTime);
    ptr +=8;
    
    //get endTime(8B): sec(4B) + nSec(4B)
    readTime(ptr,m_endTime);
    ptr +=8;
    
    //get CIC(4B)
    m_CIC = (ULONG)(((((*ptr<<8)+*(ptr+1))<<8)+*(ptr+2))<<8)+*(ptr+3);
    ptr +=4;

    //get OPC&DPC
    int ret = readBICC_PC(ptr);
    if(-1 == ret){
        return -1;
    }
    ptr +=8;

    //get release reason
   // ptr +=8;
    m_relReason = (UINT)*ptr;
    ptr +=1;
    
    //get calling party number(16B)
    getBICC_CallNumber(m_caller,ptr,16);
    ptr +=16;
    
    //get called party number(16B)
    getBICC_CallNumber(m_called,ptr,16);
    ptr +=16;
    
    //get status
    m_status = (UINT)*ptr;
    ptr +=1;

    //get TimeoutBits
    ULONG lTimeout = (ULONG)(((((*ptr<<8)+*(ptr+1))<<8)+*(ptr+2))<<8)+*(ptr+3);
    if(0 != lTimeout){
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_WARN(rootLogger, "BICC: "<<m_caller<<"->"<<m_called<<", status="<<m_status<<", timeoutBits="<<lTimeout);
        G::displayHex("BICC: TimeoutBits",m_CDRBuf,m_CDRLen);
    }
    ptr +=4;

    //get SourceIP & DestIP
    ret = readBICC_IP(ptr);
    if(-1 == ret){
        return -1;
    }
    
    return 0;
}


int CDRMessage::checkDelay(){
    long now = 0;
    int outerDelay = 0;
    int innerDelay = 0;
    time(&now);
    static UINT nDelay = 0;
    
    //check delay
    //startTime = the beginning of a call
    m_pZxClient->incIAMDRs();
    
    outerDelay = m_recvTime - m_startTime[0];
    if(outerDelay >= ZxComm::m_msgDelayLimit){
        m_pZxClient->setMaxOuterDelay(outerDelay);
        m_pZxClient->incDelayDRMsgs();
        ZxComm::m_nDelayDRs++;
        
     /*   if((++nDelay % 10000) == 0){
            LoggerPtr rootLogger = Logger::getRootLogger();  
            LOG4CXX_WARN(rootLogger, "innerDelay="<<innerDelay<<"s, outerDelay="<<outerDelay
                <<"s, startTime="<<m_startTime[0]<<", endTime="<<m_endTime[0]<<", m_recvTime="<<m_recvTime<<", now="<<now<<", type="<<m_protocolType);
            //G::displayHex("delay package ",m_CDRBuf,m_CDRLen);
        }*/
        return -1;
    }else{
        int parserDelay = now - m_startTime[0];
        if( parserDelay >= ZxComm::m_msgDelayLimit){
            if((++nDelay % 1000) == 0){
                LoggerPtr rootLogger = Logger::getRootLogger();  
                LOG4CXX_WARN(rootLogger, "!!!parser delay="<<parserDelay <<"s, startTime="<<m_startTime[0]
                <<", m_recvTime="<<m_recvTime<<", now="<<now);
                //G::displayHex("delay package ",m_CDRBuf,m_CDRLen);
            }
        }
    }

    return 0;
}

/*
Optional Load: 
IE1 + IE2 + ...

0: success
-1:failed, invalid message
*/
int CDRMessage::parseOptLoad(PUCHAR pData){
    int ret = 0;
    PUCHAR ptr = pData;
    UINT len = 0;
    
    //parse IEs
    for(int i=0; i<m_VarIENumber; i++){
        ret = parseIE(ptr, len);
        if(-1 == ret){
            return -1;
        }else{
            ptr += len;
        }
    }
        
    return 0;
}


/*
IE(Information Element): 
DataID(2B) + DataFieldLength(2B) + TimeStampFlag(1B) 
+ DataField(V) + TimeStamp(8)

0: success
-1:failed, invalid message
*/
int CDRMessage::parseIE(PUCHAR pData, UINT& len){
    int ret = 0;
    PUCHAR ptr = pData;
    UCHAR value[VARIABLE_IE_MAX_LEN];
    UINT dataID = 0;
    UINT dataLen = 0;
    UINT timeFlag = 0;
    ULONG timeStamp[2];
    timeStamp[0] = 0;
    timeStamp[1] = 0;
    len = 0;

    //get DataID
    dataID = (UINT)(*ptr<< 8) + *(ptr+1);
    ptr +=2;
    len +=2;
    
    //get DataFieldLength
    dataLen = (UINT)(*ptr<< 8) + *(ptr+1);
    ptr +=2;
    len +=2;

    //get TimeStampFlag
    timeFlag = (UINT)*ptr;
    ptr++;
    len++;
    
    //get DataField
    memset(value, 0, VARIABLE_IE_MAX_LEN);
    memcpy(value, ptr, dataLen);
    value[dataLen] = 0;
    ptr += dataLen;
    len += dataLen;

    //get TimeStamp(8B): sec(4B) + nSec(4B)
    if(1 == timeFlag){
        readTime(ptr,timeStamp);
        ptr +=8;
        len +=8;
    }
    
    if(ISUP == m_protocolType){
        ret = readMessageType(value, dataLen, dataID);
        if(0 == ret){
            return 0;
        }
        
        ret = readISUP_CIC(value, dataLen, dataID);
        if(0 == ret){
            return 0;
        }else if(-2 == ret){
            return -1;
        }

        ret = readISUP_PC(value, dataLen, dataID);
        if(0 == ret){
            return 0;
        }else if(-2 == ret){
            return -1;
        }
        
        ret = readCalledPartyNumber(value, dataLen, dataID);
        if(0 == ret){
            return 0;
        }
        
        ret = readCallingPartyNumber(value, dataLen, dataID);
        if(0 == ret){
            return 0;        
        }
        
        ret = readOriginalCalledInNumber(value, dataLen, dataID);
        if(0 == ret){
            return 0;
        }    
    }
    
    ret = readGenericNumber(value, dataLen, dataID);
    if(0 == ret){
        return 0;
    }
    
    ret = readOriginalCalledNumber(value, dataLen, dataID);
    if(0 == ret){
        return 0;
    }
    
    ret = readRedirectingNumber(value, dataLen, dataID);
    if(0 == ret){
        return 0;
    }
    
    ret = readRedirectionNumber(value, dataLen, dataID);
    if(0 == ret){
        return 0;
    }else{
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_ERROR(rootLogger, m_caller<<"->"<<m_called<<", type="<<m_protocolType<<", status="<<m_status<<", wrong dataID="<<dataID);
        G::displayHex("CDRMessage: wrong dataID",m_CDRBuf,m_CDRLen);
        return -1;
    }

    return 0;
}

/*
//check call number:
//return 0: right mobile phone number
//return -1: invalid number
int CDRMessage::checkCallNumber(){
    char* pCalled = (char*)m_called;
    char* pCaller = (char*)m_caller;
    static unsigned long  count = 0;
    int diffLen = 0;
    int calledLen = strlen(pCalled);
    int callerLen = strlen(pCaller);
    int ret = -1;

    if(callerLen > 11){
        //cut the add number of caller
        diffLen = callerLen - 11;
        pCaller = pCaller+diffLen;
        callerLen -=diffLen;
    }
    
    if(calledLen > 11){
        //cut the add number of called
        diffLen = calledLen - 11;
        pCalled = pCalled+diffLen;
        calledLen -=diffLen;
    }

    if(G::isMobilePhone(pCaller)){
        if(G::isMobilePhone(pCalled)){
            memcpy(m_caller, pCaller, callerLen);
            m_caller[callerLen] = 0;
            
            memcpy(m_called, pCalled, calledLen);
            m_called[calledLen] = 0;
            
            ret = 0;
        }else{
            ret = -1;
            ++count;
        }
    }else{
        ret = -1;
        ++count;
    }

    if(-1 == ret){
        if(count%10000 == 0){
            LoggerPtr rootLogger = Logger::getRootLogger(); 
            LOG4CXX_INFO(rootLogger, "invalid caller="<<m_caller<<", called="<<m_called);
           // G::displayHex("invalid number",m_CDRBuf,m_CDRLen);
        }
    }

    return ret;
}
*/

//get Time(8B): sec(4B) + nSec(4B)
void CDRMessage::readTime(PUCHAR data, ULONG* lTime){
    PUCHAR ptr = data;
    ULONG iValue = 0;
    
    //get sec
    iValue = (ULONG)*ptr;
    for(int i=1;i<4;i++){
        iValue= iValue<<8;
        iValue += (ULONG)*(ptr+i);
    }
    lTime[0] = iValue;
    
    //get nSec
    iValue = 0;
    iValue = (ULONG)*(ptr+4);
    for(int i=5;i<8;i++){
        iValue= iValue<<8;
        iValue += (ULONG)*(ptr+i);
    }
    ptr +=8;
    lTime[1] = iValue;
    return;
}

//BICC SourceIP&DestIP
int CDRMessage::readBICC_IP(PUCHAR data){
    UCHAR temp[5];
    memcpy(temp, data, 4);
    temp[4] = 0;

    bool bOK = G::Hex2Str(temp,m_srcIP,4);
    if(false == bOK){
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_ERROR(rootLogger, "BICC: read SourceIP failed!");
        G::displayHex("readBICC SourceIP",m_CDRBuf,m_CDRLen);
        return -1;
    }  

    memcpy(temp, data+4, 4);
    temp[4] = 0;
    bOK = G::Hex2Str(temp,m_dstIP,4);
    if(false == bOK){
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_ERROR(rootLogger, "BICC: read DestIP failed!");
        G::displayHex("readBICC DestIP",m_CDRBuf,m_CDRLen);
        return -1;
    }  

    return 0;
}

//BICC OPC&DPC
int CDRMessage::readBICC_PC(PUCHAR data){
    UCHAR temp[5];
    memcpy(temp, data, 4);
    temp[4] = 0;

    bool bOK = G::Hex2Str(temp,m_opc,4);
    if(false == bOK){
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_ERROR(rootLogger, "BICC: read OPC failed!");
        G::displayHex("readBICC OPC",m_CDRBuf,m_CDRLen);
        return -1;
    }  

    memcpy(temp, data+4, 4);
    temp[4] = 0;
    bOK = G::Hex2Str(temp,m_dpc,4);
    if(false == bOK){
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_ERROR(rootLogger, "BICC: read DPC failed!");
        G::displayHex("readBICC DPC",m_CDRBuf,m_CDRLen);
        return -1;
    }  

    return 0;
}


//ISUP OPC&DPC
int CDRMessage::readISUP_PC(PUCHAR data, UINT dataLen, UINT dataID){
    int ret = -1;  
    bool bDest = false;
    
    if(ISUP == m_protocolType){
        if(ISUP_MTP_OPC_ID == dataID){
            ret = 0;
            bDest = false;
        }else if(ISUP_M3UA_OPC_ID == dataID){
            ret = 0;
            bDest = false;
        }else if(ISUP_MTP_DPC_ID == dataID){
            ret = 0;
            bDest = true;
        }else if(ISUP_M3UA_DPC_ID == dataID){
            ret = 0;
            bDest = true;
        }else{
            ret = -1;
        }
    }

    if(0 == ret){
        if(dataLen == 4){
            if(bDest){
                //get DPC(4B)
                if(strlen((char*)m_dpc)== 0){
                    UCHAR temp[5];
                    memcpy(temp, data, dataLen);
                    temp[dataLen] = 0;
                    
                    bool bOK = false;
                    bOK = G::Hex2Str(temp,m_dpc,dataLen);
                    if(false == bOK){
                        LoggerPtr rootLogger = Logger::getRootLogger();  
                        LOG4CXX_ERROR(rootLogger, "ISUP: read DPC failed! dataID="<<dataID);
                        G::displayHex("readISUP DPC",m_CDRBuf,m_CDRLen);
                        return -2;
                    }
                }
            }else{
                //get OPC(4B)
                if(strlen((char*)m_opc)== 0){
                    UCHAR temp[5];
                    memcpy(temp, data, dataLen);
                    temp[dataLen] = 0;
                    
                    bool bOK = false;
                    bOK = G::Hex2Str(temp,m_opc,dataLen);
                    if(false == bOK){
                        LoggerPtr rootLogger = Logger::getRootLogger();  
                        LOG4CXX_ERROR(rootLogger, "ISUP: read OPC failed! dataID="<<dataID);
                        G::displayHex("readISUP OPC",m_CDRBuf,m_CDRLen);
                        return -2;
                    }
                }
            }
        }else{
            LoggerPtr rootLogger = Logger::getRootLogger();  
            LOG4CXX_ERROR(rootLogger, "ISUP: "<<m_caller<<"->"<<m_called<<", status="<<m_status
                <<", dataID="<<dataID<<", wrong PC len="<<dataLen);
            G::displayHex("readISUP PC",data,dataLen);  
            return -2;
        }
    }        

    return ret;
}


//ISUP CIC
int CDRMessage::readISUP_CIC(PUCHAR data, UINT dataLen, UINT dataID){
    int ret = -1;    
    
    if(ISUP == m_protocolType){
        if(ISUP_MTP_CIC_ID == dataID){
            ret = 0;
        }else if(ISUP_M3UA_CIC_ID == dataID){
            ret = 0;
        }else{
            ret = -1;
        }
    }

    if(0 == ret){
        //get CIC(4B)
        ULONG value = (ULONG)(((((*data<<8)+*(data+1))<<8)+*(data+2))<<8)+*(data+3);
        if(m_CIC>0){
            if(value != m_CIC){
                LoggerPtr rootLogger = Logger::getRootLogger();  
                LOG4CXX_ERROR(rootLogger, "ISUP: "<<m_caller<<"->"<<m_called<<", status="<<m_status
                    <<", dataID="<<dataID<<", read CIC="<<value<<", existed CIC="<<m_CIC);
                return -2;

            }
        }else{
            m_CIC = value;
        }
    }

    return ret;
}


int CDRMessage::readCallingPartyNumber(PUCHAR data, UINT dataLen, UINT dataID){
    int ret = -1;    
    
    if(ISUP == m_protocolType){
        if(ISUP_IAM_CALLING_PARTY_NUMBER_ID == dataID){
            ret = 0;
        }else{
            ret = -1;
        }
    }

    if(0 == ret){
        setCallNumber(m_caller,data,dataLen);
        checkCallNumber(m_caller);
    }

    return ret;
}

int CDRMessage::readCalledPartyNumber(PUCHAR data, UINT dataLen, UINT dataID){
    int ret = -1;
    if(ISUP == m_protocolType){
        if(ISUP_IAM_CALLED_PARTY_NUMBER_ID == dataID){            
            ret = 0;
        }else{
            ret = -1;
        }
    }

    if(0 == ret){
        setCallNumber(m_called,data,dataLen);
        checkCallNumber(m_called);
    }

    return ret;
}

/*
int CDRMessage::readMessageType(PUCHAR data, UINT dataLen, UINT dataID, ULONG timeStamp){
    int ret = -1;

    if(BICC == m_protocolType){
        if(BICC_CPG_MESSAGE_TYPE_ID == dataID){
            ret = 0;
        }else if(BICC_ACM_MESSAGE_TYPE_ID == dataID){
            ret = 0;
        }else if(BICC_REL_MESSAGE_TYPE_ID == dataID){
            ret = 0;
        }else if(BICC_CNF_MESSAGE_TYPE_ID == dataID){
            ret = 0;
        }else if(BICC_FRJ_MESSAGE_TYPE_ID == dataID){
            ret = 0;
        }else if(BICC_ANM_MESSAGE_TYPE_ID == dataID){
            ret = 0;
        }else if(BICC_RLC_MESSAGE_TYPE_ID == dataID){
            ret = 0;
        }else if(BICC_IR_MESSAGE_TYPE_ID == dataID){
            ret = 0;
        }else if(BICC_CON_MESSAGE_TYPE_ID == dataID){
            ret = 0;
        }else if(BICC_IAM_MESSAGE_TYPE_ID == dataID){
            ret = 0;
        }else if(BICC_SEG_MESSAGE_TYPE_ID == dataID){
            ret = 0;
        }else if(BICC_FAL_MESSAGE_TYPE_ID == dataID){
            ret = 0;
        }else{
            ret = -1;
        }
    }else if(ISUP == m_protocolType){
        if(ISUP_CPG_MESSAGE_TYPE_ID == dataID){
            ret = 0;
        }else if(ISUP_ACM_MESSAGE_TYPE_ID == dataID){
            ret = 0;
        }else if(ISUP_REL_MESSAGE_TYPE_ID == dataID){
            ret = 0;
        }else if(ISUP_CNF_MESSAGE_TYPE_ID == dataID){
            ret = 0;
        }else if(ISUP_FRJ_MESSAGE_TYPE_ID == dataID){
            ret = 0;
        }else if(ISUP_ANM_MESSAGE_TYPE_ID == dataID){
            ret = 0;
        }else if(ISUP_RLC_MESSAGE_TYPE_ID == dataID){
            m_status = Closure;
            ret = 0;
        }else if(ISUP_IR_MESSAGE_TYPE_ID == dataID){
            ret = 0;
        }else if(ISUP_CON_MESSAGE_TYPE_ID == dataID){
            ret = 0;
        }else if(ISUP_IAM_MESSAGE_TYPE_ID == dataID){
            m_status = IAM;
            ret = 0;
        }else if(ISUP_SEG_MESSAGE_TYPE_ID == dataID){
            ret = 0;
        }else if(ISUP_FAL_MESSAGE_TYPE_ID == dataID){
            ret = 0;
        }else{
            ret = -1;
        }
    }

    if(0 == ret){
        UINT iValue = (UINT)*data;
        for(int i=1;i<dataLen;i++){
            iValue= iValue<<8;
            iValue += (UINT)*(data+i);
        }
        
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_DEBUG(rootLogger, m_caller<<"->"<<m_called<<", type="<<m_protocolType<<", status="<<m_status<<", dataID="<<dataID<<", MessageType="<<iValue<<", timeStamp="<<timeStamp<<", endTime="<<m_endTime[0]);
    }
    
    return ret;
}
*/

int CDRMessage::readMessageType(PUCHAR data, UINT dataLen, UINT dataID){
    int ret = -1;

    if(ISUP == m_protocolType){
        if(ISUP_RLC_MESSAGE_TYPE_ID == dataID){
            m_status = Closure;
            ret = 0;
        }else if(ISUP_IAM_MESSAGE_TYPE_ID == dataID){
            m_status = IAM;
            ret = 0;
        }else{
            ret = -1;
        }
    }

  /*  if(0 == ret){
        UINT iValue = (UINT)*data;
        for(int i=1;i<dataLen;i++){
            iValue= iValue<<8;
            iValue += (UINT)*(data+i);
        }
        
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_DEBUG(rootLogger, m_caller<<"->"<<m_called<<", type="<<m_protocolType<<", status="<<m_status<<", dataID="<<dataID<<", MessageType="<<iValue<<", timeStamp="<<timeStamp<<", endTime="<<m_endTime[0]);
    }*/
    
    return ret;
}



//get Generic number(16B): 683158614107f4fffffffffffffffff (8613851614704)
int CDRMessage::readGenericNumber(PUCHAR data, UINT dataLen, UINT dataID){
    int ret = -1;

    if(BICC == m_protocolType){
        if(BICC_IAM_GENERIC_NNUMBER_ID == dataID){
            ret = 0;
        }else{
            ret = -1;
        }
    }else if(ISUP == m_protocolType){
        if(ISUP_IAM_GENERIC_NNUMBER_ID == dataID){
            ret = 0;
        }else{
            ret = -1;
        }
    }

    if(0 == ret){
        UCHAR phone[PHONE_NUMBER_LEN+1];
        memset(phone, 0, PHONE_NUMBER_LEN+1);
        setGenericNumber(phone,data,dataLen);
        int rc = checkCallNumber(phone);
        if(G::isTestPhone((char*)m_caller) || G::isTestPhone((char*)m_called) || G::isTestPhone((char*)phone)){
            LoggerPtr rootLogger = Logger::getRootLogger();  
            LOG4CXX_DEBUG(rootLogger, m_caller<<"->"<<m_called<<", type="<<m_protocolType<<", status="<<m_status<<", dataID="<<dataID<<", GenericNumber="<<phone);
        }
        if(-1 == rc){
            G::displayHex("GenericNumber",data,dataLen);
        }
    }

    return ret;
}

int CDRMessage::readOriginalCalledNumber(PUCHAR data, UINT dataLen, UINT dataID){
    int ret = -1;
    if(BICC == m_protocolType){
        if(BICC_IAM_ORIGINAL_CALLED_NUMBER_ID == dataID){
            ret = 0;
        }else{
            ret = -1;
        }
    }else if(ISUP == m_protocolType){
        if(ISUP_IAM_ORIGINAL_CALLED_NUMBER_ID == dataID){
            ret = 0;
        }else{
            ret = -1;
        }
    }

    if(0 == ret){
        UCHAR phone[PHONE_NUMBER_LEN+1];
        memset(phone, 0, PHONE_NUMBER_LEN+1);
        setCallNumber(phone,data,dataLen);
        int rc = checkCallNumber(phone);
        if(G::isTestPhone((char*)m_caller) || G::isTestPhone((char*)m_called) || G::isTestPhone((char*)phone)){
            LoggerPtr rootLogger = Logger::getRootLogger();  
            LOG4CXX_DEBUG(rootLogger, m_caller<<"->"<<m_called<<", type="<<m_protocolType<<", status="<<m_status<<", dataID="<<dataID<<", OriginalCalledNumber="<<phone);
        }
       /* if(-1 == rc){
            G::displayHex("OriginalCalledNumber",data,dataLen);
        }*/
    }

    return ret;
}


int CDRMessage::readOriginalCalledInNumber(PUCHAR data, UINT dataLen, UINT dataID){
    int ret = -1;
    if(ISUP == m_protocolType){
        if(ISUP_IAM_ORIGINAL_CALLED_IN_NUMBER_ID == dataID){
            ret = 0;
        }else{
            ret = -1;
        }
    }

    if(0 == ret){
        UCHAR phone[PHONE_NUMBER_LEN+1];
        memset(phone, 0, PHONE_NUMBER_LEN+1);
        setCallNumber(phone,data,dataLen);
        int rc = checkCallNumber(phone);
        if(G::isTestPhone((char*)m_caller) || G::isTestPhone((char*)m_called) || G::isTestPhone((char*)phone)){
            LoggerPtr rootLogger = Logger::getRootLogger();  
            LOG4CXX_DEBUG(rootLogger, m_caller<<"->"<<m_called<<", type="<<m_protocolType<<", status="<<m_status<<", dataID="<<dataID<<", OriginalCalledInNumber="<<phone);
        }
        if(-1 == rc){
            G::displayHex("OriginalCalledInNumber",data,dataLen);
        }
    }

    return ret;
}

int CDRMessage::readRedirectingNumber(PUCHAR data, UINT dataLen, UINT dataID){
    int ret = -1;
    if(BICC == m_protocolType){
        if(BICC_IAM_REDIRECTING_NUMBER_ID == dataID){
            ret = 0;
        }else{
            ret = -1;
        }
    }else if(ISUP == m_protocolType){
        if(ISUP_IAM_REDIRECTING_NUMBER_ID == dataID){
            ret = 0;
        }else{
            ret = -1;
        }
    }

    if(0 == ret){
        UCHAR phone[PHONE_NUMBER_LEN+1];
        memset(phone, 0, PHONE_NUMBER_LEN+1);
        setCallNumber(phone,data,dataLen);
        int rc = checkCallNumber(phone);
        if(G::isTestPhone((char*)m_caller) || G::isTestPhone((char*)m_called) || G::isTestPhone((char*)phone)){
            LoggerPtr rootLogger = Logger::getRootLogger();  
            LOG4CXX_DEBUG(rootLogger, m_caller<<"->"<<m_called<<", type="<<m_protocolType<<", status="<<m_status<<", dataID="<<dataID<<", RedirectingNumber="<<phone);
        }
        if(-1 == rc){
            G::displayHex("RedirectingNumber",data,dataLen);
        }
    }
    
    return 0;
}

int CDRMessage::readRedirectionNumber(PUCHAR data, UINT dataLen, UINT dataID){
    int ret = -1;
    if(BICC == m_protocolType){
        if(BICC_REL_REDIRECTION_NUMBER_ID == dataID){
            ret = 0;
        }else{
            ret = -1;
        }
    }
    
    if(0 == ret){
        UCHAR phone[PHONE_NUMBER_LEN+1];
        memset(phone, 0, PHONE_NUMBER_LEN+1);
        setCallNumber(phone,data,dataLen);
        int rc = checkCallNumber(phone);
        if(G::isTestPhone((char*)m_caller) || G::isTestPhone((char*)m_called) || G::isTestPhone((char*)phone)){
            LoggerPtr rootLogger = Logger::getRootLogger();  
            LOG4CXX_DEBUG(rootLogger, m_caller<<"->"<<m_called<<", type="<<m_protocolType<<", status="<<m_status<<", dataID="<<dataID<<", RedirectionNumber="<<phone);
        }
        if(-1 == rc){
            G::displayHex("RedirectionNumber",data,dataLen);
        }
    }
    return ret;
}

/*
int CDRMessage::readRedirectionInfor(PUCHAR data, UINT dataLen, UINT dataID){
    int ret = -1;
    if(BICC == m_protocolType){
        if(BICC_IAM_REDIRECTION_INFORMATION_ID == dataID){
            ret = 0;
        }else if(BICC_REL_REDIRECTION_INFORMATION_ID == dataID){
            ret = 0;
        }else{
            ret = -1;
        }
    }else if(ISUP == m_protocolType){
        if(ISUP_IAM_REDIRECTION_INFORMATION_ID == dataID){
            ret = 0;
        }else if(ISUP_REL_REDIRECTION_INFORMATION_ID == dataID){
            ret = 0;
        }else{
            ret = -1;
        }
    }
    
    if(0 == ret){
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_DEBUG(rootLogger, m_caller<<"->"<<m_called<<", type="<<m_protocolType<<", status="<<m_status<<", dataID="<<dataID);
        G::displayHex("RedirectionInformation",data,dataLen);
    }
    return ret;
}


int CDRMessage::readCauseInd(PUCHAR data, UINT dataLen, UINT dataID){
    int ret = -1;
    if(BICC == m_protocolType){
        if(BICC_REL_CAUSE_INDICATORS_ID == dataID){
            ret = 0;
        }else if(BICC_RLC_CAUSE_INDICATORS_ID == dataID){
            ret = 0;
        }else if(BICC_FRJ_CAUSE_INDICATORS_ID == dataID){
            ret = 0;
        }else if(BICC_CNF_CAUSE_INDICATORS_ID == dataID){
            ret = 0;
        }else if(BICC_ACM_CAUSE_INDICATORS_ID == dataID){
            ret = 0;
        }else if(BICC_CPG_CAUSE_INDICATORS_ID == dataID){
            ret = 0;
        }else{
            ret = -1;
        }
    }else if(ISUP == m_protocolType){
        if(ISUP_REL_CAUSE_INDICATORS_ID == dataID){
            ret = 0;
        }else if(ISUP_RLC_CAUSE_INDICATORS_ID == dataID){
            ret = 0;
        }else if(ISUP_FRJ_CAUSE_INDICATORS_ID == dataID){
            ret = 0;
        }else if(ISUP_CNF_CAUSE_INDICATORS_ID == dataID){
            ret = 0;
        }else if(ISUP_ACM_CAUSE_INDICATORS_ID == dataID){
            ret = 0;
        }else if(ISUP_CPG_CAUSE_INDICATORS_ID == dataID){
            ret = 0;
        }else{
            ret = -1;
        }
    }
    
    if(0 == ret){
        UINT iValue = (UINT)*(data+1);
        iValue= iValue - 0x80;
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_DEBUG(rootLogger, m_caller<<"->"<<m_called<<", status="<<m_status<<", dataID="<<dataID<<", cause="<<iValue);
    }
    return ret;
}
*/


//seqNum: proID_seqID
//length: 11B~13B
int CDRMessage::setSeqNum(){
    int len = 0;
    char ss[20] = {0};

    sprintf(ss, "%s_%s",m_procID,m_msgSeqID);  
    len = strlen(ss);
    if(len > MAX_SEQ_NUM_LEN){
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_ERROR(rootLogger, "Too big seqNum: m_procID="<<m_procID<<", m_msgSeqID="<<m_msgSeqID);
        G::displayHex("",m_CDRBuf,m_CDRLen);
        return -1;
    }
    
    memset(m_callSeqNum, 0, MAX_SEQ_NUM_LEN+1);
    memcpy(m_callSeqNum, ss, len);
    m_callSeqNum[len] = 0;
    
    return 0;
}

//get calling party number(16B): 683158614107f4fffffffffffffffff (8613851614704)
void CDRMessage::getBICC_CallNumber(PUCHAR dst, PUCHAR src, UINT len){
    memset(dst,0,PHONE_NUMBER_LEN+1);
    int j = 0;
    UCHAR uByte;
    for(int i=0; i<len; i++){  
        uByte = (*(src+i) & 0x0f) + 0x30;
        if(uByte == 0x3f){
            dst[j] = 0;  
            break;
        }else{
            dst[j++] = uByte;  
        }
        
        uByte = ((*(src+i) >> 4)&0x0f) + 0x30;  
        if(uByte == 0x3f){
            dst[j] = 0;  
            break;
        }else{
            dst[j++] = uByte;  
        }
    } 

    if(strlen((char*)dst) == 13){
        if((dst[0] == 0x38) && (dst[1] == 0x36)){
            PUCHAR tmp = dst+2;
            memcpy(dst, tmp, 11);
            dst[11] = 0;
        }
    }
    return;
}

void CDRMessage::setCallNumber(PUCHAR dst, PUCHAR src, UINT len){
    PUCHAR ptr = src;
    UINT i = 0;
    UINT index = 0;
    memset(dst, 0, PHONE_NUMBER_LEN+1);
    UCHAR value = 0;
    
    //check if it's the international number or not
    if(((*ptr)&0x7f) != 0x04){
      i +=2;
      ptr +=2;
    }else{
      //international number
      i +=3;
      ptr +=3;   
    } 
    
    for(; i < len; i++){ 
      value = (*ptr & 0x0f) + 0x30;
      if(value == 0x3f){
        dst[index++] = 0;
        break;
      }else{
        dst[index++] = value;
      }

      value = ((*ptr >> 4)&0x0f) + 0x30;
      if(value == 0x3f){
        dst[index++] = 0;
        break;
      }else{
        dst[index++] = value;
      }
      
      ptr++;
    } 
    dst[index]  = 0;
    
    // check the number is odd or even
    ptr = src;
    if((((*ptr)>>4)&0x08) == 0x08){
        //odd
        dst[index-1] = 0;
    }
    return;
}


void CDRMessage::setGenericNumber(PUCHAR dst, PUCHAR src, UINT len){
    PUCHAR ptr = src+1;
    UINT i = 1;
    UINT index = 0;
    
    //check if it's the international number or not
    if(((*ptr)&0x7f) != 0x04){
      i +=3;
      ptr +=3;
    }else{
      //international number
      i +=4;
      ptr +=4;   
    } 
    
    for(; i < len; i++){ 
      dst[index++] = (*ptr & 0x0f) + 0x30;
      dst[index++] = ((*ptr >> 4)&0x0f) + 0x30;    
      ptr++;
    } 
    
    dst[index]  = 0;
    if(dst[index-1] == 0x3f){
        dst[index-1] =0;    
    }
    
    // check the number is odd or even
    ptr = src+1;
    if((((*ptr)>>4)&0x08) == 0x08){
        //odd
        dst[index-1] = 0;
    }
    return;
}



//check call number:
//return 0: valid mobile phone number
//return 1: valid VPN number
//return 2: valid family number
//return -1: invalid number
int CDRMessage::checkCallNumber(PUCHAR callNumber){
    char* pNum = (char*)callNumber;
    int diffLen = 0;
    int len = strlen(pNum);
    int ret = -1;
    static int count = 0;

    for(int i=0; i<len; i++){
        if((*(pNum+i)<0x30) || (*(pNum+i)>0x39)){
           // LoggerPtr rootLogger = Logger::getRootLogger(); 
           // LOG4CXX_ERROR(rootLogger, "checkCallNumber: wrong phone!");
          //  G::displayHex("wrong phone",m_CDRBuf,m_CDRLen);
            return -1;
        }
    }

    if(len > 11){
        //cut the add number of caller
        diffLen = len - 11;
        pNum = pNum+diffLen;
        len -=diffLen;
        
        if(G::isMobilePhone(pNum)){
            memcpy(callNumber, pNum, len);
            callNumber[len] = 0;
            ret = 0;
        }else{
            ret = -1;
        }
    }else if(len == 11){
        if(G::isMobilePhone(pNum)){
            ret = 0;
        }else{
            ret = -1;
        }
    }else if(len == 8){
        //60 + VPN number (eg. 60613446)
        if((*pNum == 0x36) && (*(pNum+1) == 0x30) && (*(pNum+2) == 0x36)){
            pNum = pNum+2;
            len = 6;
            memcpy(callNumber, pNum, len);
            callNumber[len] = 0;
            ret = 1;            
        }else{
            ret = -1;
        }
    }else if(len == 6){
        //VPN number (eg. 613446)
        if(*pNum == 0x36){
            ret = 1;
        }else{
            ret = -1;
        }
    }else if(len == 5){
        //60 + family number (eg. 60520)
        if((*pNum == 0x36) && (*(pNum+1) == 0x30) && (*(pNum+2) == 0x35)){
            pNum = pNum+2;
            len = 3;
            memcpy(callNumber, pNum, len);
            callNumber[len] = 0;
            ret = 2;
        }else{
            ret = -1;
        }
    }else if(len == 3){
        //family number (eg. 520)
        if(*pNum == 0x35){
            ret = 2;
        }else{
            ret = -1;
        }
    }else if(len == 0){
       // LoggerPtr rootLogger = Logger::getRootLogger(); 
       // LOG4CXX_ERROR(rootLogger, "checkCallNumber: NULL phone!");
       // G::displayHex("NULL phone",m_CDRBuf,m_CDRLen);
        return -1;
    }    
    
    return ret;
}


//return 1: matched right IAM CDR
//return -1: error
int CDRMessage::setMatchedCall(CDRMessage* pMessage){    
    int ret = 0;
    if(!pMessage){
        LoggerPtr rootLogger = Logger::getRootLogger(); 
        LOG4CXX_ERROR(rootLogger, "setMatchedCall: NULL CDRMessage!");
        return -1;
    }

    ULONG matchedCallID = pMessage->getCallID();
    char* matchedCaller = (char*)pMessage->getCaller();
    char* matchedCalled = (char*)pMessage->getCalled();
    if(m_protocolType != pMessage->getProtocolType()){
        LoggerPtr rootLogger = Logger::getRootLogger();
        LOG4CXX_ERROR(rootLogger, "setMatchedCall: different type(IAM type="<<pMessage->getProtocolType()<<", Closure type="<<m_protocolType
            <<"), IAM("<<matchedCaller<<"->"<<matchedCaller<<"), seqNum="<<m_callSeqNum);
        return -1;
    }

     if((0 == strcmp((char*)m_caller, matchedCaller)) && (0 == strcmp((char*)m_called, matchedCalled)))
     {
         m_callID = matchedCallID;
         ret = 1;
     }else{
         ULONG diffEnd = m_endTime[0] - pMessage->getEndTime();
         LoggerPtr rootLogger = Logger::getRootLogger();
         LOG4CXX_ERROR(rootLogger, "setMatchedCall: different call, but same key="<<m_callSeqNum<<", diffEnd="<<diffEnd
             <<", IAM("<<matchedCaller<<"->"<<matchedCalled<<", id="<<pMessage->getCDRID()<<", type="<<pMessage->getProtocolType()<<", packID="<<pMessage->getPackID()
             <<"), Closure("<<m_caller<<"->"<<m_called<<", id="<<m_CDRID<<", type="<<m_protocolType<<", packID="<<m_packID<<")");

         return -1;
     }
    
        
     /*   int callerlen = strlen(matchedCaller);
        int calledlen = strlen(matchedCalled);
        if(matchedCaller && matchedCalled){
            memcpy(m_caller, matchedCaller, callerlen);
            m_caller[callerlen] = 0;
            memcpy(m_called, matchedCalled, calledlen);
            m_called[calledlen] = 0;

            if(checkCallNumber(m_caller) || checkCallNumber(m_called)){
                LoggerPtr rootLogger = Logger::getRootLogger(); 
                LOG4CXX_ERROR(rootLogger, "setMatchedCall: type=ISUP, get invalid caller="<<m_caller<<", called="<<m_called<<", seqNum="<<m_callSeqNum);
                return -1;
            }else{
                pMessage->setMatched(true);
                ret = 1;
            }            
        }else{
            LoggerPtr rootLogger = Logger::getRootLogger(); 
            LOG4CXX_ERROR(rootLogger, "setMatchedCall: type=ISUP, status=IAM, NULL caller="<<matchedCaller<<", called="<<matchedCalled<<", seqNum="<<pMessage->getSeqNum());
            ret = -1;
        }   */ 

    if(1 == ret){
        char* ss = (char*)G::m_testZone.c_str();
        if((0 == strncmp((char*)m_caller, ss, 7)) || (0 == strncmp((char*)m_called, ss, 7))){
            LoggerPtr rootLogger = Logger::getRootLogger();  
            LOG4CXX_DEBUG(rootLogger, "<== "<<m_caller<<"->"<<m_called<<", type="<<m_protocolType
                <<", status="<<m_status<<", cause="<<m_relReason
               <<", CDRID="<<m_CDRID<<", varIENum="<<m_VarIENumber<<", CIC="<<m_CIC
                <<", start="<<m_startTime[0]<<", end="<<m_endTime[0]<<", opc="<<m_opc<<", dpc="<<m_dpc);
               // <<", srcIP="<<m_srcIP<<", dstIP="<<m_dstIP);
        }
        
        if(G::isTestPhone((char*)m_caller) || G::isTestPhone((char*)m_called)){
            LoggerPtr rootLogger = Logger::getRootLogger();  
            LOG4CXX_DEBUG(rootLogger, "<== "<<m_caller<<"->"<<m_called<<", type="<<m_protocolType
               <<", status="<<m_status<<", cause="<<m_relReason
               <<", CDRID="<<m_CDRID<<", varIENum="<<m_VarIENumber<<", CIC="<<m_CIC
                <<", start="<<m_startTime[0]<<", end="<<m_endTime[0]<<", opc="<<m_opc<<", dpc="<<m_dpc);
              //  <<", srcIP="<<m_srcIP<<", dstIP="<<m_dstIP);
           // G::displayHex("",m_CDRBuf,m_CDRLen);
        }
    }
    
    return ret;
}


//return 0: send success, discard it
//return -1: send failed, discard it
int CDRMessage::sendMessage(){
    static UINT nTimeout = 0;
    int callerLen = 0;
    int calledLen = 0;
    long now = 0;    
    int sendLen = 0;
    int ret = 0;
    UCHAR sendBuf[MAX_SEND_DATA_LEN+1];  
    memset(sendBuf, 0, MAX_SEND_DATA_LEN+1);    
   
    if(!G::isMobilePhone((char*)m_caller) || !G::isMobilePhone((char*)m_called)){
        return -1;
    }
    
    //check if msg is timeout
    time(&now);
    if((now-m_startTime[0]) > ZxComm::m_msgDelayLimit){
        //timeout
        if((++nTimeout % 10000) == 0){
            LoggerPtr rootLogger = Logger::getRootLogger();
            LOG4CXX_WARN(rootLogger, "sendMessage: "<<nTimeout<<" msgs are timeout!");
        }
        return -1;
    }
    

    //send data format: len+step+state+seqNum+callerLen+caller+calledLen+called
    //min len = 15; max len = 31
    sendBuf[0]=0;
    sendBuf[1]=0x30;
    sendBuf[2]=0x30 + m_status;
    sendBuf[3] = m_callID &0xff;
    sendBuf[4] = (m_callID >> 8)&0xff;    
    sendBuf[5] = (m_callID >> 16)&0xff;
    sendBuf[6] = (m_callID >> 24)&0xff;
    sendLen += 7;
    callerLen = strlen((char*)m_caller);
    sendBuf[sendLen++] = callerLen&0xff;
    memcpy(sendBuf+sendLen,m_caller,callerLen);
    sendLen += callerLen;
    calledLen = strlen((char*)m_called);
    sendBuf[sendLen++] = calledLen&0xff;
    memcpy(sendBuf+sendLen,m_called,calledLen);
    sendLen += calledLen;
    sendBuf[sendLen]=0;
    sendBuf[0]=sendLen&0xff;
    
    if(sendLen<MIN_SEND_DATA_LEN || sendLen>MAX_SEND_DATA_LEN){
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_ERROR(rootLogger, "Error sendLen="<<sendLen<<", step="<<m_status<<", seqNum="<<m_callID<<", "<<m_caller<<"->"<<m_called<<", buf="<<sendBuf);
        return -1;
    }
    
    //send data to sugw
    ret = SugwServer::sktSend(sendBuf, sendLen);
    if(0 != ret){
        //send failed, discard it
        return -1;
    }

    SugwServer::m_nSendStart++;

    if(G::isTestPhone((char*)m_caller) || G::isTestPhone((char*)m_called)){
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_DEBUG(rootLogger, "sendToSugw: status="<<m_status<<", "<< m_caller<<"->"<<m_called<<", seqNum="<<m_callID<<", sendLen="<<sendLen);
    }

    return ret;
}


//end of file cdrmessage.cpp

