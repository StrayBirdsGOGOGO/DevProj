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
    m_msgEventNum = 0;
    m_msgSeqID = 0;
    m_pZxClient = 0;
    m_zxSocket = -1;  
    m_zxPort = -1;
    m_zxHostIP = "";
    m_packID = 0;
}

Message::Message(PUCHAR recvMsg, UINT recvLen, ZxClient *pClient){
    m_msgType = 0;
    m_msgEventNum = 0;
    m_msgSeqID = 0;    
    m_msgLen = recvLen;    
    m_msgBuf = new UCHAR[m_msgLen+1];
    memcpy(m_msgBuf,recvMsg,m_msgLen);
    m_msgBuf[m_msgLen] = 0;
    m_recvTime = 0;

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

    if(!recvMsg){
        LOG4CXX_ERROR(rootLogger, "Message::createInstance failed, null recvMsg!");
        return 0;
    }
    
    if(recvLen<MSG_HEAD_LEN|| !recvMsg){
        LOG4CXX_ERROR(rootLogger, "Message::createInstance failed, wrong recvLen="<<recvLen);
        return 0;
    }
    
    if(!pClient){
        LOG4CXX_ERROR(rootLogger, "Message::createInstance failed, null pZxClient!");
        return 0;
    }else{
        status = pClient->getStatus();
    }

    //message header: totallen(2) + msgType(2) + msgSeqID(4) + totalContents(1)
    //get message type
    UINT msgType = (UINT)(*(recvMsg+2)<< 8) + *(recvMsg+3);
    string hostIP = pClient->getHostIP();
    
    switch(msgType){
        case verNego_Req:
            if(Connect == status){
                pMessage = new VerNegoReqMessage(recvMsg,recvLen,pClient);
                if(!pMessage){
                    LOG4CXX_ERROR(rootLogger, "<==("<<hostIP<<")new VerNegoReqMessage failed!");
                }
            }else{
                LOG4CXX_WARN(rootLogger, "<==("<<hostIP<<")unexpected messge: verNego_Req, status="<<status);
            }
            break;
        case verNego_Resp:
            LOG4CXX_WARN(rootLogger, "<==("<<hostIP<<")unexpected messge: verNego_Resp, status="<<status);
            break;
        case linkAuth_Req:
            if(NegoSuc == status){
                pMessage = new LinkAuthReqMessage(recvMsg,recvLen,pClient);
                if(!pMessage){
                    LOG4CXX_ERROR(rootLogger, "<==("<<hostIP<<")new LinkAuthReqMessage failed!");
                }
            }else{
                LOG4CXX_WARN(rootLogger, "<==("<<hostIP<<")unexpected messge: linkAuth_Req, status="<<status);
            }
            break;
        case linkAuth_Resp:
            LOG4CXX_WARN(rootLogger, "<==("<<hostIP<<")unexpected messge: linkAuth_Resp, status="<<status);
            break;
        case linkCheck_Req:
            if(AuthSuc == status){
                pMessage = new LinkCheckReqMessage(recvMsg,recvLen,pClient);
                if(!pMessage){
                    LOG4CXX_ERROR(rootLogger, "<==("<<hostIP<<")new LinkCheckReqMessage failed!");
                }
            }else{
                LOG4CXX_WARN(rootLogger, "<==("<<hostIP<<")unexpected messge: linkCheck_Req, status="<<status);
            }
            break;
        case linkCheck_Resp:
            LOG4CXX_WARN(rootLogger, "<==("<<hostIP<<")unexpected messge: linkCheck_Resp, status="<<status);
            break;        
        case linkRel_Req:
            pMessage = new LinkRelReqMessage(recvMsg,recvLen,pClient);
            if(!pMessage){
                LOG4CXX_ERROR(rootLogger, "<==("<<hostIP<<")new LinkRelReqMessage failed!");
            }
            break;                
        case linkRel_Resp:
            LOG4CXX_WARN(rootLogger, "<==("<<hostIP<<")unexpected messge: linkRel_Resp, status="<<status);
            break;                
        case notifyEventData_Req:
            LOG4CXX_WARN(rootLogger, "<==("<<hostIP<<")unexpected messge: notifyEventData_Req, status="<<status);
            break;                
        case notifyEventData_Resp:
            LOG4CXX_WARN(rootLogger, "<==("<<hostIP<<")unexpected messge: notifyEventData_Resp, status="<<status);
            break;                
        case notifyCDRData_Req:
            if(AuthSuc == status){
                pMessage = new CDRReqMessage(recvMsg,recvLen,pClient);
                if(!pMessage){
                    LOG4CXX_ERROR(rootLogger, "<==("<<hostIP<<")new CDRReqMessage failed!");
                }
            }else{
                LOG4CXX_WARN(rootLogger, "<==("<<hostIP<<")unexpected messge: notifyCDRData_Req, status="<<status);
            }
            break;                
        case notifyCDRData_Resp:
            LOG4CXX_WARN(rootLogger, "<==("<<hostIP<<")unexpected messge: notifyCDRData_Resp, status="<<status);
            break;                
        case querySignalingData_Req:
            LOG4CXX_WARN(rootLogger, "<==("<<hostIP<<")unexpected messge: querySignalingData_Req, status="<<status);
            break;                
        case querySignalingData_Resp:
            LOG4CXX_WARN(rootLogger, "<==("<<hostIP<<")unexpected messge: querySignalingData_Resp, status="<<status);
            break;                
        default:
            LOG4CXX_ERROR(rootLogger, "<==("<<hostIP<<")unknown message type="<<msgType);
            break;                
    }

    return pMessage;
}


