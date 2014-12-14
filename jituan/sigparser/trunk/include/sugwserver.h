#ifndef __SUGW_SERVER_H_
#define __SUGW_SERVER_H_

#include <string>
#include <map>

using namespace std;


class CDRMessage;

class SugwServer{
public:
	SugwServer();
	~SugwServer();
	
	static int initSugwSocket();
	//send msg to sugw
	static int sktSend(PUCHAR sendBuf, int sendLen);
	static int sendMessage(CDRMessage* pMessage);
	static int addSigSock(int newFd, struct sockaddr_in* clientAddr);
	static int removeSigSock(int fd);
	static int removeSigSockList();
	static string findSigSock(int fd);
	static string getSugwClientsInfo();
	static bool ifSktNormal();
	static void sendMsgCount();

public:
	static int m_listenPort;

	static int m_sugwSocketNum;
	static long m_lastDisconnectTime;
	
	static ULONG m_nSendMsgs;
	static ULONG m_nSendStart;
	static ULONG m_nSendEnd;
};

#endif
//end of sugwserver.h

