#include "hwsshare.h"
#include "queue.h"
#include "cyclicqueue.h"
#include "hwsqueue.h"
#include "g.h"
#include "comm.h"
#include "message.h"
#include "appmonitor.h"
#include "zxclient.h"
#include "sugwserver.h"
#include "zxcomm.h"


void msgRecvThread(void* para);
void msgProcessThread(void* para);
void msgSendThread(void* para);

ZxClient::ZxClient(){
    m_tidMsgRecv = 0;
    m_zxPort = 0;
    m_zxHostIP = "";
    m_zxSocket = 0;
    m_recvMsgQueue = 0;   
    m_sendMsgQueue = 0;
    m_nThread = 0;
    m_nTotalDRs = 0;
    m_nIAM = 0;
    m_nDelayDRs = 0;
    m_nLastTotalDRs = 0;
    m_nLastDelayDRs = 0;
    m_nLastIAM = 0;
    m_status = Close;

    m_bDelayAlert = false;
    m_startDelayTime = 0;
    m_lastDelayTime = 0;
    m_nDelayCount = 0;
    m_nMaxInnerDelay = 0;
    m_nMaxOuterDelay = 0;
    m_lastHeartTime = 0;
}

ZxClient::~ZxClient(){
    if(m_recvMsgQueue){        
        delete m_recvMsgQueue;
        m_recvMsgQueue = 0;
    }

    if(m_sendMsgQueue){        
        delete m_sendMsgQueue;
        m_sendMsgQueue = 0;
    }
}

int ZxClient::init(int fd, int port, char* hostIP){
    m_zxSocket = fd;
    m_zxPort = port;
    m_zxHostIP = hostIP;
    m_status = Connect;
    ZxComm::m_nConnectedZxClients++;
    
    if (pthread_mutex_init(&m_statusMutex, 0)) {
        LoggerPtr rootLogger = Logger::getRootLogger();
        LOG4CXX_ERROR(rootLogger, "pthread_mutex_init failed!");
        return -1;
    }
    
    if (pthread_mutex_init(&m_zxSktMutex, 0)) {
        LoggerPtr rootLogger = Logger::getRootLogger();
        LOG4CXX_ERROR(rootLogger, "pthread_mutex_init failed!");
        return -1;
    }
    
    m_sendMsgQueue = new HwsQueue();
    if(!m_sendMsgQueue){
        LoggerPtr rootLogger = Logger::getRootLogger();
        LOG4CXX_ERROR(rootLogger, "new HwsQueue failed!");
        return -1;
    }
    
    m_recvMsgQueue = new CyclicQueue();
    if(!m_recvMsgQueue){
        LoggerPtr rootLogger = Logger::getRootLogger();
        LOG4CXX_ERROR(rootLogger, "new CyclicQueue failed!");
        return -1;
    }
    
    m_recvMsgQueue->init(hostIP);
    
    //create thread to process message from zx
    for (int i = 0; i < ZxComm::m_msgProcessNum; i++) {
      if (pthread_create(&m_tidMsgProcess[i], 0, (void *(*)(void*)) &msgProcessThread, (void*)this)) {
          LoggerPtr rootLogger = Logger::getRootLogger();  
          LOG4CXX_ERROR(rootLogger,"("<<hostIP<<")Create msgProcessThread failed!");
          return -1;
      }
    }    

    //create thread send message to zx
    if (pthread_create(&m_tidMsgSend, NULL, (void *(*)(void*)) &msgSendThread, (void*)this)) {
      LoggerPtr rootLogger = Logger::getRootLogger();
      LOG4CXX_ERROR(rootLogger, "("<<hostIP<<")Create msgSendThread failed!");
      return -1;
    }

    //create thread receive message from zx
    if (pthread_create(&m_tidMsgRecv, NULL, (void *(*)(void*)) &msgRecvThread, (void*)this)) {
      LoggerPtr rootLogger = Logger::getRootLogger();
      LOG4CXX_ERROR(rootLogger, "("<<hostIP<<")Create msgProcessThread failed!");
      return -1;
    }

    return 0;
}