//parse message header: totallen(2) + msgType(2) + msgSeqID(4) + totalContents(1)
int Message::parseMessageHeader(){
    //m_msgType = (UINT)(*(m_msgBuf+2)<< 8) + *(m_msgBuf+3);
    m_msgSeqID = (ULONG)(((((*(m_msgBuf+4)<<8)+*(m_msgBuf+5))<<8)+*(m_msgBuf+6))<<8)+*(m_msgBuf+7);
    m_msgEventNum = (UINT)*(m_msgBuf+8);
    return 0;
}

int Message::parseMessageBody(){
    return 0;
}

/*
message header: 9B
    totallen(2) + msgType(2) + msgSeqID(4) + totalContents(1)
*/
void Message::setMessageHeader(){
    m_msgBuf[0] = m_msgLen>>8;
    m_msgBuf[1] = m_msgLen&0xff;
    
    m_msgBuf[2] = m_msgType>>8;
    m_msgBuf[3] = m_msgType&0xff;
    
    PUCHAR ptr = (PUCHAR)&m_msgSeqID;
    m_msgBuf[4] = *(ptr+3);
    m_msgBuf[5] = *(ptr+2);
    m_msgBuf[6] = *(ptr+1);
    m_msgBuf[7] = *ptr;
    
    m_msgBuf[8] = m_msgEventNum&0xff;
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


VerNegoReqMessage::VerNegoReqMessage(){
}

VerNegoReqMessage::VerNegoReqMessage(PUCHAR recvMsg, UINT recvLen, ZxClient *pClient):Message(recvMsg,recvLen,pClient)
{
}


VerNegoReqMessage::~VerNegoReqMessage(){
}

/*
version negotiation request message body: 2B
version(1) + subVersion(1)
0: success
-1:wrong message, delete
*/
int VerNegoReqMessage::parseMessageBody(){    
    UINT ver = 0;
    UINT subVer = 0;
    PUCHAR pData = m_msgBuf + MSG_HEAD_LEN;
    UINT result = 0;
    int ret = 0;
    
    LoggerPtr rootLogger = Logger::getRootLogger();  
    LOG4CXX_INFO(rootLogger, "<==("<<m_zxHostIP<<")receive VerNegoReqMessage!");
   // G::displayHex("VerNegoReqMessage",m_msgBuf,m_msgLen);

    if(VERNEGO_REQ_MSG_LEN != m_msgLen){
        LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")VerNegoReqMessage, wrong msgLen="<<m_msgLen);
        G::displayHex("VerNegoReqMessage: wrong len",m_msgBuf,m_msgLen);
        return -2;
    }

    if(m_msgEventNum>1){
        LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")VerNegoReqMessage: eventNum="<<m_msgEventNum);
        G::displayHex("VerNegoReqMessage: eventNum>0",m_msgBuf,m_msgLen);
        return -2;
    }

    //get protocle version(1) + subVersion(1)
    ver = (UINT)*pData;
    subVer = (UINT)*(pData+1);

    //check version
    UINT value = (ver<<8) + subVer;
    UINT rightValue = ((ZxComm::m_version)<<8) + ZxComm::m_subVersion;
    if(value == rightValue){
        //correct
        result = 1;
        LOG4CXX_INFO(rootLogger, "("<<m_zxHostIP<<")verNegoReq success!");
    }else if(value>rightValue){
        //too high 
        LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")verNegoReq failed, too high version="<<ver<<"."<<subVer);
        result = 2;
    }else if(value<rightValue){
        //too low
        LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")verNegoReq failed, too low version="<<ver<<"."<<subVer);
        result = 3;
    }

    VerNegoRspMessage* pMessage = new VerNegoRspMessage();
    if(!pMessage){
        LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")new VerNegoRspMessage failed!");
        return -1;
    }else{
        pMessage->setZxClient(m_pZxClient);
        pMessage->setMessageType(verNego_Resp);
        pMessage->setMessageSeqID(m_msgSeqID);
        pMessage->setMessageEventNum(m_msgEventNum);
        pMessage->setResult(result);
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

    return ret;
}


