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

//  消息封装类
class Message: public Node {
  typedef Node super;
public:
  Message();
  virtual ~Message();
  Message(PUCHAR recvMsg,UINT recvLen,ZxClient *pClient);
  
  //解析消息(消息头+ 消息体)
  static Message* createInstance(PUCHAR recvMsg,UINT recvLen,ZxClient *pClient);
  //解析消息的消息体部分
  virtual int parseMessageBody();
  //设置消息的消息头部分
  void setMessageHeader(UINT msgLen);
  //发送消息
  virtual int sendMessage();

  PUCHAR getMessageBuf(){return m_msgBuf;}
  //取消息总长度(消息头+ 消息体)
  UINT getMessageLen(){return m_msgLen;}
  //取消息类型
  UINT	getMessageType(){return m_msgType;}
  void	setMessageType(UINT value){ m_msgType = value; }
  //取消息流水号
  ULONG	getMessageSeqID(){return m_msgSeqID;}
  void	setMessageSeqID(ULONG value){ m_msgSeqID = value; }
  //取进程ID
  UINT	getProcID(){return m_procID;}
  void	setProcID(UINT value){ m_procID = value; }

  void  setZxClient(ZxClient* pClient);
  void  setPackID(UINT value){ m_packID = value; }
  void	setRecvTime();
  
protected:
  ZxClient* m_pZxClient;
  int m_zxSocket;
  int m_zxPort;
  string m_zxHostIP;

  // 消息= 消息头+ 消息体
  PUCHAR m_msgBuf;
  // 消息总长度= 消息头+ 消息体(2B)
  UINT m_msgLen;
  //消息类型(2B)
  UINT m_msgType;
  // 消息交互流水号(4B)
  ULONG m_msgSeqID;
  UINT m_packID;

  //进程ID
  UINT m_procID;
  //消息包收到的时间
  ULONG m_recvTime;
};


//心跳消息
class HeartMessage:public Message{
public:
	HeartMessage();
	HeartMessage(PUCHAR recvMsg,UINT recvLen,ZxClient *pClient);
	~HeartMessage();
	virtual int parseMessageBody();
};


//心跳应答消息
class HeartAckMessage:public Message{
public:
	HeartAckMessage();
	~HeartAckMessage();
	virtual int sendMessage();
};


//心跳消息
class DataMessage:public Message{
public:
	DataMessage();
	DataMessage(PUCHAR recvMsg,UINT recvLen,ZxClient *pClient);
	~DataMessage();
	virtual int parseMessageBody();
};


//心跳应答消息
class DataAckMessage:public Message{
public:
	DataAckMessage();
	~DataAckMessage();
	virtual int sendMessage();
};


#endif
//end of file message.h

