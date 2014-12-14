/***************************************************************************
 fileName    : g.cpp  -  G class cpp file
 begin       : 2002-09-17
 copyright   : (C) 2002 by BobHuang
 ***************************************************************************/

#include "g.h"
#include "hwsqueue.h"
#include "comm.h"
#include "command.h"
#include "message.h"
#include "cdrmessage.h"
#include "cyclicqueue.h"
#include "appmonitor.h"
#include "sugwserver.h"
#include "zxcomm.h"

#define BIN2CH(b)               ((char)(((b)<10)?'0'+(b):'a'-10+(b)))

//queue for process msg
HwsQueue gWaitQueue;
//queue for send msg
HwsQueue gSendSugwQueue;

bool G::m_bSysStop = false;
bool G::m_sysReset = false;
map<string, string> G::m_props;

string G::m_appName;
int G::m_threadNum = 0;
LoggerPtr G::logger = Logger::getLogger("G");

//for test
string G::m_testPhones = "";
pthread_mutex_t G::m_testPhoneMutex = PTHREAD_MUTEX_INITIALIZER;
string G::m_testZone = "";
int G::m_isDebug = 0;


//系统初始化
int G::initSys() {
  //load the port config
  int ret = loadConfig();
  if(-1 == ret){
    m_bSysStop = true;
    return -1;
  }

  ret = Command::initCommandProcess();
  if(-1 == ret){
    m_bSysStop = true;
    return -1;
  }
  
  ret = initCommunication();
  if(-1 == ret){
      m_bSysStop = true;
      return -1;
  }
  return 0;
}


//系统复位
int G::resetSys() {
  LoggerPtr rootLogger = Logger::getRootLogger();

  LOG4CXX_INFO(rootLogger, "Start to reset....");
  m_bSysStop = true;
  return 0;
}

//系统退出前的处理
int G::closeSys() {
  LoggerPtr rootLogger = Logger::getRootLogger();
  LOG4CXX_INFO(rootLogger, "CloseSys, start clean....");
  
  Command::closeCommandSocket();
  SugwServer::removeSigSockList();
  
  DataControl& rDataControl1 = gWaitQueue.getDataControl();
  rDataControl1.deactivate();
  DataControl& rDataControl2 = gSendSugwQueue.getDataControl();
  rDataControl2.deactivate();

  int count = 0;
  while (m_threadNum>0){
    sleep(1);
    if(++count==5){
        break;
    }
  }
  
  ZxComm::clearZxClients();
  ZxComm::clearCDRMap();

  LOG4CXX_INFO(rootLogger, "End clean.");
  return 0;
}


//初始化与CRDS的通讯模块
int G::initCommunication() {
  int ret = 0;
  
  ret = SugwServer::initSugwSocket();
  if(-1 == ret){
      G::m_bSysStop = true;
      return -1;
  }

  ret = ZxComm::startCommunication();
  if(-1 == ret){
      G::m_bSysStop = true;
      return -1;
  }

  return 0;
}