VerNegoRspMessage::VerNegoRspMessage(){
    m_result = 0;
}


VerNegoRspMessage::~VerNegoRspMessage(){
}


/*
version negotiation response message :
    message header(9B) + message body(1B)
body: 
    result(1)
*/
int VerNegoRspMessage::sendMessage(){
    m_msgBuf = new UCHAR[VERNEGO_RSP_MSG_LEN+1];
    
    //set message body
    m_msgBuf[MSG_HEAD_LEN] = m_result&0xff;
    m_msgLen = MSG_HEAD_LEN+1;
    m_msgBuf[m_msgLen] = 0;
    
    //set message header
    setMessageHeader();

    if(!m_pZxClient){
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")send VerNegoRspMessage failed, null zxClient!");
        return -1;
    }

    LoggerPtr rootLogger = Logger::getRootLogger();  
    LOG4CXX_INFO(rootLogger, "==>("<<m_zxHostIP<<") send VerNegoRspMessage!");
   // G::displayHex("VerNegoRspMessage",m_msgBuf,m_msgLen);

    int ret = m_pZxClient->sktSend(m_msgLen,m_msgBuf);
    if((0 == ret) && (1 == m_result)){
        m_pZxClient->setStatus(NegoSuc);
    }
    /*else{
        m_pZxClient->beforeDel();
    }   */

    return ret;
}


LinkAuthReqMessage::LinkAuthReqMessage(){
}

LinkAuthReqMessage::LinkAuthReqMessage(PUCHAR recvMsg, UINT recvLen, ZxClient *pClient):Message(recvMsg,recvLen,pClient)
{
}

LinkAuthReqMessage::~LinkAuthReqMessage(){
}


