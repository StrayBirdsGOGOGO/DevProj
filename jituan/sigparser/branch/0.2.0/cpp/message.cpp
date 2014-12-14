/***************************************************************************
 fileName    : message.cpp  -  Message class cpp file
 begin       : 2002-09-17
 copyright   : (C) 2002 by BobHuang
 ***************************************************************************/
#include "message.h"
#include "cdrmessage.h"
#include "hwsqueue.h"
#include "zxcomm.h"
#include "zxclient.h"
#include "g.h"
#include "md5.h"
#include "hwsshare.h"

extern HwsQueue gWaitQueue;

Message::Message() {
    m_msgBuf = 0;
    m_msgLen = 0;
    m_msgType = 0;
    m_msgSeqID = 0;
    m_pZxClient = 0;
    m_zxSocket = -1;  
    m_zxPort = -1;
    m_zxHostIP = "";
    m_packID = 0;
    m_procID = 0;
    m_recvTime = 0;
}

Message::Message(PUCHAR recvMsg, UINT recvLen, ZxClient *pClient){
    m_msgType = 0;
    m_msgSeqID = 0;    
    m_procID = 0;
    m_recvTime = 0;
    m_msgLen = recvLen;    
    m_msgBuf = new UCHAR[m_msgLen+1];
    memcpy(m_msgBuf,recvMsg,m_msgLen);
    m_msgBuf[m_msgLen] = 0;

    setZxClient(pClient);
}


Message::~Message() {
    if(m_msgBuf){
        delete[] m_msgBuf;
        m_msgBuf = 0;
    }
}

Message*  Message::createInstance(PUCHAR recvMsg, UINT recvLen, ZxClient *pClient){
    Message* pMessage = 0;    
    Status4ZxClient status;
    LoggerPtr rootLogger = Logger::getRootLogger();  

    if(!recvMsg || recvLen<1){
        LOG4CXX_ERROR(rootLogger, "Message::createInstance failed, null recvMsg!");
        return 0;
    }
    
    if(!pClient){
        LOG4CXX_ERROR(rootLogger, "Message::createInstance failed, null pZxClient!");
        return 0;
    }else{
        status = pClient->getStatus();
    }

    //get message type
    UINT msgType = (UINT)*(recvMsg+MSG_HEAD_LEN);
    string hostIP = pClient->getHostIP();
    
    switch(msgType){
        case HEART_MSG_TYPE:
            pMessage = new HeartMessage(recvMsg,recvLen,pClient);
            if(!pMessage){
                LOG4CXX_ERROR(rootLogger, "<==("<<hostIP<<")new HeartMessage failed!");
            }else{
                pClient->setHeartTime();
            }
            break;
        case DATA_MSG_TYPE:
            pMessage = new DataMessage(recvMsg,recvLen,pClient);
            if(!pMessage){
                LOG4CXX_ERROR(rootLogger, "<==("<<hostIP<<")new DataMessage failed!");
            }
            break;                
        default:
            LOG4CXX_ERROR(rootLogger, "<==("<<hostIP<<")unknown message type="<<msgType);
            break;                
    }

    return pMessage;
}

int Message::parseMessageBody(){
    return 0;
}


/*
message header: 10B
    synHeader(8B) + msgLen(2) 
*/
void Message::setMessageHeader(UINT msgLen){
    m_msgBuf[0] = 0x00;
    m_msgBuf[1] = 0xff;
    m_msgBuf[2] = 0xff;
    m_msgBuf[3] = 0xff;
    m_msgBuf[4] = 0xff;
    m_msgBuf[5] = 0xff;
    m_msgBuf[6] = 0xff;
    m_msgBuf[7] = 0x00;

    m_msgBuf[8] = msgLen>>8;
    m_msgBuf[9] = msgLen&0xff;
    return;
}

void Message::setZxClient(ZxClient* pClient){
    m_pZxClient = pClient;
    if(pClient){
        m_zxPort = pClient->getPort();
        m_zxHostIP = pClient->getHostIP();
        m_zxSocket = pClient->getSocket();
    }
}

