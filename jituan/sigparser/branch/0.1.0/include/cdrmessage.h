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


//ISUP 可变字段
//字段CIC
const UINT ISUP_MTP_CIC_ID = 1;
const UINT ISUP_M3UA_CIC_ID = 4;
//字段DPC
const UINT ISUP_MTP_DPC_ID = 2;
const UINT ISUP_M3UA_DPC_ID = 5;
//字段OPC
const UINT ISUP_MTP_OPC_ID = 3;
const UINT ISUP_M3UA_OPC_ID = 6;

//字段Called party number
const UINT ISUP_IAM_CALLED_PARTY_NUMBER_ID = 7;       // IAM消息中携带的原被叫号码
//const UINT ISUP_RLC_CALLED_PARTY_NUMBER_ID = 101;       // IAM消息中携带的原被叫号码
//字段Calling party number
const UINT ISUP_IAM_CALLING_PARTY_NUMBER_ID = 8;      // IAM消息中携带的原被叫号码
//const UINT ISUP_RLC_CALLING_PARTY_NUMBER_ID = 103;      // IAM消息中携带的原被叫号码

//字段Message Type
const UINT ISUP_IAM_MESSAGE_TYPE_ID = 10;                // IAM消息中携带的消息类型
const UINT ISUP_RLC_MESSAGE_TYPE_ID = 14;                // RLC消息中携带的消息类型
/*const UINT ISUP_CPG_MESSAGE_TYPE_ID = 3;                //CPG消息中携带的消息类型
const UINT ISUP_ACM_MESSAGE_TYPE_ID = 6;                // ACM消息中携带的消息类型
const UINT ISUP_REL_MESSAGE_TYPE_ID = 9;                // REL消息中携带的消息类型
const UINT ISUP_CNF_MESSAGE_TYPE_ID = 13;                //Confusion消息中携带的消息类型
const UINT ISUP_FRJ_MESSAGE_TYPE_ID = 15;                //Facility reject消息中携带的消息类型
const UINT ISUP_ANM_MESSAGE_TYPE_ID = 19;                // ANM消息中携带的消息类型
const UINT ISUP_IR_MESSAGE_TYPE_ID = 24;                 //Identification response消息中携带的消息类型
const UINT ISUP_CON_MESSAGE_TYPE_ID = 22;                // CONNECT消息中携带的消息类型
const UINT ISUP_SEG_MESSAGE_TYPE_ID = 32;                // Segmentation消息中携带的消息类型
const UINT ISUP_FAL_MESSAGE_TYPE_ID = 33;                //Facility消息中携带的消息类型
*/
//字段Generic Number
const UINT ISUP_IAM_GENERIC_NNUMBER_ID = 9;             // IAM消息中携带的通用号码
/*const UINT ISUP_CPG_GENERIC_NNUMBER_ID = 2;             // Call progress消息中携带的通用号码
const UINT ISUP_ANM_GENERIC_NNUMBER_ID = 18;             // ANM消息中携带的通用号码
const UINT ISUP_CON_GENERIC_NNUMBER_ID = 21;             // CONNECT消息中携带的通用号码
const UINT ISUP_IR_GENERIC_NNUMBER_ID = 23;              // Identification response消息中携带的通用号码
const UINT ISUP_SEG_GENERIC_NNUMBER_ID = 31;             // Segmentation消息中携带的通用号码
*/
//字段Original  Called Number
const UINT ISUP_IAM_ORIGINAL_CALLED_NUMBER_ID = 12;      // IAM消息中携带的原被叫号码
//字段Original  Called IN Number
const UINT ISUP_IAM_ORIGINAL_CALLED_IN_NUMBER_ID = 11;   // IAM消息中携带的原被叫IN 号码
//字段Redirecting Number
const UINT ISUP_IAM_REDIRECTING_NUMBER_ID = 13;          // IAM消息中携带的改发号码
//字段Redirection Number
/*const UINT ISUP_ACM_REDIRECTION_NUMBER_ID = 7;          // ACM消息中携带的改发到的号码
const UINT ISUP_ANM_REDIRECTION_NUMBER_ID = 20;          // ANM消息中携带的改发到的号码
const UINT ISUP_CPG_REDIRECTION_NUMBER_ID = 4;          // CPG消息中携带的改发到的号码
const UINT ISUP_FAL_REDIRECTION_NUMBER_ID = 34;          // Facility消息中携带的改发到的号码
const UINT ISUP_REL_REDIRECTION_NUMBER_ID = 11;          // REL消息中携带的改发到的号码
//字段Redirection Information
const UINT ISUP_IAM_REDIRECTION_INFORMATION_ID = 30;     // IAM消息中携带的改发信息
const UINT ISUP_REL_REDIRECTION_INFORMATION_ID = 10;     // REL消息中携带的改发信息
//字段Cause Indicators
const UINT ISUP_CPG_CAUSE_INDICATORS_ID = 1;            //CPG消息中携带的原因指示语
const UINT ISUP_ACM_CAUSE_INDICATORS_ID = 5;            //ACM消息中携带的原因指示语
const UINT ISUP_CNF_CAUSE_INDICATORS_ID = 12;            //Confusion消息中携带的原因指示语
const UINT ISUP_FRJ_CAUSE_INDICATORS_ID = 14;            //Facility reject消息中携带的原因指示语
const UINT ISUP_REL_CAUSE_INDICATORS_ID = 8;            // REL消息中携带的原因指示语
const UINT ISUP_RLC_CAUSE_INDICATORS_ID = 16;            // RLC消息中携带的原因指示语
*/