int G::loadConfig() {
  char buff[1024];
  char keyBuff[512];
  char valueBuff[1024];
  char * strFileName = "app.cfg";
  char* pKey = keyBuff;
  char* pValue = valueBuff;
  int iValue = 0;
  string sValue = "";
  int ret = 0;

  LoggerPtr rootLogger = Logger::getRootLogger();
  ifstream cfgStream(strFileName);
  if (!cfgStream.is_open()) {
    LOG4CXX_ERROR(rootLogger, "Unable to open " << strFileName << " for read");
    return -1;
  }

  do {
    cfgStream.getline(buff, 1024);
    if (parserCfg(buff, pKey, pValue)) {
      m_props.insert(map<string, string>::value_type(string(keyBuff),
          string(valueBuff)));
    }
  } while (!cfgStream.eof());

  map<string, string>::iterator itr = m_props.begin();
  int i = 0;
  for (; itr != m_props.end(); itr++) {
    LOG4CXX_INFO(rootLogger, "prop's(" << i++ << ") key=" << itr->first.c_str()
        << " ,value = " << itr->second.c_str());
  }

  itr = m_props.find("APPNAME");
  if(itr != m_props.end()){
    m_appName = itr->second;
  }else{
    LOG4CXX_ERROR(rootLogger, "Can't find APPNAME in app.cfg !");
    return -1;
  }

  itr = m_props.find("IS_DEBUG");
  if(itr != m_props.end()){
      iValue = atoi(itr->second.c_str());
      m_isDebug = iValue;
  }else{
      LOG4CXX_ERROR(rootLogger, "Can't find IS_DEBUG in app.cfg !");
      return -1;
  }

  itr = m_props.find("LISTEN_HOST_IP");
  if(itr != m_props.end()){
    sValue = "";
    sValue = itr->second;
    if(-1 == Comm::checkHostIP(sValue)){
        LOG4CXX_ERROR(rootLogger, "wrong LISTEN_HOST_IP = "<<sValue);
        return -1;
    }else{
        Comm::m_listenHostIP = sValue;
    }
  }else{
    LOG4CXX_ERROR(rootLogger, "Can't find LISTEN_HOST_IP in app.cfg !");
    return -1;
  }  

  itr = m_props.find("TEST_PHONES");
  if(itr != m_props.end()){
    m_testPhones = itr->second;
  }

  itr = m_props.find("TEST_ZONE");
  if(itr != m_props.end()){
    m_testZone = itr->second;
  }
  
  //For command line
  itr = m_props.find("COMMAND_LISTEN_PORT");
  if(itr != m_props.end()){
    iValue = atoi(itr->second.c_str());
    ret = Comm::checkPort(iValue);
    if(-1 == ret){
        LoggerPtr rootLogger = Logger::getRootLogger();
        LOG4CXX_ERROR(rootLogger, "wrong COMMAND_LISTEN_PORT="<<iValue);
        return -1;
    }
    Command::m_listenPort = iValue;
  }else{
    LOG4CXX_ERROR(rootLogger, "Can't find COMMAND_LISTEN_PORT in app.cfg !");
    return -1;
  }

  itr = m_props.find("COMMAND_LISTEN_HOST_IP");
  if(itr != m_props.end()){
    sValue = "";
    sValue = itr->second;
    ret = Comm::checkHostIP(sValue);
    if(-1 == ret){
        LoggerPtr rootLogger = Logger::getRootLogger();
        LOG4CXX_ERROR(rootLogger, "wrong COMMAND_LISTEN_HOST_IP="<<sValue);
        return -1;
    }
    Command::m_listenHostIP = sValue;
  }else{
    LOG4CXX_ERROR(rootLogger, "Can't find COMMAND_LISTEN_HOST_IP in app.cfg !");
    return -1;
  }

  itr = m_props.find("COMMAND_LOGIN_USER_NUM");
  if(itr != m_props.end()){
    iValue = atoi(itr->second.c_str());
    if(iValue<1){
        LOG4CXX_ERROR(rootLogger, "wrong COMMAND_LOGIN_USER_NUM="<<iValue);
        return -1;
    }else{
        Command::m_userNum = iValue;
        itr = m_props.find("COMMAND_LOGIN_USER");
        if(itr != m_props.end()){
          sValue = "";
          sValue = itr->second;
          ret = Command::getLoginUsers(sValue);
          if(-1 == ret){
              LOG4CXX_ERROR(rootLogger, "wrong COMMAND_LOGIN_USER = "<<sValue);
              return -1;
          }       
        }else{
            LoggerPtr rootLogger = Logger::getRootLogger();
            LOG4CXX_ERROR(rootLogger, "Can't find COMMAND_LOGIN_USER in app.cfg !");
            return -1;
        }
    }
  }else{
      LoggerPtr rootLogger = Logger::getRootLogger();
      LOG4CXX_ERROR(rootLogger, "Can't find COMMAND_LOGIN_USER_NUM in app.cfg !");
      return -1;
  }

  //For AppMonitor
  itr = m_props.find("MONITOR_PORT");
  if(itr != m_props.end()){
    Comm::m_monitorPort = atoi(itr->second.c_str());
  }else{
    LOG4CXX_ERROR(rootLogger, "Can't find MONITOR_PORT in app.cfg !");
    return -1;
  }

  itr = m_props.find("MONITOR_HOST_IP");
  if(itr != m_props.end()){
    Comm::m_monitorHostIP = itr->second;
  }else{
    LOG4CXX_ERROR(rootLogger, "Can't find MONITOR_HOST_IP in app.cfg !");
    return -1;
  }

  //For SugwServer
  itr = m_props.find("LISTEN_SUGW_PORT");
  if(itr != m_props.end()){
    iValue = atoi(itr->second.c_str());
    if(-1 == Comm::checkPort(iValue)){
        LOG4CXX_ERROR(rootLogger, "wrong LISTEN_SUGW_PORT = "<<iValue);
        return -1;
    }else{
        SugwServer::m_listenPort = iValue;
    }
  }else{
    LOG4CXX_ERROR(rootLogger, "Can't find LISTEN_SUGW_PORT in app.cfg !");
    return -1;
  }

  //For ZxComm
  itr = m_props.find("LISTEN_ZX_DATA_PORT");
  if(itr != m_props.end()){
    iValue = atoi(itr->second.c_str());
    if(-1 == Comm::checkPort(iValue)){
        LOG4CXX_ERROR(rootLogger, "wrong LISTEN_ZX_DATA_PORT = "<<iValue);
        return -1;
    }else{
        ZxComm::m_listenDataPort = iValue;
    }
  }else{
    LOG4CXX_ERROR(rootLogger, "Can't find LISTEN_ZX_DATA_PORT in app.cfg !");
    return -1;
  }

  itr = m_props.find("MSG_PROCESS_THREAD_NUM");
  if(itr != m_props.end()){
    ZxComm::m_msgProcessNum = atoi(itr->second.c_str());
  }else{
    LOG4CXX_ERROR(rootLogger, "Can't find MSG_PROCESS_THREAD_NUM in app.cfg !");
    return -1;
  }

  itr = m_props.find("BUFF_QUEUE_SIZE");
  if(itr != m_props.end()){
      iValue = atoi(itr->second.c_str());
      if(iValue < 1024000){
          LOG4CXX_ERROR(rootLogger, "too small BUFF_QUEUE_SIZE = "<<iValue);
          return -1;
      }else{
          CyclicQueue::m_queueSize = iValue;
      }
  }else{
      LOG4CXX_ERROR(rootLogger, "Can't find BUFF_QUEUE_SIZE in app.cfg !");
      return -1;
  }


  itr = m_props.find("PACKAGE_DELAY_LIMIT");
  if(itr != m_props.end()){
    ZxComm::m_msgDelayLimit = atoi(itr->second.c_str());
  }else{
    LOG4CXX_ERROR(rootLogger, "Can't find PACKAGE_DELAY_LIMIT in app.cfg !");
    return -1;
  }

  itr = m_props.find("MAX_HEART_INTERVAL");
  if(itr != m_props.end()){
    ZxComm::m_maxHeartInterval = atoi(itr->second.c_str());
  }else{
    LOG4CXX_ERROR(rootLogger, "Can't find MAX_HEART_INTERVAL in app.cfg !");
    return -1;
  }
  return 0;
}

