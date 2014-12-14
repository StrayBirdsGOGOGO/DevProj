/***************************************************************************
 fileName    : command.h
 begin       : 2010-12-14
 copyright   : (C) 2010 by wyt
 ***************************************************************************/

#ifndef Command_h
#define Command_h 1

#include "hwsshare.h"

class Command {
public:
    //Constructors (generated)
    Command();
  
    //Destructor (generated)
    ~Command();
    

	static int initCommandProcess();
	static int commandSocketConnect();
	static int commandListen();
	static int commandLogin();
	static int commandSend(string sBuf);
	static int commandIntreract(string out, char* in, int inlen);
	static int commandConfirm(string out);
	static void closeCommandSocket();
	
	static int commandProcess(char* recvBuf);
	static int changeCommandProcess(char* object, char* value);
	static int addCommandProcess(char* object, char* value);
	static int delCommandProcess(char* object, char* value);
	static int displayCommandProcess(char * object);
	static int getLoginUsers(string value);
	static int insertUser2Map(char* user, char* pwd);	
	static int checkUser(string user, string pwd);

public:
	//for command socket
	static pthread_t m_tidCommand;
	static int m_listenPort;
	static string m_listenHostIP;
	static sockaddr_in m_myAddr;
	static int m_commandSockt;
	static bool m_bConnected;
	static int m_userNum;
	static map<string,string> m_userMap;
	static pthread_mutex_t m_userMutex;
};

#endif
//end of file command.h

