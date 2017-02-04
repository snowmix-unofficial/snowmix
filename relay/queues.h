/*
 * (c) 2014 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */

#ifndef __QUEUES_H__
#define __QUEUES_H__

#include <sys/types.h>
//#include <stdint.h>
//#include <netinet/in.h>

//#include "receivers.h"
//#include "controller.h"

struct data_buffer_t {
	u_int8_t*	data;	// Pointer to data area
	u_int32_t	len;	// length of data
	u_int32_t	ip;	// source IP address
	u_int32_t	port;	// source IP port
	struct timeval	time;	// time of reception
	struct data_buffer_t* next;	// Pointer to next structure in list
};

class CQueue {
  public:
	CQueue(const char* name = NULL);
	~CQueue();

	CQueue*		AddQueue(CQueue* pQueue = NULL);
	bool		RemoveQueue(CQueue* pQueue);
	data_buffer_t*	NewBuffer(u_int32_t len, u_int8_t* pData = NULL);
	void		DiscardBuffer(data_buffer_t*);
	int		AddBuffer2All(data_buffer_t* pBuffer, bool copy = false);
	void		AddBuffer2Head(data_buffer_t* pBuffer);
	bool		HasData() { return (m_pHead ? true : false); }
	data_buffer_t*	GetBuffer();
	int32_t		QueueCount() { return m_queues; };

  private:
	char*		m_name;
	u_int32_t	m_buffer_count;		// Buffer count in data list
	u_int32_t	m_bytes_count;		// Byte count in data list
	u_int32_t	m_queues;		// Queue count. Only in Master

	data_buffer_t*	m_pHead;		// Head of data list
	data_buffer_t*	m_pTail;		// List of data list
	CQueue*		m_pMasterQueue;		// The Master Queue has this
						// Queue in it's list
	CQueue*		m_pNextQueue;		// Next Queue in a list
};
	
#endif	// __QUEUES_H__
