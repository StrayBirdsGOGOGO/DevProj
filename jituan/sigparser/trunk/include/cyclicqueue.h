/***************************************************************************
 fileName    : cyclicqueue.h  -  CyclicQueue class header file
 begin       : 2009-08-04
 copyright   : (C) 2009 by BobHuang
 ***************************************************************************/
#ifndef __CYCLICQUEUE_H__
#define __CYCLICQUEUE_H__

#include "control.h"
#include "hwsshare.h"

class DataControl;

class CyclicQueue {
public:
	CyclicQueue();
	virtual ~CyclicQueue();
	void init(char* hostIP);
	DataControl& getDataControl();
	bool isEmpty() const { return m_noPackage; }
	bool isFull() const { return (m_count == m_queueSize); }
	UINT getSize() const {return m_count;}
    int getNextData(PUCHAR pData,UINT& len,UINT& packID);
	int push(PUCHAR data,int len);
	void clear();
	//void print(UINT startPos, UINT endPos);
	//void print(UINT startPos);
		
public:
	static UINT m_queueSize;		  // 队列总大小
	
private:
    UINT m_pushPos;
    UINT m_popPos;            // 下次pop位置
    UINT m_count;             // 有效元素个数
    bool m_noPackage;
	
    DataControl m_dataControl;
	
    PUCHAR m_rawBuff;
	int m_recvPort;
	string m_recvHostIP;
	
	UINT pushCount;
	UINT getCount;
	
	UINT readDataLen;
	long lastTime;
};
#endif
//
