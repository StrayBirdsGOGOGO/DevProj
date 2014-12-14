#ifndef __CONTROL_H__
#define __CONTROL_H__

#include "hwsshare.h"

class DataControl {
public:
  DataControl();
  virtual ~DataControl();
  int init();
  int destory();
  int activate();
  int deactivate();
  bool isActive();
  pthread_cond_t& getCond() {
    return m_cond;
  }
  pthread_mutex_t& getMutex() {
    return m_mutex;
  }

private:
  pthread_mutex_t m_mutex;
  pthread_cond_t m_cond;
  bool m_isActive;
};

#endif

//end of file control.h


