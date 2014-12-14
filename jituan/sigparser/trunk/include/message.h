/***************************************************************************
 fileName    : message.h  -  Message class header file
 begin       : 2002-09-17
 copyright   : (C) 2002 by BobHuang
 ***************************************************************************/
#ifndef __MESSAGE_H__
#define __MESSAGE_H__
#include "hwsshare.h"
#include "queue.h"

class ZxClient;

//  ��Ϣ��װ��
class Message: public Node {
  typedef Node super;
public:
  Message();
  virtual ~Message();
  Message(PUCHAR recvMsg,UINT recvLen,ZxClient *pClient);
  
  //������Ϣ(��Ϣͷ+ ��Ϣ��)
  static Message* createInstance(PUCHAR recvMsg,UINT recvLen,ZxClient *pClient);
  //������Ϣ����Ϣͷ����
  int parseMessageHeader();
  //������Ϣ����Ϣ�岿��
  virtual int parseMessageBody();
  //������Ϣ����Ϣͷ����
  void setMessageHeader();
  //������Ϣ����Ϣͷ����
  void setMessageHeader(UINT msgLen);
  //������Ϣ
  virtual int sendMessage();

  PUCHAR getMessageBuf(){return m_msgBuf;}
  //ȡ��Ϣ�ܳ���(��Ϣͷ+ ��Ϣ��)
  UINT getMessageLen(){return m_msgLen;}
  //ȡ��Ϣ����
  UINT	getMessageType(){return m_msgType;}
  void	setMessageType(UINT value){ m_msgType = value; }
  //ȡ��Ϣ��ˮ��
  ULONG	getMessageSeqID(){return m_msgSeqID;}
  void	setMessageSeqID(ULONG value){ m_msgSeqID = value; }
  //ȡ����ID
  UINT	getProcID(){return m_procID;}
  void	setProcID(UINT value){ m_procID = value; }
  
  UINT	getMessageEventNum(){return m_msgEventNum;}
  void	setMessageEventNum(UINT value){ m_msgEventNum = value; }

  void  setZxClient(ZxClient* pClient);
  void  setPackID(UINT value){ m_packID = value; }
  void	setRecvTime();
  
protected:
  ZxClient* m_pZxClient;
  int m_zxSocket;
  int m_zxPort;
  string m_zxHostIP;

  // ��Ϣ= ��Ϣͷ+ ��Ϣ��
  PUCHAR m_msgBuf;
  // ��Ϣ�ܳ���= ��Ϣͷ+ ��Ϣ��(2B)
  UINT m_msgLen;
  //��Ϣ����(2B)
  UINT m_msgType;
  // ��Ϣ������ˮ��(4B)
  ULONG m_msgSeqID;
  // ��Ϣ��Я�����¼���(1B)
  UINT m_msgEventNum;  
  UINT m_packID;

  //����ID
  UINT m_procID;
  //��Ϣ���յ���ʱ��
  ULONG m_recvTime;
};


//�汾Э��������Ϣ
class VerNegoReqMessage:public Message{
public:
	VerNegoReqMessage();
	VerNegoReqMessage(PUCHAR recvMsg,UINT recvLen,ZxClient *pClient);
	~VerNegoReqMessage();
	virtual int parseMessageBody();

};


//�汾Э��Ӧ����Ϣ
class VerNegoRspMessage:public Message{
public:
	VerNegoRspMessage();
	~VerNegoRspMessage();
	void setResult(UINT value){ m_result = value; }
	virtual int sendMessage();

private:
	//���ؽ��(1B)
	UINT m_result;
};


//��·��֤������Ϣ
class LinkAuthReqMessage:public Message{
public:
	LinkAuthReqMessage();
	LinkAuthReqMessage(PUCHAR recvMsg,UINT recvLen,ZxClient *pClient);
	~LinkAuthReqMessage();
	virtual int parseMessageBody();
	int checkRecvDigest(ULONG timeStamp, UINT rand, PUCHAR recvDigest);
	int setSendDigest(ULONG timeStamp, UINT rand, PUCHAR sendDigest);
};


//��·��֤Ӧ����Ϣ
class LinkAuthRspMessage:public Message{
public:
	LinkAuthRspMessage();
	~LinkAuthRspMessage();
	void setResult(UINT value){ m_result = value; }
	int setDigest(PUCHAR value);
	virtual int sendMessage();

private:
	//���ؽ��(1B)
	UINT m_result;
	//MD5���ܽ��(16B)
	UCHAR m_digest[DIGEST_LEN+1];
};


//��·���������Ϣ
class LinkCheckReqMessage:public Message{
public:
	LinkCheckReqMessage();
	LinkCheckReqMessage(PUCHAR recvMsg,UINT recvLen,ZxClient *pClient);
	~LinkCheckReqMessage();
	virtual int parseMessageBody();
};


//��·���Ӧ����Ϣ
class LinkCheckRspMessage:public Message{
public:
	LinkCheckRspMessage();
	~LinkCheckRspMessage();
	virtual int sendMessage();
};


//CDR/TDR����֪ͨ������Ϣ
class CDRReqMessage:public Message{
public:
	CDRReqMessage();
	CDRReqMessage(PUCHAR recvMsg,UINT recvLen,ZxClient *pClient);
	~CDRReqMessage();
	//������Ϣ����Ϣ�岿��
	virtual	int parseMessageBody();
};


//CDR/TDR����֪ͨӦ����Ϣ
class CDRRspMessage:public Message{
public:
	CDRRspMessage();
	~CDRRspMessage();
	void setResult(UINT value){ m_result = value; }
	virtual int sendMessage();

private:
	//���ؽ��(1B)
	UINT m_result;
};


//��·�ͷ�������Ϣ
class LinkRelReqMessage:public Message{
public:
	LinkRelReqMessage();
	LinkRelReqMessage(PUCHAR recvMsg,UINT recvLen,ZxClient *pClient);
	~LinkRelReqMessage();
	virtual int parseMessageBody();
};


//��·�ͷ�Ӧ����Ϣ
class LinkRelRspMessage:public Message{
public:
	LinkRelRspMessage();
	~LinkRelRspMessage();
	void setResult(UINT value){ m_result = value; }
	virtual int sendMessage();

private:
	//���ؽ��(1B)
	UINT m_result;
};


//������Ϣ
class HeartMessage:public Message{
public:
	HeartMessage();
	HeartMessage(PUCHAR recvMsg,UINT recvLen,ZxClient *pClient);
	~HeartMessage();
	virtual int parseMessageBody();
};


//����Ӧ����Ϣ
class HeartAckMessage:public Message{
public:
	HeartAckMessage();
	~HeartAckMessage();
	virtual int sendMessage();
};


//������Ϣ
class DataMessage:public Message{
public:
	DataMessage();
	DataMessage(PUCHAR recvMsg,UINT recvLen,ZxClient *pClient);
	~DataMessage();
	virtual int parseMessageBody();
};


//����Ӧ����Ϣ
class DataAckMessage:public Message{
public:
	DataAckMessage();
	~DataAckMessage();
	virtual int sendMessage();
};


#endif
//end of file message.h

