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
#include <errno.h>
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
#include <sys/select.h>

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
#include "senders.h"


CSender::CSender(CSenders* pSenders, u_int32_t id, const char* name) {
	m_id		= id;
	if (!(m_pSenders = pSenders))
                fprintf(stderr, "Error: Creating sender %u, the class CSenders was set to NULL.\n", id);

	if ((m_name = strdup(name))) trim_string(m_name);
	else fprintf(stderr, "Failed to allocate name for sender\n");
	m_port		= -1;
	m_mtu		= 0;
	m_host		= INADDR_ANY;
	m_state		= SETUP;
	m_sockfd	= -1;
	m_reuseaddr	= true;
	m_reuseport	= true;
	m_conns		= 0;
	m_max_conns	= 1;
	m_sender_list	= NULL;
	m_pCtr		= NULL;
	m_host_allow	= NULL;
	m_pQueue	= NULL;
	m_source_type	= SOURCE_UNDEFINED;
	m_source_id	= 0;
/*
	if (!(m_pQueue = new CQueue())) {
		fprintf(stderr, "Failed to get a master Queue for sender "
			"%u\n", id);
	}
*/
	if (!(m_buffer	= (u_int8_t*) malloc(m_mtu)))
		fprintf(stderr, "Failed to allocate buffer for sender\n");

	// Stats
	m_start_time.tv_sec = 0; m_start_time.tv_usec = 0;
	m_stop_time.tv_sec = 0; m_stop_time.tv_usec = 0;
	m_times_started = 0;
	m_bytes_sent = 0;
	m_bytes_dropped = 0;

	// Slave/Master
	m_master_id     = id;
	m_pSlaveList    = NULL;
}

// When a Sender is deleted, its controller must be deleted from the
// CSenders m_controller_list first
CSender::~CSender() {
	if (m_name) free(m_name);
	if (m_pCtr) delete m_pCtr;
	if (m_pQueue && m_type == SEND_TCPSERVERCON) delete m_pQueue;
	while (m_sender_list) {
		CSender* p = m_sender_list->m_sender_list;
		delete m_sender_list;
		m_sender_list = p;
	}
	while (m_pSlaveList) {
		slave_list_t* p = m_pSlaveList->next;
		if (m_pSenders) m_pSenders->Stop(m_pSlaveList->id);
		free (m_pSlaveList);
		m_pSlaveList = p;
	}

}

int CSender::AddSlave(u_int32_t slave_id, bool add, u_int32_t verbose) {
	slave_list_t* ptmp, *p = m_pSlaveList;

	if (add) {
		// We check if it already is added
		while (p) {
			if (p->id == slave_id) break;
			p = p->next;
		}
		if (p) return -1;

		if (!(ptmp = (slave_list_t*) calloc(sizeof(slave_list_t),1))) {
			if (verbose) fprintf(stderr, "Failed to allocate memory for sender slave %u for sender master %u\n",
				slave_id, m_id);
			return -1;
		}
		ptmp->id = slave_id;
		if (!m_pSlaveList || m_pSlaveList->id > slave_id) {
			ptmp->next = m_pSlaveList;
			m_pSlaveList = ptmp;
			return 0;
		}
		p = m_pSlaveList;
		while (p) {
			if (!p->next || p->next->id > slave_id) {
				ptmp->next = p->next;
				p->next = ptmp;
				return 0;
			}
			p = p->next;
		}
	} else {
		if (!m_pSlaveList) return -1;
		if (m_pSlaveList->id == slave_id) {
			p = m_pSlaveList;
			m_pSlaveList = m_pSlaveList->next;
			free (p);
			return 0;
		}
		while (p) {
			if (p->next && p->next->id == slave_id) {
				slave_list_t* ptmp = p->next;
				p->next = p->next->next;
				free(ptmp);
				return 0;
			}
			p = p->next;
		}
	}
	return -1;
}


void CSender::UpdateState() {
	if (m_state == RUNNING || m_state == CONNECTED) {
		if (m_bytes_sent == m_bytes_sent_last) m_state = STALLED;
	}
	m_bytes_sent_last = m_bytes_sent;
}


void CSender::SetQueue(CQueue* pQueue) {
	if (m_pQueue) delete m_pQueue;
	m_pQueue = pQueue;
}

int CSender::Start() {
	int yes = 1;		// For setting socket options
	if (m_state != READY && m_state != STOPPED &&
		m_state != DISCONNECTED) return -1;

	int type;
	if (m_type == SEND_UDP || m_type == SEND_MULTICAST) type = SOCK_DGRAM;
	else if (m_type == SEND_TCPCLIENT || m_type == SEND_TCPSERVER)
		type = SOCK_STREAM;
	else return 1;

	// Get a socket
	m_sockfd = socket(AF_INET, type, 0);

	if (m_sockfd < 0) {
		perror("Failed to create socket for sender");
		m_sockfd = -1;
		return 1;
	}
	// Set socket option for reuse addr and multiple use of same port
	if (m_reuseaddr)
		if (setsockopt (m_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
			sizeof (yes)) < 0)
			perror("Failed to set socket option SO_REUSEADDR");
#ifdef SO_REUSEPORT
	if (m_reuseport)
		if (setsockopt (m_sockfd, SOL_SOCKET, SO_REUSEPORT, &yes,
			sizeof (yes)) < 0)
			perror("Failed to set socket option SO_REUSEPORT");
#endif

	// Setup IP and port
	m_addr.sin_family = AF_INET;
	m_addr.sin_port = htons(m_port);
	m_addr.sin_addr.s_addr = htonl(m_host);

	int read_fd = -1;

/*
	// bind setup
	if (type == SOCK_DGRAM || m_type == SEND_TCPSERVER) {
		// Bind to port
		if (bind(m_sockfd, (struct sockaddr *) &m_addr, sizeof(m_addr))) {
			perror("Failed to bind socket for sender");
			close(m_sockfd);
			m_sockfd = -1;
			return 1;
		}
		if (m_type == SEND_TCPSERVER) {
			// Listen to IP and port
			if (listen(m_sockfd, 1) < 0) {
				perror("Failed to listen for sender");
				close(m_sockfd);
				m_sockfd = -1;
				return 1;
			}
		} else if (m_type == SEND_MULTICAST) {
			// Multicast 224.0.0.0-239.255.255.255
			// 0x11100000.0.0.0-0x11100111.255.255.255A
			if ((m_host & 0xe0000000) == 0xe0000000) {
				m_mreq.imr_multiaddr.s_addr = htonl(m_host);
				m_mreq.imr_interface.s_addr = htonl(INADDR_ANY);
				if (setsockopt(m_sockfd, IPPROTO_IP,
					IP_ADD_MEMBERSHIP, &m_mreq,
					sizeof(m_mreq)) < 0) {
					perror("Failed to join multicast group "
						"for sender");
				}
			} else {
				fprintf(stderr, "Warning. IP address is not a "
					"multicast address for sender of "
					"type multicast");
			}
		}
	} else if (m_type == SEND_TCPCLIENT) {
		// Connect to IP and port
		if (connect(m_sockfd, (struct sockaddr *) &addr,
			sizeof(addr))) {
			perror("Failed to connect for sender");
			close(m_sockfd);
			m_sockfd = -1;
			return 1;
		}
		m_conns = 1;
		write_fd = m_sockfd;
	}
*/
	m_pCtr = new CController(read_fd, m_sockfd);
	if (!m_pCtr) {
		fprintf(stderr, "Failed to get a controller for sender\n");
		close(m_sockfd);
		m_sockfd = -1;
		return 1;
	}
/*
	// We can't get the MTU without a connection
	// and the MTU is mostly interested for UDP which is connection less
	int mtu;
	socklen_t mtulen = sizeof(unsigned int);
	if (getsockopt(m_sockfd, SOL_IP, IP_MTU, &mtu, &mtulen)) {
		perror("Could not get MTU for socket");
	} else fprintf(stderr, "MTU for socket %d\n", mtu);
*/


	m_state = PENDING;
	gettimeofday(&m_start_time,NULL);
	m_stop_time.tv_sec = 0; m_stop_time.tv_usec = 0;
	m_times_started++;

	// Now check slaves if any and start them
	if (m_pSlaveList && m_pSenders) {
		slave_list_t* p = m_pSlaveList;
		while (p) {
			m_pSenders->Start(p->id);
			p = p->next;
		}
	}
	return 0;
}