/*
link authentication request message body: 34B
loginID(12) + Digest(16) + timeStamp(4) + rand(2)
0: success
-1:failed
*/
int LinkAuthReqMessage::parseMessageBody(){
    int ret = 0;
    PUCHAR pData = m_msgBuf + MSG_HEAD_LEN;
    UINT rand = 0;
    ULONG timeStamp = 0;
    UCHAR recvDigest[DIGEST_LEN+1];
    UCHAR loginID[LOGINID_LEN+1];
    UINT result = 0;
    
    LoggerPtr rootLogger = Logger::getRootLogger();  
    LOG4CXX_INFO(rootLogger, "<==("<<m_zxHostIP<<")receive LinkAuthReqMessage!");
   // G::displayHex("LinkAuthReqMessage",m_msgBuf,m_msgLen);

    if(LINKAUTH_REQ_MSG_LEN != m_msgLen){
        LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")LinkAuthReqMessage: wrong msgLen="<<m_msgLen);
        G::displayHex("LinkAuthReqMessage: wrong len",m_msgBuf,m_msgLen);
        return -2;
    }

    if(m_msgEventNum>1){
        LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")LinkAuthReqMessage: eventNum="<<m_msgEventNum);
        G::displayHex("LinkAuthReqMessage: eventNum>0",m_msgBuf,m_msgLen);
        return -2;
    }

    memset(loginID,0,LOGINID_LEN+1);
    memset(recvDigest,0,DIGEST_LEN+1);
    
    //get loginID:12B
    memcpy(loginID,pData,LOGINID_LEN);
    loginID[LOGINID_LEN] = 0;
    pData +=LOGINID_LEN;

    //get Digest:16B
    memcpy(recvDigest,pData,DIGEST_LEN);
    recvDigest[DIGEST_LEN] = 0;
    pData +=DIGEST_LEN;
    
    //get timeStamp:4B
    timeStamp = (ULONG)(((((*pData<<8)+*(pData+1))<<8)+*(pData+2))<<8)+*(pData+3);
    pData +=4;
    
    //get rand:2B
    rand = (UINT)(*pData<< 8) + *(pData+1);

    //check loginID
    /*
    if(0 != strcmp((char*)ZxComm::m_subscrbLoginID.c_str(), (char*)loginID)){
        //wrong loginID
        result = 2;
        LOG4CXX_WARN(rootLogger, "("<<m_zxHostIP<<")wrong loginID="<<loginID<<", right loginID="<<ZxComm::m_subscrbLoginID);
    }else{
        //check digest
        result = checkRecvDigest(timeStamp,rand,recvDigest);
        if(-1 == result){
            return -1;
        }
    }
    
    UCHAR sendDigest[DIGEST_LEN+1];
    ret = setSendDigest(timeStamp,rand,sendDigest);
    if(-1 == ret){
        return -1;
    }
    */
    UCHAR sendDigest[DIGEST_LEN+1];

    LinkAuthRspMessage* pMessage = new LinkAuthRspMessage();
    if(!pMessage){
        LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")new LinkAuthRspMessage failed!");
        return -1;
    }else{
        pMessage->setZxClient(m_pZxClient);
        pMessage->setMessageType(linkAuth_Resp);
        pMessage->setMessageSeqID(m_msgSeqID);
        pMessage->setMessageEventNum(m_msgEventNum);
		// 暂时放开鉴权请求，赋值1
		result = 1;
        pMessage->setResult(result);
        pMessage->setDigest(sendDigest);
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


//recv Digest = MD5(LoginID+MD5(shared secret)+Timestamp+"rand="+RAND):16B
//return -1: linkAuth failed
//return 1: linkAuth success,right digest
//return 3: linkAuth failed,wrong digest
int LinkAuthReqMessage::checkRecvDigest(ULONG timeStamp, UINT rand, PUCHAR recvDigest){
    LoggerPtr rootLogger = Logger::getRootLogger();  
   // LOG4CXX_DEBUG(rootLogger, "timeStamp="<<timeStamp);
   // LOG4CXX_DEBUG(rootLogger, "rand="<<rand);
    
    UCHAR digest[33]; 
    memset(digest,0,33);
    bool bOK = G::Hex2Str(recvDigest,digest,DIGEST_LEN);
    if(false == bOK){
        LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")Hex2Str(recvDigest) failed!");
        G::displayHex("recvDigest",recvDigest,DIGEST_LEN);
        return -1;
    }
    
    int result = -1;
    int ret = 0;
    char md5Raw[200]; 
    memset(md5Raw, 0, 200);
    int totallen = 0;
    
    ret = G::formatString((char*)ZxComm::m_subscrbLoginID.c_str(),LOGINID_LEN,md5Raw);
    if(-1 == ret){
        return -1;
    }
    totallen = LOGINID_LEN;

    //check Digest
    char* pwdMd5 = GetMD5((char*)ZxComm::m_subscrbPwd.c_str());
   // LOG4CXX_DEBUG(rootLogger, "pwd="<<ZxComm::m_subscrbPwd<<", GetMD5(pwd)="<<pwdMd5);
    if(!pwdMd5){
        LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")Get md5(pwd) failed! pwd="<<ZxComm::m_subscrbPwd);
        return -1;
    }
    
    memcpy(md5Raw+totallen,pwdMd5,32);
    totallen += 32;
    delete[] pwdMd5;
    pwdMd5 = 0;
    
    char* ss = (char*)G::long2String(timeStamp).c_str();
    int l = strlen(ss);
    memcpy(md5Raw+totallen,ss,l);
    totallen += l;
    
    md5Raw[totallen++] = 'r';
    md5Raw[totallen++] = 'a';
    md5Raw[totallen++] = 'n';
    md5Raw[totallen++] = 'd';
    md5Raw[totallen++] = '=';
    ss = 0;
    ss = (char*)G::int2String(rand).c_str();
    l = strlen(ss);
    memcpy(md5Raw+totallen,ss,l);
    totallen += l;
    
    char* rightDigest = GetMD5(md5Raw);
    //LOG4CXX_DEBUG(rootLogger, "md5Raw="<<md5Raw<<", GetMD5(md5Raw)="<<rightDigest);
    if(!rightDigest){
        LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")Get md5(md5Raw) failed! md5Raw="<<md5Raw);
        return -1;
    }

    if(0 != strcmp((char*)digest,rightDigest)){
        //wrong digest
        result = 3;
        LOG4CXX_WARN(rootLogger, "("<<m_zxHostIP<<")wrong recvDigest="<<digest<<", right Digest="<<rightDigest);
    }else{
        //linkAuth success
        result = 1;
        LOG4CXX_INFO(rootLogger, "("<<m_zxHostIP<<")linkAuth success!");
    }
    
    delete[] rightDigest;
    rightDigest = 0;
    return result;
}


