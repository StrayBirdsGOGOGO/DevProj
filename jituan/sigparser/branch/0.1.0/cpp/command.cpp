/***************************************************************************
 fileName    : command.cpp
 begin       : 2010-12-14
 copyright   : (C) 2010 by wyt
 ***************************************************************************/

#include "comm.h"
#include "command.h"
#include "hwsshare.h"
#include "g.h"
#include "zxcomm.h"
#include "sugwserver.h"

pthread_t Command::m_tidCommand = 0;
int Command::m_listenPort = 2100;
string Command::m_listenHostIP;
sockaddr_in Command::m_myAddr;
int Command::m_commandSockt = 0;
bool Command::m_bConnected = false;
int Command::m_userNum = 0;
map<string,string> Command::m_userMap;
pthread_mutex_t Command::m_userMutex = PTHREAD_MUTEX_INITIALIZER;

void commandRecvThread(void);


Command::Command() {
}

Command::~Command() {
}

void Command::closeCommandSocket(){
    LoggerPtr rootLogger = Logger::getRootLogger();
    if (m_commandSockt && close(m_commandSockt) == -1) {
      LOG4CXX_ERROR(rootLogger, "close(m_commandSockt) error!");
    } else {
      LOG4CXX_INFO(rootLogger, "close(m_commandSockt) success");
      m_commandSockt = 0;
    }

    m_bConnected = false;
    return;
}

int Command::initCommandProcess(){
    m_myAddr.sin_family = AF_INET;
    m_myAddr.sin_port = htons(m_listenPort);
    m_myAddr.sin_addr.s_addr = inet_addr(m_listenHostIP.c_str());
    bzero( &(m_myAddr.sin_zero), 8);
    
    
    // generate thread to process the command
    if (pthread_create(&m_tidCommand, NULL, (void *(*)(void*)) &commandRecvThread, NULL)) {
        LoggerPtr rootLogger = Logger::getRootLogger();
        LOG4CXX_INFO(rootLogger, "create commandProcessThread failed!");
        G::m_bSysStop = true;
        return -1;
    }
    
    return 0;

}

//connect with sume server
int Command::commandSocketConnect(){
    int ret = 0;
    int fd = 0;
        
    while(!G::m_bSysStop){
      fd = commandListen();
      if(fd > 0){
        ret = commandLogin();
        if(ret == 0){
            //login success
            break;
        }else if(ret == -1){
            //wrong username or pwd
            sleep(2);
            continue;
        }else if(ret == -2){
            //socket disconnect
            sleep(2);
            continue;
        }
      }else{
        sleep(2);
      }
    }
    return 0;
}

int Command::commandListen(){
     sockaddr_in clientAddr;
     int sin_size = 0;    
     sin_size = sizeof(sockaddr_in);  
     string name = "";   
     int newFd = 0;
     int fd = 0;
     int yes = 1;

     LoggerPtr rootLogger = Logger::getRootLogger();
     if(m_bConnected){
         m_bConnected = false;
         if(m_commandSockt && (close(m_commandSockt)==-1)){
             LOG4CXX_ERROR(rootLogger, "Error close m_commandSockt, error:"<<strerror(errno));
             return -1;
         }else{
             m_commandSockt = 0;
             LOG4CXX_INFO(rootLogger, "Success close m_commandSockt!");
         }
     }else{
         m_commandSockt = 0;
     }
     
     if( (fd = socket(AF_INET, SOCK_STREAM, 0) ) == -1 )
     {
       LOG4CXX_ERROR(rootLogger, "Receive socket() error:"<<strerror(errno));
       return -1;
     }
     if( setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1 )
     {
       LOG4CXX_ERROR(rootLogger, "Setsockopt error:"<<strerror(errno));
       close(fd);
       return -1;
     }
     
     if( bind(fd, (sockaddr*)&m_myAddr, sizeof(sockaddr)) == -1 )
     {
       LOG4CXX_ERROR(rootLogger, "Bind() error:"<<strerror(errno));
       close(fd);
       return -1;
     }
     
     LOG4CXX_INFO(rootLogger, "Bind() listen on port: "<<m_listenPort);
     
     if( listen(fd, 10) == -1 )
     {
       LOG4CXX_ERROR(rootLogger, "Listen()  error="<<errno);
       close(fd);
       return -1;
     }     
     
     fcntl(fd, F_SETFL, O_NONBLOCK);     
     LOG4CXX_INFO(rootLogger, "Start Command listen...");  
     
     while(!G::m_bSysStop){
          newFd = accept(fd, (sockaddr*)&clientAddr, (socklen_t *)&sin_size);
          if(newFd > 0)
          {
              m_commandSockt = newFd;  
              m_bConnected = true;
              name = name + inet_ntoa(clientAddr.sin_addr) + ":" + G::int2String(ntohs(clientAddr.sin_port));
              LOG4CXX_INFO(rootLogger, "Add command fd="<<newFd<<", name="<<name); 
              break;
          }else{
              //LOG4CXX_ERROR(rootLogger, "Accept error:"<<strerror(errno));
              sleep(2);
          }    
     }    

     if(fd>0){
         close(fd);
     }
     return m_commandSockt;
}


