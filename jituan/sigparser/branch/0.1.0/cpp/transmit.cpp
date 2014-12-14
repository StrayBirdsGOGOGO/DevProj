/***************************************************************************
 fileName    : transmit.cpp
 begin       : 2010-12-20
 copyright   : (C) 2010 by wyt
 ***************************************************************************/
#include "transmit.h"
#include "hwsshare.h"
using namespace std;

int  Transmit::m_socket = 0;
string Transmit::m_sHostIP;
sockaddr_in Transmit::m_server;
int Transmit::m_nPort = 8700;
int  Transmit::m_backupSocket = 0;
string Transmit::m_sBackupIP;
sockaddr_in Transmit::m_backupServer;


Transmit::Transmit(){

}

Transmit::~Transmit(){
    closeSkt();
}

int Transmit::init(){
  m_socket = 0;
  m_server.sin_family = AF_INET;
  m_server.sin_port = htons((short)m_nPort);
  m_server.sin_addr.s_addr = inet_addr(m_sHostIP.c_str());

  m_backupSocket = 0;
  m_backupServer.sin_family = AF_INET;
  m_backupServer.sin_port = htons((short)m_nPort);
  m_backupServer.sin_addr.s_addr = inet_addr(m_sBackupIP.c_str());

  int ret = initHostSocket();
  if(ret<0){
    return -1;
  }
  
  ret = initBackupSocket();
  if(ret<0){
    return -1;
  }
  return 0;
}
int Transmit::initHostSocket(){
  if(m_socket>0){
      close(m_socket);
  }
  m_socket = 0;
  
  if( (m_socket = socket(AF_INET, SOCK_DGRAM, 0) ) == -1 )
  {
      LoggerPtr rootLogger = Logger::getRootLogger();    
      LOG4CXX_ERROR(rootLogger, "create socket() error:"<<strerror(errno));
      return -1;
  }

  return 0;
}


int Transmit::initBackupSocket(){
  if(m_backupSocket>0){
      close(m_backupSocket);
  }
  m_backupSocket = 0;
  
  if( (m_backupSocket = socket(AF_INET, SOCK_DGRAM, 0) ) == -1 )
  {
      LoggerPtr rootLogger = Logger::getRootLogger();    
      LOG4CXX_ERROR(rootLogger, "create socket() error:"<<strerror(errno));
      return -1;
  }

  return 0;
}


int Transmit::sendMsg(PUCHAR msg, int len){
  static UINT i = 0;
  static UINT j = 0;
  static bool connect = true;
  int rc = 0;
  int ret = -1;

  if(m_socket>0 || m_backupSocket>0){
    if(!connect){
        connect = true;
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_INFO(rootLogger, "connect idpparser_test!");
    }
    rc = sendto(m_socket, msg, len+1, 0, (struct sockaddr*)&m_server, sizeof(m_server));
    if(rc<1){
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_ERROR(rootLogger, "Error send msg to host_idpparser_test(fd="<<m_socket<<"), rc="<<rc<<", error:"<<strerror(errno));
        initHostSocket();
    }else{
        i++;
        if(i % 10000 == 0){
            LoggerPtr rootLogger = Logger::getRootLogger();  
            LOG4CXX_INFO(rootLogger, "=>host_idpparser_test(fd="<<m_socket<<"): "<<i<<" msgs, len="<<len+1);
        }
        ret = 1;
    }
    

    if(m_backupSocket>0){
        rc = sendto(m_backupSocket, msg, len+1, 0, (struct sockaddr*)&m_backupServer, sizeof(m_backupServer));
        if(rc<1){
            LoggerPtr rootLogger = Logger::getRootLogger();  
            LOG4CXX_ERROR(rootLogger, "Error send msg to backup_idpparser_test(fd="<<m_backupSocket<<"), rc="<<rc<<", error:"<<strerror(errno));
            initHostSocket();
        }else{
            j++;
            if(j % 10000 == 0){
                LoggerPtr rootLogger = Logger::getRootLogger();  
                LOG4CXX_INFO(rootLogger, "=>backup_idpparser_test(fd="<<m_backupSocket<<"): "<<j<<" msgs, len="<<len+1);
            }
            ret = 1;
        }
    }
  }else{
    if(connect){
        connect = false;
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_ERROR(rootLogger, "No idpparser_test socket!");
        return -1;
    }
  }

  return ret;
}

int Transmit::closeSkt(){
    LoggerPtr rootLogger = Logger::getRootLogger();
    if(m_socket>0){
        if (close(m_socket) == -1) {
          LOG4CXX_ERROR(rootLogger, "close(host_idpparser_test fd="<<m_socket<<") error!");
        } else {
          LOG4CXX_INFO(rootLogger, "close(host_idpparser_test fd="<<m_socket<<") success!");
        }
        m_socket = 0;
    }

    if(m_backupSocket>0){
        if (close(m_backupSocket) == -1) {
          LOG4CXX_ERROR(rootLogger, "close(backup_idpparser_test fd="<<m_backupSocket<<") error!");
        } else {
          LOG4CXX_INFO(rootLogger, "close(backup_idpparser_test fd="<<m_backupSocket<<") success!");
        }
        m_backupSocket = 0;
    }
      
    return 0;
}
//end of file
