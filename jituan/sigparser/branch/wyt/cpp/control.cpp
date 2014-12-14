/* control.cpp
 ** These routines provide an easy way to make any type of
 ** data-structure thread-aware.  Simply associate a dataControl
 ** structure with the data structure (by creating a new struct, for
 ** example).  Then, simply lock and unlock the mutex, or
 ** wait/signal/broadcast on the condition variable in the dataControl
 ** structure as needed.
 **
 ** dataControl structs contain an int called "active".  This int is
 ** intended to be used for a specific kind of multithreaded design,
 ** where each thread checks the state of "active" every time it locks
 ** the mutex.  If active is 0, the thread knows that instead of doing
 ** its normal routine, it should stop itself.  If active is 1, it
 ** should continue as normal.  So, by setting active to 0, a
 ** controlling thread can easily inform a thread work crew to shut
 ** down instead of processing new jobs.  Use the control_activate()
 ** and control_deactivate() functions, which will also broadcast on
 ** the dataControl struct's condition variable, so that all threads
 ** stuck in pthread_cond_wait() will wake up, have an opportunity to
 ** notice the change, and then terminate.
 */

#include "control.h"
#include "g.h"

DataControl::DataControl() {
  init();
}

DataControl::~DataControl() {
  destory();
}

int DataControl::init() {
  if (pthread_mutex_init(&m_mutex, 0)) {
    return -1;
  }
  if (pthread_cond_init(&m_cond, 0)) {
    return -1;
  }
  m_isActive = false;
  return 0;
}

int DataControl::destory() {
  int ret = 0;
  ret = pthread_mutex_destroy(&m_mutex);
  if (ret) {
    printf("ret=%d\n", ret);
    return -1;
  }
  if (pthread_cond_destroy(&m_cond)) {
    return -1;
  }
  m_isActive = false;
  return 0;
}
int DataControl::activate() {
  if (pthread_mutex_lock(&m_mutex)) {
    return -1;
  }

  if (false == m_isActive) {
    m_isActive = true;
    pthread_mutex_unlock(&m_mutex);
    pthread_cond_broadcast(&m_cond);
  } else {
    pthread_mutex_unlock(&m_mutex);
  }

  return 0;
}

int DataControl::deactivate() {
  if (pthread_mutex_lock(&m_mutex)) {
    return -1;
  }
  if (m_isActive) {
    m_isActive = false;
    pthread_mutex_unlock(&m_mutex);
    pthread_cond_broadcast(&m_cond);
  } else {
    pthread_mutex_unlock(&m_mutex);
  }
  return 0;
}
bool DataControl::isActive() {
  return m_isActive;
}

//end of file control.cpp