int Command::commandLogin(){
    int socketFd = m_commandSockt;
    int len = 0;
    int nread = 0;
    int result = 0;
    unsigned char recvBuff[100];

    //send userRequest
    string userRequest("Username: ");
    len = userRequest.length();   
    LoggerPtr rootLogger = Logger::getRootLogger();
    LOG4CXX_INFO(rootLogger, "==> "<<userRequest);
    result = send(socketFd, userRequest.data(), len+1, 0);
    if (result <= 0) {
        LOG4CXX_ERROR(rootLogger, "Send userRequest fail, error=" << strerror(errno));
        return -2;
    }
    
    //recv userName
    nread = recv(socketFd, recvBuff, 99, 0); 
    if(nread < 1){
        LOG4CXX_ERROR(rootLogger, "Receive userName fail, error=" << strerror(errno));
        return -2;
    }    

    len = strlen((char*)recvBuff);
    for(int i=0; i<len; i++){
        if(recvBuff[i] == '\r'){
            len = i;
            break;
        }
    }
    recvBuff[len] = 0;
    string user = (char*)recvBuff;
    
    //send pwdRequest
    string pwdRequest = "Password: ";
    len = pwdRequest.length();
    LOG4CXX_INFO(rootLogger, "==> "<<pwdRequest);
    result = send(socketFd, pwdRequest.data(), len+1, 0);
    if (result <= 0) {        
      LOG4CXX_ERROR(rootLogger, "Send pwdRequest failed, error=" << strerror(errno));
      return -2;
    }
    
    //receive pwd
    nread = 0;
    memset(recvBuff,0,100);
    nread = recv(socketFd, recvBuff, 99, 0); 
    if(nread<1){
        LOG4CXX_ERROR(rootLogger, "Receive pwd fail, error=" << strerror(errno));
        return -2;
    }
    len = strlen((char*)recvBuff);
    for(int i=0; i<len; i++){
        if(recvBuff[i] == '\r'){
            len = i;
            break;
        }
    }
    recvBuff[len] = 0;
    string pwd = (char*)recvBuff;
    
    //check userName and password
    if(1 != checkUser(user,pwd)){
        //login failed
        return -1;
    }else{
        //login success
        LOG4CXX_INFO(rootLogger, user<<": login success! ");
    }
    
    //send '>'
    string tag =">";
    result = send(socketFd, tag.data(), 2, 0);
    if (result <= 0) {        
      LOG4CXX_ERROR(rootLogger, "Send '>' failed, error=" << strerror(errno));
      return -2;
    }
    return 0;
}


