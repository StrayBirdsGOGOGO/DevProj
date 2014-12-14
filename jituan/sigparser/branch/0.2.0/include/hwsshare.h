/***************************************************************************
 fileName    : hwsshare.h  -  base share header file for hws
 begin       : 2002-09-17
 copyright   : (C) 2002 by BobHuang
 ***************************************************************************/

#ifndef _HWSSHARE_H_
#define _HWSSHARE_H_

// system basic includes
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <assert.h>

// socket and inet includes
#include <sys/syscall.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
extern "C" {
#include <linux/if.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/socket.h>
}

//log4cxx includes
#include <log4cxx/logmanager.h> 
#include <log4cxx/xml/domconfigurator.h> 
#include <log4cxx/patternlayout.h> 
#include <log4cxx/rolling/rollingfileappender.h> 
#include <log4cxx/rolling/fixedwindowrollingpolicy.h> 
#include <log4cxx/rolling/filterbasedtriggeringpolicy.h> 
#include <log4cxx/filter/levelrangefilter.h> 
#include <log4cxx/helpers/pool.h> 
#include <log4cxx/logger.h> 
#include <log4cxx/propertyconfigurator.h> 
#include <log4cxx/dailyrollingfileappender.h> 
#include <log4cxx/helpers/stringhelper.h> 
#include <log4cxx/helpers/exception.h>

// STD library include
#include <iostream>
#include <string>
#include <list>
#include <map>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>

using namespace std;
using namespace log4cxx; 
using namespace log4cxx::rolling; 
using namespace log4cxx::xml; 
using namespace log4cxx::filter; 
using namespace log4cxx::helpers; 


#if !defined(UCHAR)
typedef unsigned char            UCHAR; 
typedef unsigned char			*PUCHAR;
#endif

#if !defined(UINT)
typedef unsigned int             UINT; 
typedef unsigned int			*PUINT;
#endif

#if  !defined(ULONG)
typedef unsigned long      		 ULONG ;
typedef unsigned long	        *PULONG;
#endif

#if !defined(ULONGLONG)
typedef unsigned long long       ULONGLONG;
typedef unsigned long long      *PULONGLONG;
#endif


#define inaddrr(x) (*(struct in_addr *) &ifr.x[sizeof(sa.sin_port)])
#define MIN(a, b)               (((a) < (b)) ? (a) : (b))
#define MAX(a, b)               (((a) > (b)) ? (a) : (b))
#define msleep(x) usleep(x*1000)


/*
#define gOsLog                                                                                  \
  do                                                                                            \
  {                                                                                             \
      long logTime;                                                                             \
      struct tm *logtm;                                                                         \
      time(&logTime);                                                                           \
      logtm =localtime(&logTime);                                                               \
      gMyLog<< logtm->tm_year+1900<<"-"<<logtm->tm_mon+1<<"-"<<logtm->tm_mday<<" "              \
          <<logtm->tm_hour<<":"<<logtm->tm_min<<":"<<logtm->tm_sec<<"  "<<__FILE__<<"("         \
          <<__LINE__<<")  ";                                                                    \
  } while(0);                                                                                   \
      gMyLog
*/


#define HTTP_REQHEAD  "%s%s HTTP/1.1\r\n" \
                                        "Host: %s\r\n"\
                                        "Accept: */*\r\n"\
                                        "User-Agent: Mozilla/4.0 (compatible; MSIE 5.00; Windows 98)\r\n"\
                                        "Connection: Keep-Alive\r\n"

//CAP协议
const UCHAR TCAP_BEGIN_TAG					= 0x62;				//TCAP 消息开始标识
const UCHAR TCAP_CONTINUE_TAG				= 0x65;				//消息继续标识
const UCHAR TCAP_END_TAG				    = 0x64;				//消息结束标识
const UCHAR TCAP_ABORT_TAG					= 0x67;				//消息终止标识

const UCHAR ORIG_TRANSACTIONO_ID_TAG		= 0x48;				//起源事务处理ID标识
const UCHAR DEST_TRANSACTIONO_ID_TAG		= 0x49;				//目的事务处理ID标识
const UCHAR DESTINATION_ROUT_ADDRESS_TAG	= 0xA0;				//目的路由地址标识(被叫全号)
const UCHAR GENERIC_NUMBER_TAG				= 0xAE;				//通用号码标识(主叫全号)

const UCHAR INITIAL_DP						= 0x00;				//操作类型--启动DP
const UCHAR CONNECT							= 0x14;				//操作类型--连接
const UCHAR RELEASECALL						= 0x16;				//操作类型--释放呼叫
const UCHAR RRBE							= 0x17;				//操作类型--请求报告BCSM事件
const UCHAR ERB      						= 0x18;				//操作类型--BCSM事件报告
const UCHAR CONTINUE						= 0x1F;				//操作类型-- 继续
const UCHAR AC  					        = 0x23;				//操作类型-- 申请计费
const UCHAR ACR						        = 0x24;				//操作类型-- 申请计费报告