int LinkAuthReqMessage::setSendDigest(ULONG timeStamp, UINT rand, PUCHAR sendDigest){
    LoggerPtr rootLogger = Logger::getRootLogger();  
    int ret = 0;
    char md5Raw[200]; 
    memset(md5Raw, 0, 200);
    int totallen = 0;
    
    //get send Digest =MD5(LoginID+MD5(shared secret) +"rand="+ RAND +Timestamp)
    ret = G::formatString((char*)ZxComm::m_subscrbLoginID.c_str(),LOGINID_LEN,md5Raw);
    if(-1 == ret){
        return -1;
    }
    totallen = LOGINID_LEN;
    
    char* pwdMd5 = GetMD5((char*)ZxComm::m_subscrbPwd.c_str());
    // LOG4CXX_DEBUG(rootLogger, "pwd="<<ZxComm::m_subscrbPwd<<", GetMD5(pwd)="<<pwdMd5);
    if(!pwdMd5){
        LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")Get md5(pwd) failed! pwd="<<ZxComm::m_subscrbPwd);
        return -1;
    }
    
    memcpy(md5Raw+totallen,pwdMd5,32);
    totallen += 32;
    delete[] pwdMd5;
    pwdMd5 = 0;
    
    md5Raw[totallen++] = 'r';
    md5Raw[totallen++] = 'a';
    md5Raw[totallen++] = 'n';
    md5Raw[totallen++] = 'd';
    md5Raw[totallen++] = '=';
    char* ss = (char*)G::int2String(rand).c_str();
    int l = strlen(ss);
    memcpy(md5Raw+totallen,ss,l);
    totallen += l;
    
    ss = 0;
    ss = (char*)G::long2String(timeStamp).c_str();
    l = strlen(ss);
    memcpy(md5Raw+totallen,ss,l);
    totallen += l;
    
    char* myMd5 = GetMD5(md5Raw);
    if(!myMd5){
        LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")Get md5(md5Raw) failed! md5Raw="<<md5Raw);
        return -1;
    }
    //LOG4CXX_DEBUG(rootLogger, "md5Raw="<<md5Raw<<", GetMD5(md5Raw)="<<myMd5);

    memset(sendDigest,0,DIGEST_LEN+1);
    bool bOK = G::Str2Hex((PUCHAR)myMd5,sendDigest,32);
    if(false == bOK){
        LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")Str2Hex("<<myMd5<<") failed!");
        delete[] myMd5;
        myMd5 = 0;
        return -1;
    }

   // G::displayHex("sendDigest",sendDigest,DIGEST_LEN);
    delete[] myMd5;
    myMd5 = 0;
    return 0;
}


LinkAuthRspMessage::LinkAuthRspMessage(){
    m_result = 0;
}


LinkAuthRspMessage::~LinkAuthRspMessage(){
}

int LinkAuthRspMessage::setDigest(PUCHAR value){
    memset(m_digest,0, DIGEST_LEN+1);
    memcpy(m_digest,value,DIGEST_LEN);
    m_digest[DIGEST_LEN] = 0;
    return 0;
}

