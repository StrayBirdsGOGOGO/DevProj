/* cyclicqueue.cpp
 */

#include "cyclicqueue.h"
#include "hwsshare.h"
#include "g.h"
#include "appmonitor.h"

UINT CyclicQueue::m_queueSize = 0;

CyclicQueue::CyclicQueue(){
    m_pushPos = 0;
    m_popPos = 0;
    m_count = 0;
    m_noPackage = true;
    m_rawBuff = 0;
    m_recvPort = 0;
    m_recvHostIP = "";
    pushCount = 0;
    getCount = 0;
    readDataLen = 0;
	lastTime = 0;
}

CyclicQueue::~CyclicQueue() {
    delete[] m_rawBuff;
    m_rawBuff = 0;
}

void CyclicQueue::init(char* hostIP){
    m_recvHostIP = string(hostIP);
    m_rawBuff = new UCHAR[m_queueSize];
}

void CyclicQueue::clear(){
    pthread_mutex_lock(&m_dataControl.getMutex());
    m_pushPos = 0;
    m_popPos = 0;
    m_count = 0;
    pushCount = 0;
    getCount = 0;
    m_noPackage = true;
    readDataLen = 0;
	lastTime = 0;
    pthread_mutex_unlock(&m_dataControl.getMutex());
}

int CyclicQueue::push(PUCHAR data,int len) {
    int rc = 0;

    pthread_mutex_lock(&m_dataControl.getMutex());
    if ((len>0)&&(m_count+len) < m_queueSize) {
        int pos = m_pushPos+len;
        if(pos<=m_queueSize){
            memcpy(m_rawBuff+m_pushPos,data,len);
            m_count +=len;
            m_pushPos += len;
            m_pushPos %= m_queueSize; 
        }else{
            int len1 = pos - m_queueSize;
            int len2 = len -len1;
            memcpy(m_rawBuff+m_pushPos,data,len2);
            memcpy(m_rawBuff,data+len2,len1);
            m_count +=len;
            m_pushPos = len1;
        }
        rc =1;

        if(m_count>=MIN_MSG_LEN){
            m_noPackage = false;
        }else{
            m_noPackage = true;
        }      
        
        ++pushCount; 
        if((pushCount % 10000) == 0){
            LoggerPtr rootLogger = Logger::getRootLogger(); 
            LOG4CXX_DEBUG(rootLogger, "("<<m_recvHostIP<<")Push count="<<pushCount<<", m_count="<<m_count);
            //G::displayHex("Push ",data,len);
        }
    }else{
        LoggerPtr rootLogger = Logger::getRootLogger(); 
        LOG4CXX_WARN(rootLogger,"("<<m_recvHostIP<<")In push m_count="<<m_count<<",m_pushPos="<<m_pushPos<<",m_popPos="<<m_popPos<<",len="<<len);
    }
    
    if(0 == readDataLen){
        time(&lastTime);
    }
    readDataLen += len;
    if(readDataLen>=0xA00000){
        //10M
        long now = 0;
        int diffTime = 0;
        time(&now);
        diffTime = (int)now-lastTime;
        lastTime = now;
        if(0 == diffTime){
            diffTime = 1;
        }
        LoggerPtr rootLogger = Logger::getRootLogger(); 
        LOG4CXX_DEBUG(rootLogger,"("<<m_recvHostIP<<")diffTime="<<diffTime<<",data="<<readDataLen<<",speed="<<(readDataLen>>20)/diffTime);
        readDataLen = 0;        
    }

    pthread_mutex_unlock(&m_dataControl.getMutex());
    pthread_cond_broadcast(&m_dataControl.getCond());
    
    return rc;
}