int CSender::Stop() {
	if (m_state != PENDING && m_state != CONNECTED && m_state != RUNNING &&
		m_state != STALLED) return -1;

	// Set state to stopped, delete controller, which closes the
	// socket if open. Set sockfd to -1
	m_state = STOPPED;
	if (m_pCtr) delete m_pCtr;
	m_pCtr = NULL;
	m_sockfd = -1;
	gettimeofday(&m_stop_time,NULL);
	if (m_type == SEND_TCPCLIENT) m_conns = 0;

	// Now check slaves if any and stop them
	if (m_pSlaveList && m_pSenders) {
		slave_list_t* p = m_pSlaveList;
		while (p) {
			m_pSenders->Stop(p->id);
			p = p->next;
		}
	}

	return 0;
}

int CSender::Connected(bool connected) {
	if (m_type == SEND_TCPSERVER) {
		if (connected && m_state == PENDING) m_state = CONNECTED;
		else if (!connected && (m_state == PENDING ||
			m_state == CONNECTED || m_state == RUNNING ||
			m_state == STALLED)) m_state = PENDING;
		else return -1;
	} else if (m_type == SEND_TCPSERVERCON) {
		m_state = CONNECTED;
	} else return -1;
	return 0;
}

int CSender::WriteQueueData() {
	int n = 0, written;
	data_buffer_t* pBuffer;
//fprintf(stderr, "Write Queued data for Sender %u\n", m_id);
	if (!m_pQueue || !m_pCtr || m_pCtr->m_write_fd < 0) return 0;
	while ((pBuffer = m_pQueue->GetBuffer())) {
//fprintf(stderr, " - Sender %u has Queued data %u\n", m_id, pBuffer->len);
		if (!pBuffer->data || !pBuffer->len) {
			m_pQueue->DiscardBuffer(pBuffer);
			continue;
		}
		if (m_type == SEND_UDP) {
			written = sendto(m_pCtr->m_write_fd, pBuffer->data,
				pBuffer->len, 0, (struct sockaddr *)
				&m_addr, sizeof(m_addr));
			if (written < 1) {
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					// Writing more data would block. Break
					// and add data back to Queue
fprintf(stderr, "Sender %u would block. Adding data back to Queue\n", m_id);
					break;
				}
				perror("Sender failed writing");
			} else {
				m_bytes_sent += written;
//fprintf(stderr, "Sender %u wrote %d bytes of %u\n", m_id, written, pBuffer->len);
				pBuffer->data += written;
				pBuffer->len -= written;
				n += written;
				// Check if we did not write all bytes
				if (pBuffer->len) break;
			}
		} else fprintf(stderr, "Unsupported sender type for "
			"WriteQueueData for sender %u\n", m_id);
		m_pQueue->DiscardBuffer(pBuffer);
	}

	// Change state if needed and data was written
	if (n && (m_state == PENDING || m_state == STALLED ||
		m_state == CONNECTED)) m_state = RUNNING;

	// If we have a buffer, it must be put back in Queue
	// The buffer will be discarded if buffer is empty
	if (pBuffer) m_pQueue->AddBuffer2Head(pBuffer);
	return n;
}

int CSender::ReadData(u_int32_t verbose, struct timeval* pTimeNow) {
	u_int32_t ip = 0, port = 0;

	// Check if we have a controller
	if (!m_pCtr || m_pCtr->m_read_fd < 0) {
		fprintf(stderr, "Error. Tried to read on closed sender %u\n",
			m_id);
		return -1;
	}

	// Now read data, check for EOF (not possible on UDP though)
	int n;
	if (0 && !m_host_allow) {		// receive from all
		n = recv(m_pCtr->m_read_fd, m_buffer, m_mtu, 0);
		if (verbose > 2)
			fprintf(stderr, "Sender %u fd %d got %d bytes\n",
				m_id, m_pCtr->m_read_fd, n);
	} else {			// receive only from list
		struct sockaddr_in	sender_addr;
		//bzero(&sender_addr, sizeof(sender_addr));
		sender_addr.sin_family = AF_INET;
		socklen_t len = sizeof(sender_addr);
		n = recvfrom(m_pCtr->m_read_fd, m_buffer, m_mtu, 0,
			(struct sockaddr*)(&sender_addr), &len);
		ip = (m_type == SEND_UDP ||
			m_type == SEND_MULTICAST || m_type == SEND_TCPSERVER ?
			htonl(*((u_int32_t*)&sender_addr.sin_addr)) :
			m_type == SEND_TCPCLIENT ? m_host : 0);
		port = htons(*((u_int16_t*)&sender_addr.sin_port));
		if (verbose > 2)
			fprintf(stderr, "Sender %u got %u bytes from "
				//"%s "
				"%u.%u.%u.%u port %u %saccepted.\n", m_id, n,
				//inet_ntoa(sender_addr.sin_addr),
				ip_parms(ip),
				port,
				!m_host_allow ? "" :
				  HostNotAllowed(ip,m_host_allow) ? "Not " :
				  "");

	}
	if (n <= 0) {
		if (verbose > 1) fprintf(stderr, "EOF on sender %u\n", m_id);
		return n;
	}

	// Update count of received
	m_bytes_sent += n;

	// Check if we should check if we should drop the packet
	if (m_host_allow) {
		if (HostNotAllowed(ip,m_host_allow)) {
			if (verbose > 2)
				fprintf(stderr, "Dropping %d bytes from "
					"%u.%u.%u.%u\n", n, ip_parms(ip));
			m_bytes_dropped += n;
			return n;
		}
		// packet is allowed
	} // else allow all
	if (m_state != RUNNING) m_state = RUNNING;

	// Check if we have a master Queue and actullay some subscribing to the
	// the Queue
	if (m_pQueue && m_pQueue->QueueCount()) {
		int added;
		data_buffer_t* p = m_pQueue->NewBuffer(n, m_buffer);
		if (!p) fprintf(stderr, "Failed to get data buffer for "
			"sender %u\n", m_id);
		else {
			p->time = *pTimeNow;
			p->ip = ip;
			p->port = port;
			if ((added = m_pQueue->AddBuffer2All(p, false))) {
			if (added < 0) {
				// Adding the buffer failed
				m_pQueue->DiscardBuffer(p);
				m_bytes_dropped += n;
			} else {
				// buffer was added to some queues but no all
				if (verbose > 1)
					fprintf(stderr, "Buffer for sender "
						"%u was only added to parts "
						"of the buffers\n", m_id);
			}
		} }
	} else m_bytes_dropped += n;

	return n;
}

