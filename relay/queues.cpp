/*
 * (c) 2014 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */

#include <unistd.h>
#include <stdio.h>
#ifdef HAVE_MALLOC
#include <malloc.h>
#else
#include <stdlib.h>
#endif
#include <stdlib.h>

//#include <stdarg.h>
//#include <stdlib.h>
//#include <errno.h>
//#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
//#include <sys/un.h>
#include <arpa/inet.h>
//#include <netinet/in.h>
//#include <fcntl.h>
#include <string.h>
#include <ctype.h>
//#include <sys/select.h>

//#include <stdint.h>
//#include <sys/select.h>
//#include <sys/time.h>
//#include <stdbool.h>
//#include <glib.h>
//#ifdef WITH_GUI
//#include <gtk/gtk.h>
//#endif

#include "relay.h"
#include "queues.h"

CQueue::CQueue(const char* name) {
	m_name		= name ? strdup(name) : NULL;
	m_buffer_count	= 0;
	m_bytes_count	= 0;
	m_pHead		= 0;
	m_pTail		= 0;
	m_queues	= 0;
	m_pMasterQueue	= NULL;
	m_pNextQueue	= NULL;
}

CQueue::~CQueue() {
	// Free all data buffers in the Queue's data list
	while (m_pHead) {
		data_buffer_t* p = m_pHead->next;
		free(m_pHead);
		m_pHead = p;
	}

	// Unlink any Queue we have in the list
	while (m_pNextQueue) {
		CQueue* p = m_pNextQueue->m_pNextQueue;
		RemoveQueue(m_pNextQueue);
		m_pNextQueue = p;
	}
	if (m_name) free (m_name);
}

// Add a queue to the queue;
CQueue* CQueue::AddQueue(CQueue* pQueue) {
	// We wont add a queue to it self
	if (this == pQueue) return NULL;
//fprintf(stderr, "Adding a queue to queue %s\n", m_name ? m_name : "no name");

	// Create a Queue if none was provided
	if (!pQueue) {
		// Fail if we could not create one
		if (!(pQueue = new CQueue())) return NULL;
	} else {
		// One was provided. Remove it from another Master if it
		// is in another Masters queue
		if (pQueue->m_pMasterQueue)
			pQueue->m_pMasterQueue->RemoveQueue(pQueue);
	}
	if (pQueue->m_pNextQueue)
		fprintf(stderr, "Warning. Adding a Queue with a list of "
			"queues\n");
	// Add the Queue to the head of this Queues queue list
	pQueue->m_pNextQueue = m_pNextQueue;
	m_pNextQueue = pQueue;
	m_queues++;
	// pQueue belongs to this Queue which is the master queue
	pQueue->m_pMasterQueue	= this;
	// 
fprintf(stderr, " - Added a queue to a queue. Count %u\n", m_queues);
	return pQueue;
}

// Remove a specific queue. Returns false if successful
// The removed Queue is not deleted
bool CQueue::RemoveQueue(CQueue* pQueue) {
	// Fail if no Queue or Queue is this Queue or we have no Queue list
	if (!pQueue || pQueue == this || !m_pNextQueue) return true;

	// Is the Queue the first Queue in the list?
	if (m_pNextQueue == pQueue) {
		// We found it in the list and will unlink it
		m_pNextQueue = m_pNextQueue->m_pNextQueue;
	} else {
		// It was not the first. Go through the list
		CQueue* p = m_pNextQueue;
		bool found = false;
		while (p) {
			if (p->m_pNextQueue == pQueue) {
				// We found it in the list and will unlink it
				p->m_pNextQueue = pQueue->m_pNextQueue;
				p = NULL;
				found = true;
				continue;
			}
			p = p->m_pNextQueue;
		}
		if (!found) return true;
	}
	// The Queue is no longer in a list
	pQueue->m_pNextQueue = NULL;
	// The Queue is no longer in a Master's Queue list
	pQueue->m_pMasterQueue = NULL;
	m_queues--;
	return false;
}

