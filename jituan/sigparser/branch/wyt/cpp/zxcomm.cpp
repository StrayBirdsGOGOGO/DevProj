#include "hwsshare.h"
#include "cyclicqueue.h"
#include "g.h"
#include "zxcomm.h"
#include "comm.h"
#include "appmonitor.h"
#include "sugwserver.h"
#include "zxclient.h"
#include "cdrmessage.h"
#include "hwsqueue.h"


extern HwsQueue gWaitQueue;
extern HwsQueue gSendSugwQueue;

static pthread_t tidZxDataListen;
static pthread_t tidZxCommCheck;
static pthread_t tidMsgCheck;
static pthread_t tidMsgWait;

int ZxComm::m_listenDataPort = 9710;
int ZxComm::m_nConnectedZxClients = 0;
pthread_mutex_t ZxComm::m_zxMapMutex = PTHREAD_MUTEX_INITIALIZER;
ZXMAP ZxComm::m_zxMap;

MSGMAP ZxComm::m_cdrMap;
pthread_mutex_t ZxComm::m_cdrMapMutex = PTHREAD_MUTEX_INITIALIZER;

int ZxComm::m_msgProcessNum = 0;

ULONG ZxComm::m_nTotalDRs = 0;
ULONG ZxComm::m_nDelayDRs = 0;
ULONG ZxComm::m_nRepeatDRs = 0;
ULONG ZxComm::m_nVpnDRs = 0;
ULONG ZxComm::m_nFmlDRs = 0;

int ZxComm::m_msgDelayLimit = 5;
int ZxComm::m_maxHeartInterval = 15;
ULONG ZxComm::m_sendNum = 0;

void zxCommCheckThread(void);
void listenZxDataThread(void);
void msgWaitThread(void);
void msgCheckThread(void);



ZxComm::ZxComm() {
}

ZxComm::~ZxComm() {
}


/*
return 0:  success
return -1: failed
*/
int ZxComm::startCommunication(){
    if (!m_cdrMap.empty()) {
      m_cdrMap.clear();
    }

    if (pthread_create(&tidMsgWait, 0, (void *(*)(void*)) &msgWaitThread, 0)) {
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_ERROR(rootLogger,"create msgWaitThread failed!");
        return -1;
    }
    
    if (pthread_create(&tidMsgCheck, 0, (void *(*)(void*)) &msgCheckThread, 0)) {
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_ERROR(rootLogger,"create msgCheckThread failed!");
        return -1;
    }
    
    if (pthread_create(&tidZxCommCheck, 0, (void *(*)(void*))&zxCommCheckThread, 0)){
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_ERROR(rootLogger,"create zxCommCheckThread failed!");
        return -1;
    }
    
    int ret = initZxDataSocket();      
    return ret;
}


int ZxComm::initZxDataSocket(){
    if (pthread_create(&tidZxDataListen, 0, (void *(*)(void*))&listenZxDataThread, 0)){
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_ERROR(rootLogger,"create listenZxDataThread failed!");
        return -1;
    }
    
    return 0;
}