int ZxClient::checkComm(){
    int ret = 0;
    long now = 0;
    time(&now);
    
    switch(m_status){        
        case Connect:
            if(now - m_lastHeartTime > ZxComm::m_maxHeartInterval){
                LoggerPtr rootLogger = Logger::getRootLogger();
                LOG4CXX_ERROR(rootLogger, "Receive heart msg from "<<m_zxHostIP<<" timeout(>"<<ZxComm::m_maxHeartInterval<<"s)!");
                beforeDel();
            }
            break;
        case Close:
            if(0 == m_nThread){
                ret = -1;
            }
            break;
        default:
            break;
    }

    return ret;
}


void ZxClient::beforeDel(){
    if(Close == m_status){
        return;
    }
    
    LoggerPtr rootLogger = Logger::getRootLogger();
    LOG4CXX_WARN(rootLogger, "close communication with ZX "<<m_zxHostIP<<":"<<m_zxPort);
    Comm::closeSocket(m_zxSocket);
    setStatus(Close);
    ZxComm::m_nConnectedZxClients--;
    
    if(m_recvMsgQueue){
        DataControl& rDataControl1 = m_recvMsgQueue->getDataControl();
        rDataControl1.deactivate();    
    }

    if(m_sendMsgQueue){
        DataControl& rDataControl1 = m_sendMsgQueue->getDataControl();
        rDataControl1.deactivate();    
    }
    return;
}


void ZxClient::setStatus(Status4ZxClient value){
    if (pthread_mutex_lock(&m_statusMutex)) {
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_ERROR(rootLogger, "pthread_mutex_lock(&m_statusMutex) failed!");
        return;
    }
    m_status = value;
    pthread_mutex_unlock(&m_statusMutex);
}

Status4ZxClient ZxClient::getStatus(){
    Status4ZxClient value = Close;
    if (pthread_mutex_lock(&m_statusMutex)) {
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_ERROR(rootLogger, "pthread_mutex_lock(&m_statusMutex) failed!");
        return value;
    }
    value =  m_status;
    pthread_mutex_unlock(&m_statusMutex);
    return value;
}


//send msg to zxclient
//return 0:   send success
//return <0: send failed
int ZxClient::sktSend(UINT sendLen, PUCHAR sendBuf) {
    int ret = -1;    
    int len = 0;
    LoggerPtr rootLogger = Logger::getRootLogger();  

    fd_set fds_w;
    int maxfd = 0; 
    int fd = m_zxSocket;
    FD_ZERO(&fds_w);
    FD_SET(fd, &fds_w);
    if (maxfd < fd)
    {
      maxfd = fd;
    }
    
    if (0 == maxfd)
    {
        LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")sktSend: wrong fd="<<fd);
        return -1;
    }
    
    struct timeval tv;
    tv.tv_sec = 10; 
    tv.tv_usec = 0;
    maxfd++;
    int nready = select(maxfd, (fd_set *) 0, &fds_w, (fd_set *) 0,&tv);
    switch (nready) 
    {
    case -1:
        //error
        LOG4CXX_ERROR(rootLogger, "("<<m_zxHostIP<<")sktSend: select error:"<<strerror(errno));
        return -1;
    case 0:  
        //timeout
        LOG4CXX_ERROR(rootLogger, "send msg to ZX("<<m_zxHostIP<<") timeout!");
        return -1;
    default:  
        break;
    }  
    
    if (nready > 0)
    {
        if (FD_ISSET(fd, &fds_w))
        {
            len = send(fd, sendBuf, sendLen, 0);
            if( 1 > len) {
               // send failed
               LOG4CXX_ERROR(rootLogger, "send msg to ZX("<<m_zxHostIP<<") failed, errno="<<errno<<", error:" << strerror(errno));
               ret = -1;
            }else{
               //send success
               ret = 0;
            }
        }else{
            LOG4CXX_ERROR(rootLogger, "send msg to ZX("<<m_zxHostIP<<") failed!");
            ret = -1;
        }
    }

    return ret;
}