state_enum_t CSender::AdjustState() {
//fprintf(stderr, "AdjustState state %s port %d mtu %u mbuffer %s type %s source %s\n", State2String(m_state), m_port, m_mtu, m_buffer ? "ptr" : "null", SenderType2String(m_type), SourceType2String(m_source_type));
	if (m_state == UNDEFINED || m_state == READY ||
		m_state == SETUP || m_state == DISCONNECTED ||
		m_state == STOPPED) {
		if (m_port < 0) m_state = SETUP;
		else if (m_mtu < 1) m_state = SETUP;
		else if (!m_buffer) m_state = SETUP;
		else if (m_type == SEND_UNDEFINED) m_state = SETUP;
		else if (m_source_type != SOURCE_RECEIVER) m_state = SETUP;
	// Add other source types here with && in the above test
		else m_state = READY;
	}
	return m_state;
}

int CSender::SetSend(u_int8_t* buffer, int32_t n) {
	if (!buffer || !n || !m_pCtr || m_pCtr->m_write_fd < 0 ||
		(m_state != RUNNING && m_state != CONNECTED &&
		 m_state != STALLED)) return -1;
	if (m_pCtr->WriteData(buffer, n, -1) != n) return -1;
	return 0;
}

int CSender::SetPort(u_int32_t port) {
	if (m_state == PENDING || m_state == RUNNING || m_state == CONNECTED ||
		m_state == STALLED || port > 65535) return -1;
	m_port = port;
	AdjustState();
	return 0;
}

int CSender::SetHost(u_int32_t host) {
	if (m_state == PENDING || m_state == RUNNING || m_state == CONNECTED ||
		m_state == STALLED) return -1;
	if (!m_mtu) {
		if (host == 0x7f000001) {
			if (SetMTU(65536)) return -1;
		} else {
			if (SetMTU(1500)) return -1;
		}
	}
	m_host = host;
	AdjustState();
	return 0;
}

int CSender::SetSource(source_type_t source_type, u_int32_t id) {
	if (m_state == PENDING || m_state == RUNNING || m_state == CONNECTED ||
		m_state == STALLED) return -1;
	if (source_type != SOURCE_RECEIVER) return -1; // Add other types here
	m_source_type = source_type;
	m_source_id = id;
	
	AdjustState();
	return 0;
}

int CSender::SetMTU(u_int32_t mtu) {
	if (m_state == PENDING || m_state == RUNNING || m_state == CONNECTED ||
		m_state == STALLED || mtu < 1) return -1;
	if (m_buffer) {
		u_int8_t* p = (u_int8_t*) malloc(mtu);
		if (!p) {
			m_state = SETUP;
			fprintf(stderr, "Failed to get buffer for sender %u",
				m_id);
			return 1;
		}
		free(m_buffer);
		m_buffer = p;
fprintf(stderr, "Buffer set to %u for sender %u\n", mtu, m_id);
	}
	m_mtu = mtu;
	AdjustState();
	return 0;
}

int CSender::SetType(sender_enum_t type) {
	if (m_state == PENDING || m_state == CONNECTED || m_state == RUNNING ||
		m_state == STALLED ||
		(type != SEND_UDP && type != SEND_MULTICAST &&
		 type != SEND_TCPCLIENT && type != SEND_TCPSERVER &&
		 type != SEND_TCPSERVERCON)) return -1;
	m_type = type;
	AdjustState();
	return 0;
}


CSenders::CSenders(u_int32_t max_id, CReceivers* pReceivers) {
	m_max_senders	= max_id;
	m_pReceivers	= pReceivers;
	m_controller_list = NULL;
	m_verbose	= 0;
	m_senders	= (CSender**) calloc(sizeof(CSender*),max_id);
	if (!m_senders) {
		fprintf(stderr, "Failed to allocate array for senders");
		m_max_senders = 0;
	}
}

CSenders::~CSenders() {

	// Delete senders. This will also delete its controller;
	for (u_int32_t id=0; id < m_max_senders; id++)
		DeleteSender(id);
	m_controller_list  = NULL;	// not strictly necessary
}

// Add or delete a slave
int CSenders::AddSlave(u_int32_t master_id, u_int32_t slave_id, bool add)
{
        if (!m_senders || master_id >= m_max_senders || slave_id >= m_max_senders || (add && !m_senders[master_id]) || !m_senders[slave_id]) return -1;
        if (add) {
                // First we check that the slave is not slave of somebdy else. Is must be its own master
                if (m_senders[slave_id]->GetMasterID() != slave_id) return -1;

                // Then we check that the slave is not started
                state_enum_t state = m_senders[slave_id]->GetState();
                if (state == PENDING || state == RUNNING || state == STALLED) return -1;

                // Then we add the slave to master
                if (m_senders[master_id]->AddSlave(slave_id, true, m_verbose)) {
                        if (m_verbose) fprintf(stderr, "Failed to add sender slave %u to master sender %u\n",
                                slave_id, master_id);
                        return 1;
                }
                m_senders[slave_id]->SetMasterID(master_id);
                // Then we check and see if we should start slave
                state = m_senders[master_id]->GetState();
                if (state == PENDING || state == RUNNING || state == STALLED) {
                        // We may fail starting slave, but the slave has been added to the master list so we see it as succeeded
                        if (m_senders[slave_id]->Start()) {
                                fprintf(stderr, "Failed to start sender slave %u that was added to sender master %u\n",
                                slave_id, master_id);
                        }
                }
        } else {
                // We will remove sender as slave if it is a slave. We don't care about the master_id parameter
                // We also don't care if it started.

                u_int32_t current_master = m_senders[slave_id]->GetMasterID();
                if (current_master == slave_id) return -1;              // sender is not a slave
                // Sender is a slave. Remove it from current_masters slave list
                m_senders[current_master]->AddSlave(slave_id, false, m_verbose);
                m_senders[slave_id]->SetMasterID(slave_id);
        }
        return 0;
}


int CSenders::Start(u_int32_t id) {
        if (id >= m_max_senders || !m_senders[id]) return -1;
        return m_senders[id]->Start();
}

int CSenders::Stop(u_int32_t id) {
        if (id >= m_max_senders || !m_senders[id]) return -1;
        return m_senders[id]->Stop();
}

bool CSenders::SenderExist(u_int32_t id) {
	return (m_senders && id < m_max_senders && m_senders[id] ?
		true : false);
}


void CSenders::UpdateState() {
	u_int32_t id;
	if (!m_senders) return;
	for (id=0; id < m_max_senders; id++) if (m_senders[id]) m_senders[id]->UpdateState();
}


// Returns Queue if successful, otherwise NULL
CQueue* CSenders::AddQueue(u_int32_t id, CQueue* pQueue) {
	if (id >= m_max_senders || !m_senders[id] ||
		!m_senders[id]->m_pQueue) return NULL;
	if (m_verbose > 1)
		fprintf(stderr, "Adding Queue to sender %u\n", id);
	return m_senders[id]->m_pQueue->AddQueue(pQueue);
}

// Returns false if successful, otherwise true
bool CSenders::RemoveQueue(u_int32_t id, CQueue* pQueue) {
	if (id >= m_max_senders || !m_senders[id] || !pQueue ||
		!m_senders[id]->m_pQueue) return true;
	if (m_verbose > 1)
		fprintf(stderr, "Removing Queue from sender %u\n", id);
	return m_senders[id]->m_pQueue->RemoveQueue(pQueue);
}

