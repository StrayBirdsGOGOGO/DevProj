/* queue.h
 */

#ifndef __QUEUE_H__
#define __QUEUE_H__

class Node {
public:
  Node();
  virtual ~Node() {
  }
  ;
  void setNext(Node* pNode) {
    m_next = pNode;
  }
  const Node* getNext() {
    return m_next;
  }
  
   void setPre(Node* pNode) {
	 m_pre = pNode;
   }
  const Node* getPre() {
	 return m_pre;
   }
  
private:
  Node* m_next;
  Node* m_pre;
};

class Queue {
public:
  Queue();
  virtual ~Queue();

  virtual void init();
  virtual void put(Node* pNode);
  virtual Node* get();
  virtual Node* getHead();
  virtual void remove(Node* pNode);

private:
  Node* m_head;
  Node* m_tail;
};

#endif

//end of file queue.h


