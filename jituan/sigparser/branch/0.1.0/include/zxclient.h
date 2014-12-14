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
	//��������˿ͻ��˵�����
    int checkComm();
	int sktSend(UINT sendLen, PUCHAR sendBuf);
	string getHostIP(){ return m_zxHostIP; }
	int getPort(){ return m_zxPort; }
	int getSocket(){ return m_zxSocket; }
	CyclicQueue* getRecvMsgQueue(){ return m_recvMsgQueue; }
	HwsQueue* getSendMsgQueue(){ return m_sendMsgQueue; }
	
	Status4ZxClient getStatus();
	void setStatus(Status4ZxClient value);		
	//����ͻ���ǰ����
	void beforeDel();
	//��ȡ�ͻ��������������߳���
	int getThreadNum(){ return m_nThread; }
	void incThreadNum(){ ++m_nThread; }
	void decThreadNum(){ --m_nThread; }

    //ͳ�����Կͻ��˵�CDT/TDR��Ϣ��, �ж��Ƿ���ʱ
    void checkRecvMsg();
	//����CDR/TDR��Ϣ�ӳٸ澯
	void sendDelayAlert();
	//���CDR/TDR��Ϣ�ӳٸ澯
	void clearDelayAlert();
	
    //�����ܵ�CDT/TDR��Ϣ��(���Կͻ���)
    void incTotalDRMsgs();
    //������ʱ��CDT/TDR��Ϣ��(���Կͻ���)
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
	//�洢�������˿ͻ��˵���Ϣ
	CyclicQueue *m_recvMsgQueue;
	//�洢���͵����˿ͻ��˵���Ϣ
	HwsQueue *m_sendMsgQueue;

    //����ͳ�����Ըÿͻ��˵�CDR/TDR��Ϣ
    ULONG m_nTotalDRs;
    ULONG m_nDelayDRs;
    ULONG m_nIAM;
    ULONG m_nClosure;
	ULONG m_nLastTotalDRs;
	ULONG m_nLastDelayDRs;
	ULONG m_nLastIAM;
	ULONG m_nLastClosure;

    bool m_bDelayAlert;
    //��ʱ��ʼʱ��
    ULONG m_startDelayTime;
    //��һ����ʱʱ��
    ULONG m_lastDelayTime;
    //��ʱ����
    ULONG m_nDelayCount;
    //1����delay���ֵ
    //�ڲ���ʱ: �¼���¼���ɵ���ʱ(�¼���ʼʱ��- �¼�����ʱ�� )
    UINT m_nMaxInnerDelay;
    //�ⲿ��ʱ: �¼���¼�յ�����ʱ(�յ�ʱ��- �¼�����ʱ�� )
    UINT m_nMaxOuterDelay;
};

#endif
//end of zcxcclient.h


