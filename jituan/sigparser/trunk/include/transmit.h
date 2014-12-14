/***************************************************************************
 fileName    : transmit.h
 begin       : 2010-12-20
 copyright   : (C) 2010 by wyt
 ***************************************************************************/
#ifndef TRANSMIT_H
#define TRANSMIT_H
#include "hwsshare.h"


class Transmit 
{
public:
  Transmit();
  ~Transmit();
  
  static int init();
  static int initHostSocket();
  static int initBackupSocket();
  static int closeSkt();
  static int sendMsg(PUCHAR msg, int len);
  
public:  
  static int m_nPort;
  //for host idpparser_transmit
  static int m_socket;
  static string m_sHostIP;
  static sockaddr_in m_server;
  //for backup idpparser_transmit
  static int m_backupSocket;
  static string m_sBackupIP;
  static sockaddr_in m_backupServer;
};

#endif 