/*
link authentication response message: 26B
    message header(9B) + message body(17B)
body: 
    result(1) + digest(16)

 return 0: send success
 <0: send failed
*/
int LinkAuthRspMessage::sendMessage(){
    m_msgBuf = new UCHAR[LINKAUTH_RSP_MSG_LEN+1];
    if(NULL == m_msgBuf){
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")new msgBuf failed!");
        return -1;
    }

    //set Message Body
    m_msgBuf[MSG_HEAD_LEN] = m_result&0xff;
    m_msgLen = MSG_HEAD_LEN+1;
    memcpy(m_msgBuf+m_msgLen,m_digest,DIGEST_LEN);
    m_msgLen += DIGEST_LEN;
    m_msgBuf[m_msgLen] = 0;
    
    //set message header
    setMessageHeader();

    if(!m_pZxClient){
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")send LinkAuthRspMessage failed, null zxClient!");
        return -1;
    }
    
    LoggerPtr rootLogger = Logger::getRootLogger(); 
    LOG4CXX_INFO(rootLogger, "==>("<<m_zxHostIP<<") send LinkAuthRspMessage!");
   // G::displayHex("LinkAuthRspMessage",m_msgBuf,m_msgLen);

    int ret = m_pZxClient->sktSend(m_msgLen,m_msgBuf);
    if((0 == ret) && (1 == m_result)){
        m_pZxClient->setStatus(AuthSuc);
       // LOG4CXX_INFO(rootLogger, "==>("<<m_zxHostIP<<") send LinkAuthRspMessage sucess!");
    }else{
        m_pZxClient->beforeDel();
    }   

    return ret;
	
}


LinkCheckReqMessage::LinkCheckReqMessage(){
}

LinkCheckReqMessage::LinkCheckReqMessage(PUCHAR recvMsg, UINT recvLen, ZxClient *pClient):Message(recvMsg,recvLen,pClient)
{
}

LinkCheckReqMessage::~LinkCheckReqMessage(){
}

/*
link check request message body: 0B
0: success
-1:failed
*/
int LinkCheckReqMessage::parseMessageBody(){    
    LoggerPtr rootLogger = Logger::getRootLogger();  
    LOG4CXX_INFO(rootLogger, "<==("<<m_zxHostIP<<")receive LinkCheckReqMessage!");

    if(LINKCHECK_REQ_MSG_LEN != m_msgLen){
        LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")LinkCheckReqMessage: wrong msgLen="<<m_msgLen);
        G::displayHex("LinkCheckReqMessage: wrong len",m_msgBuf,m_msgLen);
        return -1;
    }

    if(m_msgEventNum>1){
        LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")LinkCheckReqMessage: eventNum="<<m_msgEventNum);
        G::displayHex("LinkCheckReqMessage: eventNum>0",m_msgBuf,m_msgLen);
        return -1;
    }

    LinkCheckRspMessage* pMessage = new LinkCheckRspMessage();
    if(!pMessage){
        LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")new LinkCheckRspMessage failed!");
        return -1;
    }else{
        pMessage->setZxClient(m_pZxClient);
        pMessage->setMessageType(linkCheck_Resp);
        pMessage->setMessageSeqID(m_msgSeqID);
        pMessage->setMessageEventNum(m_msgEventNum);
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


LinkCheckRspMessage::LinkCheckRspMessage(){
}


LinkCheckRspMessage::~LinkCheckRspMessage(){
}

/*
link check response message: 9B
    message header(9B) + message body(0B)

 return 0: send success
 <0: send failed
*/
int LinkCheckRspMessage::sendMessage(){
    m_msgBuf = new UCHAR[LINKCHECK_RSP_MSG_LEN+1];

    //set Message Body
    m_msgLen = MSG_HEAD_LEN;
    m_msgBuf[m_msgLen] = 0;
    
    //set message header
    setMessageHeader();

    if(!m_pZxClient){
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")send LinkCheckRspMessage failed, null zxClient!");
        return -1;
    }

    LoggerPtr rootLogger = Logger::getRootLogger();  
    LOG4CXX_INFO(rootLogger, "==>("<<m_zxHostIP<<") send LinkCheckRspMessage!");

    int ret = m_pZxClient->sktSend(m_msgLen,m_msgBuf);
    if(0 != ret){
        m_pZxClient->beforeDel();
    }   

    return ret;
}


LinkRelReqMessage::LinkRelReqMessage(){
}