void  Message::setRecvTime(){
    long now = 0;
    time(&now);
    m_recvTime = now;
}

int Message::sendMessage(){
    return 0;
}


HeartMessage::HeartMessage(){
}

HeartMessage::HeartMessage(PUCHAR recvMsg, UINT recvLen, ZxClient *pClient):Message(recvMsg,recvLen,pClient)
{
}

HeartMessage::~HeartMessage(){
}

/*
0: success
-1:failed
*/
int HeartMessage::parseMessageBody(){    
    LoggerPtr rootLogger = Logger::getRootLogger();  
   // LOG4CXX_INFO(rootLogger, "<==("<<m_zxHostIP<<")receive HeartMessage!");

    if(HEART_MSG_BODY_LEN != m_msgLen-MSG_HEAD_LEN){
        LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")HeartMessage: wrong Len="<<m_msgLen);
        G::displayHex("HeartMessage: wrong len",m_msgBuf,m_msgLen);
        return -1;
    }

    m_procID = (UINT)*(m_msgBuf+MSG_HEAD_LEN+1);

    HeartAckMessage* pMessage = new HeartAckMessage();
    if(!pMessage){
        LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")new HeartAckMessage failed!");
        return -1;
    }else{
        pMessage->setZxClient(m_pZxClient);
        pMessage->setMessageType(HEART_MSG_TYPE);
        pMessage->setProcID(m_procID);
        if(m_pZxClient){
            HwsQueue* pQueue = m_pZxClient->getSendMsgQueue();
            if(pQueue){
                pQueue->put(pMessage);
            }else{
                delete pMessage;
                pMessage = 0;
                LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")null pSendQueue!");
                return -1;
            }
        }else{
            delete pMessage;
            pMessage = 0;
            LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")null zxClient!");
            return -1;
        }
    }
    
    return 0;
}


HeartAckMessage::HeartAckMessage(){
}


HeartAckMessage::~HeartAckMessage(){
}

/*
heart  ack message: 9B
    message header(10B) + message body(3B)
   
 return 0: send success
 <0: send failed
*/
int HeartAckMessage::sendMessage(){
    int totalLen = MSG_HEAD_LEN+HEART_ACK_MSG_BODY_LEN;
    m_msgBuf = new UCHAR[totalLen+1];
    m_msgLen = totalLen;

    //set message header
    setMessageHeader(HEART_ACK_MSG_BODY_LEN);

    //set Message Body: msgType(1B) + procID(1B) + retain value(1B)
    m_msgBuf[MSG_HEAD_LEN] = m_msgType&0xff;
    m_msgBuf[MSG_HEAD_LEN+1] = m_procID&0xff;
    m_msgBuf[MSG_HEAD_LEN+2] = 0x01;
    m_msgBuf[m_msgLen] = 0;   

    if(!m_pZxClient){
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")send HeartAckMessage failed, null zxClient!");
        return -1;
    }

  //  LoggerPtr rootLogger = Logger::getRootLogger();  
  //  LOG4CXX_INFO(rootLogger, "==>("<<m_zxHostIP<<") send HeartAckMessage!");

    int ret = m_pZxClient->sktSend(m_msgLen,m_msgBuf);
    if(0 != ret){
        m_pZxClient->beforeDel();
    }   

    return ret;
}


DataMessage::DataMessage(){
}

DataMessage::DataMessage(PUCHAR recvMsg, UINT recvLen, ZxClient *pClient):Message(recvMsg,recvLen,pClient)
{
}

DataMessage::~DataMessage(){
}