bool G::isValidCfgLine(const char* src) {
  const char* pSrc = src;
  bool findEqualMark = false;
  bool isFirstChar = true;

  while (('\0' != *pSrc) && ('\r' != *pSrc)) {
    if (' ' == *pSrc || '\t' == *pSrc) {
      pSrc++;
      continue;
    }

    if (isFirstChar) {
      isFirstChar = false;
      if (';' == *pSrc || '#' == *pSrc || '=' == *pSrc) {
        return false;
      }
      pSrc++;
    } else {
      if ('=' == *pSrc) {
        findEqualMark = true;
      } else {
        if (findEqualMark) {
          return true;
        }
      }
      pSrc++;
    }
  }//end of while
  return false;
}

bool G::parserCfg(const char* src, char*& key, char*& value) {
  const char * pStr = src;
  bool isKey = true;
  char* pKey = key;
  char* pValue = value;
  *pKey = '\0';
  *pValue = '\0';
  if (isValidCfgLine(src)) {
    while ('\0' != *pStr) {
      if (' ' == *pStr || '\t' == *pStr || '\r' == *pStr || '\n' == *pStr) {
        pStr++;
        continue;
      } else if ('=' == *pStr) {
        pStr++;
        isKey = false;
        *pKey++ = '\0';
        continue;
      }

      if (isKey) {
        *pKey++ = *pStr;
      } else {
        *pValue++ = *pStr;
      }
      pStr++;
    }
    *pValue++ = '\0';
    return true;
  } else {
    return false;
  }
  return true;
}


