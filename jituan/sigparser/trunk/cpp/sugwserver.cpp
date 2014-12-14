#include "hwsshare.h"
#include "queue.h"
#include "cyclicqueue.h"
#include "hwsqueue.h"
#include "g.h"
#include "comm.h"
#include "message.h"
#include "cdrmessage.h"
#include "appmonitor.h"
#include "sugwserver.h"
#include "transmit.h"
#include "zxcomm.h"

extern HwsQueue gSendSugwQueue;

static pthread_t tidSktSend;
static pthread_t  tidSktRecv[20];
static pthread_t  tidListen;

int SugwServer::m_listenPort = 9700;

pthread_mutex_t sockListMutex = PTHREAD_MUTEX_INITIALIZER;
map<int, string> gSockFdMap;

int SugwServer::m_sugwSocketNum = 0;
long SugwServer::m_lastDisconnectTime = 0;

ULONG SugwServer::m_nSendMsgs = 0;
ULONG SugwServer::m_nSendStart = 0;
ULONG SugwServer::m_nSendEnd = 0;


void sugwSendThread(void);
void sugwRecvThread(void* para);
void listenSugwThread(void);

SugwServer::SugwServer(){
}

SugwServer::~SugwServer(){
}


//起消息发送处理的线程
int SugwServer::initSugwSocket() {
    
  // generate thread to send msg to sugw  
  if (pthread_create(&tidListen, 0, (void *(*)(void*))&listenSugwThread, 0)){
      LoggerPtr rootLogger = Logger::getRootLogger();  
      LOG4CXX_ERROR(rootLogger,"create listenSugwThread failed!");
      return -1;
  }

  if (pthread_create(&tidSktSend, 0, (void *(*)(void*)) &sugwSendThread, 0)) {
     LoggerPtr rootLogger = Logger::getRootLogger();  
     LOG4CXX_ERROR(rootLogger,"create sugwSendThread failed!");
     return -1;
  }  
  return 0;
}


//send msg to sugw
//return 0:   send success
//return <0: send failed
int SugwServer::sktSend(PUCHAR sendBuf, int sendLen) {
    int ret = -1;    
    int len = 0;
    bool tag = false;

    fd_set fds_w; // set of file descriptors to check.
    int nready = 0; // # fd's ready. 
    int fd = 0;
    string ss = "";
    string name = "";
    int maxfd = 0;  // fd's 0 to maxfd-1 checked.
    struct timeval tv;
    map<int, string>::iterator itr;
    map<int, string>::iterator tmpItr;
    FD_ZERO(&fds_w);
    
    pthread_mutex_lock(&sockListMutex);
    itr = gSockFdMap.begin();
    for (; itr!=gSockFdMap.end(); itr++)
    {
        fd = itr->first;
        FD_SET(fd, &fds_w);
        if (maxfd < fd)
        {
          maxfd = fd;
        }
    }
    pthread_mutex_unlock(&sockListMutex);
    
    if (0 ==  maxfd)
    {
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_ERROR(rootLogger, "No sugw client!");
        return -1;
    }
    
    tv.tv_sec = 3; 
    tv.tv_usec = 0;
    maxfd ++;
    nready = select(maxfd, (fd_set *) 0, &fds_w, (fd_set *) 0,&tv);
    LoggerPtr rootLogger = Logger::getRootLogger();  
    switch (nready) 
    {
    case -1:
        //error
        LOG4CXX_ERROR(rootLogger, "select error:"<<strerror(errno));
        removeSigSockList();
        return -1;
    case 0:  
        //timeout
        LOG4CXX_ERROR(rootLogger, "sktSend: select timeout!");
        removeSigSockList();
        return -1;
    default:  
        break;
    }  
    
    if (nready > 0)
    {
        pthread_mutex_lock(&sockListMutex);
        itr = gSockFdMap.begin();
        for (; itr!=gSockFdMap.end(); ){
            name = "";
            ss = "";
            tmpItr = itr;
            fd = itr->first;
            if (FD_ISSET(fd, &fds_w))
            {
                if(tag){
                    tag = false;
                    LOG4CXX_WARN(rootLogger, "Switch to send to sugw " + itr->second + ", fd="<<fd);
                }
                
                len = send(fd, sendBuf, sendLen+1, 0);
                if( 1 > len) {
                   // send failed
                   name = itr->second;
                   LOG4CXX_ERROR(rootLogger, "Err send msg to fd="<<fd<<", errno="<<errno<<", error:" << strerror(errno));
                   //ss = ss + "(" + Comm::m_listenHostIP + ")sugw " + name + " disconnect!";
                   //AppMonitor::getInstance()->alert(G::m_appName.c_str(),ss.c_str(),8);
                   tmpItr++;
                   gSockFdMap.erase(itr);
                   Comm::closeSocket(fd, name);
                   time(&m_lastDisconnectTime);
                   m_sugwSocketNum--;
                   itr = tmpItr;
                }else{
                   //send success
                   ret = 0;
                   break;
                }
            }else{
                itr++;
                if(itr == gSockFdMap.end()){
                    LOG4CXX_ERROR(rootLogger, "sktSend: no skt to write data="<<sendBuf);
                    break;
                }else{
                    if(!tag){
                        tag = true;
                    }
                }
            }
        }
        pthread_mutex_unlock(&sockListMutex);
    }

    return ret;
}


