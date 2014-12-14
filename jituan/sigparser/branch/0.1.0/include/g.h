/***************************************************************************
 fileName    : g.h  -  G class header file
 begin       : 2002-09-17
 copyright   : (C) 2002 by BobHuang
 ***************************************************************************/

#ifndef _G_H_
#define _G_H_ 1

#include "hwsshare.h"
#include "message.h"


//  所有的全局函数都放在该类中
class G {

public:
  G() {
  }
  ;
  ~G() {
  }
  ;


  //系统初始化
  static int initSys();

  //系统复位
  static int resetSys();

  //系统退出前的处理
  static int closeSys();

  //初始化与CRDS的通讯模块
  static int initCommunication();

  //读配置
  static int loadConfig();
  static bool isValidCfgLine(const char* src);
  static bool parserCfg(const char* src, char*& key, char*& value);
  
  static int displayHex(char* prefix, PUCHAR m,int len) ;
  static bool Hex2Str(PUCHAR input, PUCHAR output, int len) ;
  static bool Str2Hex(PUCHAR input, PUCHAR output, int len) ;
  static string int2String(int value);
  static string long2String(ULONG value);
  static string longlong2String(ULONGLONG value);
  
  static int formatString(char* oldStr, int formatLen, char* newStr);
  static int parseLoginString(string src, char*& key, char*& value);

  static char* markPhoneNum(const char* phoneNum, char* out);
  static int isMobilePhone(char* phone);
  static int isTestPhone(char* phone);
  //修改测试号码
  static int changeTestPhones(char* phones);
  // 添加指定测试号码
  static int addTestPhones(char* phones);
  // 删除指定测试号码
  static int delTestPhones(char* phones);
  //修改测试区号
  static int changeTestZone(char* zone);
  // 删除指定测试区号
  static int delTestZone(char* zone);
  
public:
  static bool m_bSysStop;
  static bool m_sysReset;
  static map<string, string> m_props;
  static string m_appName;
  //线程总数
  static int m_threadNum;
  //系统是否处于调试模式
  static int m_isDebug;
  
  static string m_testPhones;
  static pthread_mutex_t m_testPhoneMutex;
  
  static string m_testZone;
  static LoggerPtr logger;
};

#endif

//end of file g.h


