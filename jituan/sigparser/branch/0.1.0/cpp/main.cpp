/*************************************************
 *      File name:    main.cpp           *
 *************************************************/
#include "hwsshare.h"
#include "g.h"
#include "appmonitor.h"


void setup_signal_handlers(void);
void signal_handler(int signum);
int checkSingletonHws();

void backGround() {
  //  Disconnect from session
  pid_t pid = fork();
  if (pid < 0) {
    cout << "Couldn't fork.error=" <<strerror(errno);
    exit(1);
  }

  if (pid > 0) {
    exit(0);
  }

  int devnull;
  devnull = open("/dev/null", O_RDWR);
  dup2(devnull, STDIN_FILENO);
  dup2(devnull, STDOUT_FILENO);
  dup2(devnull, STDERR_FILENO);
  close(devnull);
  return;
}

int main(int argc, char *argv[]) {
  int sockfd = 0;
  int i = 0;
  
  sockfd = checkSingletonHws();
  if (0 >= sockfd) {
    cout << "OohoO,There is another one runing now!" << endl;
    return 0;
  }  

  if (1 >= argc) {
    backGround();
  }

  //log initial
  PropertyConfigurator::configure("log4cxx.properties");
  LoggerPtr rootLogger = Logger::getRootLogger();
  LOG4CXX_WARN(rootLogger, "System start!"); 
  
  setup_signal_handlers();
  
  pid_t pidt = getpid();
  LOG4CXX_INFO(rootLogger, "main thread pid="<<(int) pidt);

  //system inital
  G::initSys();
  
  // main loop 
  while (!G::m_bSysStop) {
    sleep(5);
    if((i++)%12 == 0){
        AppMonitor::getInstance()->watchDog(G::m_appName.c_str());
    }
  }

  // system close
  G::closeSys();

  close(sockfd);
  LOG4CXX_INFO(rootLogger, "Exit from the main thread!");   
  LOG4CXX_WARN(rootLogger, "System quit!"); 
  return 0;
}

void setup_signal_handlers(void) {
  struct sigaction act;

  act.sa_handler = signal_handler;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  sigaction(SIGINT, &act, NULL);
  sigaction(SIGQUIT, &act, NULL);
  sigaction(SIGHUP, &act, NULL);
  sigaction(SIGPIPE, &act, NULL);
  sigaction(SIGTERM, &act, NULL);
}

void signal_handler(int signum) {
  LoggerPtr rootLogger = Logger::getRootLogger();
  LOG4CXX_INFO(rootLogger, "signum="<<signum);  
  long now = 0;
  
  switch (signum){
    case SIGINT:
      LOG4CXX_WARN(rootLogger, "signum=SIGINT");    
      break;
    
    case SIGHUP:
      LOG4CXX_WARN(rootLogger, "signum=SIGHUP");     
      break;
    case SIGQUIT:
      LOG4CXX_WARN(rootLogger, "signum=SIGQUIT");     
      break;
    case SIGPIPE:
      LOG4CXX_WARN(rootLogger, "signum=SIGPIPE");
      break;
    case SIGTERM:
      time(&now);
      LOG4CXX_WARN(rootLogger, "signum=SIGTERM "<<now);     
      G::m_bSysStop = true;
      break;
  }
}

int checkSingletonHws() {
  int sockfd = 0;
  int portNumber = 5245;
  struct sockaddr_in serv_addr;

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    return 0;
  }
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(portNumber);

  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    close(sockfd);
    return 0;
  }

  return sockfd;
}

//end of main.cpp