//return 0: send success
//return -2: send fail, reconnect
//return -3: wrong data
int Command::commandSend(string sBuf){
    int socketFd = m_commandSockt;
    int result = 0;
    //unsigned char sendBuff[400];
    
    if(socketFd<1){
        LoggerPtr rootLogger = Logger::getRootLogger();
        LOG4CXX_ERROR(rootLogger, "wrong commandSocket="<<socketFd);
        return -2;
    }

    int len = sBuf.length();
    if(len<1){
        return -3;
    }

    len = sBuf.length()+1;
    
    //send value
    result = send(socketFd, sBuf.data(), len, 0);
    if (result <= 0) {
        LoggerPtr rootLogger = Logger::getRootLogger();
        LOG4CXX_ERROR(rootLogger, "Send to commandLine failed, error=" << strerror(errno));
        return -2;
    }
    
    LoggerPtr rootLogger = Logger::getRootLogger();
    LOG4CXX_INFO(rootLogger, "==>output: "<<sBuf);
    return 0;
}


//return 0: success
//return -2: failed, reconnect command socket
//return -3: failed, wrong data
int Command::commandIntreract(string out, char* in, int inlen){
    int result = 0;
    int len = 0;
    int nread = 0;
    int socketFd = m_commandSockt;

    if(socketFd<1){
        LoggerPtr rootLogger = Logger::getRootLogger();
        LOG4CXX_ERROR(rootLogger, "wrong commandSocket="<<socketFd);
        return -2;
    }

    //receive remain character, and clear recvBuff
    memset(in,0,inlen);
    nread = recv(socketFd, in, inlen, 0); 
    if(nread<1){
        LoggerPtr rootLogger = Logger::getRootLogger();
        LOG4CXX_ERROR(rootLogger, "Receive error: " << strerror(errno));
        return -2;
    }
    
    //send 
    len = out.length();
    if(len<1){
        LoggerPtr rootLogger = Logger::getRootLogger();
        LOG4CXX_ERROR(rootLogger, "Send to commandLine failed: no data!");
        return -3;
    }

    LoggerPtr rootLogger = Logger::getRootLogger();
    LOG4CXX_INFO(rootLogger, "==>output: "<<out);
    result = send(socketFd, out.data(), len+1, 0);
    if (result <= 0) {   
        LoggerPtr rootLogger = Logger::getRootLogger();
        LOG4CXX_ERROR(rootLogger, "Send to commandLine failed, error=" << strerror(errno));
        return -2;
    }
    
    //receive
    nread = 0;
    memset(in,0,inlen);
    int recvLen = 0;
    while(1){
        nread = recv(socketFd, in+recvLen, 1, 0); 
        if(1 == nread){
            if(in[recvLen] == '\r'){
                in[recvLen] = 0;
                break;
            }
            
            recvLen++;
            if(recvLen > inlen){
                return -1;                
            }
        }else{
            LoggerPtr rootLogger = Logger::getRootLogger();
            LOG4CXX_ERROR(rootLogger, "Receive from commandLine failed, error=" << strerror(errno));
            return -2;            
        }
    }
    
    LOG4CXX_INFO(rootLogger, "<==input: "<<in);
    return 0;    
}


//return 1: yes
//return 0: no
//return -2: failed, reconnect command socket
int Command::commandConfirm(string out){
    int ret = 0;
    int inLen = 20;
    char in[inLen];
    
    while(1){
        if(G::m_bSysStop){
            return -2;
        }
        
        LoggerPtr rootLogger = Logger::getRootLogger();
        LOG4CXX_INFO(rootLogger, "==>output: "<<out);
        ret = commandIntreract(out, in, inLen);
        if(-2 == ret){
            //reconnect 
            break;
        }else if(0 == ret){
            if(0 == strcmp(in, "yes")){
                ret = 1;
                break;
            }else if(0 == strcmp(in, "no")){
                ret = 0;
                break;
            }else{
                continue;
            }
        }else{
            continue;
        }
    }

    return ret;
}