// Set fds for read and write for sender
int CSenders::SetFds(int max_fd, fd_set* read_fds, fd_set* write_fds) {
	CController* pCtr;
	u_int32_t id;

/*
	// Go through list of started senders
	for (pCtr = m_controller_list; pCtr != NULL ; pCtr = pCtr->m_next) {

		// If read_fd is non negative, we may read
		if (pCtr->m_read_fd > -1) {
			// Add descriptor to read bitmap
			FD_SET(pCtr->m_read_fd, read_fds);
			if (pCtr->m_read_fd > max_fd) max_fd = pCtr->m_read_fd;
		} // else not reading
	}
*/
	if (!m_senders) return max_fd;
	for (id=0; id < m_max_senders ; id++) {
		// Check if sender exist and has a controller
		if (!m_senders[id] || !(pCtr = m_senders[id]->m_pCtr)) continue;

		// If read_fd is non negative, we may read
		if (pCtr->m_read_fd > -1) {
			// Add descriptor to read bitmap
			FD_SET(pCtr->m_read_fd, read_fds);
			if (pCtr->m_read_fd > max_fd) max_fd = pCtr->m_read_fd;
		} // else not reading

		// If write_fd is non negative and if we have data in queue,
		// we will add the write_fd
		if (pCtr->m_write_fd > -1 && m_senders[id]->m_pQueue &&
			m_senders[id]->m_pQueue->HasData()) {
			// Data to write. Add descriptor to write bitmap
			FD_SET(pCtr->m_write_fd, write_fds);
//fprintf(stderr, "Sender %u has something to write\n", id);
			if (pCtr->m_write_fd > max_fd)
				max_fd = pCtr->m_write_fd;
		} // else not writing
	}
	return max_fd;
}

int CSenders::AcceptNewConnection(u_int32_t id) {
	int new_fd;
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	if (id >= m_max_senders || !m_senders[id] ||
		m_senders[id]->GetType() != SEND_TCPSERVER ||
		!m_senders[id]->m_pCtr ||
		m_senders[id]->m_pCtr->m_read_fd < 0) return -1;

	// Accept incoming connection
	new_fd = accept(m_senders[id]->m_pCtr->m_read_fd,
		(struct sockaddr*) &addr, &len);

	// Check if accept failed
	if (new_fd < 0) {
		perror("Failed accept for sender tcpserver");
		return -1;
	}

	// Check max connections;
	if (m_senders[id]->m_conns >= m_senders[id]->m_max_conns) {
		fprintf(stderr, "sender %u max connections %u reached. "
			"Dropping new connection.\n", id,
			m_senders[id]->m_max_conns);
		close(new_fd);
		return -1;
	}

	// Check that we have a IPv4 connection
	if (addr.sin_family != AF_INET) {
		fprintf(stderr, "Warning. Non IPv4 connection for sender "
			"%u tcpserver. Closing connection.\n",id);
		close(new_fd);
		return -1;
	}

	// Check ip against host_allow list if any
	u_int32_t ip = ntohl(*((u_int32_t*)&addr.sin_addr));
	u_int16_t port = ntohs(*((u_int16_t*)&addr.sin_port));
	if (m_senders[id]->m_host_allow &&
		HostNotAllowed(ip,m_senders[id]->m_host_allow)) {
		if (m_verbose) fprintf(stderr, "Connection for sender %u ip "
			"%u.%u.%u.%u denied\n", id, ip_parms(ip));
		close(new_fd);
		return -1;
	}
	if (m_verbose > 1) fprintf(stderr, "Connection for sender %u ip "
		"%u.%u.%u.%u accepted\n", id, ip_parms(ip));

	// We have a new connection

	// First we set it to non blocking
	if (set_fd_nonblocking(new_fd)) {
		// We failed to set non blocking
		fprintf(stderr, "Warning. Failed to set nonblocking for new incomming connection for sender %u. Closing connection.\n", id);
		close(new_fd);
		return -1;

	}

	// name = "tcp connection 01234567890"
	//         123456789012345678901234567890
	char name[64];
	sprintf(name, "tcp connection sender %u fd %d", id, new_fd );

	CSender* pSender = new CSender(this, id, name);
	if (!pSender) {
		fprintf(stderr, "Failed to create a new sender for sender "
			"%u\n", id);
		close(new_fd);
		return -1;
	}

	// Set settings for the new sender
	pSender->SetPort(port);
	pSender->SetMTU(m_senders[id]->GetMTU());
	pSender->SetType(SEND_TCPSERVERCON);
	pSender->SetHost(ip);
	// This TCPSERVERCON will use the TCPSERVER Queue for data and discard its own
	pSender->SetQueue(m_senders[id]->GetQueue());
	if (!(pSender->m_pCtr = new CController(new_fd, new_fd))) {
		fprintf(stderr, "Failed to get a controller for new sender %u"
			" connection\n", id);
		close(new_fd);
		delete pSender;	// Safe to delete a sender as its controller has not been added to the m_controller_list
		return -1;
	}
	if (!m_senders[id]->m_sender_list)
		m_senders[id]->m_sender_list = pSender;
	else {
		CSender* p = m_senders[id]->m_sender_list;
		while (p->m_sender_list) p = p->m_sender_list;
		p->m_sender_list = pSender;
	}
	list_add_head(m_controller_list, pSender->m_pCtr);

	m_senders[id]->m_conns++;
	m_senders[id]->Connected(true);
	pSender->Connected(true);
	return 0;
}

void CSenders::CheckReadWrite(fd_set *read_fds, fd_set *write_fds, struct timeval* pTimeNow) {
	if (!m_senders) return;
	for (u_int32_t id = 0; id < m_max_senders; id++) {
		if (!m_senders[id] || !m_senders[id]->m_pCtr) continue;

		CController* pCtr = m_senders[id]->m_pCtr;
		// First we check for writes
		if (pCtr->m_write_fd > 0 &&
			FD_ISSET(pCtr->m_write_fd, write_fds)) {
			// Ready for writing
			m_senders[id]->WriteQueueData();
			//pCtr->WriteBuffers();
		}

		// Check if sender is server.
		if (m_senders[id]->GetType() == SEND_TCPSERVER) {

			// We have a server and need to check its senders
			CSender* pSender = m_senders[id]->m_sender_list;
			while (pSender) {
				CSender* pNextSender =
					pSender->m_sender_list;
				SenderCheckReadWrite(pSender, read_fds,
					write_fds, pTimeNow);
				pSender = pNextSender;
			}

			// Check server fd for validity
			if (pCtr->m_read_fd < 0) {
				fprintf(stderr, "Server fd for sender %u was "
					"unexpectedly invalid\n", id);
				continue;
			}

			// Now we need to check if we have a new connection
			// if we have a new connection we will accept it
			// and add it to the m_sender_list for the sender
			// and its controller to m_controller_list
			if (!FD_ISSET(pCtr->m_read_fd, read_fds)) continue;

			// FD was set and we will accept the connection
			if (AcceptNewConnection(id)) {
				fprintf(stderr, "Failed to accept incomming "
					"connection for sender %u\n", id);
			}
			continue;

		}
		SenderCheckReadWrite(m_senders[id], read_fds, write_fds,
			pTimeNow);
	}
}