//byte order: big-endian
int  CyclicQueue::getNextData(PUCHAR pData, UINT& len, UINT& packID) {
  int rc = 0;
  UINT  lengthByte[2];
  UCHAR byte;
  UINT startPos = 0;
  UINT msgBodyLen = 0;
  len = 0;
  
  if(pData){
      pthread_mutex_lock(&m_dataControl.getMutex());
      packID = m_popPos;
      startPos = m_popPos;
      if (m_count >= MIN_MSG_LEN) {
        //check synchronized header(8B, 0x00 FF FF FF FF FF FF 00)
        int result = 0;
        while(1 != result){
            if(MIN_MSG_LEN > m_count)
            {
                m_noPackage = true;
                LoggerPtr rootLogger = Logger::getRootLogger(); 
                LOG4CXX_ERROR(rootLogger, "("<<m_recvHostIP<<")Wrong synchronized header, count="<<m_count<<", startPos="<<startPos<<", m_popPos="<<m_popPos<<", m_pushPos="<<m_pushPos);
                G::displayHex("Wrong synchronized header", m_rawBuff+startPos,MAX_SDTP_MSG_LEN);
                pthread_mutex_unlock(&m_dataControl.getMutex());
                return -1;
            }
            
            if(0x00 != *(m_rawBuff + m_popPos)){                
                m_popPos = (++m_popPos)%m_queueSize;
                m_count--;
                continue;
            }else{
                startPos = m_popPos;
            }
            
            for(int i=1; i<8; i++){
                m_popPos = (m_popPos+i)%m_queueSize;
                if(7 == i){
                    if(0x00 == *(m_rawBuff + m_popPos)){
                        result = 1;
                        break;
                    }else{                
                        m_count -= i;
                        result = 0;
                        break;
                    }
                }else{
                    if(0xFF == *(m_rawBuff + m_popPos)){
                        continue;
                    }else{                
                        m_count -= i;
                        result = 0;
                        break;
                    }
                }
            }
        }


        //get lengthByte:2B
        m_popPos = (++m_popPos)%m_queueSize;
        if(m_popPos==(m_queueSize-1)){
            lengthByte[0] = *(m_rawBuff+m_popPos);
            lengthByte[1] = *(m_rawBuff);
        }else{
            lengthByte[0] = *(m_rawBuff+m_popPos);
            lengthByte[1] = *(m_rawBuff+m_popPos+1);
        }
        msgBodyLen  = (lengthByte[0]  << 8) + lengthByte[1];
        len = msgBodyLen+MSG_HEAD_LEN;
        
        if(m_count-MSG_HEAD_LEN >= msgBodyLen){
            m_popPos = (m_popPos+2) % m_queueSize;
            UINT pos = m_popPos+msgBodyLen; 
            if(pos<=m_queueSize){
                memcpy(pData,m_rawBuff+startPos,MSG_HEAD_LEN);
                memcpy(pData+MSG_HEAD_LEN,m_rawBuff+m_popPos,msgBodyLen);
                m_count  -= msgBodyLen+MSG_HEAD_LEN; 
                m_popPos += msgBodyLen; 
                m_popPos %= m_queueSize; 
            }else{
                UINT len1 = pos-m_queueSize;
                UINT len2 = msgBodyLen -len1;
                memcpy(pData,m_rawBuff+startPos,MSG_HEAD_LEN);
                memcpy(pData+MSG_HEAD_LEN,m_rawBuff+m_popPos,len2);
                memcpy(pData+MSG_HEAD_LEN+len2,m_rawBuff,len1);
                m_count  -= msgBodyLen+MSG_HEAD_LEN; 
                m_popPos = len1; 
            }
            rc = 1;
            if(m_count>=MIN_MSG_LEN){
                m_noPackage = false;
            }else{
                m_noPackage = true;
            }
            
            ++getCount;
            if((getCount % 10000) == 0){
               LoggerPtr rootLogger = Logger::getRootLogger(); 
               LOG4CXX_DEBUG(rootLogger, "("<<m_recvHostIP<<")Get count="<<getCount<<", m_count="<<m_count);
            }
        }else{
            //uncomplete data, waiting for remaining data
            m_popPos = startPos;
            m_noPackage = false;
            len = 0;
        }
      }
      pthread_mutex_unlock(&m_dataControl.getMutex());
  }

  return rc;
}


DataControl& CyclicQueue::getDataControl() {
  return m_dataControl;
}
/*
void CyclicQueue::print(int startPos, int endPos){
    int diff = 0;
    if(endPos == startPos){
        return;
    }
    
    if(endPos>startPos){
        diff = endPos - startPos;
        if(diff<500){
            G::displayHex("startPos-32: ", m_rawBuff+startPos-32,diff+MAX_ZCXC_DATA_LEN);
        }else{
            G::displayHex("startPos-32: ", m_rawBuff+startPos-32,MAX_ZCXC_DATA_LEN);
            G::displayHex("endPos-32: ", m_rawBuff+endPos-32,MAX_ZCXC_DATA_LEN);
        }
        return;
    }else{
        diff = startPos - endPos;
        if(diff<500){
            G::displayHex("endPos-32: ", m_rawBuff+endPos-32,diff+MAX_ZCXC_DATA_LEN);
        }else{
            G::displayHex("endPos-32: ", m_rawBuff+endPos-32,MAX_ZCXC_DATA_LEN);
            G::displayHex("startPos-32: ", m_rawBuff+startPos-32,MAX_ZCXC_DATA_LEN);
        }
        return;
    }
    
}


void CyclicQueue::print(int startPos){
    G::displayHex("startPos-32: ", m_rawBuff+startPos-32,MIN_SDTP_MSG_LEN+32);
    return;
}
*/
//end of file msgqueue.cpp

