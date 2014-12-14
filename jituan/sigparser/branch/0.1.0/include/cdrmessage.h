/***************************************************************************
 fileName    : cdrmessage.h  -  CDRMessage class header file
 begin       : 2011-06-16
 copyright   : (C) 2011 by wyt
 ***************************************************************************/
#ifndef __CDR_MESSAGE_H__
#define __CDR_MESSAGE_H__
#include "hwsshare.h"
#include "queue.h"

#define BSSAP  0x0001
#define RANAP  0x0002
#define BICC   0x0003
#define H248   0x0004
#define ISUP   0x0007


//ISUP �ɱ��ֶ�
//�ֶ�CIC
const UINT ISUP_MTP_CIC_ID = 1;
const UINT ISUP_M3UA_CIC_ID = 4;
//�ֶ�DPC
const UINT ISUP_MTP_DPC_ID = 2;
const UINT ISUP_M3UA_DPC_ID = 5;
//�ֶ�OPC
const UINT ISUP_MTP_OPC_ID = 3;
const UINT ISUP_M3UA_OPC_ID = 6;

//�ֶ�Called party number
const UINT ISUP_IAM_CALLED_PARTY_NUMBER_ID = 7;       // IAM��Ϣ��Я����ԭ���к���
//const UINT ISUP_RLC_CALLED_PARTY_NUMBER_ID = 101;       // IAM��Ϣ��Я����ԭ���к���
//�ֶ�Calling party number
const UINT ISUP_IAM_CALLING_PARTY_NUMBER_ID = 8;      // IAM��Ϣ��Я����ԭ���к���
//const UINT ISUP_RLC_CALLING_PARTY_NUMBER_ID = 103;      // IAM��Ϣ��Я����ԭ���к���

//�ֶ�Message Type
const UINT ISUP_IAM_MESSAGE_TYPE_ID = 10;                // IAM��Ϣ��Я������Ϣ����
const UINT ISUP_RLC_MESSAGE_TYPE_ID = 14;                // RLC��Ϣ��Я������Ϣ����
/*const UINT ISUP_CPG_MESSAGE_TYPE_ID = 3;                //CPG��Ϣ��Я������Ϣ����
const UINT ISUP_ACM_MESSAGE_TYPE_ID = 6;                // ACM��Ϣ��Я������Ϣ����
const UINT ISUP_REL_MESSAGE_TYPE_ID = 9;                // REL��Ϣ��Я������Ϣ����
const UINT ISUP_CNF_MESSAGE_TYPE_ID = 13;                //Confusion��Ϣ��Я������Ϣ����
const UINT ISUP_FRJ_MESSAGE_TYPE_ID = 15;                //Facility reject��Ϣ��Я������Ϣ����
const UINT ISUP_ANM_MESSAGE_TYPE_ID = 19;                // ANM��Ϣ��Я������Ϣ����
const UINT ISUP_IR_MESSAGE_TYPE_ID = 24;                 //Identification response��Ϣ��Я������Ϣ����
const UINT ISUP_CON_MESSAGE_TYPE_ID = 22;                // CONNECT��Ϣ��Я������Ϣ����
const UINT ISUP_SEG_MESSAGE_TYPE_ID = 32;                // Segmentation��Ϣ��Я������Ϣ����
const UINT ISUP_FAL_MESSAGE_TYPE_ID = 33;                //Facility��Ϣ��Я������Ϣ����
*/
//�ֶ�Generic Number
const UINT ISUP_IAM_GENERIC_NNUMBER_ID = 9;             // IAM��Ϣ��Я����ͨ�ú���
/*const UINT ISUP_CPG_GENERIC_NNUMBER_ID = 2;             // Call progress��Ϣ��Я����ͨ�ú���
const UINT ISUP_ANM_GENERIC_NNUMBER_ID = 18;             // ANM��Ϣ��Я����ͨ�ú���
const UINT ISUP_CON_GENERIC_NNUMBER_ID = 21;             // CONNECT��Ϣ��Я����ͨ�ú���
const UINT ISUP_IR_GENERIC_NNUMBER_ID = 23;              // Identification response��Ϣ��Я����ͨ�ú���
const UINT ISUP_SEG_GENERIC_NNUMBER_ID = 31;             // Segmentation��Ϣ��Я����ͨ�ú���
*/
//�ֶ�Original  Called Number
const UINT ISUP_IAM_ORIGINAL_CALLED_NUMBER_ID = 12;      // IAM��Ϣ��Я����ԭ���к���
//�ֶ�Original  Called IN Number
const UINT ISUP_IAM_ORIGINAL_CALLED_IN_NUMBER_ID = 11;   // IAM��Ϣ��Я����ԭ����IN ����
//�ֶ�Redirecting Number
const UINT ISUP_IAM_REDIRECTING_NUMBER_ID = 13;          // IAM��Ϣ��Я���ĸķ�����
//�ֶ�Redirection Number
/*const UINT ISUP_ACM_REDIRECTION_NUMBER_ID = 7;          // ACM��Ϣ��Я���ĸķ����ĺ���
const UINT ISUP_ANM_REDIRECTION_NUMBER_ID = 20;          // ANM��Ϣ��Я���ĸķ����ĺ���
const UINT ISUP_CPG_REDIRECTION_NUMBER_ID = 4;          // CPG��Ϣ��Я���ĸķ����ĺ���
const UINT ISUP_FAL_REDIRECTION_NUMBER_ID = 34;          // Facility��Ϣ��Я���ĸķ����ĺ���
const UINT ISUP_REL_REDIRECTION_NUMBER_ID = 11;          // REL��Ϣ��Я���ĸķ����ĺ���
//�ֶ�Redirection Information
const UINT ISUP_IAM_REDIRECTION_INFORMATION_ID = 30;     // IAM��Ϣ��Я���ĸķ���Ϣ
const UINT ISUP_REL_REDIRECTION_INFORMATION_ID = 10;     // REL��Ϣ��Я���ĸķ���Ϣ
//�ֶ�Cause Indicators
const UINT ISUP_CPG_CAUSE_INDICATORS_ID = 1;            //CPG��Ϣ��Я����ԭ��ָʾ��
const UINT ISUP_ACM_CAUSE_INDICATORS_ID = 5;            //ACM��Ϣ��Я����ԭ��ָʾ��
const UINT ISUP_CNF_CAUSE_INDICATORS_ID = 12;            //Confusion��Ϣ��Я����ԭ��ָʾ��
const UINT ISUP_FRJ_CAUSE_INDICATORS_ID = 14;            //Facility reject��Ϣ��Я����ԭ��ָʾ��
const UINT ISUP_REL_CAUSE_INDICATORS_ID = 8;            // REL��Ϣ��Я����ԭ��ָʾ��
const UINT ISUP_RLC_CAUSE_INDICATORS_ID = 16;            // RLC��Ϣ��Я����ԭ��ָʾ��
*/


