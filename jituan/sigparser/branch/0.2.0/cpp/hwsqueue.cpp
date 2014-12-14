/* hwsqueue.cpp
 */

#include "hwsqueue.h"


HwsQueue::HwsQueue() {
  init();
}

HwsQueue::~HwsQueue() {

}

void HwsQueue::init() {
  super::init();
}

void HwsQueue::put(Node* pNode) {
  //gOsLog<<"In put() ..."<<endl;
  pthread_mutex_lock(&m_dataControl.getMutex());
  super::put(pNode);
  pthread_mutex_unlock(&m_dataControl.getMutex());
  pthread_cond_broadcast(&m_dataControl.getCond());
  //gOsLog<<"Out put() "<<endl;

}

void HwsQueue::remove(Node* pNode) {
  pthread_mutex_lock(&m_dataControl.getMutex());
  super::remove(pNode);
  pthread_mutex_unlock(&m_dataControl.getMutex());
}

Node* HwsQueue::get() {
  //get from root
  //gOsLog<<"In get() ..."<<endl;
  Node* pNode = 0;
  pthread_mutex_lock(&m_dataControl.getMutex());
  pNode = super::get();
  pthread_mutex_unlock(&m_dataControl.getMutex());
  //gOsLog<<"Out get() "<<endl;
  return pNode;
}

Node* HwsQueue::getHead() {
  //get from root
  //gOsLog<<"In getHead() ..."<<endl;
  Node* pNode = 0;
  pthread_mutex_lock(&m_dataControl.getMutex());
  pNode = super::getHead();
  pthread_mutex_unlock(&m_dataControl.getMutex());
  //gOsLog<<"Out getHead() "<<endl;
  return pNode;
}

DataControl& HwsQueue::getDataControl() {
  return m_dataControl;
}
//end of file msgqueue.cpp

