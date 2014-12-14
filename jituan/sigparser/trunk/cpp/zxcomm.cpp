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
MSGLIST ZxComm::m_closureList;
pthread_mutex_t ZxComm::m_closureListMutex = PTHREAD_MUTEX_INITIALIZER;

int ZxComm::m_msgProcessNum = 0;

string ZxComm::m_sysID = "";
string ZxComm::m_sysPwd = "";
string ZxComm::m_subscrbLoginID = "";
string ZxComm::m_subscrbPwd = "";

ULONGLONG ZxComm::m_CDROrderID = 0; 
UINT ZxComm::m_version = 0;
UINT ZxComm::m_subVersion = 0;

ULONG ZxComm::m_nTotalDRs = 0;
ULONG ZxComm::m_nDelayDRs = 0;
ULONG ZxComm::m_nRepeatDRs = 0;
ULONG ZxComm::m_nVpnDRs = 0;
ULONG ZxComm::m_nFmlDRs = 0;

//bool ZxComm::m_bDelayAlert = false;
//UINT ZxComm::m_nMaxOuterDelay = 0;
//UINT ZxComm::m_nMaxInnerDelay = 0;
int ZxComm::m_msgDelayLimit = 5;
int ZxComm::m_beginEndInterval = 1800;
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

    if (!m_closureList.empty()) {
      m_closureList.clear();
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
  bool bSend = false;
  int ret = 0;
  UINT status = 0;

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

    bSend = false;
    status = 0;
    pMessage = (CDRMessage*) gWaitQueue.get();
    if (pMessage) { 
        status = pMessage->getStatus();
        ret = ZxComm::ifDuplicateCDR(pMessage);
        if(1 == ret){
            //duplicate message
            delete pMessage;
            pMessage = 0;
        }else{
            //send now
            bSend = true;
        }

        if(bSend){
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
        ZxComm::checkClosureList();

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

/*
//check if msg delay every 1s
void ZxComm::ifDRMsgDelay(){
    static ULONG lastTotalDRs = 0;
    static ULONG lastDelayDRs = 0;
    static ULONG lastVpnDRs = 0;
    static ULONG lastFmlDRs = 0;
    static ULONG startDelayTime = 0;
    static ULONG lastDelayTime = 0;
    static ULONG delayCount = 0;
    
    long i = m_nTotalDRs - lastTotalDRs;
    lastTotalDRs = m_nTotalDRs;

    long j = m_nDelayDRs - lastDelayDRs;
    lastDelayDRs = m_nDelayDRs;

    long m = m_nVpnDRs - lastVpnDRs;
    lastVpnDRs = m_nVpnDRs;

    long n = m_nFmlDRs - lastFmlDRs;
    lastFmlDRs = m_nFmlDRs;

    long now = 0;
    time(&now);
    if(i>0){
        LoggerPtr rootLogger = Logger::getRootLogger();
        LOG4CXX_DEBUG(G::logger, "total="<<i<<"/s, delay="<<j<<"/s, now="<<now);
        float per1 = (float)m*100/i;
        float per2 = (float)n*100/i;
        LOG4CXX_DEBUG(G::logger, "VPN="<<per1<<"%, Family="<<per2<<"%");
    }
    
    if(j>0 && i>0){
        float per = (float)j*100/i;
        LoggerPtr rootLogger = Logger::getRootLogger();
        LOG4CXX_WARN(rootLogger, "delay="<<j<<"/"<<i<<"("<<per<<"%), maxInnerDelay="<<m_nMaxInnerDelay<<"s, maxOuterDelay="<<m_nMaxOuterDelay<<"s, now="<<now);

        if(0 == delayCount){
            startDelayTime = now;
        }
        lastDelayTime = now;
        delayCount++;
        if(per>10){
            if(!m_bDelayAlert){
                sendDelayAlert();
            }
        }else{
            if(delayCount>=5){
                if(!m_bDelayAlert){
                    if(lastDelayTime-startDelayTime <= 60){
                        sendDelayAlert();
                    }else{
                        delayCount = 1;   
                        startDelayTime = now;
                    }
                }
            }
        }
    }

    if(m_bDelayAlert){
        if(now-lastDelayTime > 120){
            delayCount = 0;
            clearDelayAlert();
        }
    }
    
    m_nMaxInnerDelay = 0;
    m_nMaxOuterDelay = 0;
    return;
}


void ZxComm::sendDelayAlert(){
    m_bDelayAlert = true;
    string ss("(");
    ss += Comm::m_listenHostIP;
    ss += ") delay!";
    
    string errMsg("zxDelay");
    int isErr = 1;    
    AppMonitor::getInstance()->alert(G::m_appName.c_str(), errMsg.c_str(), isErr, ss.c_str(),8);
    return;
}


void ZxComm::clearDelayAlert(){
    m_bDelayAlert = false;
    string ss("(");
    ss += Comm::m_listenHostIP;
    ss += ") no delay!";
    
    string errMsg("zxDelay");
    int isErr = 0;    
    AppMonitor::getInstance()->alert(G::m_appName.c_str(), errMsg.c_str(), isErr, ss.c_str(),8);
    return;
}
*/


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
   // key += "_";
   // key += G::int2String(status);
    
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
                <<"), new(packID="<<pMessage->getPackID()<<", status="<<pMessage->getStatus()<<", "<<caller<<"->"<<called<<")");
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


//return 1: find matched IAM CDR
//return 0: can't find matched IAM CDR
//return -1: error
int ZxComm::findMatchedIAM(CDRMessage *pMessage){
    MSGMAP::iterator iter;
    CDRMessage *matchedMessage = 0;
    char* seqNum = (char*)pMessage->getSeqNum();
    int ret = 0;
    
    if (pthread_mutex_lock(&m_cdrMapMutex)) 
    {
        LoggerPtr rootLogger = Logger::getRootLogger(); 
        LOG4CXX_ERROR(rootLogger, "pthread_mutex_lock fail while findMatchedIAM!");
        return 0;
    } 

    string key = string(seqNum);
    key += "_";   
    key += G::int2String(IAM);
    
    iter = m_cdrMap.find(key);  
    if (iter != m_cdrMap.end())
    {
        matchedMessage = iter->second;  
        if(matchedMessage){
            ret = pMessage->setMatchedCall(matchedMessage);   
            if(1 == ret){
                matchedMessage->setMatched(true);
            }
        }else{
            m_cdrMap.erase(iter);
            ret = -1;
        }
    }else{
        ret = 0;
    }
    
    pthread_mutex_unlock(&m_cdrMapMutex); 
    return ret;
}


//return 1: duplicate message
//return 0: not duplicate message
int ZxComm::ifDuplicateCDR(CDRMessage *pMessage){
    int ret = 0;
    static int num = 0;
    MSGMAP::iterator iter;
    CDRMessage *matchedMessage = 0;
    long diffStart = 0;
    long diffEnd = 0;
    long now = 0;
    char* caller = (char*)pMessage->getCaller();
    char* called = (char*)pMessage->getCalled();
    ULONG startTime = pMessage->getStartTime();
   // ULONG endTime = pMessage->getEndTime();
    UINT status = pMessage->getStatus();
    char* seqNum = (char*)pMessage->getSeqNum();
    
    if (pthread_mutex_lock(&m_cdrMapMutex)) 
    {
        LoggerPtr rootLogger = Logger::getRootLogger(); 
        LOG4CXX_ERROR(rootLogger, "pthread_mutex_lock fail while ifDuplicateCDR!");
        return 0;
    } 

    string key = string(seqNum);
  //  key += "_";   
  //  key += G::int2String(status);
    
    iter = m_cdrMap.find(key);  
    if (iter != m_cdrMap.end())
    {
        matchedMessage = iter->second;  
        if(matchedMessage)
        {
            char* matchedCaller = (char*)matchedMessage->getCaller();
            char* matchedCalled = (char*)matchedMessage->getCalled();
            diffStart = startTime-(matchedMessage->getStartTime()); 
          //  diffEnd = endTime - (matchedMessage->getEndTime());
            time(&now);
            if((0 == strcmp(caller, matchedCaller)) && (0 == strcmp(called, matchedCalled)))
            {
                // duplicate call
                LoggerPtr rootLogger = Logger::getRootLogger();
                LOG4CXX_WARN(rootLogger, ++m_nRepeatDRs<<": same call, same key="<<key<<", diffStart="<<diffStart<<", "<<caller<<"->"<<called
                    <<", old(packID="<<matchedMessage->getPackID()<<", status="<<matchedMessage->getStatus()
                    <<"), new(packID="<<pMessage->getPackID()<<", status="<<pMessage->getStatus()<<")");
            }else{
                // different call
                LoggerPtr rootLogger = Logger::getRootLogger();
                LOG4CXX_ERROR(rootLogger, ++num<<": different call, same key="<<key<<", diffStart="<<diffStart
                    <<", old("<<matchedCaller<<"->"<<matchedCalled<<", packID="<<matchedMessage->getPackID()<<", status="<<matchedMessage->getStatus()
                    <<"), new("<<caller<<"->"<<called<<", packID="<<pMessage->getPackID()<<", status="<<pMessage->getStatus()<<")");
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


int ZxComm::insert2ClosureList(CDRMessage* pMessage){ 
    MSGLIST::iterator iter; 
    if (pthread_mutex_lock(&m_closureListMutex)) 
    {
        LoggerPtr rootLogger = Logger::getRootLogger();
        LOG4CXX_ERROR(rootLogger, "pthread_mutex_lock fail while insert2ClosureList!");
        return -1;
    } 

    m_closureList.push_back(pMessage);
    pthread_mutex_unlock(&m_closureListMutex);  
    
    return 0;
}


void ZxComm::checkClosureList(){
    if (m_closureList.empty()) {
      return;
    }

    int nDismatched = 0;
    CDRMessage* pMessage = 0;
    int ret = 0;
    long now = 0;
    ULONG cdrTime = 0;    
    PUCHAR pCaller = 0;
    PUCHAR pCalled = 0;
    
    time(&now);
    if (pthread_mutex_lock(&m_closureListMutex)) 
    {
        LoggerPtr rootLogger = Logger::getRootLogger(); 
        LOG4CXX_ERROR(rootLogger, "pthread_mutex_lock fail while checkClosureList!");
        return;
    } 

    MSGLIST::iterator iter = m_closureList.begin();
    for (; iter!=m_closureList.end(); ) {
        pMessage = *iter;
        if(pMessage){
            ret = findMatchedIAM(pMessage);
            if(1 == ret){
                //matched
                iter = m_closureList.erase(iter);
                ret = insert2CDRMap(pMessage);
                if(0 == ret){
                    gSendSugwQueue.put(pMessage);
                }else{
                    //insert failed
                    LoggerPtr rootLogger = Logger::getRootLogger();
                    LOG4CXX_ERROR(rootLogger, "insert2CDRMap failed: type="<<pMessage->getProtocolType()<<", status="<<pMessage->getStatus()<<", seqNum="<<pMessage->getSeqNum());
                    delete pMessage;
                    pMessage = 0;
                }
            }else if(0 == ret){
                //dismatched
                cdrTime = pMessage->getEndTime();
                pCaller = pMessage->getCaller();
                pCalled = pMessage->getCalled();
                if((now-cdrTime) > m_msgDelayLimit){
                    if(G::isTestPhone((char*)pCaller) || G::isTestPhone((char*)pCalled)){
                        LoggerPtr rootLogger = Logger::getRootLogger();  
                        LOG4CXX_DEBUG(rootLogger, "checkClosureList: dismatched Closure, "<<pCaller<<"->"<<pCalled<<", type="<<pMessage->getProtocolType()
                           <<", status="<<pMessage->getStatus()<<", CDRID="<<pMessage->getCDRID()<<", end="<<cdrTime<<", seqNum="<<pMessage->getSeqNum()<<", now="<<now);
                    }
                    
                    iter = m_closureList.erase(iter);
                    delete pMessage;
                    pMessage = 0;
                    nDismatched++;
                }else{
                    iter++;
                }
            }else{
                iter = m_closureList.erase(iter);
                delete pMessage;
                pMessage = 0;
                nDismatched++;
            }
        }else{
            iter = m_closureList.erase(iter);
        }
    }    
   
    pthread_mutex_unlock(&m_closureListMutex);   

    if(nDismatched>0){
        LoggerPtr rootLogger = Logger::getRootLogger();
        LOG4CXX_DEBUG(rootLogger, nDismatched<<" dismatched Closure CDRs are timeout(>"<<m_msgDelayLimit<<"s)!");
    }
    return;    
}


void ZxComm::clearClosureList(){
  CDRMessage* pMessage = 0;
  if (m_closureList.empty()) {
    return;
  }
  
  if (pthread_mutex_lock(&m_closureListMutex)){
     LoggerPtr rootLogger = Logger::getRootLogger();
     LOG4CXX_ERROR(rootLogger, "pthread_mutex_lock fail while clearClosureList!");
    return;
  }  
  
  LoggerPtr rootLogger = Logger::getRootLogger();
  LOG4CXX_INFO(rootLogger, "clear m_closureList size="<<m_closureList.size());
  
  MSGLIST::iterator itr = m_closureList.begin();
  for (; itr!=m_closureList.end(); ) {
      pMessage = *itr;
      itr = m_closureList.erase(itr);
      if (pMessage) {
          delete pMessage;
          pMessage = 0;
      }
  }        

  pthread_mutex_unlock(&m_closureListMutex);
}


//end of zxcomm.cpp


