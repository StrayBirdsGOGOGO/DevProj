/***************************************************************************
 fileName    : g.h  -  G class header file
 begin       : 2002-09-17
 copyright   : (C) 2002 by BobHuang
 ***************************************************************************/

#ifndef _G_H_
#define _G_H_ 1

#include "hwsshare.h"
#include "message.h"


//  ���е�ȫ�ֺ��������ڸ�����
class G {

public:
  G() {
  }
  ;
  ~G() {
  }
  ;


  //ϵͳ��ʼ��
  static int initSys();

  //ϵͳ��λ
  static int resetSys();

  //ϵͳ�˳�ǰ�Ĵ���
  static int closeSys();

  //��ʼ����CRDS��ͨѶģ��
  static int initCommunication();

  //������
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
  //�޸Ĳ��Ժ���
  static int changeTestPhones(char* phones);
  // ���ָ�����Ժ���
  static int addTestPhones(char* phones);
  // ɾ��ָ�����Ժ���
  static int delTestPhones(char* phones);
  //�޸Ĳ�������
  static int changeTestZone(char* zone);
  // ɾ��ָ����������
  static int delTestZone(char* zone);
  
public:
  static bool m_bSysStop;
  static bool m_sysReset;
  static map<string, string> m_props;
  static string m_appName;
  //�߳�����
  static int m_threadNum;
  //ϵͳ�Ƿ��ڵ���ģʽ
  static int m_isDebug;
  
  static string m_testPhones;
  static pthread_mutex_t m_testPhoneMutex;
  
  static string m_testZone;
  static LoggerPtr logger;
};

#endif

//end of file g.h