/*
link check request message body: 0B
0: success
-1:failed
*/
int DataMessage::parseMessageBody(){    
   // LoggerPtr rootLogger = Logger::getRootLogger();  
   // LOG4CXX_INFO(rootLogger, "<==("<<m_zxHostIP<<")receive DataMessage!");
    int ret = 0;
    UINT result = 1;
    PUCHAR pData = m_msgBuf+ MSG_HEAD_LEN+1;
    UINT remainLen = m_msgLen-MSG_HEAD_LEN-1;

    if(MAX_DATA_MSG_BODY_LEN < m_msgLen-MSG_HEAD_LEN){
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")DataMessage: too long msgLen="<<m_msgLen);
        G::displayHex("DataMessage: wrong len",m_msgBuf,m_msgLen);
        return -1;
    }
    
    m_procID = (UINT)*pData;
    pData++;
    remainLen--;

    //get msgSeqID(4B)
    UINT seqID = (UINT)*pData;
    for(int i=1;i<4;i++){
        seqID = seqID<<8;
        seqID += (UINT)*(pData+i);
    }
    pData +=4;
    remainLen -=4;        
    
    ++ZxComm::m_nTotalDRs;
    m_pZxClient->incTotalDRMsgs();
    CDRMessage* pMessage = new CDRMessage();
    if(pMessage){
        pMessage->setRecvTime(m_recvTime);
        pMessage->setPackID(m_packID);
        pMessage->setZxClient(m_pZxClient);
        pMessage->setProcID(m_procID);
        pMessage->setMessageSeqID(seqID);
        pMessage->setMessageBuf(pData, remainLen);
        ret = pMessage->parseMessage();
        if(0 == ret){
            gWaitQueue.put(pMessage);
        }else{
            delete pMessage;
            pMessage = 0;
        }
    }else{
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")new CDRMessage failed!");
    }  
    
    if(0 != ret){
        result = 0;
    }else{
        result = 1;
    }

   
    //send response message to zx
    DataAckMessage* sMessage = new DataAckMessage();
    if(!sMessage){
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")new DataAckMessage failed!");
        return -1;
    }else{
        sMessage->setZxClient(m_pZxClient);
        sMessage->setMessageType(DATA_MSG_TYPE);
        sMessage->setMessageSeqID(seqID);
        sMessage->setProcID(m_procID);
        if(m_pZxClient){
            HwsQueue* pQueue = m_pZxClient->getSendMsgQueue();
            if(pQueue){
                pQueue->put(sMessage);
            }else{
                delete sMessage;
                sMessage = 0;
                LoggerPtr rootLogger = Logger::getRootLogger();  
                LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")null pSendQueue!");
                return -1;
            }
        }else{
            delete sMessage;
            sMessage = 0;
            LoggerPtr rootLogger = Logger::getRootLogger();  
            LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")null zxClient!");
            return -1;
        }
    }
    
    return 0;
}


DataAckMessage::DataAckMessage(){
}


DataAckMessage::~DataAckMessage(){
}

/*
link check response message: 9B
    message header(10B) + message body(6B)

 return 0: send success
 <0: send failed
*/
int DataAckMessage::sendMessage(){
    int totalLen = MSG_HEAD_LEN+DATA_ACK_MSG_BODY_LEN;
    m_msgBuf = new UCHAR[totalLen+1];    

    //set Message Body
    m_msgLen = totalLen;

    //set message header
    setMessageHeader(DATA_ACK_MSG_BODY_LEN);

    //set Message Body: msgType(1B) + procID(1B) + seqID(4B)
    m_msgBuf[MSG_HEAD_LEN] = m_msgType&0xff;
    m_msgBuf[MSG_HEAD_LEN+1] = m_procID&0xff;
    m_msgBuf[MSG_HEAD_LEN+2] = m_msgSeqID>>24;
    m_msgBuf[MSG_HEAD_LEN+3] = m_msgSeqID>>16;
    m_msgBuf[MSG_HEAD_LEN+4] = m_msgSeqID>>8;
    m_msgBuf[MSG_HEAD_LEN+5] = m_msgSeqID&0xff;
    m_msgBuf[m_msgLen] = 0;   

    if(!m_pZxClient){
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")send DataAckMessage failed, null zxClient!");
        return -1;
    }

   // LoggerPtr rootLogger = Logger::getRootLogger();  
   // LOG4CXX_INFO(rootLogger, "==>("<<m_zxHostIP<<") send DataAckMessage!");

    int ret = m_pZxClient->sktSend(m_msgLen,m_msgBuf);
    if(0 != ret){
        m_pZxClient->beforeDel();
    }   

    return ret;
}

//end of file message.cpp