void ZxClient::setHeartTime(){
    long now = 0;
    time(&now);
    m_lastHeartTime = now;
}



//process msg from zcxc 
void msgRecvThread(void *para) {
  LoggerPtr rootLogger = Logger::getRootLogger();  
  ZxClient *pClient = 0;
  pClient = (ZxClient*)para;  
  if(!pClient){
     LOG4CXX_ERROR(rootLogger, "msgRecvThread param is NULL!");
     return;
  }
  
  string hostIP = pClient->getHostIP();
  CyclicQueue *pBuffQueue = pClient->getRecvMsgQueue();  
  int fd = pClient->getSocket();
  pClient->incThreadNum();
  
  pthread_t tid = syscall(SYS_gettid);
  LOG4CXX_INFO(rootLogger, "("<<hostIP<<")msgRecvThread tid=" << tid<<", threadNum="<<++G::m_threadNum);

  fd_set fds; // set of file descriptors to check.
  int nready = 0; // # fd's ready. 
  int maxfd = 0;  // fd's 0 to maxfd-1 checked.
  struct timeval tv;  
  int readNum = 0;
  int rc = 0;
  UCHAR packageBuff[4096];
  Status4ZxClient status;
  
  DataControl& rDataControl = pBuffQueue->getDataControl();
  rDataControl.activate();
  while (!G::m_bSysStop  && rDataControl.isActive()) {
      status = pClient->getStatus();
      if(Close == status){
          break;
      }else{
          fd = pClient->getSocket();
      }      
      
      FD_ZERO(&fds);
      maxfd = 0;
      FD_SET(fd, &fds);
      if (maxfd < fd)
      {
        maxfd = fd;
      }
      
      if (0 ==  maxfd)
      {
        //there is no socket connected 
        pClient->beforeDel();
        break;
      }
      
      tv.tv_sec = NODATA_TIME_LIMIT; 
      tv.tv_usec = 0;
      maxfd ++;
      nready = select(maxfd, &fds, (fd_set *) 0, (fd_set *) 0,&tv);
      switch (nready) 
      {
      case -1:
          //error
          LOG4CXX_ERROR(rootLogger, "("<<hostIP<<")select error:"<<strerror(errno));
          pClient->beforeDel();
          break;
      case 0:  
          //timeout 
          LOG4CXX_ERROR(rootLogger, "Receive msg from ZX("<<hostIP<<") timeout!");
          pClient->beforeDel();
          break;
      default:  
          break;
      }  
  
      if (nready > 0)
      {
          if (FD_ISSET(fd, &fds))
          {
              //fd have something to read
              readNum = recv(fd, packageBuff, 4095, 0);            
              if( 1 > readNum) {
                 LOG4CXX_ERROR(rootLogger, "Receive from ZX("<<hostIP<<") failed, nread= "<<readNum<<", errno="<<errno<<", error:" << strerror(errno));
                 pClient->beforeDel();
                 break;
              }else {
                 rc = pBuffQueue->push(packageBuff, readNum);
                 if(0==rc){
                    LOG4CXX_ERROR(rootLogger, "("<<hostIP<<")Push raw buff to queue failed! queue size="<<pBuffQueue->getSize());
                    sleep(1);
                 }
              }
          }
      }
  }
  
  pClient->decThreadNum();
  LOG4CXX_INFO(rootLogger, "("<<hostIP<<")Exit from msgRecvThread threadNum="<<--G::m_threadNum);
  return;
}


