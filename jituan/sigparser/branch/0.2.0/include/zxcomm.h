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
  //�������˿ͻ���
  static int addZxClient(int newFd, int port, char* hostIP);
  //��ȡ�������˿ͻ�����Ϣ
  static string getZxClientsInfo();
  //����������������˿ͻ���
  static void clearZxClients();
  //�ر����������˿ͻ��˵�����
  static void closeZxComm();
  //������������˿ͻ��˵�����
  static void checkZxClients();
  

  //ͳ���������˿ͻ��˵�CDR/TDR��Ϣ������Ƿ����ӳ�
  static void checkRecvMsg();

  static int insert2CDRMap(CDRMessage* pMessage);
  static int ifDuplicateCDR(CDRMessage* pMessage);
  static void checkCDR();
  static void clearCDRMap();

public:
  //��������CDR/TDR��������֪ͨ����Ķ˿�
  static int m_listenDataPort;  
  //�����е����˿ͻ�����
  static int m_nConnectedZxClients;
  static map<int,ZxClient*> m_zxMap;
  static pthread_mutex_t m_zxMapMutex;
  
  //���ÿ���ͻ�������������Ϣ�����߳���
  static int m_msgProcessNum;
  
  // ��Ϣ����ʱʱ��
  static int m_msgDelayLimit;
  // �����������������
  static int m_maxHeartInterval;
  
  //ͳ�Ʋ���: ͳ���������˿ͻ��˵�CDR/TDR��Ϣ
  static ULONG m_nTotalDRs;
  static ULONG m_nDelayDRs;
  static ULONG m_nRepeatDRs;	
  static ULONG m_nVpnDRs;
  static ULONG m_nFmlDRs;

  //�洢�����ѷ��͵�CDR��= IAM+Closure
  //key: seqNum_status, Value: CDRMessage
  static MSGMAP m_cdrMap;
  static pthread_mutex_t m_cdrMapMutex;

  //���͵�sugw�ͻ��˵ĺ�����
  static ULONG m_sendNum;
};

#endif
//end of file zxcomm.h


