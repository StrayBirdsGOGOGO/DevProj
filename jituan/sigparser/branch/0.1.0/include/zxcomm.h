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
  //���CDR/TDR��Ϣ�Ƿ����ӳ�
  //static void ifDRMsgDelay();
  //����CDR/TDR��Ϣ�ӳٸ澯
  //static void sendDelayAlert();
  //���CDR/TDR��Ϣ�ӳٸ澯
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
  //��������CDR/TDR��������֪ͨ����Ķ˿�
  static int m_listenDataPort;  
  //�����е����˿ͻ�����
  static int m_nConnectedZxClients;
  static map<int,ZxClient*> m_zxMap;
  static pthread_mutex_t m_zxMapMutex;
  
  //���ÿ���ͻ�������������Ϣ�����߳���
  static int m_msgProcessNum;
  
  //Ӧ��ϵͳƽ̨��ʶ
  static string m_sysID;
  static string m_sysPwd;
  
  //����ʱ����ƽ̨�����ϵͳ�˺�(12B)
  //12B
  static string m_subscrbLoginID;
  static string m_subscrbPwd;
  
  //����CDR/TDR���������ƽ̨���ɵ�ȫ��Ψһ�Ķ��ı�ʶ��
  //8B
  static ULONGLONG m_CDROrderID; 

  //�����˿ͻ��˵�ͨ��Э��汾
  //1B
  static UINT m_version;
  //1B
  static UINT m_subVersion;
  
  // call start(IAM)��call end(Closure)��Ϣ��������ʱ��
  static int m_beginEndInterval;  
  // ��Ϣ����ʱʱ��
  static int m_msgDelayLimit;

  //static bool m_bDelayAlert;
  //1����delay���ֵ
  //�ڲ���ʱ: �¼���¼���ɵ���ʱ(�¼���ʼʱ��- �¼�����ʱ�� )
  //static UINT m_nMaxInnerDelay;
  //�ⲿ��ʱ: �¼���¼�յ�����ʱ(�յ�ʱ��- �¼�����ʱ�� )
  //static UINT m_nMaxOuterDelay;
  
  //ͳ�Ʋ���: ͳ���������˿ͻ��˵�CDR/TDR��Ϣ
  static ULONG m_nTotalDRs;
  static ULONG m_nDelayDRs;
  static ULONG m_nVpnDRs;
  static ULONG m_nFmlDRs;
  static ULONG m_nRepeatDRs;	
  
  //�洢�����ѷ��͵�CDR��= IAM+Closure
  //key: seqNum_status, Value: CDRMessage
  static MSGMAP m_cdrMap;
  static pthread_mutex_t m_cdrMapMutex;
  //�洢δƥ���Closure��
  static pthread_mutex_t m_closureListMutex;
  static MSGLIST m_closureList;
};

#endif
//end of file zxcomm.h