void msgProcessThread(void* para) {
  LoggerPtr rootLogger = Logger::getRootLogger();  
  ZxClient *pClient = 0;
  pClient = (ZxClient*)para;  
  if(!pClient){
     LOG4CXX_ERROR(rootLogger, "msgProcessThread param is NULL!");
     return;
  }
  
  string hostIP = pClient->getHostIP();
  CyclicQueue *pRecvQueue = pClient->getRecvMsgQueue();  
  pClient->incThreadNum();
  
  pthread_t tid = syscall(SYS_gettid);
  LOG4CXX_INFO(rootLogger, "("<<hostIP<<")msgProcessThread tid=" << tid<<", threadNum="<<++G::m_threadNum);

  int ret = 0;
  int rc = 0;
  UINT recvLen = 0;
  UCHAR recvBuf[4096];
  UINT packID = 0;  
  Message* pMessage = 0;
  Status4ZxClient status;
  
  DataControl& rDataControl = pRecvQueue->getDataControl();
  rDataControl.activate();
  
  while (!G::m_bSysStop && rDataControl.isActive()) {
    status = pClient->getStatus();
    if(pRecvQueue->isEmpty()) {
        if(Close == status){
            break;
        }
        
        pthread_mutex_lock(&rDataControl.getMutex());
        pthread_cond_wait(&rDataControl.getCond(), &rDataControl.getMutex());
        pthread_mutex_unlock(&rDataControl.getMutex());
        continue;
    }
    
    memset(recvBuf,0,4096);
    if (rDataControl.isActive()) {
        rc = pRecvQueue->getNextData(recvBuf,recvLen,packID);
        if(1 == rc){
            pMessage = Message::createInstance(recvBuf,recvLen,pClient);
            if(pMessage){
                pMessage->setPackID(packID);
                ret = pMessage->parseMessageBody();
                if(-2 == ret){
                    pClient->beforeDel();
                }
                delete pMessage;
                pMessage = 0;
            }
        }else if(0 == rc){
            //no package get
            LOG4CXX_DEBUG(rootLogger, "("<<hostIP<<")No package there. Buff size="<<pRecvQueue->getSize()<<", isEmpty="<<pRecvQueue->isEmpty());
            sleep(1);
        }else if (-1==rc){
            //package len is too big,reconnect
            //LOG4CXX_WARN(rootLogger, "Package len is too big, clear queque!");
            pRecvQueue->clear();
        }else{
           //unknow result
          LOG4CXX_ERROR(rootLogger, "("<<hostIP<<")Unknow result, rc="+G::int2String(rc));
        }
    }
  }//end of while (!m_bSysStop && pDataControl)

  pClient->decThreadNum();
  LOG4CXX_INFO(rootLogger, "("<<hostIP<<")Exit msgProcessThread, threadNum="<<--G::m_threadNum);
  return;
}


//socket send to zxclient 
void msgSendThread(void *para) {
  ZxClient *pClient = 0;
  pClient = (ZxClient*)para;  
  if(!pClient){
     LoggerPtr rootLogger = Logger::getRootLogger();  
     LOG4CXX_ERROR(rootLogger, "msgSendThread param is NULL!");
     return;
  }
  
  string hostIP = pClient->getHostIP();
  HwsQueue *pSendQueue = pClient->getSendMsgQueue();  
  pClient->incThreadNum();
  Status4ZxClient status;
  
  pthread_t tid = syscall(SYS_gettid);
  LoggerPtr rootLogger = Logger::getRootLogger();  
  LOG4CXX_INFO(rootLogger, "("<<hostIP<<")msgSendThread tid=" << tid<<", threadNum="<<++G::m_threadNum);

  Message* pMessage = 0;

  DataControl& rDataControl = pSendQueue->getDataControl();
  rDataControl.activate();
  while (!G::m_bSysStop && rDataControl.isActive()) {
      status = pClient->getStatus();
      if(Close == status){
          break;
      }
      
      if(0 == pSendQueue->getHead()) {
        pthread_mutex_lock(&rDataControl.getMutex());
        pthread_cond_wait(&rDataControl.getCond(), &rDataControl.getMutex());
        pthread_mutex_unlock(&rDataControl.getMutex());
        continue;
      }

      pMessage = (Message*) pSendQueue->get();
      if (pMessage){
          pMessage->sendMessage();
          delete pMessage;
          pMessage = 0;
      }
  }

  //delete all message when exit
  while (G::m_bSysStop) {
      if(0 == pSendQueue->getHead()) {
          break;
      }
      pMessage = (Message*) pSendQueue->get();
      if (pMessage) {  
          delete pMessage;
          pMessage = 0;
      }
  }
  
  pClient->decThreadNum();
  LOG4CXX_INFO(rootLogger, "("<<hostIP<<")Exit msgSendThread, threadNum="<<--G::m_threadNum);
  return;
}