//count the speed of processing package
void SugwServer::sendMsgCount(){
    static ULONG nLastSendStart = 0;
    static ULONG nLastSendEnd = 0;

    long i = m_nSendStart - nLastSendStart;
    nLastSendStart = m_nSendStart;
    
    long j = m_nSendEnd - nLastSendEnd;
    nLastSendEnd = m_nSendEnd;
    
    long now = 0;
    time(&now);

    if(i>0 || j>0){
        LOG4CXX_DEBUG(G::logger, "ToSugw: start="<<i<<"/s, end="<<j<<"/s, now="<<now);
    }

    return;
}


bool SugwServer::ifSktNormal(){
    long now = 0;
    if(m_sugwSocketNum>0){
        return true;
    }else{
        time(&now);
        if(now - m_lastDisconnectTime >= 300){
            return false;
        }else{
            return true;
        }
    }
}


//socket send to sugw 
void sugwSendThread(void) {
  CDRMessage* pMessage = 0;
  int ret = 0;

  LoggerPtr rootLogger = Logger::getRootLogger();
  pthread_t tid = syscall(SYS_gettid);
  LOG4CXX_INFO(rootLogger, "sugwSendThread tid=" << tid<<", threadNum="<<++G::m_threadNum);

  DataControl& rDataControl = gSendSugwQueue.getDataControl();
  rDataControl.activate();
  while (!G::m_bSysStop && rDataControl.isActive()) {
    if(0 == gSendSugwQueue.getHead()) {
      pthread_mutex_lock(&rDataControl.getMutex());
      pthread_cond_wait(&rDataControl.getCond(), &rDataControl.getMutex());
      pthread_mutex_unlock(&rDataControl.getMutex());
      continue;
    }

    pMessage = (CDRMessage*) gSendSugwQueue.get();
    if (pMessage){
        pMessage->sendMessage();
        pMessage->setSend(true);
    }
  }

  //delete all message when exit
  while (G::m_bSysStop) {
    if(0 == gSendSugwQueue.getHead()) {
        break;
    }
    pMessage = (CDRMessage*) gSendSugwQueue.get();
    if (pMessage) {  
        delete pMessage;
        pMessage = 0;
    }
  }
  LOG4CXX_INFO(rootLogger, "Exit sugwSendThread, threadNum="<<--G::m_threadNum);
}


