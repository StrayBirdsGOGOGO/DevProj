/***************************************************************************
 fileName    : Comm.cpp  -  Communication class cpp file
 begin       : 2002-09-17
 copyright   : (C) 2002 by BobHuang
 ***************************************************************************/

#include "comm.h"
#include "hwsqueue.h"
#include "message.h"
#include "g.h"
#include "hwsshare.h"
#include "cyclicqueue.h"
#include "appmonitor.h"
#include "zxclient.h"

string Comm::m_listenHostIP;

string Comm::m_monitorHostIP;
int Comm::m_monitorPort = 0;


Comm::Comm() {
}

Comm::~Comm() {
}


int Comm::checkPort(int port){
    int ret = 0;
    if(port<1024 || port>65535){
        ret = -1;
    }

    return ret;
}


//return 0: correct IP
//return -1: wrong IP
int Comm::checkHostIP(string hostIP){
    if(hostIP.length()<7 || hostIP.length()>15){
        return -1;
    }
    
    struct in_addr addr;
    int ret = inet_pton(AF_INET, hostIP.c_str(), &addr);
    if(ret<1){
        return -1;
    }

    return 0;
}


int Comm::closeSocket(int fd, string name){
    if (fd <1)
    {
      return -1;
    }

    if (close(fd))
    {
      LoggerPtr rootLogger = Logger::getRootLogger();
      LOG4CXX_ERROR(rootLogger, "Error remove "<<name<<", fd="<<fd<<", error:"<<strerror(errno)); 
    }
    else
    {
      LoggerPtr rootLogger = Logger::getRootLogger();
      LOG4CXX_WARN(rootLogger, "remove "<<name<<", fd="<<fd);
    }
    fd = 0;
    return 0;
}


int Comm::closeSocket(int fd){
    if(fd<1){
        return -1;
    }
      
    if (close(fd) == -1) {
        LoggerPtr rootLogger = Logger::getRootLogger();
        LOG4CXX_ERROR(rootLogger, "close(fd="<<fd<<") error!");
    } else {
        LoggerPtr rootLogger = Logger::getRootLogger();
        LOG4CXX_INFO(rootLogger, "close(fd="<<fd<<") success!");
    }
    fd = 0;
    return 0;
}

//end of file comm.cpp

