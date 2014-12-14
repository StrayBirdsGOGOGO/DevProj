/***************************************************************************
 fileName    : Comm.h  -  Communication class header file
 begin       : 2002-09-17
 copyright   : (C) 2002 by BobHuang
 ***************************************************************************/

#ifndef Comm_h
#define Comm_h 1

#include "hwsshare.h"
#include "g.h"
#include "message.h"

class ZxClient;
class Comm {
public:
  //Constructors (generated)
  Comm();

  //Destructor (generated)
  ~Comm();

  static int checkPort(int port);
  static int checkHostIP(string hostIP);
  static int closeSocket(int fd, string name);	  
  static int closeSocket(int fd);


public:
  static string m_listenHostIP;

  //for TCP communication with appmonitor server
  static string m_monitorHostIP;
  static int m_monitorPort;
};

#endif
//end of file comm.h