//Recv heart Msg from sugw, just for checking socket
void sugwRecvThread(void* para) {
  fd_set fds_r; // set of file descriptors to check.
  int nready = 0; // # fd's ready. 
  int fd = 0;
  int maxfd = 0;  // fd's 0 to maxfd-1 checked.
  struct timeval tv;
  int readNum = 0;
  UCHAR packageBuff[128];
  
  if(para == NULL){
     LoggerPtr rootLogger = Logger::getRootLogger();
     LOG4CXX_ERROR(rootLogger, "sugwRecvThread para is NULL!");
     return;
  }
  fd = (int)para; 
  if(fd<1){
     LoggerPtr rootLogger = Logger::getRootLogger();
     LOG4CXX_ERROR(rootLogger, "sugwRecvThread: error fd="<<fd);
     return;
  }
  
  pthread_t tid = syscall(SYS_gettid);
  LoggerPtr rootLogger = Logger::getRootLogger();
  LOG4CXX_INFO(rootLogger, "sugwRecvThread fd="<<fd<<", tid=" << tid<<", threadNum="<<++G::m_threadNum);
  
  while (!G::m_bSysStop) {
      FD_ZERO(&fds_r);
      maxfd = 0;
      
      FD_SET(fd, &fds_r);
      if (maxfd < fd)
      {
         maxfd = fd;
      }
      
      if (0 == maxfd)
      {
          LoggerPtr rootLogger = Logger::getRootLogger();  
          LOG4CXX_ERROR(rootLogger, "sugwRecvThread: error fd="<<fd);
          break;
      }
      
      tv.tv_sec = 8; 
      tv.tv_usec = 0;
      maxfd ++;
      nready = select(maxfd, &fds_r, (fd_set *) 0, (fd_set *) 0,&tv);
      switch (nready) 
      {
      case -1:
          //error
          LOG4CXX_ERROR(rootLogger, "select error:"<<strerror(errno));
          break;
      case 0:  
          LOG4CXX_ERROR(rootLogger, "receive heart msg from sugw fd="<<fd<<" timeout!");
          break;
      default:  
          break;
      }  
  
      if (nready > 0)
      {
          //fd have something to read
          readNum = recv(fd, packageBuff, 127, 0);            
          if( 1 > readNum) {
             string name = SugwServer::findSigSock(fd);
             LOG4CXX_ERROR(rootLogger, "Receive failed from sugw "<<name<<", nread= "<<readNum<<", errno="<<errno<<", error:" << strerror(errno));
             //string ss = "(" + Comm::m_listenHostIP + ") sugw " + name + " disconnect!";
             //AppMonitor::getInstance()->alert(G::m_appName.c_str(),ss.c_str(),8);
             break;
          }else{
             LOG4CXX_DEBUG(rootLogger, "sugw is in connect, fd="<<fd);
          }
      }else{
         break;
      }
  }

  SugwServer::removeSigSock(fd);
  LOG4CXX_INFO(rootLogger, "Exit from sugwRecvThread fd="<<fd<<", threadNum="<<--G::m_threadNum);
  return;
}


void listenSugwThread(void)
{
  int listenSockt = 0;
  int yes = 1;

  // init socket to receive message from AppServer
  LoggerPtr rootLogger = Logger::getRootLogger();
  pthread_t tid = syscall(SYS_gettid);
  LOG4CXX_INFO(rootLogger, "listenSugwThread tid=" << tid<<", threadNum="<<++G::m_threadNum);
  
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

  //  int appserversock_fd;
  sockaddr_in myAddr,clientAddr;
  // fill in connection parameters
  myAddr.sin_family = AF_INET;
  myAddr.sin_port = htons(SugwServer::m_listenPort);
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
    LOG4CXX_INFO(rootLogger, "Bind() listen on port "<<SugwServer::m_listenPort);
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
  LOG4CXX_INFO(rootLogger, "Start listen...");
  
  int sin_size = sizeof(sockaddr_in);
  int newFd = 0;
  
  while(!G::m_bSysStop)
  {
    newFd = accept(listenSockt, (sockaddr*)&clientAddr, (socklen_t *)&sin_size);
    if( -1 == newFd)
    {
        //LOG4CXX_ERROR(rootLogger, "accept  error:"<<strerror(errno));
        sleep(1);
        continue; // try to get in another connection request from ep
    }
    else
    {
        SugwServer::addSigSock(newFd,&clientAddr);
    }
  }
  close(listenSockt);
  LOG4CXX_INFO(rootLogger, "Exit from listenSugwThread, threadNum="<<--G::m_threadNum);
  return;
}