/*
 * displayHex- displayshex messages
 */
 int G::displayHex(char* prefix, PUCHAR m,int len) {
  char buff[1024];
  int i = 0;
  string msg;
  
  sprintf(buff, "%s \r\n00     ", prefix);
  msg = string(buff);

  while (len--) {
        i++;
        if((i%32) ==0){
            sprintf(buff, "%c%c  \r\n%02x     ", BIN2CH(*m / 16), BIN2CH(*m % 16),i);            
        }else  if((i %16) ==0){
            sprintf(buff, "%c%c  ", BIN2CH(*m / 16), BIN2CH(*m % 16));            
        }else  if((i %8) ==0){
            sprintf(buff, "%c%c ", BIN2CH(*m / 16), BIN2CH(*m % 16));            
        }else{
            sprintf(buff, "%c%c", BIN2CH(*m / 16), BIN2CH(*m % 16));
        }
        m++;
        msg = msg + string(buff);
  }
  LoggerPtr rootLogger = Logger::getRootLogger();
  LOG4CXX_DEBUG(rootLogger, msg);
  return (0);
}


//将16 字节的md5加密值转换为32位的Hex进制字符串
//如:0x1213....19 -> "1213...19"
bool G::Hex2Str(PUCHAR input, PUCHAR output, int len) {
  int i = 0;
  bool bReturn = false;
  int pos = len;

  while (pos--) {
        output[i++] = BIN2CH(*input / 16);
        output[i++] = BIN2CH(*input % 16);
        input++;
  }

  output[i] = 0;
  
  if (i == len * 2)
  {
    bReturn = true;
  }
  
  return bReturn;
}

//将32位的Hex进制字符串转换为16字节
//如:"1213...19" -> 0x1213....19
bool G::Str2Hex(PUCHAR input, PUCHAR output, int len)
{
  bool bReturn = false;
  char cTmp = 0;
  int j = 0;
  
  for (int i = 0; i < len; i += 2, j ++)
  {   
    // 先获取高位字节
    cTmp = input[i];
    if (cTmp >= 0x30 && cTmp <= 0x39) // 数字 0 - 9
    {
      cTmp -= 0x30;
    }else if (cTmp >= 0x61 && cTmp <= 0x66) // a - f
    {
      cTmp -= 0x57;   
    }else if (cTmp >= 0x41 && cTmp <= 0x46) // A - F
    {
      cTmp -= 0x37;  
    }else  
    {
      break; // 不该出现的字符
    }
    
    output[j] = cTmp;  
    output[j] = output[j] << 4; // 变成高位字节，左移4位
    
    // 再获取低位字节
    cTmp = input[i + 1];
    if (cTmp >= 0x30 && cTmp <= 0x39) // 数字 0 - 9
    {
      cTmp -= 0x30;
    }else if (cTmp >= 0x61 && cTmp <= 0x66) // a - f
    {
      cTmp -= 0x57;   
    }else if (cTmp >= 0x41 && cTmp <= 0x46) // A - F
    {
      cTmp -= 0x37;  
    }else  
    {
      break; // 不该出现的字符
    }
    
    output[j] |= cTmp; // 获取低位字节   
  }
  
  if (j == len / 2)
  {
    bReturn = true;
  }
  
  return bReturn;
}


string G::int2String(int value) {
  ostringstream os;
  os << value;
  return os.str();
}

string G::long2String(ULONG value) {
  ostringstream os;
  os << value;
  return os.str();
}

string G::longlong2String(ULONGLONG value) {
  ostringstream os;
  os << value;
  return os.str();
}

int G::isTestPhone(char* phone){
   int rs = 0;
    if( strlen(phone)<MIN_PHONE_NUMBER_LEN || strlen(phone)>MAX_PHONE_NUMBER_LEN)  {
        return 0;
    }

   if(m_testPhones == ""){
       return 0;
   }
   
   string::size_type pos = 0;
   pos = G::m_testPhones.find(phone);
   if (pos != string::npos) {
       rs = 1;
   }
   return rs;
}


//mark 15824104619 to 158****4619
char* G::markPhoneNum(const char* phoneNum, char* out) 
{
    memset(out, 0, sizeof(out));
    if (0 != phoneNum ) 
    {
         int length = MIN((int)strlen(phoneNum),11);
         memcpy(out, phoneNum, length);
         out[3] = '*';
         out[4] = '*';
         out[5] = '*';
         out[6] = '*';
    }else{
         return 0;
    }
    
    return out;
}


