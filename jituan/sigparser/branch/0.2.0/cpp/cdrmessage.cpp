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
	
	m_startTime[0] = 0;
	m_startTime[1] = 0;
	m_endTime[0] = 0;
	m_endTime[1] = 0;
	m_caller[0] = 0;
	m_called[0] = 0;
    m_status = 0;
    m_callSeqNum[0] = 0;
    m_packID = 0;
    m_callID = 0;
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
            LOG4CXX_ERROR(rootLogger, "CDRMessage: wrong redirectNumber len="<<len);
            G::displayHex("CDRMessage: wrong redirectNumber len",m_CDRBuf,m_CDRLen);
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


int CDRMessage::checkDelay(){
    long now = 0;
    int outerDelay = 0;
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


//seqNum: proID_seqID
//length: 11B~13B
int CDRMessage::setSeqNum(){
    int len = 0;
    char ss[20] = {0};

    sprintf(ss, "%d_%lu",m_procID,m_msgSeqID);  
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
		LoggerPtr rootLogger = Logger::getRootLogger();
		LOG4CXX_ERROR(rootLogger, "msg timeout "<<now-m_startTime[0]<<" s, m_caller="<<m_caller<<", m_called="<<m_called);
        //timeout
        if((++nTimeout % 10000) == 0){
            LoggerPtr rootLogger = Logger::getRootLogger();
            LOG4CXX_ERROR(rootLogger, "sendMessage: "<<nTimeout<<" msgs are timeout!");
        }
        return -1;
    }

    //send data format: len+step+state+seqNum+callerLen+caller+calledLen+called
    //min len = 15; max len = 31
    sendBuf[0]=0;
    sendBuf[1]=IAM+0x30;
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