//BICC �ɱ��ֶ�
//�ֶ�Message Type
/*const UINT BICC_CPG_MESSAGE_TYPE_ID = 37;                //CPG��Ϣ��Я������Ϣ����
const UINT BICC_ACM_MESSAGE_TYPE_ID = 40;                // ACM��Ϣ��Я������Ϣ����
const UINT BICC_REL_MESSAGE_TYPE_ID = 43;                // REL��Ϣ��Я������Ϣ����
const UINT BICC_CNF_MESSAGE_TYPE_ID = 47;                //Confusion��Ϣ��Я������Ϣ����
const UINT BICC_FRJ_MESSAGE_TYPE_ID = 49;                //Facility reject��Ϣ��Я������Ϣ����
const UINT BICC_ANM_MESSAGE_TYPE_ID = 53;                // ANM��Ϣ��Я������Ϣ����
const UINT BICC_RLC_MESSAGE_TYPE_ID = 51;                // RLC��Ϣ��Я������Ϣ����
const UINT BICC_IR_MESSAGE_TYPE_ID = 58;                 //Identification response��Ϣ��Я������Ϣ����
const UINT BICC_CON_MESSAGE_TYPE_ID = 56;                // CONNECT��Ϣ��Я������Ϣ����
const UINT BICC_IAM_MESSAGE_TYPE_ID = 60;                // IAM��Ϣ��Я������Ϣ����
const UINT BICC_SEG_MESSAGE_TYPE_ID = 65;                // Segmentation��Ϣ��Я������Ϣ����
const UINT BICC_FAL_MESSAGE_TYPE_ID = 66;                //Facility��Ϣ��Я������Ϣ����
*/
//�ֶ�Generic Number
const UINT BICC_IAM_GENERIC_NNUMBER_ID = 15;             // IAM��Ϣ��Я����ͨ�ú���
/*const UINT BICC_CPG_GENERIC_NNUMBER_ID = 36;             // Call progress��Ϣ��Я����ͨ�ú���
const UINT BICC_ANM_GENERIC_NNUMBER_ID = 52;             // ANM��Ϣ��Я����ͨ�ú���
const UINT BICC_CON_GENERIC_NNUMBER_ID = 55;             // CONNECT��Ϣ��Я����ͨ�ú���
const UINT BICC_IR_GENERIC_NNUMBER_ID = 57;              // Identification response��Ϣ��Я����ͨ�ú���
const UINT BICC_SEG_GENERIC_NNUMBER_ID = 64;             // Segmentation��Ϣ��Я����ͨ�ú���
*/
//�ֶ�Original  Called Number
const UINT BICC_IAM_ORIGINAL_CALLED_NUMBER_ID = 16;      // IAM��Ϣ��Я����ԭ���к���
//�ֶ�Redirecting Number
const UINT BICC_IAM_REDIRECTING_NUMBER_ID = 17;          // IAM��Ϣ��Я���ĸķ�����
//�ֶ�Redirection Number
const UINT BICC_REL_REDIRECTION_NUMBER_ID = 18;          // REL��Ϣ��Я���ĸķ����ĺ���
/*const UINT BICC_ANM_REDIRECTION_NUMBER_ID = 54;          // ANM��Ϣ��Я���ĸķ����ĺ���
const UINT BICC_CPG_REDIRECTION_NUMBER_ID = 38;          // CPG��Ϣ��Я���ĸķ����ĺ���
const UINT BICC_FAL_REDIRECTION_NUMBER_ID = 67;          // Facility��Ϣ��Я���ĸķ����ĺ���
const UINT BICC_ACM_REDIRECTION_NUMBER_ID = 41;          // ACM��Ϣ��Я���ĸķ����ĺ���
//�ֶ�Redirection Information
const UINT BICC_IAM_REDIRECTION_INFORMATION_ID = 63;     // IAM��Ϣ��Я���ĸķ���Ϣ
const UINT BICC_REL_REDIRECTION_INFORMATION_ID = 44;     // REL��Ϣ��Я���ĸķ���Ϣ
//�ֶ�Cause Indicators
const UINT BICC_CPG_CAUSE_INDICATORS_ID = 35;            //CPG��Ϣ��Я����ԭ��ָʾ��
const UINT BICC_ACM_CAUSE_INDICATORS_ID = 39;            //ACM��Ϣ��Я����ԭ��ָʾ��
const UINT BICC_CNF_CAUSE_INDICATORS_ID = 46;            //Confusion��Ϣ��Я����ԭ��ָʾ��
const UINT BICC_FRJ_CAUSE_INDICATORS_ID = 48;            //Facility reject��Ϣ��Я����ԭ��ָʾ��
const UINT BICC_REL_CAUSE_INDICATORS_ID = 42;            // REL��Ϣ��Я����ԭ��ָʾ��
const UINT BICC_RLC_CAUSE_INDICATORS_ID = 50;            // RLC��Ϣ��Я����ԭ��ָʾ��
*/



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
	//����BICC��
	int parseBICC_Message(PUCHAR pData);
	//����BICCЭ��Ĺ̶�����(74B)
	int parseBICC_FixLoad(PUCHAR pData);
	
	//����ISUP��
	int parseISUP_Message(PUCHAR pData);
	//����ISUPЭ��Ĺ̶�����(16B)
	int parseISUP_FixLoad(PUCHAR pData);
	//������ѡ����
	
	int parseOptLoad(PUCHAR pData);
	//������ѡ�����е���Ϣ��Ԫ
	int parseIE(PUCHAR pData, UINT& len);	
	//�����Ϣ��������ʱ
	int checkDelay();
	
	PUCHAR getCaller(){ return m_caller; }
	PUCHAR getCalled(){ return m_called; }
	ULONGLONG getCDRID(){ return m_CDRID; }
	ULONG getStartTime(){ return m_startTime[0]; }
	ULONG getEndTime(){ return m_endTime[0]; }
	UINT getStatus(){ return m_status; }
	PUCHAR getSeqNum(){ return m_seqNum; }
	int setSeqNum();
	void  setRecvTime(ULONG value){ m_recvTime = value; }
	void  setPackID(UINT value){ m_packID = value; }
	UINT getPackID(){ return m_packID; }
	void  setProtocolType(UINT value){ m_protocolType = value; }
	UINT getProtocolType(){ return m_protocolType; }
	PUCHAR getDPC(){ return m_dpc; }
	PUCHAR getOPC(){ return m_opc; }
    void  setMatched(bool value){ m_bMatched = value; }
    bool ifMatched(){ return m_bMatched; }
    void  setSend(bool value){ m_bSend = value; }
    bool ifSend(){ return m_bSend; }
	
	void readTime(PUCHAR data, ULONG* lTime);
	int readBICC_IP(PUCHAR data);
	int readBICC_PC(PUCHAR data);
	int readISUP_PC(PUCHAR data, UINT dataLen, UINT dataID);
	int readISUP_CIC(PUCHAR data, UINT dataLen, UINT dataID);
    int readMessageType(PUCHAR data, UINT dataLen, UINT dataID);
	int readCalledPartyNumber(PUCHAR data, UINT dataLen, UINT dataID);
    int readCallingPartyNumber(PUCHAR data, UINT dataLen, UINT dataID);
	int readGenericNumber(PUCHAR data, UINT dataLen, UINT dataID);
    int readOriginalCalledNumber(PUCHAR data, UINT dataLen, UINT dataID);
    int readOriginalCalledInNumber(PUCHAR data, UINT dataLen, UINT dataID);
	int readRedirectingNumber(PUCHAR data, UINT dataLen, UINT dataID);
    int readRedirectionNumber(PUCHAR data, UINT dataLen, UINT dataID);
    int readRedirectionInfor(PUCHAR data, UINT dataLen, UINT dataID);
    int readCauseInd(PUCHAR data, UINT dataLen, UINT dataID);	

    void getBICC_CallNumber(PUCHAR dst, PUCHAR src, UINT len);
    void setCallNumber(PUCHAR dst, PUCHAR src, UINT len);
    void setGenericNumber(PUCHAR dst, PUCHAR src, UINT len);
    int checkCallNumber(PUCHAR callNumber);
	int setMatchedCall(CDRMessage* pMessage);

