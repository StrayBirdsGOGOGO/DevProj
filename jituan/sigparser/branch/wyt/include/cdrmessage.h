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

//版本协商请求消息
class CDRMessage:public Node{
	typedef Node super;
public:
	CDRMessage();
	~CDRMessage();
	//设置CDR 数据
	void setMessageBuf(PUCHAR pData, UINT dataLen);
	//取消息流水号
	ULONG  getMessageSeqID(){return m_msgSeqID;}
	void  setMessageSeqID(ULONG value){ m_msgSeqID = value; }
	void  setZxClient(ZxClient* pClient);
	
	//解析CDR 数据
	int parseMessage();
	//检查消息包有无延时
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
	// 消息交互流水号(4B)
	ULONG m_msgSeqID;	
	bool m_bSend;

	//notify CDR/TDR data: 
	//CDRLength(2B) + CDRVersion(1B) + VariableIENumber(2B) + ProtocolType(2B) + CDRID(8B)
	//+ FixedLoad(74B) + OptionalLoad(?)
	UCHAR m_CDRBuf[CDR_DATA_MAX_LEN];
	//CDR/TDR数据总长度(2B)
	UINT m_CDRLen;

	//消息包收到的时间
	ULONG m_recvTime;
    //事件开始时间(8B): sec(4B) + nSec(4B),即呼叫事件的第一条信令的时间 
	ULONG m_startTime[2];
    //事件结束时间(8B): sec(4B) + nSec(4B),即当前关键状态点对应的信令消息的时间
	ULONG m_endTime[2];
	
	UCHAR m_caller[PHONE_NUMBER_LEN+1];
	UCHAR m_called[PHONE_NUMBER_LEN+1];	
	//状态关键点(1B)
	UINT m_status;
    //区别从中兴接收的呼叫的序列号: CIC-StartTime
	UCHAR m_callSeqNum[MAX_SEQ_NUM_LEN+1];
	//区别发送给sugw 的呼叫的序列号
	ULONG m_callID;
	UINT m_packID;
	
	//进程ID
	UINT m_procID;
};


#endif
//end of file cdrmessage.h


