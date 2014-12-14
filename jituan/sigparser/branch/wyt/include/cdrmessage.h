/***************************************************************************
 fileName    : cdrmessage.h  -  CDRMessage class header file
 begin       : 2011-06-16
 copyright   : (C) 2011 by wyt
 ***************************************************************************/
#ifndef __CDR_MESSAGE_H__
#define __CDR_MESSAGE_H__
#include "hwsshare.h"
#include "queue.h"



class ZxClient;

//�汾Э��������Ϣ
class CDRMessage:public Node{
	typedef Node super;
public:
	CDRMessage();
	~CDRMessage();
	//����CDR ����
	void setMessageBuf(PUCHAR pData, UINT dataLen);
	//ȡ��Ϣ��ˮ��
	ULONG  getMessageSeqID(){return m_msgSeqID;}
	void  setMessageSeqID(ULONG value){ m_msgSeqID = value; }
	void  setZxClient(ZxClient* pClient);
	
	//����CDR ����
	int parseMessage();
	//�����Ϣ��������ʱ
	int checkDelay();
	
	PUCHAR getCaller(){ return m_caller; }
	PUCHAR getCalled(){ return m_called; }
	ULONG getStartTime(){ return m_startTime[0]; }
	ULONG getEndTime(){ return m_endTime[0]; }
	UINT getStatus(){ return m_status; }
	PUCHAR getSeqNum(){ return m_callSeqNum; }
	int setSeqNum();
	ULONG getCallID(){ return m_callID; }
	void setCallID(ULONG value){ m_callID = value; }
	void  setPackID(UINT value){ m_packID = value; }
	UINT getPackID(){ return m_packID; }
    void  setSend(bool value){ m_bSend = value; }
    bool ifSend(){ return m_bSend; }
	int sendMessage();
	
	void readTime(PUCHAR data, ULONG* lTime);
    int readMessageType(PUCHAR data, UINT dataLen, UINT dataID);
    void setCallNumber(PUCHAR dst, PUCHAR src, UINT len);
    int checkCallNumber(PUCHAR callNumber);
	void  setRecvTime(ULONG value){ m_recvTime = value; }
	void  setProcID(UINT value){ m_procID = value; }

private:
	ZxClient* m_pZxClient;
	int m_zxSocket;
	string m_zxHostIP;
	int m_zxPort;
	// ��Ϣ������ˮ��(4B)
	ULONG m_msgSeqID;	
	bool m_bSend;

	//notify CDR/TDR data: 
	//CDRLength(2B) + CDRVersion(1B) + VariableIENumber(2B) + ProtocolType(2B) + CDRID(8B)
	//+ FixedLoad(74B) + OptionalLoad(?)
	UCHAR m_CDRBuf[CDR_DATA_MAX_LEN];
	//CDR/TDR�����ܳ���(2B)
	UINT m_CDRLen;

	//��Ϣ���յ���ʱ��
	ULONG m_recvTime;
    //�¼���ʼʱ��(8B): sec(4B) + nSec(4B),�������¼��ĵ�һ�������ʱ�� 
	ULONG m_startTime[2];
    //�¼�����ʱ��(8B): sec(4B) + nSec(4B),����ǰ�ؼ�״̬���Ӧ��������Ϣ��ʱ��
	ULONG m_endTime[2];
	
	UCHAR m_caller[PHONE_NUMBER_LEN+1];
	UCHAR m_called[PHONE_NUMBER_LEN+1];	
	//״̬�ؼ���(1B)
	UINT m_status;
    //��������˽��յĺ��е����к�: CIC-StartTime
	UCHAR m_callSeqNum[MAX_SEQ_NUM_LEN+1];
	//�����͸�sugw �ĺ��е����к�
	ULONG m_callID;
	UINT m_packID;
	
	//����ID
	UINT m_procID;
};


#endif
//end of file cdrmessage.h