int SugwServer::addSigSock(int newFd, struct sockaddr_in* clientAddr)
{
  if (newFd <1)
  {
    return -1;
  }

  int ret = 0;
  static int i = 0;
  string name = "";
  name = name+inet_ntoa(clientAddr->sin_addr)+":"+G::int2String(ntohs(clientAddr->sin_port));
  
  fcntl(newFd, F_SETFL, O_NONBLOCK);
  socklen_t  sendbuflen = 20480;
  socklen_t len = sizeof(sendbuflen);
  setsockopt(newFd, SOL_SOCKET, SO_SNDBUF, (void*)&sendbuflen, len);

  LoggerPtr rootLogger = Logger::getRootLogger();
  LOG4CXX_INFO(rootLogger, "Add sugwClient: fd="<<newFd<<", name="<<name); 
  
  if (pthread_mutex_lock(&sockListMutex))
  {
     LOG4CXX_ERROR(rootLogger, "pthread_mutex_lock failed while addSigSock!"); 
     Comm::closeSocket(newFd, name);
     return -1;
  }

  //sugw client maxnumber = 2
  if(gSockFdMap.size() < 2){
     //<2, add 
     if(i == 10){
        i = 0;
     }
     
     // generate thread to recv heart msg from sugw
     if (pthread_create(&tidSktRecv[i++], 0, (void *(*)(void*)) &sugwRecvThread, (void*)newFd)) {
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_ERROR(rootLogger,"create sugwRecvThread fd="<<newFd<<", name="<<name<<" failed!");
        Comm::closeSocket(newFd, name);
        ret = -1;
     }else{
         gSockFdMap.insert(std::pair<int,string>(newFd,name));
         m_sugwSocketNum++;
         ret = 0;
     }
  }else{
     //>2, disconnect
     LoggerPtr rootLogger = Logger::getRootLogger();
     LOG4CXX_WARN(rootLogger, "Too many sugwClient, close sockfd="<<newFd<<",name="<<name); 
     Comm::closeSocket(newFd, name);
     newFd = 0;
     ret = -1;
  }
  pthread_mutex_unlock(&sockListMutex);  
  
  return ret;
}

string SugwServer::findSigSock(int fd)
{
    char* name = 0;
    if (pthread_mutex_lock(&sockListMutex))
    {
      return "";
    }
    
    map<int, string>::iterator itr = gSockFdMap.begin();
    map<int, string>::iterator tmpItr;
    for (; itr!=gSockFdMap.end(); )
    {
      if (fd == itr->first)
      {
          name = (char*)itr->second.c_str();
          break;  
      }else{
          itr++;
      }
    }
    pthread_mutex_unlock(&sockListMutex);

    if(!name){
        return "";
    }
    return name;
}

int SugwServer::removeSigSock(int fd)
{
  string name("");
  if (pthread_mutex_lock(&sockListMutex))
  {
    Comm::closeSocket(fd, name);
    return -1;
  }

  map<int, string>::iterator itr = gSockFdMap.begin();
  map<int, string>::iterator tmpItr;
  for (; itr!=gSockFdMap.end(); )
  {
    if (fd == itr->first)
    {
        name = itr->second;
        Comm::closeSocket(fd,name);        
        time(&m_lastDisconnectTime);
        m_sugwSocketNum--;
        gSockFdMap.erase(itr);
        break;  
    }else{
        itr++;
    }
  }
  pthread_mutex_unlock(&sockListMutex);
  
  return 0;
}


int SugwServer::removeSigSockList()
{
    map<int,string>::iterator itr;
    map<int,string>::iterator tmpItr;
    int fd = 0;
    string name("");

    if (pthread_mutex_lock(&sockListMutex))
    {
        LoggerPtr rootLogger = Logger::getRootLogger();
        LOG4CXX_ERROR(rootLogger, "pthread_mutex_lock failed while removeSigSockList!");
        return -1;
    }
    
    if(!gSockFdMap.empty()){
        itr = gSockFdMap.begin();
        for (; itr!=gSockFdMap.end(); )
        {
          name = "";
          tmpItr = itr;
          fd = itr->first;
          name = itr->second;
          Comm::closeSocket(fd, name);
          time(&m_lastDisconnectTime);
          m_sugwSocketNum--;
          tmpItr++;
          gSockFdMap.erase(itr);
          itr = tmpItr;
        }
    }
    pthread_mutex_unlock(&sockListMutex);

    return 0;
}

string SugwServer::getSugwClientsInfo(){
    int fd = 0;
    string name = "";
    string info = "";
    
    if(!gSockFdMap.empty()){
        if (pthread_mutex_lock(&sockListMutex)) {
          LoggerPtr rootLogger = Logger::getRootLogger();
          LOG4CXX_ERROR(rootLogger, "pthread_mutex_lock fail while getSugwClientsInfo!");
          return info;
        }

        map<int, string>::iterator itr = gSockFdMap.begin();
        for (; itr!=gSockFdMap.end(); itr++) {
            name = "";
            fd = itr->first;
            name = itr->second;
            if (fd>0) {                
                info += name;
                info += " ";
            }
        }        
        pthread_mutex_unlock(&sockListMutex);        
    }

    return info;
}

//end of sugwserver.cpp