// Returns a new buffer
data_buffer_t* CQueue::NewBuffer(u_int32_t len, u_int8_t* pData) {
	if (!len) return NULL;
	data_buffer_t* pBuffer = (data_buffer_t*)
		calloc(sizeof(data_buffer_t)+len,1);
	if (pBuffer) {
		pBuffer->data = ((u_int8_t*) pBuffer) + sizeof(data_buffer_t);
		if (pData) memcpy(pBuffer->data, pData, len);
		pBuffer->len = len;
	}
	return pBuffer;
}

// For discarding a used buffer. May be used for reusing buffer in future
void CQueue::DiscardBuffer(data_buffer_t* pBuffer) {
//fprintf(stderr, "Queues : DiscardBuffer()\n");
	if (pBuffer) free(pBuffer);
}

void CQueue::AddBuffer2Head(data_buffer_t* pBuffer) {
	if (pBuffer) {
		if (pBuffer->len) {
			pBuffer->next = m_pHead;
			m_pHead = pBuffer;
			m_buffer_count++;
			m_bytes_count += pBuffer->len;
		} else DiscardBuffer(pBuffer);
	}
}

// Add a buffer to all Queues except the master. Returns 0 if succesfull
// Returns 1 if buffer was added to at least one Queue, but not all
// Returns -1 if the buffer was not added to a Queue at all
int CQueue::AddBuffer2All(data_buffer_t* pBuffer, bool copy) {
	// If there is no buffer, we fail
	if (!pBuffer) return -1;
	// We will not add a buffer to a Master Queue, only its list of Queues
	if (!m_pMasterQueue) {
//fprintf(stderr, "Adding buffer to Masters queue list\n");
		// This is the Master Queue. Fail if there are no Queues in
		// the Master's Queue list
		if (!m_pNextQueue) return -1;
		// We have a list of Queues and will add the buffer to the first
		CQueue* p = m_pNextQueue;
//fprintf(stderr, " - Adding buffer to Masters queue list first\n");
		if (p->AddBuffer2All(pBuffer, copy)) {
			// We failed to add buffer to the list of Queues
			return -1;
		}
		// Now add a copy of the buffer to the rest of the list of Queues
		while (p->m_pNextQueue) {
//fprintf(stderr, " - Adding buffer to Masters queue list next\n");
			if (p->m_pNextQueue->AddBuffer2All(pBuffer, true)) {
				// We added at least one, but failed further on
				return 1;
			}
			p = p->m_pNextQueue;
		}
		// We have added buffers to all Queues in Master's Queue list
		return 0;
	}

	// This is not the Master Queue and we can continue adding the buffer
	// to this queue and copy it before doing so if necessary
	if (copy) {
		pBuffer = NewBuffer(pBuffer->len, pBuffer->data);
		if (!pBuffer) return -1;
	}

	// Add it to the head, if the buffer list is empty
	if (!m_pHead) m_pHead = m_pTail = pBuffer;
	else {
		// Else add it to the tail
		m_pTail->next = pBuffer;
		m_pTail = pBuffer;
	}
	m_buffer_count++;
	m_bytes_count += pBuffer->len;
	pBuffer->next = NULL;
	return false;
}

data_buffer_t* CQueue::GetBuffer() {
	data_buffer_t* pBuffer = m_pHead;
//fprintf(stderr, "Queues : Get Buffer\n");
	if (pBuffer) {
		m_buffer_count--;
		m_bytes_count -= pBuffer->len;
		m_pHead = pBuffer->next;
		if (!m_pHead) m_pTail = NULL;
	}
//fprintf(stderr, "Queues : GetBuffer has %sbuffer len %u\n",
//	pBuffer ? "" : "no ", pBuffer ? pBuffer->len : 0);
	return pBuffer;
}
