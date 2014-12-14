#ifndef __ZCXC_CLIENT_H_
#define __ZCXC_CLIENT_H_

#include "hwsshare.h"

//wait response max time limit
const int RESPONSE_TIME_LIMIT = 60;
//resend max times if no reponse
const int RESEND_TIMES = 3;
//max time limit with no data communication 
const int NODATA_TIME_LIMIT = 180;


class CyclicQueue;
class HwsQueue;

class ZxClient{
public:	
	ZxClient();
	~ZxClient();
	
	int init(int fd, int port, char* hostIP);
	//检查与中兴客户端的连接
    int checkComm();
	int sktSend(UINT sendLen, PUCHAR sendBuf);
	string getHostIP(){ return m_zxHostIP; }
	int getPort(){ return m_zxPort; }
	int getSocket(){ return m_zxSocket; }
	CyclicQueue* getRecvMsgQueue(){ return m_recvMsgQueue; }
	HwsQueue* getSendMsgQueue(){ return m_sendMsgQueue; }
	
	Status4ZxClient getStatus();
	void setStatus(Status4ZxClient value);		
	//清除客户端前处理
	void beforeDel();
	//获取客户端所创建的子线程数
	int getThreadNum(){ return m_nThread; }
	void incThreadNum(){ ++m_nThread; }
	void decThreadNum(){ --m_nThread; }

    //统计来自客户端的CDT/TDR消息数, 判断是否延时
    void checkRecvMsg();
	//发送CDR/TDR消息延迟告警
	void sendDelayAlert();
	//清除CDR/TDR消息延迟告警
	void clearDelayAlert();
	
    //增加总的CDT/TDR消息数(来自客户端)
    void incTotalDRMsgs();
    //增加延时的CDT/TDR消息数(来自客户端)
    void incDelayDRMsgs();
	void incIAMDRs();
	void incClosureDRs();
	void setMaxInnerDelay(int value);
	void setMaxOuterDelay(int value);
	
private:
	pthread_mutex_t m_zxSktMutex;   
    int m_zxSocket;
    int m_zxPort;
	string m_zxHostIP;
	int m_nThread;
	
	Status4ZxClient m_status;
	pthread_mutex_t m_statusMutex;   
	
	pthread_t m_tidMsgProcess[20];
	pthread_t m_tidMsgRecv;
	pthread_t m_tidMsgSend;
	//存储来自中兴客户端的消息
	CyclicQueue *m_recvMsgQueue;
	//存储发送到中兴客户端的消息
	HwsQueue *m_sendMsgQueue;

    //用于统计来自该客户端的CDR/TDR消息
    ULONG m_nTotalDRs;
    ULONG m_nDelayDRs;
    ULONG m_nIAM;
    ULONG m_nClosure;
	ULONG m_nLastTotalDRs;
	ULONG m_nLastDelayDRs;
	ULONG m_nLastIAM;
	ULONG m_nLastClosure;

    bool m_bDelayAlert;
    //延时开始时刻
    ULONG m_startDelayTime;
    //上一次延时时刻
    ULONG m_lastDelayTime;
    //延时次数
    ULONG m_nDelayCount;
    //1秒内delay最大值
    //内部延时: 事件记录生成的延时(事件开始时间- 事件生成时间 )
    UINT m_nMaxInnerDelay;
    //外部延时: 事件记录收到的延时(收到时间- 事件生成时间 )
    UINT m_nMaxOuterDelay;
};

#endif
//end of zcxcclient.h


