/***************************************************************************
 fileName    : msgqueue.h  -  MsgQueue class header file
 begin       : 2002-09-17
 copyright   : (C) 2002 by BobHuang
 ***************************************************************************/
#ifndef __HWSQUEUE_H__
#define __HWSQUEUE_H__

#include "control.h"
#include "queue.h"

class DataControl;

class HwsQueue: public Queue {
  typedef Queue super;
public:
  HwsQueue();
  virtual ~HwsQueue();
  virtual void init();
  virtual void put(Node* pNode);
  void remove(Node* pNode);
  virtual Node* get();
  virtual Node* getHead();
  DataControl& getDataControl();

private:
  DataControl m_dataControl;

};
#endif
//