int CSenders::SenderCheckReadWrite(CSender* pSender, fd_set *read_fds, fd_set *write_fds, struct timeval* pTimeNow) {

	CController* pCtr = pSender->m_pCtr;

	if (pCtr->m_read_fd < 0) return 1;	// Not readable

	if (!FD_ISSET(pCtr->m_read_fd, read_fds))
		return 1;			// No data ready
	//pSender->GetID(), pSender->GetName(), pSender->m_pCtr->m_read_fd);

	// Now read data, check for EOF (not possible on UDP though)
	int n = pSender->ReadData(m_verbose, pTimeNow);
	if (n <= 0) {

		// We have EOF and need to remove the controller from the
		// m_controller_list
		if (pCtr) {
			list_del_element(m_controller_list, pCtr);
		}

		// Now we can stop the sender which will close the connection
		pSender->Stop();

		// If the sender is a connection generated by a TCP_SERVER
		// we must delete the sender
		if (pSender->GetType() == SEND_TCPSERVERCON) {

			// A server connection and the server has same ID
			u_int32_t id = pSender->GetID();
			if (!m_senders || !m_senders[id]) {
				fprintf(stderr, "Error. Could not find the "
					"sender server for a closed server "
					"sender %u connection. Will cause a "
					"memory leak\n", id);
				return -1;
			}
			CSender* p = m_senders[id];
			if (!p->m_sender_list) {
				fprintf(stderr, "Error. Server sender %u "
					"had no sender list for a closed "
					"server connection. Will cause a "
					"memory leak.\n", id);
				return -1;
			}
			if (p->m_sender_list == pSender) {
				p->m_sender_list =
					pSender->m_sender_list;
				pSender->m_sender_list = NULL;
				delete pSender;	// Safe to delete a sender as its controller has been removed from the m_controller_list
			} else {
				while (p->m_sender_list->m_sender_list) {
					if (p->m_sender_list->m_sender_list == pSender) {
						p->m_sender_list->m_sender_list = pSender->m_sender_list;
						pSender->m_sender_list = NULL;
						delete pSender;
						pSender = NULL;
						break;
					}
					p = p->m_sender_list; // Safe to delete a sender as its controller has been removed from the m_controller_list
				}
				if (pSender) {
					fprintf(stderr, "Failed to find the "
						"server sender closed in the "
						"servers sender %u list of "
						"senders. Will cause a "
						"memory leak.\n", id);
					return -1;
				}
			}
			if (!m_senders[id]->m_sender_list) {
				m_senders[id]->Connected(false);
			}
			m_senders[id]->m_conns--;
		}
	}
	return 0;
}

// Parsing commands starting with 'sender '
// Returns : 0=OK, 1=System error, -1=Syntax error
int CSenders::ParseCommand(CController* pCtr, const char* str) {

	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) return -1;
	if (!strncmp(str, "stat ", 5) || !strncmp(str, "status ", 7))
		return list_status(pCtr, str+ (str[4] == ' ' ? 5 : 7));
	else if (!strncmp(str, "info ", 5))
		return list_info(pCtr, str+5);
	else if (!strncmp(str, "send ", 5))
		return set_send(pCtr, str+5);
	else if (!strncmp(str, "start ", 6))
		return startstop_sender(pCtr, str+6, true);
	else if (!strncmp(str, "stop ", 5))
		return startstop_sender(pCtr, str+5, false);
	else if (!strncmp(str, "port ", 5))
		return set_port(pCtr, str+5);
	else if (!strncmp(str, "type ", 5))
		return set_type(pCtr, str+5);
	else if (!strncmp(str, "mtu ", 4))
		return set_mtu(pCtr, str+4);
	else if (!strncmp(str, "source ", 7))
		return set_source(pCtr, str+7);
	else if (!strncmp(str, "conns ", 6))
		return set_conns(pCtr, str+6);
	else if (!strncmp(str, "add ", 4))
		return add_sender(pCtr, str+4);
	else if (!strncmp(str, "host allow ", 11))
		return set_host_allow(pCtr, str+11);
	else if (!strncmp(str, "host ", 5))
		return set_host(pCtr, str+5);
	else if (!strncmp(str, "verbose ", 8))
		return set_verbose(pCtr, str+8);
	else if (!strncmp(str, "master ", 7))
		return set_master(pCtr, str+7);
	else if (!strncmp(str, "help ", 5))
		return list_help(pCtr, str+5);
	//else fprintf(stderr, "No match for <%s>", str);
	// else no match
	return -1;
}

// sender help
int CSenders::list_help(CController* pCtr, const char* str) {
	if (!pCtr) return 1;
	pCtr->WriteMsg("MSG: sender commands:\n"
		"MSG: sender add [<sender id> [<sender name>]]\n"
		"MSG: sender conns [<sender id> <max connections>]\n"
		"MSG: sender host allow [<sender id> ( <ip> | <network> ... )]\n"
		"MSG: sender info [<sender id>]\n"
		"MSG: sender master [<sender id> [sender <sender master id>]]\n"
		"MSG: sender mtu [<sender id> <mtu>]\n"
		"MSG: sender port [<sender id> <port>]\n"
		"MSG: sender start [<sender id>]\n"
		"MSG: sender send <sender id> <message>\n"
		"MSG: sender stop [<sender id>]\n"
		"MSG: sender verbose [<level>]\n"
		"MSG: sender help\n"
		"MSG:\n");
	return 0;
}

// sender master [<sender id> [sender <sender master id>]]
int CSenders::set_master(CController* pCtr, const char* str) {

        u_int32_t id, master_id;
        int n;

        if (!str) return -1;
        if (!m_senders) return 1;
        while (isspace(*str)) str++;

        // See if we should list senders
        if (!(*str)) {
                if (!pCtr) return 1;
                for (id = 0; id < m_max_senders; id++)
                        if (m_senders[id])
                                pCtr->WriteMsg("MSG: sender %u master %u slaves %s\n",
                                        id, m_senders[id]->GetMasterID(), m_senders[id]->GetSlaveList() ? "yes" : "no");
                pCtr->WriteMsg("MSG:\n");
                return 0;
        }

        // We are to change the name
        if (sscanf(str, "%u", &id) != 1 || !m_senders[id]) return -1;
        while (isdigit(*str)) str++;
        while (isspace(*str)) str++;
        if (!(*str)) {
                // We are to unset sender id as slave
                n = AddSlave(0, id, false);
        } else {
                if (strncmp(str, "rec", 3)) return -1;
                while (*str && !isspace(*str)) str++;
                while (isspace(*str)) str++;
                if (!(*str)) return -1;
                if (sscanf(str, "%u", &master_id) != 1) return -1;
                n = AddSlave(master_id, id, true);
        }
        if (m_verbose && !n && pCtr) pCtr->WriteMsg("MSG: sender %u master sender %u\n",
                id, *str ? master_id : id);
        return n;
}


