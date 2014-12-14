#ifndef INCLUDED_AppMonitor
#define INCLUDED_AppMonitor
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>


class AppMonitor 
{
public:
  AppMonitor();
  ~AppMonitor();
  static AppMonitor* getInstance();

  void watchDog(const char* appName);
  
  void alert(const char* appName,const char* msg,int level);
  void alert(const char* appName,const char* errMsg, int isErr,const char* msg,int level);
private:
  void sendMsg(const char* msg);
    void init();
  void getServerIP();
private:
  ULONG m_nIpAddress;
  int m_nPort;
  int  m_socket;
  struct sockaddr_in m_server;
  static AppMonitor* m_instance;

};

#endif // INCLUDED_AppMonitor