private:
	ZxClient* m_pZxClient;
	int m_zxSocket;
	string m_zxHostIP;
	int m_zxPort;
	// ��Ϣ������ˮ��(4B)
	ULONG m_msgSeqID;	

	//notify CDR/TDR data: 
	//CDRLength(2B) + CDRVersion(1B) + VariableIENumber(2B) + ProtocolType(2B) + CDRID(8B)
	//+ FixedLoad(74B) + OptionalLoad(?)
	UCHAR m_CDRBuf[CDR_DATA_MAX_LEN];
	//CDR/TDR�����ܳ���(2B)
	UINT m_CDRLen;
	//CDR/TDR�汾�ţ���4λ��ʾ���汾����4λΪ�Ӱ汾(1B)
	UINT m_CDRVersion;
	//CDR�а����Ŀɱ�IE����Ŀ(2B)
	UINT m_VarIENumber;
	//��ʶЭ������(2B) 0x0001:BSSAP  0x0002:RANAP  0x0003:BICC 0x0004:H.248 0x0007:ISUP
	UINT m_protocolType;
	//ƽ̨���ɵ�ȫ��Ψһ��CDR��ʶ(8B)
	ULONGLONG m_CDRID;

    //��Ϣ���յ���ʱ��
	ULONG m_recvTime;
    //�¼���ʼʱ��(8B): sec(4B) + nSec(4B),�������¼��ĵ�һ�������ʱ�� 
	ULONG m_startTime[2];
    //�¼�����ʱ��(8B): sec(4B) + nSec(4B),����ǰ�ؼ�״̬���Ӧ��������Ϣ��ʱ��
	ULONG m_endTime[2];
	// Call Instance Code ����ʵ����(4B): ԭʼ��������ʽ
	ULONG m_CIC;
	UCHAR m_opc[9];
	UCHAR m_dpc[9];
	
	// Release Reason(1B)
	UINT m_relReason;
	UCHAR m_caller[PHONE_NUMBER_LEN+1];
	UCHAR m_called[PHONE_NUMBER_LEN+1];	
	//״̬�ؼ���(1B)
	UINT m_status;
	UCHAR m_srcIP[9];
	UCHAR m_dstIP[9];
    //��ʶͬһ���е����к�: CIC-StartTime
	UCHAR m_seqNum[MAX_SEQ_NUM_LEN+1];
	UINT m_packID;
    // IAM: �Ƿ�����ͬһ���е�Closure��ƥ��
	bool m_bMatched;
	//�Ƿ��ѷ��͵�sugw�ͻ���
	bool m_bSend;
};


#endif
//end of file cdrmessage.h


