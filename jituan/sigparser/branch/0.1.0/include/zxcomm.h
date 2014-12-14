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
  //检查CDR/TDR消息是否有延迟
  //static void ifDRMsgDelay();
  //发送CDR/TDR消息延迟告警
  //static void sendDelayAlert();
  //清除CDR/TDR消息延迟告警
  //static void clearDelayAlert();

  static int insert2CDRMap(CDRMessage* pMessage);
  static int findMatchedIAM(CDRMessage* pMessage);
  static int ifDuplicateCDR(CDRMessage* pMessage);
  static void checkCDR();
  static void clearCDRMap();

  static int insert2ClosureList(CDRMessage* pMessage);
  static void checkClosureList();
  static void clearClosureList();

public:
  //监听中兴CDR/TDR信令数据通知请求的端口
  static int m_listenDataPort;  
  //连接中的中兴客户端数
  static int m_nConnectedZxClients;
  static map<int,ZxClient*> m_zxMap;
  static pthread_mutex_t m_zxMapMutex;
  
  //针对每个客户端所开启的消息处理线程数
  static int m_msgProcessNum;
  
  //应用系统平台标识
  static string m_sysID;
  static string m_sysPwd;
  
  //订阅时共享平台分配的系统账号(12B)
  //12B
  static string m_subscrbLoginID;
  static string m_subscrbPwd;
  
  //订阅CDR/TDR后由信令共享平台生成的全局唯一的订阅标识码
  //8B
  static ULONGLONG m_CDROrderID; 

  //与中兴客户端的通信协议版本
  //1B
  static UINT m_version;
  //1B
  static UINT m_subVersion;
  
  // call start(IAM)与call end(Closure)消息包间的最大时差
  static int m_beginEndInterval;  
  // 消息包延时时限
  static int m_msgDelayLimit;

  //static bool m_bDelayAlert;
  //1秒内delay最大值
  //内部延时: 事件记录生成的延时(事件开始时间- 事件生成时间 )
  //static UINT m_nMaxInnerDelay;
  //外部延时: 事件记录收到的延时(收到时间- 事件生成时间 )
  //static UINT m_nMaxOuterDelay;
  
  //统计参数: 统计来自中兴客户端的CDR/TDR消息
  static ULONG m_nTotalDRs;
  static ULONG m_nDelayDRs;
  static ULONG m_nVpnDRs;
  static ULONG m_nFmlDRs;
  static ULONG m_nRepeatDRs;	
  
  //存储所有已发送的CDR包= IAM+Closure
  //key: seqNum_status, Value: CDRMessage
  static MSGMAP m_cdrMap;
  static pthread_mutex_t m_cdrMapMutex;
  //存储未匹配的Closure包
  static pthread_mutex_t m_closureListMutex;
  static MSGLIST m_closureList;
};

#endif
//end of file zxcomm.h