//check if phone is china mobile number
//return 0: no
//          1: yes
int G::isMobilePhone(char* phone){
    int rc = 0;

    if(strlen(phone) != 11){
        return rc;
    }

    if(strncmp(phone, "134", 3) == 0){
        rc = 1;
    }else if(strncmp(phone, "135", 3) == 0){
        rc = 1;
    }else if(strncmp(phone, "136", 3) == 0){
        rc = 1;
    }else if(strncmp(phone, "137", 3) == 0){
        rc = 1;
    }else if(strncmp(phone, "138", 3) == 0){
        rc = 1;
    }else if(strncmp(phone, "139", 3) == 0){
        rc = 1;
    }else if(strncmp(phone, "147", 3) == 0){
        rc = 1;
    }else if(strncmp(phone, "148", 3) == 0){
        rc = 1;
    }else if(strncmp(phone, "150", 3) == 0){
        rc = 1;
    }else if(strncmp(phone, "151", 3) == 0){
        rc = 1;
    }else if(strncmp(phone, "152", 3) == 0){
        rc = 1;
    }else if(strncmp(phone, "157", 3) == 0){
        rc = 1;
    }else if(strncmp(phone, "158", 3) == 0){
        rc = 1;
    }else if(strncmp(phone, "159", 3) == 0){
        rc = 1;
    }else if(strncmp(phone, "182", 3) == 0){
        rc = 1;
    }else if(strncmp(phone, "183", 3) == 0){
        rc = 1;
    }else if(strncmp(phone, "187", 3) == 0){
        rc = 1;
    }else if(strncmp(phone, "188", 3) == 0){
        rc = 1;
    }else{
        rc = 0;
    }

    return rc;
}


/*
格式化字符串，将oldStr 填补到规定长度formatLen，
若字节不足，则以空格“ ”补齐，如"user     "
返回
0: 格式化成功
-1:格式化失败
*/
int G::formatString(char* oldStr, int formatLen, char* newStr){
    int len = strlen(oldStr);
    if(len>formatLen){
        LoggerPtr rootLogger = Logger::getRootLogger();  
        LOG4CXX_ERROR(rootLogger, "formatString: overlength str="<<oldStr<<", formatLen="<<formatLen);
        return -1;
    }else{
        memcpy(newStr,oldStr,len);
        for(int i=len;i<formatLen;i++){
            newStr[i] = 0x20;
        }
        newStr[formatLen] = 0;
    }

    return 0;
}

int G::parseLoginString(string src, char*& key, char*& value){
    int ret = 0;
    char* tmpStr = 0;
    int i = 0;
    int len = 0;
    char* pKey = key;
    char* pValue = value;
    *pKey = '\0';
    *pValue = '\0';
    char* pStr = (char*)src.c_str();
    if(!pStr){
        return -1;
    }
    
    tmpStr = strtok(pStr, ":");
    while(tmpStr){     
        if(i == 0){
            len = strlen(tmpStr);
            if(len<1 || len>=MAX_PARAM_LEN){
                return -1;
            }else{
                memcpy(pKey, tmpStr, len);
                pKey[len] = 0;
            }
        }else if(i == 1){
            len = strlen(tmpStr);
            if(len<1 || len>=MAX_PARAM_LEN){
                return -1;
            }else{
                memcpy(pValue, tmpStr, len);
                pValue[len] = 0;
            }
        }else{
            return -1;
        }
            
        i++;
        tmpStr = strtok(NULL, ":");
    }    

    if(i<2){
        ret = -1;
    }

    return ret;
}

//return 0 : change success
//return -1: change failed ,  wong phones
int G::changeTestPhones(char* phones){
   char* tmpStr = 0;
   int len = 0;
   int ret = 0;
   string out = string("");

   //check phones
   if(!phones){
       LoggerPtr rootLogger = Logger::getRootLogger();
       LOG4CXX_ERROR(rootLogger, "changeTestPhones failed, phones is null!");
       return -3;
   }

   len = strlen(phones);
   if(len>MAX_TEST_PHONES_LEN){
       out += "phones is too big!\n";
       ret = Command::commandSend(out);
       return ret;
   }
   
   string ss = string(phones);
  /* tmpStr = strtok(phones, ",");
   while(tmpStr){
     if((strlen(tmpStr) != 11) || (tmpStr[0] != 0x31)){
         out += tmpStr;
         out += ": wrong phone!\n";
         ret = -1;
     }
     tmpStr = strtok(NULL, ",");
   }*/

   //change phones
   if(0 == ret){
       pthread_mutex_lock(&m_testPhoneMutex);   
       m_testPhones = ss;
       pthread_mutex_unlock(&m_testPhoneMutex);
   }else{
       ret = Command::commandSend(out);
   }
   return ret;
}