// sender status [<sender id>]
int CSenders::list_status(CController* pCtr, const char* str) {
	u_int32_t id;
	bool all = true;
	if (!pCtr) return 1;
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (*str) {
		if (sscanf(str, "%u", &id) != 1 || !m_senders ||
			!m_senders[id]) return -1;
		all = false;
	}
	struct timeval time;
	gettimeofday(&time,NULL);
	if (all) pCtr->WriteMsg("MSG: sender status "
#ifdef HAVE_MACOSX                  // suseconds_t is long int on OS X 10.9
                                "%ld.%03d"
#else                               // suseconds_t is unsigned int on Linux
                    		"%ld.%03ld"
#endif
		"\nMSG: sender id state started senders sent dropped\n",
		time.tv_sec, time.tv_usec/1000,
		m_verbose, m_count_senders, m_max_senders);
	for (u_int32_t i=0 ; i < m_max_senders; i++)
		if (m_senders && m_senders[i] && (all || i == id)) {
			state_enum_t state = m_senders[i]->GetState();
			//sender_enum_t type = m_senders[i]->GetType();
			//u_int32_t ip = m_senders[i]->GetHost();
			pCtr->WriteMsg(
				"MSG: sender %u %s %u %u:%u %u %u\n",
				i,
				State2String(state),
				m_senders[i]->m_times_started,
				m_senders[i]->m_conns,
				m_senders[i]->m_max_conns,
//				m_senders[i]->m_start_time.tv_sec,
//				m_senders[i]->m_start_time.tv_usec/1000,
//				m_senders[i]->m_stop_time.tv_sec,
//				m_senders[i]->m_stop_time.tv_usec/1000,
				m_senders[i]->m_bytes_sent,
				m_senders[i]->m_bytes_dropped);
			if (m_senders[i]->GetType() != SEND_TCPSERVER)
				continue;
			CSender* pSender = m_senders[i]->m_sender_list;
			while (pSender) {
				state_enum_t state = pSender->GetState();
				//sender_enum_t type = pSender->GetType();
				//u_int32_t ip = pSender->GetHost();
				pCtr->WriteMsg(
					"MSG: sender %u %s %u %u %u\n",
					i,
					State2String(state),
					pSender->m_times_started,
					pSender->m_bytes_sent,
					pSender->m_bytes_dropped);
					pSender = pSender->m_sender_list;
			}

		}
	if (all) pCtr->WriteMsg("MSG:\n");
	return 0;
}

// sender info [<sender id>]
int CSenders::list_info(CController* pCtr, const char* str) {
	u_int32_t id;
	bool all = true;
	if (!pCtr) return 1;
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (*str) {
		if (sscanf(str, "%u", &id) != 1 || !m_senders ||
			!m_senders[id]) return -1;
		all = false;
	}
	if (all) pCtr->WriteMsg("MSG: sender info verbose %u count %u of "
		"%u\nMSG: sender id type host port mtu queues state start stop fd queues\n",
		m_verbose, m_count_senders, m_max_senders);
	for (u_int32_t i=0 ; i < m_max_senders; i++)
		if (m_senders && m_senders[i] && (all || i == id)) {
			state_enum_t state = m_senders[i]->GetState();
			sender_enum_t type = m_senders[i]->GetType();
			u_int32_t ip = m_senders[i]->GetHost();
			pCtr->WriteMsg("MSG: sender %u %s %u.%u.%u.%u "
				"%d %d %s "
#ifdef HAVE_MACOSX                  // suseconds_t is long int on OS X 10.9
                                	"%ld.%03d %ld.%03d"
#else                               // suseconds_t is unsigned int on Linux
                    			"%ld.%03ld %ld.%03ld"
#endif
				" %d,%d %d <%s>\n",
				i,
				SenderType2String(type),
				ip_parms(ip),
				m_senders[i]->GetPort(),
				m_senders[i]->GetMTU(),
				State2String(state),
				m_senders[i]->m_start_time.tv_sec,
				m_senders[i]->m_start_time.tv_usec/1000,
				m_senders[i]->m_stop_time.tv_sec,
				m_senders[i]->m_stop_time.tv_usec/1000,
				m_senders[i]->m_pCtr ?
				  m_senders[i]->m_pCtr->m_read_fd : -2,
				m_senders[i]->m_pCtr ?
				  m_senders[i]->m_pCtr->m_write_fd : -2,
				m_senders[i]->m_pQueue ?
				  m_senders[i]->m_pQueue->QueueCount() : -1,
				m_senders[i]->GetName() ?
					m_senders[i]->GetName() : "");
			if (m_senders[i]->GetType() != SEND_TCPSERVER)
				continue;
			CSender* pSender = m_senders[i]->m_sender_list;
			while (pSender) {
				state_enum_t state = pSender->GetState();
				sender_enum_t type = pSender->GetType();
				u_int32_t ip = pSender->GetHost();
				pCtr->WriteMsg("MSG: sender %u %s "
					"%u.%u.%u.%u %d %d %s "
#ifdef HAVE_MACOSX                  // suseconds_t is long int on OS X 10.9
                                	"%ld.%03d %ld.%03d"
#else                               // suseconds_t is unsigned int on Linux
                    			"%ld.%03ld %ld.%03ld"
#endif
					"%d - <%s>\n",
					i,
					SenderType2String(type),
					ip_parms(ip),
					pSender->GetPort(),
					pSender->GetMTU(),
					pSender->m_start_time.tv_sec,
					pSender->m_start_time.tv_usec/1000,
					pSender->m_stop_time.tv_sec,
					pSender->m_stop_time.tv_usec/1000,
					State2String(state),
				pSender->m_pCtr ? pSender->m_pCtr->m_read_fd : -2,
					pSender->GetName() ?
						pSender->GetName() : "");
					pSender = pSender->m_sender_list;
			}

		}
	if (all) pCtr->WriteMsg("MSG:\n");
	return 0;
}

// sender verbose [<level>]
int CSenders::set_verbose(CController* pCtr, const char* str) {
	if (!str) return -1;
	while (isspace(*str)) str++;

	// Should we print verbose level?
	if (!(*str)) {
		if (!pCtr) return 1;
		pCtr->WriteMsg("MSG: sender verbose %u\n", m_verbose);
		return 0;
	}
	// We should set level
	if ((sscanf(str,"%u", &m_verbose) != 1)) return -1;
	if (pCtr) pCtr->WriteMsg("MSG: sender verbose level set to %u\n",
		m_verbose);
	return 0;
}

// sender start [ <sender id> ]
// sender stop [ <sender id> ]
int CSenders::startstop_sender(CController* pCtr, const char* str, bool start) {

	u_int32_t id;
	if (!str) return -1;
	if (!m_senders) return 1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (!pCtr) return 1;
		for (id = 0; id < m_max_senders; id++) {
			if (m_senders[id]) {
				state_enum_t state =
					m_senders[id]->GetState();
				pCtr->WriteMsg("MSG: sender %u %s\n",
					id, State2String(state));
			}
		}
		pCtr->WriteMsg("MSG:\n");
		return 0;
	}

	// We are to start a Sender
	if (sscanf(str, "%u", &id) != 1) return -1;
	if (id >= m_max_senders || !m_senders[id])
		return -1;

	int n = 1;
	if (start) {
		if (!(n = m_senders[id]->Start())) {
			if (m_senders[id]->m_pCtr) {
				if (!m_senders[id]->m_pQueue) {
					if (m_senders[id]->m_source_type == SOURCE_RECEIVER) {
						m_senders[id]->m_pQueue = m_pReceivers->AddQueue(m_senders[id]->m_source_id, NULL);
						if (!m_senders[id]->m_pQueue) {
							fprintf(stderr, "Failed to add queue for "
								"sender %u to Receiver %u\n",
								id, m_senders[id]->m_source_id);
						}
					}
				} else fprintf(stderr, "Receiver had already a queue associated. Strange\n");
				if (m_senders[id]->m_pQueue) {
					list_add_head(m_controller_list,
						m_senders[id]->m_pCtr);
					if (m_verbose && pCtr) pCtr->WriteMsg("MSG: "
						"sender %u started\n", id);
				} else {
					fprintf(stderr, "No queue for sender %u added. Stopping.", id);
					m_senders[id]->Stop();
					n=1;
				}
			} else {
				fprintf(stderr, "Expected to add controller to "
					"list for Start Sender, but "
					"controller was empty\n");
				m_senders[id]->Stop();
				n = 1;
			}
		}
	} else {	// Stopping
		if (m_senders[id]->m_pCtr) {
			if (m_senders[id]->m_pQueue) {
				if (m_senders[id]->m_source_type == SOURCE_RECEIVER) {
					if (m_pReceivers->RemoveQueue(m_senders[id]->m_source_id, m_senders[id]->m_pQueue)) {
						fprintf(stderr, "Failed to remove sender %u queue from receiver %u.\n",
							id, m_senders[id]->m_source_id);
					} else {
						delete m_senders[id]->m_pQueue;
						m_senders[id]->m_pQueue = NULL;
					}
				} else if (m_verbose) fprintf(stderr, "Stopping sender %u, but its source type was unsupported.\n", id);
			} else if (m_verbose > 1) fprintf(stderr, "Stopping sender %u, but it had no queue.\n", id);
			list_del_element(m_controller_list,
				m_senders[id]->m_pCtr);
//		} else {
//			fprintf(stderr, "Expected to remove controller to list "
//				"for Stop Sender, but controller was "
//				"empty\n");
		}
		n =  m_senders[id]->Stop();
		if (n == 0 && m_verbose && pCtr) pCtr->WriteMsg("MSG: "
			"sender %u stopped\n", id);
	}
	return n;
}