void listenZxDataThread(void)
{
  int listenSockt = 0;
  int yes = 1;

  // init socket to receive message from AppServer
  LoggerPtr rootLogger = Logger::getRootLogger();
  pthread_t tid = syscall(SYS_gettid);
  LOG4CXX_INFO(rootLogger, "listenZxDataThread tid=" << tid<<", threadNum="<<++G::m_threadNum);
  
  if( (listenSockt = socket(AF_INET, SOCK_STREAM, 0) ) == -1 )
  {
    LOG4CXX_ERROR(rootLogger, "Receive socket() error:"<<strerror(errno));
    G::m_bSysStop = true;
    G::m_threadNum--;
    return;
  }
  
  if( setsockopt(listenSockt, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1 )
  {
    LOG4CXX_ERROR(rootLogger, "Setsockopt error:"<<strerror(errno));
    close(listenSockt);
    G::m_bSysStop = true;
    G::m_threadNum--;
    return;
  }

  sockaddr_in myAddr,clientAddr;
  myAddr.sin_family = AF_INET;
  myAddr.sin_port = htons(ZxComm::m_listenDataPort);
  //myAddr.sin_addr.s_addr = INADDR_ANY;
  myAddr.sin_addr.s_addr = inet_addr(Comm::m_listenHostIP.c_str());
  bzero( &(myAddr.sin_zero), 8);

  if( bind(listenSockt, (sockaddr*)&myAddr, sizeof(sockaddr)) == -1 )
  {
    LOG4CXX_ERROR(rootLogger, "Bind() error:"<<strerror(errno));
    close(listenSockt);
    G::m_bSysStop = true;
    G::m_threadNum--;
    return;
  }
  else
  {
    LOG4CXX_INFO(rootLogger, "Bind() on zxDataPort("<<ZxComm::m_listenDataPort<<")");
  }
  
  if( listen(listenSockt, 10) == -1 )
  {
    LOG4CXX_ERROR(rootLogger, "Listen()  error="<<errno);
    close(listenSockt);
    G::m_bSysStop = true;
    G::m_threadNum--;
    return;
  }
  
  fcntl(listenSockt, F_SETFL, O_NONBLOCK);
  LOG4CXX_INFO(rootLogger, "Start listen on zxDataPort("<<ZxComm::m_listenDataPort<<")...");
  
  int sin_size = sizeof(sockaddr_in);
  int newFd = 0;
  
  while(!G::m_bSysStop)
  {
    if((0 == G::m_isDebug) && (SugwServer::m_sugwSocketNum<1)){
        sleep(1);
        continue;            
    }
    
    newFd = accept(listenSockt, (sockaddr*)&clientAddr, (socklen_t *)&sin_size);
    if( -1 == newFd)
    {
        //LOG4CXX_ERROR(rootLogger, "accept  error:"<<strerror(errno));
        sleep(1);
        continue; 
    }
    else
    {
        fcntl(newFd, F_SETFL, O_NONBLOCK);  
        int port = ntohs(clientAddr.sin_port);
        char* hostIP = inet_ntoa(clientAddr.sin_addr);
        ZxComm::addZxClient(newFd,port,hostIP);
    }
  }
  close(listenSockt);
  LOG4CXX_INFO(rootLogger, "Exit from listenZxDataThread, threadNum="<<--G::m_threadNum);
  return;
}


int ZxComm::addZxClient(int newFd, int port, char* hostIP)
{
  if (newFd <1)
  {
    return -1;
  }
  
  LoggerPtr rootLogger = Logger::getRootLogger();
  LOG4CXX_INFO(rootLogger, "Add zxClient: fd="<<newFd<<", name="<<hostIP<<":"<<port); 

  ZxClient *pClient = 0;  
  pClient = new ZxClient();
  if(!pClient){
      LoggerPtr rootLogger = Logger::getRootLogger();
      LOG4CXX_ERROR(rootLogger, "new ZxClient failed!");
      Comm::closeSocket(newFd);
      return -1;
  }

  if (pthread_mutex_lock(&m_zxMapMutex))
  {
     LOG4CXX_ERROR(rootLogger, "pthread_mutex_lock failed while addZxClient!"); 
     Comm::closeSocket(newFd);
     delete pClient;
     pClient = 0;
     return -1;
  }
  
  m_zxMap.insert(std::pair<int,ZxClient*>(newFd,pClient));
  pthread_mutex_unlock(&m_zxMapMutex);  

  int ret = pClient->init(newFd, port, hostIP);
  if(-1 == ret){
      pClient->beforeDel();
      return -1;
  }
  
  return 0;
}


string ZxComm::getZxClientsInfo(){
    ZxClient* pClient = 0;
    string info = "";
    int port = 0;
    
    if(!m_zxMap.empty()){
        if (pthread_mutex_lock(&m_zxMapMutex)) {
          LoggerPtr rootLogger = Logger::getRootLogger();
          LOG4CXX_ERROR(rootLogger, "pthread_mutex_lock fail while getZxClientsInfo!");
          return info;
        }

        ZXMAP::iterator itr = m_zxMap.begin();
        for (; itr!=m_zxMap.end(); itr++) {
            pClient = itr->second;
            if (pClient) {
                port = pClient->getPort();
                info += pClient->getHostIP();
                info += ":";
                info += G::int2String(port);
                info += " ";
            }
        }        
        pthread_mutex_unlock(&m_zxMapMutex);        
    }

    return info;
}


void ZxComm::clearZxClients(){
    ZxClient* pClient = 0;
    if(!m_zxMap.empty()){
        if (pthread_mutex_lock(&m_zxMapMutex)) {
          LoggerPtr rootLogger = Logger::getRootLogger();
          LOG4CXX_ERROR(rootLogger, "pthread_mutex_lock fail while clearZxClients!");
          return;
        }

        ZXMAP::iterator tmpItr;
        ZXMAP::iterator itr = m_zxMap.begin();
        for (; itr!=m_zxMap.end(); ) {
            tmpItr = itr;
            pClient = itr->second;
            if (pClient) {
                delete pClient;
                pClient = 0;
            }

            tmpItr++;
            m_zxMap.erase(itr);
            itr = tmpItr;
        }        
        pthread_mutex_unlock(&m_zxMapMutex);        
    }

    return;
}


void ZxComm::closeZxComm(){
    ZxClient* pClient = 0;
    if(!m_zxMap.empty()){
        if (pthread_mutex_lock(&m_zxMapMutex)) {
          LoggerPtr rootLogger = Logger::getRootLogger();
          LOG4CXX_ERROR(rootLogger, "pthread_mutex_lock fail while closeZxComm!");
          return;
        }
        
        ZXMAP::iterator itr = m_zxMap.begin();
        for (; itr!=m_zxMap.end(); itr++) {
            pClient = itr->second;
            if (pClient) {
                pClient->beforeDel();
            }     
        }
        pthread_mutex_unlock(&m_zxMapMutex);        
    }

    return;
}


void ZxComm::checkZxClients(){
    int ret = 0;
    int fd = 0;
    ZxClient *pClient = 0;
    ZXMAP::iterator itr;
    ZXMAP::iterator tmpItr;

    if(m_zxMap.empty()){
        return;
    }

    if (pthread_mutex_lock(&m_zxMapMutex)) {
      LoggerPtr rootLogger = Logger::getRootLogger();
      LOG4CXX_ERROR(rootLogger, "pthread_mutex_lock fail while checkZxClients!");
      return;
    }
    
    itr = m_zxMap.begin();
    for (; itr != m_zxMap.end(); ){
        pClient = 0;
        fd = 0;
        tmpItr = itr;
        fd = itr->first;
        pClient = itr->second;
        if(pClient){
            ret = pClient->checkComm();
            if(-1 == ret){
                tmpItr++;
                m_zxMap.erase(itr);
                itr = tmpItr;
                delete pClient;
                pClient = 0;
            }else{
                itr++;
            }
        }else{
            tmpItr++;
            m_zxMap.erase(itr);
            itr = tmpItr;
            Comm::closeSocket(fd);
        }
    }
    pthread_mutex_unlock(&m_zxMapMutex);
    
    return;
}


void zxCommCheckThread(){
    int i = 0;
    pthread_t tid = syscall(SYS_gettid);
    LoggerPtr rootLogger = Logger::getRootLogger();
    LOG4CXX_INFO(rootLogger, "zxCommCheckThread tid="<<tid<<", threadNum="<<++G::m_threadNum);
    
    while(!G::m_bSysStop){
        sleep(1);       
        if(0 == G::m_isDebug){
            if(!SugwServer::ifSktNormal() && ZxComm::m_nConnectedZxClients>0){
                LoggerPtr rootLogger = Logger::getRootLogger();  
                LOG4CXX_WARN(rootLogger, "No sugw client, now close communication with ZX!");
                ZxComm::closeZxComm();        
            }
        }

        if(++i == 5){
            ZxComm::checkZxClients(); 
            i = 0;
        }
    }

    LOG4CXX_INFO(rootLogger, "Exit from zxCommCheckThread, threadNum="<<--G::m_threadNum);
    return;    
}


//message wait process thread
void msgWaitThread(void) {
  CDRMessage* pMessage = 0;
  int ret = 0;

  LoggerPtr rootLogger = Logger::getRootLogger();
  pthread_t tid = syscall(SYS_gettid);
  LOG4CXX_INFO(rootLogger, "msgWaitThread tid=" << tid<<", threadNum="<<++G::m_threadNum);
  
  DataControl& rDataControl = gWaitQueue.getDataControl();
  rDataControl.activate();
  while (!G::m_bSysStop && rDataControl.isActive()) {
    if(0 == gWaitQueue.getHead()) {
      pthread_mutex_lock(&rDataControl.getMutex());
      pthread_cond_wait(&rDataControl.getCond(), &rDataControl.getMutex());
      pthread_mutex_unlock(&rDataControl.getMutex());
      continue;
    }

    pMessage = (CDRMessage*) gWaitQueue.get();
    if (pMessage) { 
        ret = ZxComm::ifDuplicateCDR(pMessage);
        if(1 == ret){
            //duplicate message
            delete pMessage;
            pMessage = 0;
        }else{
            //insert CDR
            ret = ZxComm::insert2CDRMap(pMessage);
            if(-1 == ret){
                //insert failed
                LoggerPtr rootLogger = Logger::getRootLogger();
                LOG4CXX_ERROR(rootLogger, "insertCDR2Map failed: "<<pMessage->getCaller()<<"->"<<pMessage->getCalled()
                    <<", status="<<pMessage->getStatus()<<", seqNum="<<pMessage->getSeqNum());
                delete pMessage;
                pMessage = 0;
            }else{
                //send 
                ULONG num = (++ZxComm::m_sendNum)%2000000000;
                pMessage->setCallID(num);
                gSendSugwQueue.put(pMessage); 
            }
        }
    }
    //usleep(300);
  }

  //delete all message when exit
  while (G::m_bSysStop) {
    if(0 == gWaitQueue.getHead()) {
        break;
    }
    pMessage = (CDRMessage*) gWaitQueue.get();
    if (pMessage) {  
        delete pMessage;
        pMessage = 0;
    }
  }
  
  LOG4CXX_INFO(rootLogger, "Exit msgWaitThread, threadNum="<<--G::m_threadNum);
}


void msgCheckThread(void){
    LoggerPtr rootLogger = Logger::getRootLogger();
    pthread_t tid = syscall(SYS_gettid);
    LOG4CXX_INFO(rootLogger, "msgCheckThread tid=" << tid<<", threadNum="<<++G::m_threadNum);
    
    
    while (!G::m_bSysStop) {   
        sleep(1);
        //count msgs receiving from zx  every second, and check if msg delay
        ZxComm::checkRecvMsg();
        
        //count msgs sending to sugw every second
        SugwServer::sendMsgCount();

        //check cdrMap
        ZxComm::checkCDR();
    }
    
    LOG4CXX_INFO(rootLogger, "Exit msgCheckThread, threadNum="<<--G::m_threadNum);  
    return;
}


void ZxComm::checkRecvMsg(){
    ZxClient* pClient = 0;
    if(!m_zxMap.empty()){
        if (pthread_mutex_lock(&m_zxMapMutex)) {
          LoggerPtr rootLogger = Logger::getRootLogger();
          LOG4CXX_ERROR(rootLogger, "pthread_mutex_lock fail while checkRecvMsg!");
          return;
        }
        
        ZXMAP::iterator itr = m_zxMap.begin();
        for (; itr!=m_zxMap.end(); itr++) {
            pClient = itr->second;
            if (pClient) {
                pClient->checkRecvMsg();
            }
        }        
        pthread_mutex_unlock(&m_zxMapMutex);        
    }

    return;
}

int ZxComm::insert2CDRMap(CDRMessage* pMessage){ 
    CDRMessage* oldMessage = 0;
    long diffTime = 0;
    long now = 0;
    char* caller = (char*)pMessage->getCaller();
    char* called = (char*)pMessage->getCalled();
    ULONG startTime = pMessage->getStartTime();
    char* seqNum = (char*)pMessage->getSeqNum();
    UINT status = pMessage->getStatus();
    
    MSGMAP::iterator iter; 
    if (pthread_mutex_lock(&m_cdrMapMutex)) 
    {
        LoggerPtr rootLogger = Logger::getRootLogger();
        LOG4CXX_ERROR(rootLogger, "pthread_mutex_lock fail while insert2CDRMap!");
        return -1;
    } 

    string key = string(seqNum);
    
    iter = m_cdrMap.find(key);
    if (iter != m_cdrMap.end())
    {
        oldMessage = iter->second;
        m_cdrMap.erase(iter);
        if(oldMessage)
        {
            time(&now);
            diffTime = startTime - oldMessage->getStartTime();
            LoggerPtr rootLogger = Logger::getRootLogger();
            LOG4CXX_ERROR(rootLogger, ++m_nRepeatDRs<<": same key="<<key<<", diff="<<diffTime
                <<", old(packID="<<oldMessage->getPackID()<<", status="<<oldMessage->getStatus()<<", "<<oldMessage->getCaller()<<"->"<<oldMessage->getCalled()
                <<"), new(packID="<<pMessage->getPackID()<<", status="<<status<<", "<<caller<<"->"<<called<<")");
            delete oldMessage;
            oldMessage = 0;
        }
    } 

    m_cdrMap.insert(pair<string, CDRMessage*> (key, pMessage));
    pthread_mutex_unlock(&m_cdrMapMutex);  
    
    if(G::isTestPhone(caller) || G::isTestPhone(called))
    {
        LoggerPtr rootLogger = Logger::getRootLogger();
        LOG4CXX_DEBUG(rootLogger, "insert2CDRMap: key="<<key<<", "<<caller<<"->"<<called<<", status="<<pMessage->getStatus());
    }
    return 0;
}


//return 1: duplicate message
//return 0: not duplicate message
int ZxComm::ifDuplicateCDR(CDRMessage *pMessage){
    int ret = 0;
    static int num = 0;
    MSGMAP::iterator iter;
    CDRMessage *matchedMessage = 0;
    long diffStart = 0;
    long now = 0;
    char* caller = (char*)pMessage->getCaller();
    char* called = (char*)pMessage->getCalled();
    ULONG startTime = pMessage->getStartTime();
    UINT status = pMessage->getStatus();
    char* seqNum = (char*)pMessage->getSeqNum();
    
    if (pthread_mutex_lock(&m_cdrMapMutex)) 
    {
        LoggerPtr rootLogger = Logger::getRootLogger(); 
        LOG4CXX_ERROR(rootLogger, "pthread_mutex_lock fail while ifDuplicateCDR!");
        return 0;
    } 

    string key = string(seqNum);
    
    iter = m_cdrMap.find(key);  
    if (iter != m_cdrMap.end())
    {
        matchedMessage = iter->second;  
        if(matchedMessage)
        {
            char* matchedCaller = (char*)matchedMessage->getCaller();
            char* matchedCalled = (char*)matchedMessage->getCalled();
            diffStart = startTime-(matchedMessage->getStartTime()); 
            time(&now);
            if((0 == strcmp(caller, matchedCaller)) && (0 == strcmp(called, matchedCalled)))
            {
                // duplicate call
                LoggerPtr rootLogger = Logger::getRootLogger();
                LOG4CXX_WARN(rootLogger, ++m_nRepeatDRs<<": same call, same key="<<key<<", diffStart="<<diffStart<<", "<<caller<<"->"<<called
                    <<", old(packID="<<matchedMessage->getPackID()<<", status="<<matchedMessage->getStatus()
                    <<"), new(packID="<<pMessage->getPackID()<<", status="<<status<<")");
            }else{
                // different call
                LoggerPtr rootLogger = Logger::getRootLogger();
                LOG4CXX_ERROR(rootLogger, ++num<<": different call, same key="<<key<<", diffStart="<<diffStart
                    <<", old("<<matchedCaller<<"->"<<matchedCalled<<", packID="<<matchedMessage->getPackID()<<", status="<<matchedMessage->getStatus()
                    <<"), new("<<caller<<"->"<<called<<", packID="<<pMessage->getPackID()<<", status="<<status<<")");
            }
            ret = 1;
        }
    }
    
    pthread_mutex_unlock(&m_cdrMapMutex);   
    return ret;    
}


void ZxComm::checkCDR(){
  MSGMAP::iterator iter; 
  MSGMAP::iterator tmpIter; 
  CDRMessage* pMessage = 0;
  long now = 0;
  UINT status = 0;
  PUCHAR seqNum = 0;
  ULONG cdrTime = 0;
  PUCHAR caller = 0;
  PUCHAR called = 0;
  bool bDel = false;  
  
  if (m_cdrMap.empty()) {
    return;
  }
  
  if (pthread_mutex_lock(&m_cdrMapMutex)) {
    LoggerPtr rootLogger = Logger::getRootLogger();
    LOG4CXX_ERROR(rootLogger, "pthread_mutex_lock fail while checkCDR!");
    return;
  }  
  
  time(&now);
  for (iter = m_cdrMap.begin(); iter != m_cdrMap.end(); ) {
      bDel = false;
      tmpIter = iter;
      pMessage = iter->second;
      if(pMessage){
          status = pMessage->getStatus();
          caller = pMessage->getCaller();
          called = pMessage->getCalled();
          seqNum = pMessage->getSeqNum();
          cdrTime = pMessage->getStartTime();
          if(pMessage->ifSend()){
            if(now - cdrTime > m_msgDelayLimit){
                bDel = true;
            }else{
                bDel = false;
            }
          }else{
              bDel = false;
              if(now - cdrTime > m_msgDelayLimit){
                  LoggerPtr rootLogger = Logger::getRootLogger();
                  LOG4CXX_WARN(rootLogger, "timeout CDR: unsend, status="<<status<<", "<<caller<<"->"<<called
                    <<", seqNum="<<seqNum<<", startTime="<<cdrTime<<", now="<<now);                  
              }
          }
      }else{
          bDel = true;
      }
      
      if(bDel){
          tmpIter++;
          m_cdrMap.erase(iter);
          iter = tmpIter;
          if(pMessage){
              delete pMessage;
              pMessage = 0;
          }
      }else{
          iter++;
      }
  }
  pthread_mutex_unlock(&m_cdrMapMutex);
  return;
}


void ZxComm::clearCDRMap(){
  MSGMAP::iterator iter; 
  MSGMAP::iterator tmpIter; 
  CDRMessage* pMessage = 0;

  if (m_cdrMap.empty()) {
    return;
  }
  
  if (pthread_mutex_lock(&m_cdrMapMutex)){
     LoggerPtr rootLogger = Logger::getRootLogger();
     LOG4CXX_ERROR(rootLogger, "pthread_mutex_lock fail while clearCDRMap");
    return;
  }  
  
  LoggerPtr rootLogger = Logger::getRootLogger();
  LOG4CXX_INFO(rootLogger, "clearCDRMap size="<<m_cdrMap.size());
  for (iter = m_cdrMap.begin(); iter != m_cdrMap.end(); ) {
      tmpIter = iter;
      pMessage = iter->second;
      tmpIter++;
      m_cdrMap.erase(iter);
      iter = tmpIter;
      if(pMessage){
          delete pMessage;
          pMessage = 0;
      }
   }

  pthread_mutex_unlock(&m_cdrMapMutex);
}

//end of zxcomm.cpp


