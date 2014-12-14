/***************************************************************************
 fileName    : zxcomm.h  -  Communication class header file
 begin       : 2011-05-26
 copyright   : (C) 2011 by WYT
 ***************************************************************************/

#ifndef __ZX_COMM_H_
#define __ZX_COMM_H_

#include "hwsshare.h"

class CDRMessage;

typedef map<int, ZxClient*> ZXMAP;
typedef map<string, CDRMessage*> MSGMAP;
typedef list<CDRMessage*> MSGLIST;

class ZxComm {
public:
	
  ZxComm();

  ~ZxComm();
  
  static int startCommunication();
  
  static int initZxDataSocket();
  //创建中兴客户端
  static int addZxClient(int newFd, int port, char* hostIP);
  //获取所有中兴客户端信息
  static string getZxClientsInfo();
  //清除创建的所有中兴客户端
  static void clearZxClients();
  //关闭所有与中兴客户端的连接
  static void closeZxComm();
  //检查所有与中兴客户端的连接
  static void checkZxClients();
  

  //统计来自中兴客户端的CDR/TDR消息，检查是否有延迟
  static void checkRecvMsg();

  static int insert2CDRMap(CDRMessage* pMessage);
  static int ifDuplicateCDR(CDRMessage* pMessage);
  static void checkCDR();
  static void clearCDRMap();

public:
  //监听中兴CDR/TDR信令数据通知请求的端口
  static int m_listenDataPort;  
  //连接中的中兴客户端数
  static int m_nConnectedZxClients;
  static map<int,ZxClient*> m_zxMap;
  static pthread_mutex_t m_zxMapMutex;
  
  //针对每个客户端所开启的消息处理线程数
  static int m_msgProcessNum;
  
  // 消息包延时时限
  static int m_msgDelayLimit;
  // 允许心跳包的最大间隔
  static int m_maxHeartInterval;
  
  //统计参数: 统计来自中兴客户端的CDR/TDR消息
  static ULONG m_nTotalDRs;
  static ULONG m_nDelayDRs;
  static ULONG m_nRepeatDRs;	
  static ULONG m_nVpnDRs;
  static ULONG m_nFmlDRs;

  //存储所有已发送的CDR包= IAM+Closure
  //key: seqNum_status, Value: CDRMessage
  static MSGMAP m_cdrMap;
  static pthread_mutex_t m_cdrMapMutex;

  //发送到sugw客户端的呼叫数
  static ULONG m_sendNum;
};

#endif
//end of file zxcomm.h