LinkRelReqMessage::LinkRelReqMessage(PUCHAR recvMsg, UINT recvLen, ZxClient *pClient):Message(recvMsg,recvLen,pClient)
{
}

LinkRelReqMessage::~LinkRelReqMessage(){
}

/*
link release request message body: 1B
    reason(1)
0: success
-1:failed
*/
int LinkRelReqMessage::parseMessageBody(){    
    int ret = 0;
    PUCHAR pData = m_msgBuf + MSG_HEAD_LEN;
    
    LoggerPtr rootLogger = Logger::getRootLogger();  
    LOG4CXX_INFO(rootLogger, "<==("<<m_zxHostIP<<")receive LinkRelReqMessage!");

    if(LINKREL_REQ_MSG_LEN != m_msgLen){
        LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")LinkRelReqMessage: wrong msgLen="<<m_msgLen);
        G::displayHex("LinkRelReqMessage: wrong len",m_msgBuf,m_msgLen);
        return -2;
    }

    if(m_msgEventNum>1){
        LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")LinkRelReqMessage: eventNum="<<m_msgEventNum);
        G::displayHex("LinkRelReqMessage: eventNum>0",m_msgBuf,m_msgLen);
        return -2;
    }

    UINT reason = (UINT)pData&0xff;
    LOG4CXX_WARN(rootLogger, "("<<m_zxHostIP<<")LinkRelReqMessage: reason="<<reason);

    UINT result = 1;
    
    LinkRelRspMessage* pMessage = new LinkRelRspMessage();
    if(!pMessage){
        LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")new LinkRelRspMessage failed!");
        return -2;
    }else{
        pMessage->setZxClient(m_pZxClient);
        pMessage->setMessageType(linkRel_Resp);
        pMessage->setMessageSeqID(m_msgSeqID);
        pMessage->setMessageEventNum(m_msgEventNum);
        pMessage->setResult(result);
        if(m_pZxClient){
            HwsQueue* pQueue = m_pZxClient->getSendMsgQueue();
            if(pQueue){
                pQueue->put(pMessage);
            }else{
                delete pMessage;
                pMessage = 0;
                LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")null pSendQueue!");
                return -2;
            }
        }else{
            delete pMessage;
            pMessage = 0;
            LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")null zxClient!");
            return -2;
        }
    }

    return ret;
}


LinkRelRspMessage::LinkRelRspMessage() {
}


LinkRelRspMessage::~LinkRelRspMessage() {
}


/*
link release response message :
    message header(9B) + message body(1B)
body: 
    result(1)

 return 0: send success
 <0: send failed
*/
int LinkRelRspMessage::sendMessage(){
    m_msgBuf = new UCHAR[LINKREL_RSP_MSG_LEN+1];
    
    //set message body
    m_msgBuf[MSG_HEAD_LEN] = m_result&0xff;
    m_msgLen = MSG_HEAD_LEN+1;
    m_msgBuf[m_msgLen] = 0;
    
    //set message header
    setMessageHeader();

    if(!m_pZxClient){
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")send LinkRelRspMessage failed, null zxClient!");
        return -2;
    }

    LoggerPtr rootLogger = Logger::getRootLogger();  
    LOG4CXX_INFO(rootLogger, "==>("<<m_zxHostIP<<") send LinkRelRspMessage!");

    int ret = m_pZxClient->sktSend(m_msgLen,m_msgBuf);
    m_pZxClient->beforeDel();

    return ret;
}


CDRReqMessage::CDRReqMessage() {
}


CDRReqMessage::CDRReqMessage(PUCHAR recvMsg, UINT recvLen, ZxClient *pClient):Message(recvMsg,recvLen,pClient)
{
}

CDRReqMessage::~CDRReqMessage() {
}