//return 0 : add success
//return -1: add failed,  wong phones
int G::addTestPhones(char* phones){
    int ret = 0;
    int len = 0;
    int size = 0;
    char* tmpStr = 0;
    string::size_type pos = 0;
    string out = string("");
    
    if(!phones){
        LoggerPtr rootLogger = Logger::getRootLogger();
        LOG4CXX_ERROR(rootLogger, "addTestPhones failed, phones is null!");
        return -3;
    }

    len = strlen(phones);
    size = m_testPhones.length();
    if(len+size > MAX_TEST_PHONES_LEN){
        out += "phones is too big!\n";
        ret = Command::commandSend(out);
        return ret;
    }
    
    pthread_mutex_lock(&m_testPhoneMutex);
    tmpStr = strtok(phones, ",");
    while(tmpStr){
      //check phone        
     /* if((strlen(tmpStr) != 11) || (tmpStr[0] != 0x31)){
          //wrong phone
          out += tmpStr;
          out += ": wrong phone!\n";
      }else{*/
          //add phone
          pos = m_testPhones.find(tmpStr);
          if (pos != string::npos){
              out += tmpStr;
              out += ": existed phone!\n";
          }else{
              if(m_testPhones.size()>0){
                  m_testPhones += ",";
              }
              m_testPhones += tmpStr;
          }
     // }
      
      tmpStr = strtok(NULL, ",");
    }
    pthread_mutex_unlock(&m_testPhoneMutex);

    if(out.length()>0){
        ret = Command::commandSend(out);
    }
    return ret;    
}


//return 0 : delete success
//return -1: delete failed,  wong phones
int G::delTestPhones(char* phones){
    int ret = 0;
    char* tmpStr = 0;
    string::size_type pos = 0;
    string out = string("");
    int len = 0;
    
    if(!phones){
        LoggerPtr rootLogger = Logger::getRootLogger();
        LOG4CXX_ERROR(rootLogger, "delTestPhones failed, phones is null!");
        return -3;
    }
    
    pthread_mutex_lock(&m_testPhoneMutex);
    
    //clear all test phones
    if(0 == strcmp("*", phones)){
        m_testPhones = "";
        return 0;
    }
    
    tmpStr = strtok(phones, ",");
    while(tmpStr){
      pos = 0;
      //check phone
    /*  if((strlen(tmpStr) != 11) || (tmpStr[0] != 0x31)){
          //wrong phone
          out += tmpStr;
          out += "wrong phone!\n";
      }else{*/
          //delete phone
          len = strlen(tmpStr);
          pos = m_testPhones.find(tmpStr);
          if (pos != string::npos){
              if(pos>0){
                  m_testPhones.replace(pos-1,len+1,"");
              }else{
                  m_testPhones.replace(pos,len,"");
              }
          }else{
              out += tmpStr;
              out += "inexisted phone!\n";
          }
    //  }
      
      tmpStr = strtok(NULL, ",");      
    }   
    pthread_mutex_unlock(&m_testPhoneMutex);    

    if(out.length()>0){
        ret = Command::commandSend(out);
    }
    return ret;
}


//return 0 : change success
//return -1: change failed ,  wong phones
int G::changeTestZone(char* zone){
   char* tmpStr = 0;
   int len = 0;
   int ret = 0;
   string out = string("");

   //check phones
   if(!zone){
       LoggerPtr rootLogger = Logger::getRootLogger();
       LOG4CXX_ERROR(rootLogger, "changeTestZone failed, zone is null!");
       return -3;
   }

   len = strlen(zone);
   if(len != 7){
       out += "wrong zone!\n";
       ret = Command::commandSend(out);
       return ret;
   }
   
   //change zone
   m_testZone = string(zone);
   ret = Command::commandSend(out);
   return ret;
}



//return 0 : delete success
//return -1: delete failed,  wong phones
int G::delTestZone(char* zone){
    int ret = 0;
    char* tmpStr = 0;
    string::size_type pos = 0;
    string out = string("");
    int len = 0;
    
    if(!zone){
        LoggerPtr rootLogger = Logger::getRootLogger();
        LOG4CXX_ERROR(rootLogger, "delTestZone failed, zone is null!");
        return -3;
    }
    
    //clear all test phones
    if(0 == strcmp("*", zone)){
        m_testZone = "";
        return 0;
    }else if(0 == strcmp((char*)m_testZone.c_str(), zone)){
        m_testZone = "";
        return 0;
    }else{
        //wrong input
        out += "wrong input!\n";
    }

    if(out.length()>0){
        ret = Command::commandSend(out);
    }
    return ret;
}


//end of g.cpp