//BICC 可变字段
//字段Message Type
/*const UINT BICC_CPG_MESSAGE_TYPE_ID = 37;                //CPG消息中携带的消息类型
const UINT BICC_ACM_MESSAGE_TYPE_ID = 40;                // ACM消息中携带的消息类型
const UINT BICC_REL_MESSAGE_TYPE_ID = 43;                // REL消息中携带的消息类型
const UINT BICC_CNF_MESSAGE_TYPE_ID = 47;                //Confusion消息中携带的消息类型
const UINT BICC_FRJ_MESSAGE_TYPE_ID = 49;                //Facility reject消息中携带的消息类型
const UINT BICC_ANM_MESSAGE_TYPE_ID = 53;                // ANM消息中携带的消息类型
const UINT BICC_RLC_MESSAGE_TYPE_ID = 51;                // RLC消息中携带的消息类型
const UINT BICC_IR_MESSAGE_TYPE_ID = 58;                 //Identification response消息中携带的消息类型
const UINT BICC_CON_MESSAGE_TYPE_ID = 56;                // CONNECT消息中携带的消息类型
const UINT BICC_IAM_MESSAGE_TYPE_ID = 60;                // IAM消息中携带的消息类型
const UINT BICC_SEG_MESSAGE_TYPE_ID = 65;                // Segmentation消息中携带的消息类型
const UINT BICC_FAL_MESSAGE_TYPE_ID = 66;                //Facility消息中携带的消息类型
*/
//字段Generic Number
const UINT BICC_IAM_GENERIC_NNUMBER_ID = 15;             // IAM消息中携带的通用号码
/*const UINT BICC_CPG_GENERIC_NNUMBER_ID = 36;             // Call progress消息中携带的通用号码
const UINT BICC_ANM_GENERIC_NNUMBER_ID = 52;             // ANM消息中携带的通用号码
const UINT BICC_CON_GENERIC_NNUMBER_ID = 55;             // CONNECT消息中携带的通用号码
const UINT BICC_IR_GENERIC_NNUMBER_ID = 57;              // Identification response消息中携带的通用号码
const UINT BICC_SEG_GENERIC_NNUMBER_ID = 64;             // Segmentation消息中携带的通用号码
*/
//字段Original  Called Number
const UINT BICC_IAM_ORIGINAL_CALLED_NUMBER_ID = 16;      // IAM消息中携带的原被叫号码
//字段Redirecting Number
const UINT BICC_IAM_REDIRECTING_NUMBER_ID = 17;          // IAM消息中携带的改发号码
//字段Redirection Number
const UINT BICC_REL_REDIRECTION_NUMBER_ID = 18;          // REL消息中携带的改发到的号码
/*const UINT BICC_ANM_REDIRECTION_NUMBER_ID = 54;          // ANM消息中携带的改发到的号码
const UINT BICC_CPG_REDIRECTION_NUMBER_ID = 38;          // CPG消息中携带的改发到的号码
const UINT BICC_FAL_REDIRECTION_NUMBER_ID = 67;          // Facility消息中携带的改发到的号码
const UINT BICC_ACM_REDIRECTION_NUMBER_ID = 41;          // ACM消息中携带的改发到的号码
//字段Redirection Information
const UINT BICC_IAM_REDIRECTION_INFORMATION_ID = 63;     // IAM消息中携带的改发信息
const UINT BICC_REL_REDIRECTION_INFORMATION_ID = 44;     // REL消息中携带的改发信息
//字段Cause Indicators
const UINT BICC_CPG_CAUSE_INDICATORS_ID = 35;            //CPG消息中携带的原因指示语
const UINT BICC_ACM_CAUSE_INDICATORS_ID = 39;            //ACM消息中携带的原因指示语
const UINT BICC_CNF_CAUSE_INDICATORS_ID = 46;            //Confusion消息中携带的原因指示语
const UINT BICC_FRJ_CAUSE_INDICATORS_ID = 48;            //Facility reject消息中携带的原因指示语
const UINT BICC_REL_CAUSE_INDICATORS_ID = 42;            // REL消息中携带的原因指示语
const UINT BICC_RLC_CAUSE_INDICATORS_ID = 50;            // RLC消息中携带的原因指示语
*/



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
	//解析BICC包
	int parseBICC_Message(PUCHAR pData);
	//解析BICC协议的固定部分(74B)
	int parseBICC_FixLoad(PUCHAR pData);
	
	//解析ISUP包
	int parseISUP_Message(PUCHAR pData);
	//解析ISUP协议的固定部分(16B)
	int parseISUP_FixLoad(PUCHAR pData);
	//解析可选部分
	
	int parseOptLoad(PUCHAR pData);
	//解析可选部分中的信息单元
	int parseIE(PUCHAR pData, UINT& len);	
	//检查消息包有无延时
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
	// 消息交互流水号(4B)
	ULONG m_msgSeqID;	

	//notify CDR/TDR data: 
	//CDRLength(2B) + CDRVersion(1B) + VariableIENumber(2B) + ProtocolType(2B) + CDRID(8B)
	//+ FixedLoad(74B) + OptionalLoad(?)
	UCHAR m_CDRBuf[CDR_DATA_MAX_LEN];
	//CDR/TDR数据总长度(2B)
	UINT m_CDRLen;
	//CDR/TDR版本号，高4位表示主版本，低4位为子版本(1B)
	UINT m_CDRVersion;
	//CDR中包含的可变IE总数目(2B)
	UINT m_VarIENumber;
	//标识协议类型(2B) 0x0001:BSSAP  0x0002:RANAP  0x0003:BICC 0x0004:H.248 0x0007:ISUP
	UINT m_protocolType;
	//平台生成的全局唯一的CDR标识(8B)
	ULONGLONG m_CDRID;

    //消息包收到的时间
	ULONG m_recvTime;
    //事件开始时间(8B): sec(4B) + nSec(4B),即呼叫事件的第一条信令的时间 
	ULONG m_startTime[2];
    //事件结束时间(8B): sec(4B) + nSec(4B),即当前关键状态点对应的信令消息的时间
	ULONG m_endTime[2];
	// Call Instance Code 呼叫实例码(4B): 原始信令编码格式
	ULONG m_CIC;
	UCHAR m_opc[9];
	UCHAR m_dpc[9];
	
	// Release Reason(1B)
	UINT m_relReason;
	UCHAR m_caller[PHONE_NUMBER_LEN+1];
	UCHAR m_called[PHONE_NUMBER_LEN+1];	
	//状态关键点(1B)
	UINT m_status;
	UCHAR m_srcIP[9];
	UCHAR m_dstIP[9];
    //标识同一呼叫的序列号: CIC-StartTime
	UCHAR m_seqNum[MAX_SEQ_NUM_LEN+1];
	UINT m_packID;
    // IAM: 是否已与同一呼叫的Closure包匹配
	bool m_bMatched;
	//是否已发送到sugw客户端
	bool m_bSend;
};


#endif
//end of file cdrmessage.h