/*
notify CDR/TDR data request message body: 
CDROrderID(8B) + Load1 + Load2

Load:
CDRLength(2B) + CDRVersion(1B) + VariableIENumber(2B) + ProtocolType(2B) + CDRID(8B)
+ FixedLoad(74B) + OptionalLoad(?)

0: success
-1:failed, invalid message
*/
int CDRReqMessage::parseMessageBody(){    
    int ret = 0;
    UINT result = 1;
    PUCHAR pData = m_msgBuf+ MSG_HEAD_LEN;
    UINT len = 0;

    if(m_msgEventNum>1){
         LoggerPtr rootLogger = Logger::getRootLogger();  
         LOG4CXX_WARN(rootLogger, "<==("<<m_zxHostIP<<")receive CDRReqMessage message! msgLen="<<m_msgLen<<", eventNum="<<m_msgEventNum);
        // G::displayHex("CDRReqMessage",m_msgBuf,m_msgLen);
    }

    //get CDR Data
    for(int i=0; i<m_msgEventNum; i++){
        ++ZxComm::m_nTotalDRs;
        m_pZxClient->incTotalDRMsgs();
        
        pData += len;

        //get CDROrderID(8B)
        ULONGLONG orderID = (ULONGLONG)*pData;
        for(int i=1;i<8;i++){
            orderID = orderID<<8;
            orderID += (ULONGLONG)*(pData+i);
        }
        pData +=8;
        
        //check CDROrderID
        if(ZxComm::m_CDROrderID != orderID){
            LoggerPtr rootLogger = Logger::getRootLogger();  
            LOG4CXX_ERROR(rootLogger, "CDRReqMessage: wrong CDROrderID="<<orderID);
            G::displayHex("CDRReqMessage: wrong CDROrderID",m_msgBuf,m_msgLen);
            ret = -1;
            break;
        }
    
        //get CDRLen(2B)
        len = (UINT)(*pData<< 8) + *(pData+1);
    
        //check CDRLen
        if(CDR_DATA_MAX_LEN < len){
             ret = -1;
             LoggerPtr rootLogger = Logger::getRootLogger();  
             LOG4CXX_ERROR(rootLogger, "CDRReqMessage: too long CDRLen="<<len);
             G::displayHex("CDRReqMessage: wrong CDRLen",m_msgBuf,m_msgLen);
             break;
        }
    
        CDRMessage* pMessage = new CDRMessage();
        if(pMessage){
            pMessage->setRecvTime(m_recvTime);
            pMessage->setPackID(m_packID);
            pMessage->setZxClient(m_pZxClient);
            pMessage->setMessageSeqID(m_msgSeqID);
            pMessage->setMessageBuf(pData, len);
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
    }
    
    if(0 != ret){
        result = 0;
    }else{
        result = 1;
    }

   
    //send response message to zx
    CDRRspMessage* pMessage = new CDRRspMessage();
    if(!pMessage){
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")new CDRRspMessage failed!");
        return -1;
    }else{
        pMessage->setZxClient(m_pZxClient);
        pMessage->setMessageType(notifyCDRData_Resp);
        pMessage->setMessageSeqID(m_msgSeqID);
        pMessage->setMessageEventNum(m_msgEventNum);
        pMessage->setResult(result);
        if(m_pZxClient){
            HwsQueue* pQueue = m_pZxClient->getSendMsgQueue();
            if(pQueue){
                pQueue->put(pMessage);
            }else{
                delete pMessage;
                pMessage = 0;
                LoggerPtr rootLogger = Logger::getRootLogger();  
                LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")null pSendQueue!");
                return -1;
            }
        }else{
            delete pMessage;
            pMessage = 0;
            LoggerPtr rootLogger = Logger::getRootLogger();  
            LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")null zxClient!");
            return -1;
        }
    }

    return 0;
}

CDRRspMessage::CDRRspMessage() {
}

CDRRspMessage::~CDRRspMessage(){
}

int CDRRspMessage::sendMessage(){
    m_msgBuf = new UCHAR[CDR_RSP_MSG_LEN+1];
    
    //set message body
    m_msgBuf[MSG_HEAD_LEN] = m_result&0xff;
    m_msgLen = MSG_HEAD_LEN+1;
    m_msgBuf[m_msgLen] = 0;
    
    //set message header
    setMessageHeader();

    if(!m_pZxClient){
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")send CDRRspMessage failed, null zxClient!");
        return -1;
    }

  //  LoggerPtr rootLogger = Logger::getRootLogger();  
  //  LOG4CXX_INFO(rootLogger, "==>("<<m_zxHostIP<<") send CDRRspMessage!");

    int ret = m_pZxClient->sktSend(m_msgLen,m_msgBuf);
    if(0 != ret){
        m_pZxClient->beforeDel();
    }    

    return ret;
}

//end of file message.cpp