void ZxClient::incTotalDRMsgs(){
    m_nTotalDRs++;
}

void ZxClient::incDelayDRMsgs(){
    m_nDelayDRs++;
}

void ZxClient::incIAMDRs(){
    m_nIAM++;
}


void ZxClient::setMaxInnerDelay(int value){
    m_nMaxInnerDelay = MAX(m_nMaxInnerDelay,value);
}


void ZxClient::setMaxOuterDelay(int value){
    m_nMaxOuterDelay = MAX(m_nMaxOuterDelay,value);
}

void ZxClient::checkRecvMsg(){
    long i = m_nTotalDRs - m_nLastTotalDRs;
    m_nLastTotalDRs = m_nTotalDRs;

    long j = m_nDelayDRs - m_nLastDelayDRs;
    m_nLastDelayDRs = m_nDelayDRs;

    long k = m_nIAM - m_nLastIAM;
    m_nLastIAM = m_nIAM;

    long now = 0;
    time(&now);
    if(i>0){
        LOG4CXX_DEBUG(G::logger, "From "<<m_zxHostIP<<": total="<<i<<"/s(IAM="<<k<<"/s), now="<<now);
       /* if(j>0){
            LOG4CXX_WARN(G::logger, "From "<<m_zxHostIP<<": total="<<i<<"/s(IAM="<<k<<"/s, Closure="<<l<<"/s), delay="<<j<<"/s, now="<<now);
        }else{
            LOG4CXX_DEBUG(G::logger, "From "<<m_zxHostIP<<": total="<<i<<"/s(IAM="<<k<<"/s, Closure="<<l<<"/s), delay="<<j<<"/s, now="<<now);
        }*/
    }

    //check delay every 1s
    if(j>0 && i>0){
        float per = (float)j*100/i;
        LoggerPtr rootLogger = Logger::getRootLogger();
        LOG4CXX_WARN(rootLogger, "From "<<m_zxHostIP<<": delay="<<j<<"/"<<i<<"("<<per<<"%), maxInnerDelay="<<m_nMaxInnerDelay<<"s, maxOuterDelay="<<m_nMaxOuterDelay<<"s, now="<<now);

        if(0 == m_nDelayCount){
            m_startDelayTime = now;
        }
        m_lastDelayTime = now;
        m_nDelayCount++;
        if(per>10){
            if(!m_bDelayAlert){
                sendDelayAlert();
            }
        }else{
            if(m_nDelayCount>=5){
                if(!m_bDelayAlert){
                    if(m_lastDelayTime-m_startDelayTime <= 60){
                        sendDelayAlert();
                    }else{
                        m_nDelayCount = 1;   
                        m_startDelayTime = now;
                    }
                }
            }
        }
    }

    if(m_bDelayAlert){
        if(now-m_lastDelayTime > 120){
            m_nDelayCount = 0;
            clearDelayAlert();
        }
    }
    
    m_nMaxInnerDelay = 0;
    m_nMaxOuterDelay = 0;
    return;
}


void ZxClient::sendDelayAlert(){
    m_bDelayAlert = true;
    string ss("From ");
    ss += m_zxHostIP;
    ss += ": delay!";

    string errMsg("(");
    errMsg += m_zxHostIP;
    errMsg += ")Delay";
    int isErr = 1;    
    AppMonitor::getInstance()->alert(G::m_appName.c_str(), errMsg.c_str(), isErr, ss.c_str(),8);
    return;
}


void ZxClient::clearDelayAlert(){
    m_bDelayAlert = false;
    string ss("From");
    ss += m_zxHostIP;
    ss += ": no delay!";
    
    string errMsg("(");
    errMsg += m_zxHostIP;
    errMsg += ")Delay";
    int isErr = 0;    
    AppMonitor::getInstance()->alert(G::m_appName.c_str(), errMsg.c_str(), isErr, ss.c_str(),8);
    return;
}

//end of zcxcclient.cpp