//command process thread
void commandRecvThread(void) {
  int nread = 0;
  int ret = 0;
  int fd = 0;
  unsigned char readByte[1];
  unsigned char recvBuf[1024];
  int inputLen = 0;
  LoggerPtr rootLogger = Logger::getRootLogger();
  pthread_t tid = syscall(SYS_gettid);
  LOG4CXX_INFO(rootLogger, "commandRecvThread tid=" << tid<<", threadNum="<<++G::m_threadNum);
  
  Command::commandSocketConnect();
  
  while (!G::m_bSysStop) {
      if(!Command::m_bConnected){
          sleep(1);
          continue;
      }
      fd = Command::m_commandSockt;
      
      nread = 0;
      nread = recv(fd, readByte, 1, 0);
      if(nread == 1){
          if((readByte[0] == '\r')){     
              //wrong input
             if((inputLen>0) && (inputLen<MAX_COMMAND_LEN)){
                  recvBuf[inputLen] = 0;
                  ret = Command::commandProcess((char*)recvBuf);
                  memset(recvBuf,0,1024);
                  inputLen = 0;
                  if((1 == ret) || (-2 == ret)){
                      //quit, reconnect
                      Command::commandSocketConnect();
                      continue;
                  }
              }else if(inputLen >= MAX_COMMAND_LEN){
                  memset(recvBuf, 0, 1024);
                  inputLen = 0;
                  string out = string("wrong input!\n");
                  ret = Command::commandSend(out);
                  if(-2 == ret){
                      Command::commandSocketConnect();
                      continue;
                  }
              }
              
              //send char '>'
              string tag =">";
              int result = send(fd, tag.data(), 2, 0);
              if (result <= 0) {
                  LOG4CXX_ERROR(rootLogger, "Send '>' failed, error=" << strerror(errno));
                  Command::commandSocketConnect();
              }              
          }else  if((readByte[0] == '\n') || (readByte[0] == 0)){
              recvBuf[0] = 0;
              inputLen = 0;
          }else{
              memcpy(recvBuf+inputLen, readByte, 1);
              inputLen++;
          }
      }else{
          LoggerPtr rootLogger = Logger::getRootLogger();
          LOG4CXX_DEBUG(rootLogger, "closed commandSocket="<<fd);
          Command::commandSocketConnect();
      }
  }

  LOG4CXX_INFO(rootLogger, "Exit commandRecvThread, threadNum="<<--G::m_threadNum);
  return;
}


//返回1: 正常退出telnet
//返回 0: 命令正确
//返回-1:命令错误
//返回-2:socket 错误，退出重连
int Command::commandProcess(char* recvBuf){
    char param[3][MAX_PARAM_LEN]; 
    char* tmpStr = 0;
    int len = 0;
    int paramNum = 0;
    int i = 0;
    int ret = 0;
    LoggerPtr rootLogger = Logger::getRootLogger();
    LOG4CXX_DEBUG(rootLogger, "<==input: "<<recvBuf);

    string ss = recvBuf;
    tmpStr = strtok(recvBuf, " ");
    while(tmpStr){     
        if(i == 3){
            ret = -1;
            break;
        }
            
        len = strlen(tmpStr);
        if(len >= MAX_PARAM_LEN){
            ret = -1;
            break;
        }else{
            memcpy(param[i], tmpStr, len);
            param[i][len] = 0;
        }   
        i++;
        tmpStr = strtok(NULL, " ");
    }    

    if(0 == ret){
        paramNum = i;
        if(paramNum == 1){        
            if(0 == strcmp(recvBuf,"quit")){
               LoggerPtr rootLogger = Logger::getRootLogger();
               LOG4CXX_INFO(rootLogger, "quit from command!");
               return 1;
            }else{
               ret = -1;
            }
        }else if(paramNum == 2){
            if(0 == strcmp(param[1], "-v")){
               ret = displayCommandProcess(param[0]);
            }else{
               ret = -1;
            }
        }else if(paramNum == 3){
            if(0 == strcmp(param[1], "-r")){
                ret = changeCommandProcess(param[0],param[2]);
            }else if(0 == strcmp(param[1], "-a")){
                ret = addCommandProcess(param[0],param[2]);
            }else if(0 == strcmp(param[1], "-d")){
                ret = delCommandProcess(param[0],param[2]);
            }else{
                ret = -1;
            }        
        }else{
            ret = -1;
        }
    }

    if(-1 == ret){
        ret = commandSend("wrong input!\n");
    }
    return ret;
}