const UCHAR CALLER_NUMBER_TAG			    = 0x83;				//主叫参数标签
const UCHAR CALLED_NUMBER_TAG			    = 0x82;				//被叫参数标签
const UCHAR CALLED_BCD_NUMBER_TAG		    = 0x9f; 			//被叫BCD参数标签
const UCHAR TAG_EXTEND_BYTE				    = 0x38;
const UCHAR CALLED_TRANSFER_TAG	            = 0x8c; 			//转接参数标签

const int USER_ADDRESS_LENGTH 		= 11;          //用户地址长度

//SDTP协议
#define verNego_Req                 0x0001
#define verNego_Resp                0x8001
#define linkAuth_Req                0x0002
#define linkAuth_Resp               0x8002
#define linkCheck_Req               0x0003
#define linkCheck_Resp              0x8003
#define linkRel_Req                 0x0004
#define linkRel_Resp                0x8004
#define notifyEventData_Req         0x0006
#define notifyEventData_Resp        0x8006
#define notifyCDRData_Req           0x0007
#define notifyCDRData_Resp          0x8007
#define querySignalingData_Req      0x000B
#define querySignalingData_Resp     0x800B

const UINT MAX_MSG_LEN              = 61;           //消息最大长度
const UINT MIN_MSG_LEN              = 13;           //消息最小长度
const UINT MSG_HEAD_LEN             = 10;           //消息头长度(同步头+ 长度)
const UINT HEART_MSG_BODY_LEN       = 3;            //心跳消息长度(不含消息头)
const UINT HEART_ACK_MSG_BODY_LEN   = 3;            //心跳应答消息长度(不含消息头)
const UINT MAX_DATA_MSG_BODY_LEN    = 51;           //数据消息最大长度(不含消息头)
const UINT DATA_ACK_MSG_BODY_LEN    = 6;           //数据应答消息长度(不含消息头)
const UINT HEART_MSG_TYPE           = 0;
const UINT DATA_MSG_TYPE            = 1;

const UINT MAX_SDTP_MSG_LEN         = 61;          //SDTP协议消息最大长度
const UINT MIN_SDTP_MSG_LEN         = 13;           //SDTP协议消息最小长度
//const UINT MSG_HEAD_LEN             = 10;                     //SDTP协议消息包头长度
const UINT VERNEGO_REQ_MSG_LEN      = 11;          //版本协商请求消息长度
const UINT VERNEGO_RSP_MSG_LEN      = 10;          //版本协商应答消息长度
const UINT LINKAUTH_REQ_MSG_LEN     = 43;          //链路认证请求消息长度
const UINT LINKAUTH_RSP_MSG_LEN     = 26;          //链路认证应答消息长度
const UINT LINKCHECK_REQ_MSG_LEN    = 9;           //链路检测请求消息长度
const UINT LINKCHECK_RSP_MSG_LEN    = 9;           //链路检测应答消息长度
const UINT LINKREL_REQ_MSG_LEN      = 10;          //链路释放请求消息长度
const UINT LINKREL_RSP_MSG_LEN      = 10;          //链路释放应答消息长度
const UINT CDR_RSP_MSG_LEN          = 10;          //CDR/TDR数据通知应答消息长度
const UINT CDR_DATA_MAX_LEN         = 800;         //1个CDR/TDR数据长度
const UINT VARIABLE_IE_MAX_LEN      = 35;          //可变IE数据最大长度

const UINT LOGINID_LEN              = 12;          //账号ID长度
const UINT DIGEST_LEN               = 16;          //MD5加密结果长度
const UINT PHONE_NUMBER_LEN 	    = 32;         //用户号码长度
const UINT MAX_PHONE_NUMBER_LEN 	= 11;         //用户号码最大长度
const UINT MIN_PHONE_NUMBER_LEN 	= 3;         //用户号码最小长度

//for command
const int MAX_PARAM_LEN  = 100;
const int MAX_COMMAND_LEN  = 1000;
const int MAX_TEST_PHONES_LEN = 550;


//send to sugw
const UINT MAX_SEQ_NUM_LEN   = 13;
const UINT MIN_SEND_DATA_LEN = 15;
const UINT MAX_SEND_DATA_LEN = 31;

const UINT IAM = 1;
const UINT ACM = 2;
const UINT ANM = 3;
const UINT Closure = 9;
const UINT PeriodicTime = 10;   //一次通话中定时生成CDR/TDR的时间间隔，取值1~24（单位：小时）
const UINT ElapsedTime = 11;    //CDR/TDR生成的门限值，取值1~60（单位：分钟）


//中兴客户端状态
enum Status4ZxClient{
	Close = 0,		  //通信失败,  关闭socket	 
	Connect,	 	  //socket连接成功
	NegoSuc,	 	  //已通过版本协商
	AuthSuc 	 	  //已通过鉴权
};

#endif
//end of file hwsshare.h


