#include <stdio.h> 
#include <string> 
#include <stdlib.h>
#include <sys/vfs.h>
#include <unistd.h>
#include "hwsshare.h"
#include "comm.h"
#include "appmonitor.h"
#include "g.h"
using namespace std;

int getDisk();
void getCPU();
static int dogCount = 0;
static int disk = 0;
static int cpu = 0;
ULONGLONG userOld = 0;
ULONGLONG niceOld = 0;
ULONGLONG sysOld = 0;
ULONGLONG idleOld = 0;
ULONGLONG iowaitOld = 0;
char statPath[256];

AppMonitor* AppMonitor::m_instance=0;


AppMonitor::AppMonitor(){

}

AppMonitor::~AppMonitor(){
  if(m_socket>0){
    close(m_socket); 
  }
}
void AppMonitor::getServerIP(){
  char *pAddress;
  char *pPort;
  char *mPath;

  mPath = getenv("APPMS_PATH");
  if(mPath){
    strncpy(statPath, mPath, 256);
  }else{
    statPath[0]='/';
    statPath[1]=0;
  }
  
  m_nIpAddress = INADDR_NONE;
  m_nPort = 0;
  pAddress = getenv("APPMS_IP");
  if(pAddress){
    m_nIpAddress = inet_addr(pAddress);
  }
  pPort = getenv("APPMS_PORT");
  if(pPort){
    m_nPort = atoi(pPort);
  }
  
  if (m_nIpAddress == INADDR_NONE){
    m_nIpAddress = inet_addr(Comm::m_monitorHostIP.c_str());
  }
  if(m_nPort<1){
    m_nPort = Comm::m_monitorPort;
  } 

  m_server.sin_family = AF_INET;
  m_server.sin_port = htons((short)m_nPort);
  m_server.sin_addr.s_addr = m_nIpAddress;
}

void AppMonitor::init(){
  getServerIP();
  m_socket = socket(AF_INET, SOCK_DGRAM, 0);
}

AppMonitor* AppMonitor::getInstance(){
  if(0==m_instance){
    m_instance = new AppMonitor();
    m_instance->init();
    userOld = 0;
    getDisk();
    getCPU();
  }
  return m_instance;
}

  /**
  * 定时心跳，超过1分钟后端监控系统没收到心跳，会发送告警短信给指定人员
  * @param appName 应用程序名称
  */
void AppMonitor::watchDog(const char* appName){
  
  if(dogCount%2 == 0){
    getDisk();
    getCPU();
    char diskBuf[16];
    sprintf(diskBuf,"%d",disk);
    char cpuBuf[16];
    sprintf(cpuBuf,"%d",cpu);

    string msg = "<type>dog</type><app>"+string(appName)+"</app><cpu>"+string(cpuBuf)+"</cpu><disk>"+string(diskBuf)+"</disk>";
    sendMsg(msg.c_str());
  }else{
    string msg = "<type>dog</type><app>"+string(appName)+"</app>";
    sendMsg(msg.c_str());
  }
  dogCount++;
}
  
  /**
   *  发送告警日志到后端监控平台
   * @param appName 应用程序名称
   * @param msg 要记录的消息
   * @param level 0-9，数字越大，等级越高，level>=7需要发送短信告警
   */
void AppMonitor::alert(const char* appName,const char* msg,int level){
	long now = 0;
	time(&now);
    char levelBuf[16];
    sprintf(levelBuf,"%d",level);
    string info = "<type>alert</type><app>"+string(appName)+"</app><info>"+string(msg)+"</info><level>"+string(levelBuf)+"</level>";
    sendMsg(info.c_str());
    LoggerPtr rootLogger = Logger::getRootLogger();
    LOG4CXX_WARN(rootLogger, "alert: "<<msg<<" now="<<now);    
}


void AppMonitor::alert(const char* appName,const char* errMsg, int isErr,const char* msg,int level){
	long now = 0;
	time(&now);
    char levelBuf[16];
    sprintf(levelBuf,"%d",level);
    string info = "<type>alert</type><app>"+string(appName)+"</app><info>"+string(msg)+"</info><level>"
        +string(levelBuf)+"</level><errMsg>"+string(errMsg)+"</errMsg><isErr>"+G::int2String(isErr)+"</isErr>";
    sendMsg(info.c_str());
    LoggerPtr rootLogger = Logger::getRootLogger();
    LOG4CXX_WARN(rootLogger, msg);    
    LOG4CXX_WARN(rootLogger, "alert: "<<errMsg<<" "<<isErr<<" "<<now);    
}
  

void AppMonitor::sendMsg(const char* msg){
  int rc = 0;
  rc = sendto(m_socket, msg, strlen(msg), 0, (struct sockaddr*)&m_server, sizeof(m_server));
}

int getDisk(){
  struct statfs buf;
  memset(&buf, 0, sizeof(buf));
  statfs(statPath, &buf);
  //disk = 100-(int)(buf.f_bavail*100/buf.f_blocks);
  disk = 100-(int)(buf.f_bfree*100/buf.f_blocks);
  return disk;
}

void getCPU(){
  FILE *fp;
  char buffer[128],name[15];
  ULONGLONG user =0;
  ULONGLONG  nice = 0;
  ULONGLONG  sys=0;
  ULONGLONG  idle = 0;
  ULONGLONG  iowait = 0;
  ULONGLONG  sum =0;
  
  if( (fp=fopen("/proc/stat","r") )==NULL) {
    printf("Can not open file!");
    return;
  }
  fgets (buffer, sizeof(buffer),fp);
  fclose(fp);
  sscanf (buffer, "%s  %lld  %lld  %lld  %lld  %lld", name, &user,&nice,&sys, &idle,&iowait);
  if(0 < userOld){
        sum = (user + nice + sys + idle + iowait) - (userOld + niceOld + sysOld + idleOld + iowaitOld);
        if(sum >0){
          long long sum2 = (user + sys + nice) - (userOld + sysOld + niceOld);
          cpu = sum2*100/sum;
        }
  }
  userOld = user;
  niceOld = nice;
  sysOld = sys;
  idleOld = idle;
  iowaitOld = iowait;
  
  return;
}
//end of file