//修改命令处理
//返回 0: 命令正确
//返回-1:命令错误
//返回-2:socket断开，telnet 重连
//返回-3:运行出错
int Command::changeCommandProcess(char* object, char* value){
    int ret = 0;
    int iValue = 0;
    if(!object || !value){
        LoggerPtr rootLogger = Logger::getRootLogger();
        LOG4CXX_ERROR(rootLogger, "changeCommandProcess failed, param is null!");
        return -3;
    }
    
    if(0 == strcmp(object, "PACKAGE_DELAY_LIMIT")){
        iValue = atoi(value);
        if(iValue<0){
            ret = -1;
        }else{
            ZxComm::m_msgDelayLimit = iValue;
        }
        return ret;
    }
    
    if(0 == strcmp(object, "BEGIN_END_INTERVAL")){
        iValue = atoi(value);
        if(iValue<0){
            ret = -1;
        }else{
            ZxComm::m_beginEndInterval = iValue;
        }
        return ret;
    }
    
    if(0 == strcmp(object, "TEST_PHONES")){        
        ret = G::changeTestPhones(value);
        return ret;
    }
    
    if(0 == strcmp(object, "TEST_ZONE")){        
        ret = G::changeTestZone(value);
        return ret;
    }
    
    if(0 == strcmp(object, "IS_DEBUG")){
        iValue = atoi(value);
        G::m_isDebug = iValue;
        return ret;
    }

    return -1;
}


//添加命令处理
//返回 0: 命令正确
//返回-1:命令错误
int Command::addCommandProcess(char* object, char* value){
    int ret = 0;
    if(!object || !value){
        LoggerPtr rootLogger = Logger::getRootLogger();
        LOG4CXX_ERROR(rootLogger, "addCommandProcess failed, param is null!");
        return -3;
    }

    if(0 == strcmp(object, "TEST_PHONES")){
        ret = G::addTestPhones(value);
    }else{
        ret = -1;
    }

    return ret;
}

//删除命令处理
//返回 0: 命令正确
//返回-1:命令错误
int Command::delCommandProcess(char* object, char* value){
    int ret = 0;
    if(!object || !value){
        LoggerPtr rootLogger = Logger::getRootLogger();
        LOG4CXX_ERROR(rootLogger, "delCommandProcess failed, param is null!");
        return -3;
    }

    if(0 == strcmp(object, "TEST_PHONES")){
        ret = G::delTestPhones(value);
    }else if(0 == strcmp(object, "TEST_ZONE")){
        ret = G::delTestZone(value);
    }else{
        ret = -1;
    }

    return ret;
}


//查看命令处理
//返回 0: 命令正确
//返回-1:命令错误
//返回-2:socket 断开，重连
//返回-3:运行出错
int Command::displayCommandProcess(char* object){
    int ret = 0;
    int iValue = 0;
    string sValue = "";
    
    if(!object){
        LoggerPtr rootLogger = Logger::getRootLogger();
        LOG4CXX_ERROR(rootLogger, "displayCommandProcess failed, param is null!");
        return -3;
    }

    if(0 == strcmp(object, "ZX_CLIENTS")){
        sValue = ZxComm::getZxClientsInfo();
    }else if(0 == strcmp(object, "SUGW_CLIENTS")){
        sValue = SugwServer::getSugwClientsInfo();
    }else if(0 == strcmp(object, "BEGIN_END_INTERVAL")){
        iValue = ZxComm::m_beginEndInterval;
        sValue = G::int2String(iValue);
    }else if(0 == strcmp(object, "PACKAGE_DELAY_LIMIT")){
        iValue = ZxComm::m_msgDelayLimit;
        sValue = G::int2String(iValue);
    }else if(0 == strcmp(object, "MSG_PROCESS_THREAD_NUM")){
        iValue = ZxComm::m_msgProcessNum;
        sValue = G::int2String(iValue);
    }else if(0 == strcmp(object, "TEST_PHONES")){
        sValue = G::m_testPhones;
    }else if(0 == strcmp(object, "TEST_ZONE")){
        sValue = G::m_testZone;
    }else if(0 == strcmp(object, "IS_DEBUG")){
        iValue = G::m_isDebug;
        sValue = G::int2String(iValue);
    }else{
        return -1;
    }

    string out = string(sValue);
    out += "\n";
    ret = commandSend(out);
    return ret;
}