// sender add [ <sender id> [ <name> ]]
int CSenders::add_sender(CController* pCtr, const char* str) {

	u_int32_t id;

	if (!str) return -1;
	if (!m_senders) return 1;
	while (isspace(*str)) str++;

	// See if we should list senders
	if (!(*str)) {
		if (!pCtr) return 1;
		for (id = 0; id < m_max_senders; id++)
			if (m_senders[id])
				pCtr->WriteMsg("MSG: sender %u name %s\n",
					id, m_senders[id]->GetName() ?
					m_senders[id]->GetName() : "");
		pCtr->WriteMsg("MSG:\n");
		return 0;
	}

	// We are to create a Sender
	if (sscanf(str, "%u", &id) != 1) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	if (!(*str)) return -1;
	int n = CreateSender(id, str);
	if (n == 0 && pCtr && m_verbose) pCtr->WriteMsg("MSG: sender %u "
		"created name <%s>\n", id, str);
	return n;
}

// sender send <sender id> <message>
int CSenders::set_send(CController* pCtr, const char* str) {

	u_int32_t id;
	int n, i;
	u_int8_t *p1, *p2;

	if (!str) return -1;
	if (!m_senders) return 1;
	while (isspace(*str)) str++;
	if (sscanf(str, "%u", &id) != 1) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	if (!(*str)) return -1;

	// Now we will parse for escape sequences
	u_int8_t* buffer = (u_int8_t*) strdup(str);
	if (!buffer) return -1;
	i = strlen((char*)buffer);
	if (i > 0 && buffer[i-1] == ' ') buffer[i-1] = '\0';
	p1 = p2 = buffer;
	i = 0;
	while (*p2) {
		if (*p2 == '\\') {
			p2++;
			if (*p2 == '\\') *p1++ = '\\';
			else if (*p2 == ' ') *p1++ = ' ';
			else if (*p2 == 'n') *p1++ = '\n';
			else if (*p2 == 'r') *p1++ = '\r';
			else if (*p2 == '0') *p1++ = '\0';
			else if (*p2 == 't') *p1++ = '\t';
			else if (*p2 == 'b') *p1++ = '\b';
			else return -1;
			p2++;
		} else *p1++ = *p2++;
		i++;
	}
	*p1 = '\0';
	if (!i) {
		if (buffer) free(buffer);
		return -1;
	}
	n = SetSend(id, buffer, i);
	if (n == 0 && pCtr && m_verbose)
		pCtr->WriteMsg("MSG: send sender %u %d bytes\n", id, i);
	if (buffer) free(buffer);

	return n;
}

// sender port <sender id> <port>
int CSenders::set_port(CController* pCtr, const char* str) {

	u_int32_t id, port;

	if (!str) return -1;
	if (!m_senders) return 1;
	while (isspace(*str)) str++;

	// See if we should list ports
	if (!(*str)) {
		if (!pCtr) return 1;
		for (id = 0; id < m_max_senders; id++)
			if (m_senders[id])
				pCtr->WriteMsg("MSG: sender %u port %d\n",
					id, m_senders[id]->GetPort());
		pCtr->WriteMsg("MSG:\n");
		return 0;
	}

	// We are to set the port
	if (sscanf(str, "%u %u", &id, &port) != 2) return -1;
	int n = SetPort(id, port);
	if (n == 0 && pCtr && m_verbose) pCtr->WriteMsg("MSG: sender %u "
		"port %u\n", id, port);
	return n;
}

// sender mtu [ <sender id> <mtu> ]
int CSenders::set_mtu(CController* pCtr, const char* str) {

	u_int32_t id, mtu;

	if (!str) return -1;
	if (!m_senders) return 1;
	while (isspace(*str)) str++;

	// See if we should list ports
	if (!(*str)) {
		if (!pCtr) return 1;
		for (id = 0; id < m_max_senders; id++)
			if (m_senders[id])
				pCtr->WriteMsg("MSG: sender %u mtu %d\n",
					id, m_senders[id]->GetMTU());
		pCtr->WriteMsg("MSG:\n");
		return 0;
	}

	// We are to set the mtu
	if (sscanf(str, "%u %u", &id, &mtu) != 2) return -1;
	int n = SetMTU(id, mtu);
	if (n == 0 && pCtr && m_verbose) pCtr->WriteMsg("MSG: sender %u "
		"mtu %u\n", id, mtu);
	return n;
}

// sender conns [ <sender id> <max connections> ]
int CSenders::set_conns(CController* pCtr, const char* str) {

	u_int32_t id, max;

	if (!str) return -1;
	if (!m_senders) return 1;
	while (isspace(*str)) str++;

	// See if we should list ports
	if (!(*str)) {
		if (!pCtr) return 1;
		for (id = 0; id < m_max_senders; id++)
			if (m_senders[id])
				pCtr->WriteMsg("MSG: sender %u connections "
					"%u of %u\n",
					id, m_senders[id]->m_conns,
					m_senders[id]->m_max_conns);
		pCtr->WriteMsg("MSG:\n");
		return 0;
	}

	// We are to set the max connections
	if (sscanf(str, "%u %u", &id, &max) != 2) return -1;
	int n = SetMaxConnections(id, max);
	if (n == 0 && pCtr && m_verbose) pCtr->WriteMsg("MSG: sender %u "
		"connections %u\n", id, max);
	return n;
}

// sender source [ <sender id> receiver <id> ]
int CSenders::set_source(CController* pCtr, const char* str) {

	u_int32_t id, source_id;

	if (!str) return -1;
	if (!m_senders) return 1;
	while (isspace(*str)) str++;

	// See if we should list ports
	if (!(*str)) {
		if (!pCtr) return 1;
		for (id = 0; id < m_max_senders; id++)
			if (m_senders[id])
				pCtr->WriteMsg("MSG: sender %u source %s %u\n",
					id, SourceType2String(
						m_senders[id]->m_source_type),
					m_senders[id]->m_source_id);
		pCtr->WriteMsg("MSG:\n");
		return 0;
	}

	// sources : receiver
	//           1234567890

	char source[10];
	source_type_t source_type = SOURCE_UNDEFINED;

	if (sscanf(str, "%u %9s %u", &id, source, &source_id) != 3) return -1;
	if (!(strncmp(source, "rec", 3))) {
		source_type = SOURCE_RECEIVER;
		if (!m_pReceivers || !m_pReceivers->ReceiverExist(source_id))
			return -1;
	} // Add other source types here
	else return -1;

	int n = SetSource(id, source_type, source_id);
	if (n == 0 && pCtr && m_verbose) pCtr->WriteMsg("MSG: sender %u "
		"source %s %u\n", id, SourceType2String(source_type), source_id);
	return n;
}

