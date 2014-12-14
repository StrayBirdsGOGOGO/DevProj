/* queue.cpp
 ** This set of queue functions routines
 ** thread-ignorant. the caller can lock the mutex once at
 ** the beginning, then insert 5 items, and then unlock at the end.
 ** Moving the lock/unlock code out of the queue functions allows for
 ** optimizations that aren't possible otherwise.  It also makes this
 ** code useful for non-threaded applications.
 **/

#include <stdio.h>
#include "queue.h"
#include "g.h"


Node::Node() {
  m_next = 0;
}

Queue::Queue() {
  init();
}

Queue::~Queue() {
}

void Queue::init() {
  m_head = 0;
  m_tail = 0;
}

void Queue::put(Node* pNode) {
  if (0 == pNode) {
    return;
  }

  if (0 == m_head) {
    m_tail = 0;
  }

  pNode->setNext(0);
  if (m_tail) {
    m_tail->setNext(pNode);
  }
  m_tail = pNode;
  if (0 == m_head) {
    m_head = pNode;
  }
}

Node* Queue::get() {
  //get from root
  Node* pNode = 0;
  pNode = m_head;
  if (0 != m_head) {
    m_head = (Node*) m_head->getNext();//(Node*)是将const Node*转化为Node*.
  }
  return pNode;
}

Node* Queue::getHead() {

  return m_head;
}

void Queue::remove(Node* pNode) {
  if (0 == pNode) {
    return;
  }
  Node* preNode = (Node*)pNode->getPre();
  Node* nextNode = (Node*)pNode->getNext();
  if(preNode){
    preNode->setNext(nextNode);
  }else{
    m_head = nextNode;
  }

  if(nextNode){
    nextNode->setPre(preNode);
  }else{
    m_tail = preNode;
  }
}

//end of file queue.cpp