int Command::getLoginUsers(string value){
    int ret = 0;
    char* tmpStr = 0;
    int len = 0;
    int i = 0;
    char param[m_userNum][MAX_PARAM_LEN];
    char* pStr = (char*)value.c_str();
    if(!pStr){
        LoggerPtr rootLogger = Logger::getRootLogger();
        LOG4CXX_ERROR(rootLogger, "getLoginUsers: null param!");
        return -1;
    }
    
    tmpStr = strtok(pStr, ",");
    while(tmpStr){        
        if(i<m_userNum){
            len = strlen(tmpStr);
            if(len<1 || len>=MAX_PARAM_LEN){
                LoggerPtr rootLogger = Logger::getRootLogger();
                LOG4CXX_ERROR(rootLogger, "wrong param: "<<tmpStr);
                return -1;
            }else{
                memcpy(param[i], tmpStr, len);
                param[i][len] = 0;
            }
        }else{
            return -1;
        }
            
        i++;
        tmpStr = strtok(NULL, ",");
    }        

    if(i < m_userNum){
        return -1;
    }

    char user[MAX_PARAM_LEN];
    char pwd[MAX_PARAM_LEN];
    char* pKey = user;
    char* pValue = pwd;

    for(i=0; i<m_userNum; i++){
        ret = G::parseLoginString(param[i],pKey,pValue);
        if(-1 == ret){
            LoggerPtr rootLogger = Logger::getRootLogger();
            LOG4CXX_ERROR(rootLogger, "wrong param: "<<tmpStr);
            return -1;
        }else{
            insertUser2Map(pKey,pValue);
        }
    }

    return ret;
}

int Command::insertUser2Map(char* user,char* pwd){    
    map<string, string>::iterator itr;

    if(strlen(user)<1 || strlen(pwd)<1){
        LoggerPtr rootLogger = Logger::getRootLogger();
        LOG4CXX_ERROR(rootLogger, "insertUser2Map failed, key is Null!");
        return -1;
    }
    
    if (pthread_mutex_lock(&m_userMutex)) {
      LoggerPtr rootLogger = Logger::getRootLogger();
      LOG4CXX_ERROR(rootLogger, "pthread_mutex_lock fail while insertUser2Map!");
      return -1;
    }
    
    itr = m_userMap.find(user);
    if(m_userMap.end() != itr){
        //found
        pthread_mutex_unlock(&m_userMutex);
        LoggerPtr rootLogger = Logger::getRootLogger();
        LOG4CXX_ERROR(rootLogger, "insertUser2Map: existed user="<<user);
        return -1;
    }else{
        //not found
        m_userMap.insert(std::pair<string,string>(string(user),string(pwd)));
        LoggerPtr rootLogger = Logger::getRootLogger();
        LOG4CXX_DEBUG(rootLogger, "insertUser2Map: user="<<user);
    }
    
    pthread_mutex_unlock(&m_userMutex);
    return 0;
}


int Command::checkUser(string user, string pwd){
    int ret = 0;
    map<string, string>::iterator itr;

    if (pthread_mutex_lock(&m_userMutex)) {
      LoggerPtr rootLogger = Logger::getRootLogger();
      LOG4CXX_ERROR(rootLogger, "pthread_mutex_lock fail while checkUser!");
      return -1;
    }
    
    itr = m_userMap.find(user);
    if(m_userMap.end() != itr){
        //found
        string value = itr->second;
        if(pwd == value){
            //right user and pwd
            ret = 1;
        }else{
            //wrong pwd
            ret = 0;
            LoggerPtr rootLogger = Logger::getRootLogger();
            LOG4CXX_ERROR(rootLogger, "checkUser: wrong pwd="<<pwd<<" of user="<<user);
        }
    }else{
        //wrong user
        ret = 0;
        LoggerPtr rootLogger = Logger::getRootLogger();
        LOG4CXX_ERROR(rootLogger, "checkUser: wrong user="<<user);
    }
    pthread_mutex_unlock(&m_userMutex);
    
    return ret;    
}

//end of file command.cpp