// sender host [ <sender id> <host ip> ]
int CSenders::set_host(CController* pCtr, const char* str) {

	u_int32_t id, a, b, c, d, host;

	if (!str) return -1;
	if (!m_senders) return 1;
	while (isspace(*str)) str++;

	// See if we should list ports
	if (!(*str)) {
		if (!pCtr) return 1;
		for (id = 0; id < m_max_senders; id++)
			if (m_senders[id]) {
				u_int32_t ip = m_senders[id]->GetHost();
				pCtr->WriteMsg("MSG: sender %u host %u.%u.%u.%u\n",
					id, ip_parms(ip));
			}
		pCtr->WriteMsg("MSG:\n");
		return 0;
	}

	// We are to set the mtu
	if (sscanf(str, "%u %u.%u.%u.%u", &id, &a, &b, &c, &d) != 5 ||
		a>255 || b>255 || c>255 || d>255) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	host = (a<<24) | (b<<16) | (c<<8) | d; 
	int n = SetHost(id, host);
	if (n == 0 && pCtr && m_verbose) pCtr->WriteMsg("MSG: sender %u "
		"host %s\n", id, str);
	return n;
}

// sender type [ <sender id> ( udp | multicast | tcpserver | tcpclient ) ]
int CSenders::set_type(CController* pCtr, const char* str) {

	u_int32_t id;
	sender_enum_t type;

	if (!str) return -1;
	if (!m_senders) return 1;
	while (isspace(*str)) str++;

	// See if we should list ports
	if (!(*str)) {
		if (!pCtr) return 1;
		for (id = 0; id < m_max_senders; id++)
			if (m_senders[id]) {
				type = m_senders[id]->GetType();
				pCtr->WriteMsg("MSG: sender %u type %s\n",
					id, SenderType2String(type));
			}
		pCtr->WriteMsg("MSG:\n");
		return 0;
	}

	// We are to set the type
	if (sscanf(str, "%u", &id) != 1) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	if (!strncmp(str,"udp ", 4)) type = SEND_UDP;
	else if (!strncmp(str,"multicast ", 10)) type = SEND_MULTICAST;
	else if (!strncmp(str,"tcpclient ", 10)) type = SEND_TCPCLIENT;
	else if (!strncmp(str,"tcpserver ", 10)) type = SEND_TCPSERVER;
	else return -1;
	int n = SetType(id, type);
	if (n == 0 && pCtr && m_verbose) pCtr->WriteMsg("MSG: sender %u "
		"type %s\n", id, str);
	return n;
}

// sender host allow <sender id> (<ip> | <network> [ (<ip> | ... ) ])
int CSenders::set_host_allow(CController* pCtr, const char* str) {

	u_int32_t id;
	char prepend[32];

	if (!str) return -1;
	if (!m_senders) return 1;
	while (isspace(*str)) str++;

	// See if we should list ports
	if (!(*str)) {
		if (!pCtr) return 1;
		for (id = 0; id < m_max_senders; id++)
			if (m_senders[id]) {
				if (!m_senders[id]->m_host_allow) {
					pCtr->WriteMsg("MSG: sender %u host "
						"allow any\n");
					continue;
				}
				// "sender 1234567890 host allow"
				//  123456789012345678901234567890
				sprintf(prepend, "sender %u host allow", id);
				SetHostAllow(pCtr, str, &(m_senders[id]->m_host_allow),
					prepend, false);
			}
		pCtr->WriteMsg("MSG:\n");
		return 0;
	}
	if (sscanf(str, "%u", &id) != 1 || !m_senders[id]) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	if (!(*str)) return -1;
	int n = SetHostAllow(pCtr, str, &(m_senders[id]->m_host_allow), "");
	if (n == 0 && pCtr && m_verbose) pCtr->WriteMsg("MSG: sender %u "
		"host allow added %s\n", id, str);
	return n;
}

// Delete a sender holder
int CSenders::DeleteSender(u_int32_t id) {
	if (id >= m_max_senders || !m_senders ||
		!m_senders[id]) return -1;
	delete m_senders[id];
	m_senders[id] = NULL;
	m_count_senders--;
	return 0;
}

// Create a sender holder
int CSenders::CreateSender(u_int32_t id, const char* name) {

	if (id >= m_max_senders || !name) return -1;
	if (!m_senders) return 1;
	while (isspace(*name)) name++;
	if (!(*name)) return -1;

	if (m_senders[id]) DeleteSender(id);

	m_senders[id] = new CSender(this, id, name);
	if (!m_senders[id]) {
		fprintf(stderr, "Failed to create new sender\n");
		return 1;
	}
	if (!m_senders[id]->GetName() || !m_senders[id]->GetBuffer()) {
		fprintf(stderr, "Error in creating sender %u. Dropping it\n",
			id);
		delete m_senders[id];
		m_senders[id] = NULL;
		return 1;
	}
	m_count_senders++;
	return 0;
}

// Set the port for a sender
int CSenders::SetSend(u_int32_t id, u_int8_t* buffer, int32_t n) {

	if (id >= m_max_senders || !m_senders[id] || !n || !buffer)
		return -1;
	return m_senders[id]->SetSend(buffer,n);
}

// Set the port for a sender
int CSenders::SetPort(u_int32_t id, u_int32_t port) {

	if (id >= m_max_senders || !m_senders[id] || port > 65535)
		return -1;
	return m_senders[id]->SetPort(port);
}

// Set the Host for a sender
int CSenders::SetHost(u_int32_t id, u_int32_t host) {

	if (id >= m_max_senders || !m_senders[id])
		return -1;
	return m_senders[id]->SetHost(host);
}

// Set the source for a sender
int CSenders::SetSource(u_int32_t id, source_type_t source_type, u_int32_t source_id) {

	if (id >= m_max_senders || !m_senders[id])
		return -1;
	// Check that we do not already have a Queue
	if (m_senders[id]->m_pQueue) {
		if (m_senders[id]->m_source_type == SOURCE_RECEIVER) {
			if (!m_pReceivers || m_pReceivers->RemoveQueue(m_senders[id]->m_source_id, m_senders[id]->m_pQueue)) {
				fprintf(stderr, "Warning. Failed to remove already set queue for sender %u set to receiver %u\n", id, m_senders[id]->m_source_id);
				return -1;
			} else return -1;
		} else return -1;
	}
	return m_senders[id]->SetSource(source_type, source_id);
}

// Set the mtu for a sender
int CSenders::SetMTU(u_int32_t id, u_int32_t mtu) {

	if (id >= m_max_senders || !m_senders[id] || mtu < 1)
		return -1;
	return m_senders[id]->SetMTU(mtu);
}

// Set the max connections for a sender
int CSenders::SetMaxConnections(u_int32_t id, u_int32_t max) {

	if (id >= m_max_senders || !m_senders[id] || max < 1)
		return -1;
	m_senders[id]->m_max_conns = max;
	return 0;
}

// Set the type for a sender
int CSenders::SetType(u_int32_t id, sender_enum_t type) {

	if (id >= m_max_senders || !m_senders[id] || type == SEND_UNDEFINED)
		return -1;
	return m_senders[id]->SetType(type);
}
