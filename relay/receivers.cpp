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
#include "receivers.h"
#include "queues.h"

/*
CAnalyzerRTP::CAnalyzerRTP() {
	m_verbose		= 0;
	m_packets		= 0;
	m_invalid_packets	= 0;
	m_version		= -1;	// -1 == no packets received
	m_out_of_seq		= 0;
	m_last_payload_type	= 0;
	m_last_seq_no		= 0;
	m_last_timestamp	= 0;
	m_last_SSRC		= 0;
	m_last_CSRC		= 0;
}

CAnalyzerRTP::~CAnalyzerRTP() {
}

int CAnalyzerRTP::NewPacket(u_int8_t *packet, u_int32_t packet_len) {
	if (!packet) return -1;

	if (packet_len < 12) {
		if (m_verbose > 1) fprintf(stderr, "Invalid RTP len %u for packet %u\n", packet_len, m_packets);
		m_invalid_packets++;
		return -2;
	}
	m_packets++;

	u_int16_t seq_no = ntohs(*((u_int16_t*) (packet+2)));
	int version = (0xc0 & (*packet));
	bool padding = (0x20 & (*packet));
	bool extension = (0x10 & (*packet));
	bool csrc_count = (0x0f & (*packet));
	bool marker = (0x80 & (*(packet+1)));
	unsigned int payload_type = (0x7f & (*(packet+1)));
	u_int32_t timestamp = ntohl(*((u_int32_t*) (packet+4)));
	u_int32_t SSRC = ntohl(*((u_int32_t*) (packet+8)));
	u_int32_t* p_CSRC = (u_int32_t*) (packet+12);

	if (extension) {
		
	}
	if (padding || marker || csrc_count) {
	}

	if (m_version < 0) {
		// This is the first valid packet
		m_version = version;
		m_last_payload_type = payload_type;
		m_last_seq_no = seq_no;
		m_last_timestamp = timestamp;
		m_last_SSRC = SSRC;
		m_last_CSRC = ntohl(*p_CSRC);
	} else {
		if (version != m_version) {
			fprintf(stderr, "RTP version shift from %d to %d\n",
				m_version, version);
			m_version = version;
		}
		if (m_last_payload_type != payload_type) {
			fprintf(stderr, "RTP payload shift from %d to %d\n",
				m_last_payload_type, payload_type);
			m_last_payload_type = payload_type;
		}
	}
	m_last_timestamp = timestamp;
	m_last_SSRC = SSRC;
	m_last_CSRC = *p_CSRC;

	return 0;
}
*/


CReceiver::CReceiver(CReceivers* pReceivers, u_int32_t id, const char* name) {
	char QueueName[30];
	if (!(m_pReceivers = pReceivers))
		fprintf(stderr, "Error: Creating receiver %u, the class CReceivers was set to NULL.\n", id);
	m_id		= id;
	if ((m_name = strdup(name))) trim_string(m_name);
	else fprintf(stderr, "Failed to allocate name for receiver\n");
	m_port		= -1;
	m_mtu		= 0;
	m_host		= INADDR_ANY;
	m_state		= SETUP;
	m_sockfd	= -1;
	m_reuseaddr	= true;
	m_reuseport	= true;
	m_conns		= 0;
	m_max_conns	= 1;
	m_receiver_list	= NULL;
	m_pCtr		= NULL;
	m_host_allow	= NULL;
	sprintf(QueueName, "Receiver Queue %u", m_id);
	if (!(m_pQueue = new CQueue(QueueName))) {
		fprintf(stderr, "Failed to get a master Queue for receiver "
			"%u\n", id);
	}
	if (!(m_buffer	= (u_int8_t*) malloc(m_mtu)))
		fprintf(stderr, "Failed to allocate buffer for receiver\n");

	// Stats
	m_start_time.tv_sec = 0; m_start_time.tv_usec = 0;
	m_stop_time.tv_sec = 0; m_stop_time.tv_usec = 0;
	m_times_started = 0;
	m_bytes_received = 0;
	m_bytes_dropped = 0;
	m_bytes_received_last = 0;

	m_pAnalyzerList	= NULL;

	// Slave/Master
	m_master_id	= id;
	m_pSlaveList	= NULL;
}

// When a Receiver is deleted, its controller must be deleted from the
// CReceivers m_controller_list first
CReceiver::~CReceiver() {
	if (m_name) free(m_name);
	if (m_pCtr) delete m_pCtr;
	if (m_pQueue && m_type == REC_TCPSERVERCON) delete m_pQueue;
	while (m_receiver_list) {
		CReceiver* p = m_receiver_list->m_receiver_list;
		delete m_receiver_list;
		m_receiver_list = p;
	}
	while (m_pSlaveList) {
		slave_list_t* p = m_pSlaveList->next;
 		if (m_pReceivers) m_pReceivers->Stop(m_pSlaveList->id);
		free (m_pSlaveList);
		m_pSlaveList = p;
	}
}

int CReceiver::AddSlave(u_int32_t slave_id, bool add, u_int32_t verbose) {
	slave_list_t* ptmp, *p = m_pSlaveList;

	if (add) {
		// We check if it already is added
		while (p) {
			if (p->id == slave_id) break;
			p = p->next;
		}
		if (p) return -1;

		if (!(ptmp = (slave_list_t*) calloc(sizeof(slave_list_t),1))) {
			if (verbose) fprintf(stderr, "Failed to allocate memory for receiver slave %u for receiver master %u\n",
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


int CReceiver::AddAnalyzer(CAnalyzer* pAnalyzer) {
	analyzer_list_t* pAnalyzerList;
	if (!pAnalyzer) return -1;
	if (!(pAnalyzerList = (analyzer_list_t*) calloc(sizeof(analyzer_list_t),1))) return -1;
	pAnalyzerList->pAnalyzer = pAnalyzer;
	pAnalyzerList->next = m_pAnalyzerList;
	m_pAnalyzerList = pAnalyzerList;
	return 0;
}

int CReceiver::RemoveAnalyzer(CAnalyzer* pAnalyzer) {
	analyzer_list_t* pAnalyzerList;
	if (!pAnalyzer || !m_pAnalyzerList) return -1;
	pAnalyzerList = m_pAnalyzerList;
	if (pAnalyzerList->pAnalyzer == pAnalyzer) {
		m_pAnalyzerList = pAnalyzerList->next;
		free(pAnalyzerList);
		return 0;
	}
	while (pAnalyzerList->next) {
		if (pAnalyzerList->next->pAnalyzer == pAnalyzer) {
			pAnalyzerList = pAnalyzerList->next;
			pAnalyzerList->next = pAnalyzerList->next->next;
			free (pAnalyzerList);
			return 0;
		}
		pAnalyzerList = pAnalyzerList->next;
	}
	return -1;
}

void CReceiver::UpdateState() {
	if (m_state == RUNNING || m_state == CONNECTED) {
		if (m_bytes_received == m_bytes_received_last) m_state = STALLED;
	}
	m_bytes_received_last = m_bytes_received;
}

void CReceiver::SetQueue(CQueue* pQueue) {
	if (m_pQueue) delete m_pQueue;
	m_pQueue = pQueue;
}

int CReceiver::Start() {
	int yes = 1;		// For setting socket options
	if (m_state != READY && m_state != STOPPED &&
		m_state != DISCONNECTED) return -1;

	int type;
	if (m_type == REC_UDP || m_type == REC_MULTICAST) type = SOCK_DGRAM;
	else if (m_type == REC_TCPCLIENT || m_type == REC_TCPSERVER)
		type = SOCK_STREAM;
	else return 1;

	// Get a socket
	m_sockfd = socket(AF_INET, type, 0);

	if (m_sockfd < 0) {
		perror("Failed to create socket for receiver");
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
	addr.sin_family = AF_INET;
	addr.sin_port = htons(m_port);
	addr.sin_addr.s_addr = htonl(m_host);

	int write_fd = -1;

	// bind setup
	if (type == SOCK_DGRAM || m_type == REC_TCPSERVER) {
		// Bind to port
		if (bind(m_sockfd, (struct sockaddr *) &addr, sizeof(addr))) {
			perror("Failed to bind socket for receiver");
			close(m_sockfd);
			m_sockfd = -1;
			return 1;
		}
		if (m_type == REC_TCPSERVER) {
			// Listen to IP and port
			if (listen(m_sockfd, 1) < 0) {
				perror("Failed to listen for receiver");
				close(m_sockfd);
				m_sockfd = -1;
				return 1;
			}
		} else if (m_type == REC_MULTICAST) {
			// Multicast 224.0.0.0-239.255.255.255
			// 0x11100000.0.0.0-0x11100111.255.255.255A
			if ((m_host & 0xe0000000) == 0xe0000000) {
				m_mreq.imr_multiaddr.s_addr = htonl(m_host);
				m_mreq.imr_interface.s_addr = htonl(INADDR_ANY);
				if (setsockopt(m_sockfd, IPPROTO_IP,
					IP_ADD_MEMBERSHIP, &m_mreq,
					sizeof(m_mreq)) < 0) {
					perror("Failed to join multicast group "
						"for receiver");
				}
			} else {
				fprintf(stderr, "Warning. IP address is not a "
					"multicast address for receiver of "
					"type multicast");
			}
		}
	} else if (m_type == REC_TCPCLIENT) {
		// Connect to IP and port
		if (connect(m_sockfd, (struct sockaddr *) &addr,
			sizeof(addr))) {
			perror("Failed to connect for receiver");
			close(m_sockfd);
			m_sockfd = -1;
			return 1;
		}
		m_conns = 1;
		write_fd = m_sockfd;
	}
	m_pCtr = new CController(m_sockfd, write_fd);
	if (!m_pCtr) {
		fprintf(stderr, "Failed to get a controller for receiver\n");
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
	if (m_pSlaveList && m_pReceivers) {
		slave_list_t* p = m_pSlaveList;
		while (p) {
			m_pReceivers->Start(p->id);
			p = p->next;
		}
	}
	return 0;
}

int CReceiver::Stop() {
	if (m_state != PENDING && m_state != CONNECTED && m_state != RUNNING &&
		m_state != STALLED) return -1;

	// Set state to stopped, delete controller, which closes the
	// socket if open. Set sockfd to -1
	m_state = STOPPED;
	if (m_pCtr) delete m_pCtr;
	m_pCtr = NULL;
	m_sockfd = -1;
	gettimeofday(&m_stop_time,NULL);
	if (m_type == REC_TCPCLIENT) m_conns = 0;

	// Now check slaves if any and stop them
	if (m_pSlaveList && m_pReceivers) {
		slave_list_t* p = m_pSlaveList;
		while (p) {
			m_pReceivers->Stop(p->id);
			p = p->next;
		}
	}
	return 0;
}

int CReceiver::Connected(bool connected) {
	if (m_type == REC_TCPSERVER) {
		if (connected && m_state == PENDING) m_state = CONNECTED;
		else if (!connected && (m_state == PENDING ||
			m_state == CONNECTED || m_state == RUNNING ||
			m_state == STALLED)) m_state = PENDING;
		else return -1;
	} else if (m_type == REC_TCPSERVERCON) {
		m_state = CONNECTED;
	} else return -1;
	return 0;
}
	

int CReceiver::ReadData(u_int32_t verbose, struct timeval* pTimeNow) {
	u_int32_t ip = 0, port = 0;

	// Check if we have a controller
	if (!m_pCtr || m_pCtr->m_read_fd < 0) {
		fprintf(stderr, "Error. Tried to read on closed receiver %u\n",
			m_id);
		return -1;
	}

	// Now read data, check for EOF (not possible on UDP though)
	int n;
	if (0 && !m_host_allow) {		// receive from all
		n = recv(m_pCtr->m_read_fd, m_buffer, m_mtu, 0);
		if (verbose > 2)
			fprintf(stderr, "Receiver %u fd %d got %d bytes\n",
				m_id, m_pCtr->m_read_fd, n);
	} else {			// receive only from list
		struct sockaddr_in	sender_addr;
		//bzero(&sender_addr, sizeof(sender_addr));
		sender_addr.sin_family = AF_INET;
		socklen_t len = sizeof(sender_addr);
		n = recvfrom(m_pCtr->m_read_fd, m_buffer, m_mtu, 0,
			(struct sockaddr*)(&sender_addr), &len);
		ip = (m_type == REC_UDP ||
			m_type == REC_MULTICAST || m_type == REC_TCPSERVER ?
			htonl(*((u_int32_t*)&sender_addr.sin_addr)) :
			m_type == REC_TCPCLIENT ? m_host : 0);
		port = htons(*((u_int16_t*)&sender_addr.sin_port));
		if (verbose > 2)
			fprintf(stderr, "Receiver %u got %u bytes from "
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
		if (verbose > 1) fprintf(stderr, "EOF on receiver %u\n", m_id);
		return n;
	}

	// Update count of received
	m_bytes_received += n;

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

	// Check if we should analyze
	if (m_pAnalyzerList) {
		analyzer_list_t* p = m_pAnalyzerList;
		while (p) {
			if (p->pAnalyzer) {
				int i = p->pAnalyzer->NewPackets(m_buffer, n, Timeval2U64((*pTimeNow)));
				if (i && verbose > 1) fprintf(stderr, "TS Analyze packets errors %d for receiver %u\n", i, m_id);
			}
			p = p->next;
		}
	}
/*
	// Check if we should drop ts null packets
	if (0 && m_tsanalyzer && m_tsdropnull) {
		int i = m_tsanalyzer->DropNullPackets(m_buffer, n);
		if (i < n) m_bytes_dropped += (n - i);
		if (i == 0) return n;
		n = i;
	}
*/

	if (m_state != RUNNING) m_state = RUNNING;

	// Check if we have a master Queue and actullay some subscribing to the
	// the Queue
	if (m_pQueue && m_pQueue->QueueCount()) {
		int added;
		data_buffer_t* p = m_pQueue->NewBuffer(n, m_buffer);
		if (!p) fprintf(stderr, "Failed to get data buffer for "
			"receiver %u\n", m_id);
		else {
			p->time = *pTimeNow;
			p->ip = ip;
			p->port = port;
//fprintf(stderr, "Adding buffer\n");
			if ((added = m_pQueue->AddBuffer2All(p, false))) {
fprintf(stderr, "Adding buffer failed partly or completely %d\n", added);
				if (added < 0) {
fprintf(stderr, "Adding buffer discarding\n");
					// Adding the buffer failed
					m_pQueue->DiscardBuffer(p);
					m_bytes_dropped += n;
fprintf(stderr, "Adding buffer discarded\n");
				} else {
					// buffer was added to some queues but
					// not all
					if (verbose > 1)
						fprintf(stderr, "Buffer for "
							"receiver %u was only "
							"added to parts of the "
							"buffers\n", m_id);
				}
			}
		}
	} else m_bytes_dropped += n;

	return n;
}

state_enum_t CReceiver::AdjustState() {
	if (m_state == UNDEFINED || m_state == READY ||
		m_state == SETUP || m_state == DISCONNECTED ||
		m_state == STOPPED) {
		if (m_port < 0) m_state = SETUP;
		else if (m_mtu < 1) m_state = SETUP;
		else if (!m_buffer) m_state = SETUP;
		else if (m_type == REC_UNDEFINED) m_state = SETUP;
		else m_state = READY;
	}
	return m_state;
}

int CReceiver::SetSend(u_int8_t* buffer, int32_t n) {
	if (!buffer || !n || !m_pCtr || m_pCtr->m_write_fd < 0 ||
		(m_state != RUNNING && m_state != CONNECTED &&
		 m_state != STALLED)) return -1;
	if (m_pCtr->WriteData(buffer, n, -1) != n) return -1;
	return 0;
}

int CReceiver::SetName(const char* name) {
	if (!name) return -1;
	char* p = strdup(name);
	if (!p) return -1;
	trim_string(p);
	if (m_name) free(m_name);
	m_name = p;
	return 0;
}

int CReceiver::SetPort(u_int32_t port) {
	if (m_state == PENDING || m_state == RUNNING || m_state == CONNECTED ||
		m_state == STALLED || port > 65535) return -1;
	m_port = port;
	AdjustState();
	return 0;
}

int CReceiver::SetHost(u_int32_t host) {
	if (m_state == PENDING || m_state == RUNNING || m_state == CONNECTED ||
		m_state == STALLED) return -1;
	if (!m_mtu) {
/*
		if (host == 0x7f000001) {
			if (SetMTU(65536)) return -1;
		} else {
			if (SetMTU(1500)) return -1;
		}
*/
	}
	m_host = host;
	AdjustState();
	return 0;
}

int CReceiver::SetMTU(u_int32_t mtu) {
	if (m_state == PENDING || m_state == RUNNING || m_state == CONNECTED ||
		m_state == STALLED || mtu < 1) return -1;
	if ((m_type == REC_UDP || m_type == REC_MULTICAST) && mtu > 1492)
		return -1;
	if ((m_type == REC_TCPCLIENT || m_type == REC_TCPSERVER) && mtu > 1480)
		return -1;
	if (m_buffer) {
		u_int8_t* p = (u_int8_t*) malloc(mtu);
		if (!p) {
			m_state = SETUP;
			fprintf(stderr, "Failed to get buffer for receiver %u",
				m_id);
			return 1;
		}
		free(m_buffer);
		m_buffer = p;
fprintf(stderr, "Buffer set to %u for receiver %u\n", mtu, m_id);
	}
	m_mtu = mtu;
	AdjustState();
	return 0;
}

int CReceiver::SetType(receiver_enum_t type) {
	if (m_state == PENDING || m_state == CONNECTED || m_state == RUNNING ||
		m_state == STALLED ||
		(type != REC_UDP && type != REC_MULTICAST &&
		 type != REC_TCPCLIENT && type != REC_TCPSERVER &&
		 type != REC_TCPSERVERCON)) return -1;
	m_type = type;
	AdjustState();
	return 0;
}


CReceivers::CReceivers(u_int32_t max_id) {
	m_max_receivers = max_id;
	m_controller_list = NULL;
	m_verbose = 0;
	m_receivers = (CReceiver**) calloc(sizeof(CReceiver*),max_id);
	if (!m_receivers) {
		fprintf(stderr, "Failed to allocate array for receivers");
		m_max_receivers = 0;
	}
}

CReceivers::~CReceivers() {

	// Delete receivers. This will also delete its controller;
	for (u_int32_t id=0; id < m_max_receivers; id++)
		DeleteReceiver(id);
	m_controller_list  = NULL;	// not strictly necessary
}

// Add or delete a slave
int CReceivers::AddSlave(u_int32_t master_id, u_int32_t slave_id, bool add)
{
	if (!m_receivers || master_id >= m_max_receivers || slave_id >= m_max_receivers || (add && !m_receivers[master_id]) || !m_receivers[slave_id]) return -1;
	if (add) {
		// First we check that the slave is not slave of somebdy else. Is must be its own master
		if (m_receivers[slave_id]->GetMasterID() != slave_id) return -1;

		// Then we check that the slave is not started
		state_enum_t state = m_receivers[slave_id]->GetState();
		if (state == PENDING || state == RUNNING || state == STALLED) return -1;

		// Then we add the slave to master
		if (m_receivers[master_id]->AddSlave(slave_id, true, m_verbose)) {
			if (m_verbose) fprintf(stderr, "Failed to add receiver slave %u to master receiver %u\n",
				slave_id, master_id);
			return 1;
		}
		m_receivers[slave_id]->SetMasterID(master_id);
		// Then we check and see if we should start slave
		state = m_receivers[master_id]->GetState();
		if (state == PENDING || state == RUNNING || state == STALLED) {
			// We may fail starting slave, but the slave has been added to the master list so we see it as succeeded
			if (m_receivers[slave_id]->Start()) {
				fprintf(stderr, "Failed to start receiver slave %u that was added to receiver master %u\n",
				slave_id, master_id);
			}
		}
	} else {
		// We will remove receiver as slave if it is a slave. We don't care about the master_id parameter
		// We also don't care if it started.

		u_int32_t current_master = m_receivers[slave_id]->GetMasterID();
		if (current_master == slave_id) return -1;		// receiver is not a slave

		// Receiver is a slave. Remove it from current_masters slave list
 		m_receivers[current_master]->AddSlave(slave_id, false, m_verbose);
 		m_receivers[slave_id]->SetMasterID(slave_id);
	}
	return 0;
}

int CReceivers::Start(u_int32_t id) {
	if (id >= m_max_receivers || !m_receivers[id]) return -1;
	return m_receivers[id]->Start();
}

int CReceivers::Stop(u_int32_t id) {
	if (id >= m_max_receivers || !m_receivers[id]) return -1;
	return m_receivers[id]->Stop();
}

// Add an analyzer to receiver
int CReceivers::AddAnalyzer(u_int32_t id, CAnalyzer* pAnalyzer) {
	if (id >= m_max_receivers || !m_receivers || !m_receivers[id]) return -1;
	return m_receivers[id]->AddAnalyzer(pAnalyzer);
}
// Remove an analyzer to receiver
int CReceivers::RemoveAnalyzer(u_int32_t id, CAnalyzer* pAnalyzer) {
	if (id >= m_max_receivers || !m_receivers || !m_receivers[id]) return -1;
	return m_receivers[id]->RemoveAnalyzer(pAnalyzer);
}

// Returns Queue if successful, otherwise NULL
CQueue* CReceivers::AddQueue(u_int32_t id, CQueue* pQueue) {
if (m_verbose > 2) fprintf(stderr, "AddQueue for receiver %u queue %s\n", id, pQueue ? "ptr" : "null");
	if (id >= m_max_receivers || !m_receivers[id] ||
		!m_receivers[id]->m_pQueue) return NULL;
	if (m_verbose > 1)
		fprintf(stderr, "Adding Queue to receiver %u\n", id);
	return m_receivers[id]->m_pQueue->AddQueue(pQueue);
}

// Returns false if successful, otherwise true
bool CReceivers::RemoveQueue(u_int32_t id, CQueue* pQueue) {
	if (id >= m_max_receivers || !m_receivers[id] || !pQueue ||
		!m_receivers[id]->m_pQueue) return true;
	if (m_verbose > 1)
		fprintf(stderr, "Removing Queue from receiver %u\n", id);
	return m_receivers[id]->m_pQueue->RemoveQueue(pQueue);
}

void CReceivers::UpdateState() {
	u_int32_t id;
	if (!m_receivers) return;
	for (id=0; id < m_max_receivers; id++) if (m_receivers[id]) m_receivers[id]->UpdateState();
}

int CReceivers::SetFds(int max_fd, fd_set* read_fds, fd_set* write_fds) {
	CController* pCtr;
	for (pCtr = m_controller_list; pCtr != NULL ; pCtr = pCtr->m_next) {
		if (pCtr->m_read_fd > -1) {
			// Add descriptor to read bitmap
			FD_SET(pCtr->m_read_fd, read_fds);
			if (pCtr->m_read_fd > max_fd) max_fd = pCtr->m_read_fd;
		} // else not reading
		if (pCtr->m_write_fd > -1 && pCtr->m_pBuffers) {
			// Data to write. Add descriptor to write bitmap
			FD_SET(pCtr->m_write_fd, write_fds);
			if (pCtr->m_write_fd > max_fd)
				max_fd = pCtr->m_write_fd;
		} // else not writing
	}
	return max_fd;
}

int CReceivers::AcceptNewConnection(u_int32_t id) {
	int new_fd;
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	if (id >= m_max_receivers || !m_receivers[id] ||
		m_receivers[id]->GetType() != REC_TCPSERVER ||
		!m_receivers[id]->m_pCtr ||
		m_receivers[id]->m_pCtr->m_read_fd < 0) return -1;

	// Accept incoming connection
	new_fd = accept(m_receivers[id]->m_pCtr->m_read_fd,
		(struct sockaddr*) &addr, &len);

	// Check if accept failed
	if (new_fd < 0) {
		perror("Failed accept for receiver tcpserver");
		return -1;
	}

	// Check max connections;
	if (m_receivers[id]->m_conns >= m_receivers[id]->m_max_conns) {
		fprintf(stderr, "receiver %u max connections %u reached. "
			"Dropping new connection.\n", id,
			m_receivers[id]->m_max_conns);
		close(new_fd);
		return -1;
	}

	// Check that we have a IPv4 connection
	if (addr.sin_family != AF_INET) {
		fprintf(stderr, "Warning. Non IPv4 connection for receiver "
			"%u tcpserver. Closing connection.\n",id);
		close(new_fd);
		return -1;
	}

	// Check ip against host_allow list if any
	u_int32_t ip = ntohl(*((u_int32_t*)&addr.sin_addr));
	u_int16_t port = ntohs(*((u_int16_t*)&addr.sin_port));
	if (m_receivers[id]->m_host_allow &&
		HostNotAllowed(ip,m_receivers[id]->m_host_allow)) {
		if (m_verbose) fprintf(stderr, "Connection for receiver %u ip "
			"%u.%u.%u.%u denied\n", id, ip_parms(ip));
		close(new_fd);
		return -1;
	}
	if (m_verbose > 1) fprintf(stderr, "Connection for receiver %u ip "
		"%u.%u.%u.%u accepted\n", id, ip_parms(ip));

	// We have a new connection

	// First we set it to non blocking
	if (set_fd_nonblocking(new_fd)) {
		// We failed to set non blocking
		fprintf(stderr, "Warning. Failed to set nonblocking for new incomming connection for receiver %u. Closing connection.\n", id);
		close(new_fd);
		return -1;

	}

	// name = "tcp connection 01234567890"
	//         123456789012345678901234567890
	char name[64];
	sprintf(name, "tcp connection receiver %u fd %d", id, new_fd );

	CReceiver* pReceiver = new CReceiver(this, id, name);
	if (!pReceiver) {
		fprintf(stderr, "Failed to create a new receiver for receiver "
			"%u\n", id);
		close(new_fd);
		return -1;
	}

	// Set settings for the new receiver
	pReceiver->SetPort(port);
	pReceiver->SetMTU(m_receivers[id]->GetMTU());
	pReceiver->SetType(REC_TCPSERVERCON);
	pReceiver->SetHost(ip);
	// This TCPSERVERCON will use the TCPSERVER Queue for data and discard its own
	pReceiver->SetQueue(m_receivers[id]->GetQueue());
	if (!(pReceiver->m_pCtr = new CController(new_fd, new_fd))) {
		fprintf(stderr, "Failed to get a controller for new receiver %u"
			" connection\n", id);
		close(new_fd);
		delete pReceiver;	// Safe to delete a receiver as its controller has not been added to the m_controller_list
		return -1;
	}
	if (!m_receivers[id]->m_receiver_list)
		m_receivers[id]->m_receiver_list = pReceiver;
	else {
		CReceiver* p = m_receivers[id]->m_receiver_list;
		while (p->m_receiver_list) p = p->m_receiver_list;
		p->m_receiver_list = pReceiver;
	}
	list_add_head(m_controller_list, pReceiver->m_pCtr);

	m_receivers[id]->m_conns++;
	m_receivers[id]->Connected(true);
	pReceiver->Connected(true);
	return 0;
}

void CReceivers::CheckReadWrite(fd_set *read_fds, fd_set *write_fds, struct timeval* pTimeNow) {
	if (!m_receivers) return;
	for (u_int32_t id = 0; id < m_max_receivers; id++) {
		if (!m_receivers[id] || !m_receivers[id]->m_pCtr) continue;

		CController* pCtr = m_receivers[id]->m_pCtr;
		// First we check for writes
		if (pCtr->m_write_fd > 0 &&
			FD_ISSET(pCtr->m_write_fd, write_fds)) {
			// Ready for writing
			pCtr->WriteBuffers();
		}

		// Check if receiver is server.
		if (m_receivers[id]->GetType() == REC_TCPSERVER) {

			// We have a server and need to check its receivers
			CReceiver* pReceiver = m_receivers[id]->m_receiver_list;
			while (pReceiver) {
				CReceiver* pNextReceiver =
					pReceiver->m_receiver_list;
				ReceiverCheckReadWrite(pReceiver, read_fds,
					write_fds, pTimeNow);
				pReceiver = pNextReceiver;
			}

			// Check server fd for validity
			if (pCtr->m_read_fd < 0) {
				fprintf(stderr, "Server fd for receiver %u was "
					"unexpectedly invalid\n", id);
				continue;
			}

			// Now we need to check if we have a new connection
			// if we have a new connection we will accept it
			// and add it to the m_receiver_list for the receiver
			// and its controller to m_controller_list
			if (!FD_ISSET(pCtr->m_read_fd, read_fds)) continue;

			// FD was set and we will accept the connection
			if (AcceptNewConnection(id)) {
				fprintf(stderr, "Failed to accept incomming "
					"connection for receiver %u\n", id);
			}
			continue;

		}
		ReceiverCheckReadWrite(m_receivers[id], read_fds, write_fds,
			pTimeNow);
	}
}

int CReceivers::ReceiverCheckReadWrite(CReceiver* pReceiver, fd_set *read_fds, fd_set *write_fds, struct timeval* pTimeNow) {

	CController* pCtr = pReceiver->m_pCtr;

	if (pCtr->m_read_fd < 0) return 1;	// Not readable

	if (!FD_ISSET(pCtr->m_read_fd, read_fds))
		return 1;			// No data ready
	//pReceiver->GetID(), pReceiver->GetName(), pReceiver->m_pCtr->m_read_fd);

	// Now read data, check for EOF (not possible on UDP though)
	int n = pReceiver->ReadData(m_verbose, pTimeNow);
	if (n <= 0) {

		// We have EOF and need to remove the controller from the
		// m_controller_list
		if (pCtr) {
			list_del_element(m_controller_list, pCtr);
		}

		// Now we can stop the receiver which will close the connection
		pReceiver->Stop();

		// If the receiver is a connection generated by a TCP_SERVER
		// we must delete the receiver
		if (pReceiver->GetType() == REC_TCPSERVERCON) {

			// A server connection and the server has same ID
			u_int32_t id = pReceiver->GetID();
			if (!m_receivers || !m_receivers[id]) {
				fprintf(stderr, "Error. Could not find the "
					"receiver server for a closed server "
					"receiver %u connection. Will cause a "
					"memory leak\n", id);
				return -1;
			}
			CReceiver* p = m_receivers[id];
			if (!p->m_receiver_list) {
				fprintf(stderr, "Error. Server receiver %u "
					"had no receiver list for a closed "
					"server connection. Will cause a "
					"memory leak.\n", id);
				return -1;
			}
			if (p->m_receiver_list == pReceiver) {
				p->m_receiver_list =
					pReceiver->m_receiver_list;
				pReceiver->m_receiver_list = NULL;
				delete pReceiver;	// Safe to delete a receiver as its controller has been removed from the m_controller_list
			} else {
				while (p->m_receiver_list->m_receiver_list) {
					if (p->m_receiver_list->m_receiver_list == pReceiver) {
						p->m_receiver_list->m_receiver_list = pReceiver->m_receiver_list;
						pReceiver->m_receiver_list = NULL;
						delete pReceiver;
						pReceiver = NULL;
						break;
					}
					p = p->m_receiver_list; // Safe to delete a receiver as its controller has been removed from the m_controller_list
				}
				if (pReceiver) {
					fprintf(stderr, "Failed to find the "
						"server receiver closed in the "
						"servers receiver %u list of "
						"receivers. Will cause a "
						"memory leak.\n", id);
					return -1;
				}
			}
			if (!m_receivers[id]->m_receiver_list) {
				m_receivers[id]->Connected(false);
			}
			m_receivers[id]->m_conns--;
		}
	}
	return 0;
}

bool CReceivers::ReceiverExist(u_int32_t id) {
	return (m_receivers && id < m_max_receivers && m_receivers[id] ?
		true : false);
}

// Parsing commands starting with 'receiver '
// Returns : 0=OK, 1=System error, -1=Syntax error
int CReceivers::ParseCommand(CController* pCtr, const char* str) {

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
		return startstop_receiver(pCtr, str+6, true);
	else if (!strncmp(str, "stop ", 5))
		return startstop_receiver(pCtr, str+5, false);
	else if (!strncmp(str, "port ", 5))
		return set_port(pCtr, str+5);
	else if (!strncmp(str, "type ", 5))
		return set_type(pCtr, str+5);
	else if (!strncmp(str, "mtu ", 4))
		return set_mtu(pCtr, str+4);
	else if (!strncmp(str, "conns ", 6))
		return set_conns(pCtr, str+6);
	else if (!strncmp(str, "add ", 4))
		return add_receiver(pCtr, str+4);
	else if (!strncmp(str, "host allow ", 11))
		return set_host_allow(pCtr, str+11);
	else if (!strncmp(str, "host ", 5))
		return set_host(pCtr, str+5);
	else if (!strncmp(str, "name ", 5))
		return set_name(pCtr, str+5);
	else if (!strncmp(str, "master ", 7))
		return set_master(pCtr, str+7);
	else if (!strncmp(str, "verbose ", 8))
		return set_verbose(pCtr, str+8);
//	else if (!strncmp(str, "analyzer ", 9))
//		return set_analyzer(pCtr, str+9);
	else if (!strncmp(str, "help ", 5))
		return list_help(pCtr, str+5);
	//else fprintf(stderr, "No match for <%s>", str);
	// else no match
	return -1;
}

// receiver help
int CReceivers::list_help(CController* pCtr, const char* str) {
	if (!pCtr) return 1;
	pCtr->WriteMsg("MSG: receiver commands:\n"
		"MSG: receiver add [<receiver id> [<receiver name>]]\n"
		"MSG: receiver conns [<receiver id> <max connections>]\n"
		"MSG: receiver host allow [<receiver id> ( <ip> | <network> ... )]\n"
		"MSG: receiver info [<receiver id>]\n"
		"MSG: receiver master [<receiver id> [receiver <receiver master id>]]\n"
		"MSG: receiver mtu [<receiver id> <mtu>]\n"
		"MSG: receiver port [<receiver id> <port>]\n"
		"MSG: receiver start [<receiver id>]\n"
		"MSG: receiver send <receiver id> <message>\n"
		"MSG: receiver slave [<receiver id> [receiver <receiver master id>]]\n"
		"MSG: receiver stop [<receiver id>]\n"
		"MSG: receiver verbose [<level>]\n"
//		"MSG: receiver analyzer ts <receiver id> [(start|stop|list)]\n"
		"MSG: receiver help\n"
		"MSG:\n");
	return 0;
}

// receiver status [<receiver id>]
int CReceivers::list_status(CController* pCtr, const char* str) {
	u_int32_t id;
	bool all = true;
	if (!pCtr) return 1;
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (*str) {
		if (sscanf(str, "%u", &id) != 1 || !m_receivers ||
			!m_receivers[id]) return -1;
		all = false;
	}
	struct timeval time;
	gettimeofday(&time,NULL);
	if (all) pCtr->WriteMsg("MSG: receiver status "
#ifdef HAVE_MACOSX                  // suseconds_t is long int on OS X 10.9
                                "%ld.%03d"
#else                               // suseconds_t is unsigned int on Linux
                    		"%ld.%03ld"
#endif
		"\nMSG: receiver id state started receivers received dropped\n",
		time.tv_sec, time.tv_usec/1000,
		m_verbose, m_count_receivers, m_max_receivers);
	for (u_int32_t i=0 ; i < m_max_receivers; i++)
		if (m_receivers && m_receivers[i] && (all || i == id)) {
			state_enum_t state = m_receivers[i]->GetState();
			//receiver_enum_t type = m_receivers[i]->GetType();
			//u_int32_t ip = m_receivers[i]->GetHost();
			pCtr->WriteMsg(
				"MSG: receiver %u %s %u %u:%u %u %u\n",
				i,
				State2String(state),
				m_receivers[i]->m_times_started,
				m_receivers[i]->m_conns,
				m_receivers[i]->m_max_conns,
//				m_receivers[i]->m_start_time.tv_sec,
//				m_receivers[i]->m_start_time.tv_usec/1000,
//				m_receivers[i]->m_stop_time.tv_sec,
//				m_receivers[i]->m_stop_time.tv_usec/1000,
				m_receivers[i]->m_bytes_received,
				m_receivers[i]->m_bytes_dropped);
			if (m_receivers[i]->GetType() != REC_TCPSERVER)
				continue;
			CReceiver* pReceiver = m_receivers[i]->m_receiver_list;
			while (pReceiver) {
				state_enum_t state = pReceiver->GetState();
				//receiver_enum_t type = pReceiver->GetType();
				//u_int32_t ip = pReceiver->GetHost();
				pCtr->WriteMsg(
					"MSG: receiver %u %s %u %u %u\n",
					i,
					State2String(state),
					pReceiver->m_times_started,
//					pReceiver->m_start_time.tv_sec,
//					pReceiver->m_start_time.tv_usec/1000,
//					pReceiver->m_stop_time.tv_sec,
//					pReceiver->m_stop_time.tv_usec/1000,
					pReceiver->m_bytes_received,
					pReceiver->m_bytes_dropped);
					pReceiver = pReceiver->m_receiver_list;
			}

		}
	if (all) pCtr->WriteMsg("MSG:\n");
	return 0;
}

// receiver info [<receiver id>]
int CReceivers::list_info(CController* pCtr, const char* str) {
	u_int32_t id;
	bool all = true;
	if (!pCtr) return 1;
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (*str) {
		if (sscanf(str, "%u", &id) != 1 || !m_receivers ||
			!m_receivers[id]) return -1;
		all = false;
	}
	if (all) pCtr->WriteMsg("MSG: receiver info verbose %u count %u of "
		"%u\nMSG: receiver id type host port mtu queues state start stop fd queues\n",
		m_verbose, m_count_receivers, m_max_receivers);
	for (u_int32_t i=0 ; i < m_max_receivers; i++)
		if (m_receivers && m_receivers[i] && (all || i == id)) {
			state_enum_t state = m_receivers[i]->GetState();
			receiver_enum_t type = m_receivers[i]->GetType();
			u_int32_t ip = m_receivers[i]->GetHost();
			pCtr->WriteMsg("MSG: receiver %u %s %u.%u.%u.%u %d %d %s "
#ifdef HAVE_MACOSX                  // suseconds_t is long int on OS X 10.9
                                "%ld.%03d %ld.%03d"
#else                               // suseconds_t is unsigned int on Linux
                    		"%ld.%03ld %ld.%03ld"
#endif
				" %d,%d %d <%s>\n",
				i,
				ReceiverType2String(type),
				ip_parms(ip),
				m_receivers[i]->GetPort(),
				m_receivers[i]->GetMTU(),
				State2String(state),
				m_receivers[i]->m_start_time.tv_sec,
				m_receivers[i]->m_start_time.tv_usec/1000,
				m_receivers[i]->m_stop_time.tv_sec,
				m_receivers[i]->m_stop_time.tv_usec/1000,
				m_receivers[i]->m_pCtr ?
				  m_receivers[i]->m_pCtr->m_read_fd : -2,
				m_receivers[i]->m_pCtr ?
				  m_receivers[i]->m_pCtr->m_write_fd : -2,
				m_receivers[i]->m_pQueue ?
				  m_receivers[i]->m_pQueue->QueueCount() : -1,
				m_receivers[i]->GetName() ?
					m_receivers[i]->GetName() : "");
			if (m_receivers[i]->GetType() != REC_TCPSERVER)
				continue;
			CReceiver* pReceiver = m_receivers[i]->m_receiver_list;
			while (pReceiver) {
				state_enum_t state = pReceiver->GetState();
				receiver_enum_t type = pReceiver->GetType();
				u_int32_t ip = pReceiver->GetHost();
				pCtr->WriteMsg("MSG: receiver %u %s "
					"%u.%u.%u.%u %d %d %s "
#ifdef HAVE_MACOSX                  // suseconds_t is long int on OS X 10.9
                                	"%ld.%03d %ld.%03d"
#else                               // suseconds_t is unsigned int on Linux
                    			"%ld.%03ld %ld.%03ld"
#endif
					"%d - <%s>\n",
					i,
					ReceiverType2String(type),
					ip_parms(ip),
					pReceiver->GetPort(),
					pReceiver->GetMTU(),
					State2String(state),
					pReceiver->m_start_time.tv_sec,
					pReceiver->m_start_time.tv_usec/1000,
					pReceiver->m_stop_time.tv_sec,
					pReceiver->m_stop_time.tv_usec/1000,
					pReceiver->m_pCtr ? pReceiver->m_pCtr->m_read_fd : -2,
					pReceiver->GetName() ?
						pReceiver->GetName() : "");
					pReceiver = pReceiver->m_receiver_list;
			}

		}
	if (all) pCtr->WriteMsg("MSG:\n");
	return 0;
}

// receiver verbose [<level>]
int CReceivers::set_verbose(CController* pCtr, const char* str) {
	if (!str) return -1;
	while (isspace(*str)) str++;

	// Should we print verbose level?
	if (!(*str)) {
		if (!pCtr) return 1;
		pCtr->WriteMsg("MSG: receiver verbose %u\n", m_verbose);
		return 0;
	}
	// We should set level
	if ((sscanf(str,"%u", &m_verbose) != 1)) return -1;
	if (pCtr) pCtr->WriteMsg("MSG: receiver verbose level set to %u\n",
		m_verbose);
	return 0;
}

// receiver start [ <receiver id> ]
// receiver stop [ <receiver id> ]
int CReceivers::startstop_receiver(CController* pCtr, const char* str, bool start) {

	u_int32_t id;
	if (!str) return -1;
	if (!m_receivers) return 1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (!pCtr) return 1;
		for (id = 0; id < m_max_receivers; id++) {
			if (m_receivers[id]) {
				state_enum_t state =
					m_receivers[id]->GetState();
				pCtr->WriteMsg("MSG: receiver %u %s\n",
					id, State2String(state));
			}
		}
		pCtr->WriteMsg("MSG:\n");
		return 0;
	}

	// We are to start a Receiver
	if (sscanf(str, "%u", &id) != 1) return -1;
	if (id >= m_max_receivers || !m_receivers[id])
		return -1;

	int n = 1;
	if (start) {
		if (!(n = m_receivers[id]->Start())) {
			if (m_receivers[id]->m_pCtr) {
				list_add_head(m_controller_list,
					m_receivers[id]->m_pCtr);
				if (m_verbose && pCtr) pCtr->WriteMsg("Msg: "
					"receiver %u started\n", id);
			} else {
				fprintf(stderr, "Expected to add controller to "
					"list for Start Receiver, but "
					"controller was empty\n");
				m_receivers[id]->Stop();
				n = 1;
			}
		}
	} else {
		if (m_receivers[id]->m_pCtr) {
			list_del_element(m_controller_list,
				m_receivers[id]->m_pCtr);
//		} else {
//			fprintf(stderr, "Expected to remove controller to list "
//				"for Stop Receiver, but controller was "
//				"empty\n");
		}
		n =  m_receivers[id]->Stop();
		if (n == 0 && m_verbose && pCtr) pCtr->WriteMsg("Msg: "
			"receiver %u stopped\n", id);
	}
	return n;
}

// receiver add [ <receiver id> [ <name> ]]
int CReceivers::add_receiver(CController* pCtr, const char* str) {

	u_int32_t id;

	if (!str) return -1;
	if (!m_receivers) return 1;
	while (isspace(*str)) str++;

	// See if we should list receivers
	if (!(*str)) {
		if (!pCtr) return 1;
		for (id = 0; id < m_max_receivers; id++)
			if (m_receivers[id])
				pCtr->WriteMsg("MSG: receiver %u name %s\n",
					id, m_receivers[id]->GetName() ?
					m_receivers[id]->GetName() : "");
		pCtr->WriteMsg("MSG:\n");
		return 0;
	}

	// We are to create a Receiver
	if (sscanf(str, "%u", &id) != 1) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	if (!(*str)) return -1;
	int n = CreateReceiver(id, str);
	if (n == 0 && pCtr && m_verbose) pCtr->WriteMsg("MSG: receiver %u "
		"created name <%s>\n", id, str);
	return n;
}

// receiver name [ <receiver id> <name> ]
int CReceivers::set_name(CController* pCtr, const char* str) {

	u_int32_t id;

	if (!str) return -1;
	if (!m_receivers) return 1;
	while (isspace(*str)) str++;

	// See if we should list receivers
	if (!(*str)) {
		if (!pCtr) return 1;
		for (id = 0; id < m_max_receivers; id++)
			if (m_receivers[id])
				pCtr->WriteMsg("MSG: receiver %u name %s\n",
					id, m_receivers[id]->GetName() ?
					m_receivers[id]->GetName() : "");
		pCtr->WriteMsg("MSG:\n");
		return 0;
	}

	// We are to change the name
	if (sscanf(str, "%u", &id) != 1) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	if (!(*str)) return -1;
	int n = SetName(id, str);
	if (n == 0 && pCtr && m_verbose) pCtr->WriteMsg("MSG: receiver %u "
		"changed name to <%s>\n", id, str);
	return n;
}

// receiver master [<receiver id> [receiver <receiver master id>]]
int CReceivers::set_master(CController* pCtr, const char* str) {

	u_int32_t id, master_id;
	int n;

	if (!str) return -1;
	if (!m_receivers) return 1;
	while (isspace(*str)) str++;

	// See if we should list receivers
	if (!(*str)) {
		if (!pCtr) return 1;
		for (id = 0; id < m_max_receivers; id++)
			if (m_receivers[id])
				pCtr->WriteMsg("MSG: receiver %u master %u slaves %s\n",
					id, m_receivers[id]->GetMasterID(), m_receivers[id]->GetSlaveList() ? "yes" : "no");
		pCtr->WriteMsg("MSG:\n");
		return 0;
	}

	// We are to change the name
	if (sscanf(str, "%u", &id) != 1 || !m_receivers[id]) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	if (!(*str)) {
		// We are to unset receiver id as slave
		n = AddSlave(0, id, false);
	} else {
		if (strncmp(str, "rec", 3)) return -1;
		while (*str && !isspace(*str)) str++;
		while (isspace(*str)) str++;
		if (!(*str)) return -1;
		if (sscanf(str, "%u", &master_id) != 1) return -1;
		n = AddSlave(master_id, id, true);
	}
	if (m_verbose && !n && pCtr) pCtr->WriteMsg("MSG: receiver %u master receiver %u\n",
		id, *str ? master_id : id);
	return n;
}

// receiver send <receiver id> <message>
int CReceivers::set_send(CController* pCtr, const char* str) {

	u_int32_t id;
	int n, i;
	u_int8_t *p1, *p2;

	if (!str) return -1;
	if (!m_receivers) return 1;
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
		pCtr->WriteMsg("MSG: send receiver %u %d bytes\n", id, i);
	if (buffer) free(buffer);

	return n;
}

// receiver port <receiver id> <port>
int CReceivers::set_port(CController* pCtr, const char* str) {

	u_int32_t id, port;

	if (!str) return -1;
	if (!m_receivers) return 1;
	while (isspace(*str)) str++;

	// See if we should list ports
	if (!(*str)) {
		if (!pCtr) return 1;
		for (id = 0; id < m_max_receivers; id++)
			if (m_receivers[id])
				pCtr->WriteMsg("MSG: receiver %u port %d\n",
					id, m_receivers[id]->GetPort());
		pCtr->WriteMsg("MSG:\n");
		return 0;
	}

	// We are to set the port
	if (sscanf(str, "%u %u", &id, &port) != 2) return -1;
	int n = SetPort(id, port);
	if (n == 0 && pCtr && m_verbose) pCtr->WriteMsg("MSG: receiver %u "
		"port %u\n", id, port);
	return n;
}

// receiver mtu [ <receiver id> <mtu> ]
int CReceivers::set_mtu(CController* pCtr, const char* str) {

	u_int32_t id, mtu;

	if (!str) return -1;
	if (!m_receivers) return 1;
	while (isspace(*str)) str++;

	// See if we should list ports
	if (!(*str)) {
		if (!pCtr) return 1;
		for (id = 0; id < m_max_receivers; id++)
			if (m_receivers[id])
				pCtr->WriteMsg("MSG: receiver %u mtu %d\n",
					id, m_receivers[id]->GetMTU());
		pCtr->WriteMsg("MSG:\n");
		return 0;
	}

	// We are to set the mtu
	if (sscanf(str, "%u %u", &id, &mtu) != 2) return -1;
	int n = SetMTU(id, mtu);
	if (n == 0 && pCtr && m_verbose) pCtr->WriteMsg("MSG: receiver %u "
		"mtu %u\n", id, mtu);
	return n;
}

// receiver conns [ <receiver id> <max connections> ]
int CReceivers::set_conns(CController* pCtr, const char* str) {

	u_int32_t id, max;

	if (!str) return -1;
	if (!m_receivers) return 1;
	while (isspace(*str)) str++;

	// See if we should list ports
	if (!(*str)) {
		if (!pCtr) return 1;
		for (id = 0; id < m_max_receivers; id++)
			if (m_receivers[id])
				pCtr->WriteMsg("MSG: receiver %u connections "
					"%u of %u\n",
					id, m_receivers[id]->m_conns,
					m_receivers[id]->m_max_conns);
		pCtr->WriteMsg("MSG:\n");
		return 0;
	}

	// We are to set the max connections
	if (sscanf(str, "%u %u", &id, &max) != 2) return -1;
	int n = SetMaxConnections(id, max);
	if (n == 0 && pCtr && m_verbose) pCtr->WriteMsg("MSG: receiver %u "
		"connections %u\n", id, max);
	return n;
}

// receiver host [ <receiver id> <host ip> ]
int CReceivers::set_host(CController* pCtr, const char* str) {

	u_int32_t id, a, b, c, d, host;

	if (!str) return -1;
	if (!m_receivers) return 1;
	while (isspace(*str)) str++;

	// See if we should list ports
	if (!(*str)) {
		if (!pCtr) return 1;
		for (id = 0; id < m_max_receivers; id++)
			if (m_receivers[id]) {
				u_int32_t ip = m_receivers[id]->GetHost();
				pCtr->WriteMsg("MSG: receiver %u host "
					"%u.%u.%u.%u\n", id, ip_parms(ip));
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
	if (n == 0 && pCtr && m_verbose) pCtr->WriteMsg("MSG: receiver %u "
		"host %s\n", id, str);
	return n;
}

// receiver type [ <receiver id> ( udp | multicast | tcpserver | tcpclient ) ]
int CReceivers::set_type(CController* pCtr, const char* str) {

	u_int32_t id;
	receiver_enum_t type;

	if (!str) return -1;
	if (!m_receivers) return 1;
	while (isspace(*str)) str++;

	// See if we should list ports
	if (!(*str)) {
		if (!pCtr) return 1;
		for (id = 0; id < m_max_receivers; id++)
			if (m_receivers[id]) {
				type = m_receivers[id]->GetType();
				pCtr->WriteMsg("MSG: receiver %u type %s\n",
					id, ReceiverType2String(type));
			}
		pCtr->WriteMsg("MSG:\n");
		return 0;
	}

	// We are to set the type
	if (sscanf(str, "%u", &id) != 1) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	if (!strncmp(str,"udp ", 4)) type = REC_UDP;
	else if (!strncmp(str,"multicast ", 10)) type = REC_MULTICAST;
	else if (!strncmp(str,"tcpclient ", 10)) type = REC_TCPCLIENT;
	else if (!strncmp(str,"tcpserver ", 10)) type = REC_TCPSERVER;
	else return -1;
	int n = SetType(id, type);
	if (n == 0 && pCtr && m_verbose) pCtr->WriteMsg("MSG: receiver %u "
		"type %s\n", id, str);
	return n;
}

// receiver host allow <receiver id> (<ip> | <network> [ (<ip> | ... ) ])
int CReceivers::set_host_allow(CController* pCtr, const char* str) {

	u_int32_t id;
	char prepend[32];

	if (!str) return -1;
	if (!m_receivers) return 1;
	while (isspace(*str)) str++;

	// See if we should list ports
	if (!(*str)) {
		if (!pCtr) return 1;
		for (id = 0; id < m_max_receivers; id++)
			if (m_receivers[id]) {
				if (!m_receivers[id]->m_host_allow) {
					pCtr->WriteMsg("MSG: receiver %u host "
						"allow any\n");
					continue;
				}
				// "receiver 1234567890 host allow"
				//  123456789012345678901234567890
				sprintf(prepend, "receiver %u host allow", id);
				SetHostAllow(pCtr, str, &(m_receivers[id]->m_host_allow),
					prepend, false);
			}
		pCtr->WriteMsg("MSG:\n");
		return 0;
	}
	if (sscanf(str, "%u", &id) != 1 || !m_receivers[id]) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	if (!(*str)) return -1;
	int n = SetHostAllow(pCtr, str, &(m_receivers[id]->m_host_allow), "");
	if (n == 0 && pCtr && m_verbose) pCtr->WriteMsg("MSG: receiver %u "
		"host allow added %s\n", id, str);
	return n;
}

// Delete a receiver holder
int CReceivers::DeleteReceiver(u_int32_t id) {
	if (id >= m_max_receivers || !m_receivers ||
		!m_receivers[id]) return -1;
	delete m_receivers[id];
	m_receivers[id] = NULL;
	m_count_receivers--;
	return 0;
}

// Create a receiver holder
int CReceivers::CreateReceiver(u_int32_t id, const char* name) {

	if (id >= m_max_receivers || !name) return -1;
	if (!m_receivers) return 1;
	while (isspace(*name)) name++;
	if (!(*name)) return -1;

	if (m_receivers[id]) DeleteReceiver(id);

	m_receivers[id] = new CReceiver(this, id, name);
	if (!m_receivers[id]) {
		fprintf(stderr, "Failed to create new receiver\n");
		return 1;
	}
	if (!m_receivers[id]->GetName() || !m_receivers[id]->GetBuffer()) {
		fprintf(stderr, "Error in creating receiver %u. Dropping it\n",
			id);
		delete m_receivers[id];
		m_receivers[id] = NULL;
		return 1;
	}
	m_count_receivers++;
	return 0;
}

// Change a receiver name
int CReceivers::SetName(u_int32_t id, const char* name) {

	if (id >= m_max_receivers || !name) return -1;
	if (!m_receivers) return 1;
	if (!m_receivers[id]) return -1;
	while (isspace(*name)) name++;
	if (!(*name)) return -1;

	return m_receivers[id]->SetName(name);
	return 0;
}

// Set the port for a receiver
int CReceivers::SetSend(u_int32_t id, u_int8_t* buffer, int32_t n) {

	if (id >= m_max_receivers || !m_receivers[id] || !n || !buffer)
		return -1;
	return m_receivers[id]->SetSend(buffer,n);
}

// Set the port for a receiver
int CReceivers::SetPort(u_int32_t id, u_int32_t port) {

	if (id >= m_max_receivers || !m_receivers[id] || port > 65535)
		return -1;
	return m_receivers[id]->SetPort(port);
}

// Set the Host for a receiver
int CReceivers::SetHost(u_int32_t id, u_int32_t host) {

	if (id >= m_max_receivers || !m_receivers[id])
		return -1;
	return m_receivers[id]->SetHost(host);
}

// Set the mtu for a receiver
int CReceivers::SetMTU(u_int32_t id, u_int32_t mtu) {

	if (id >= m_max_receivers || !m_receivers[id] || mtu < 1)
		return -1;
	return m_receivers[id]->SetMTU(mtu);
}

// Set the max connections for a receiver
int CReceivers::SetMaxConnections(u_int32_t id, u_int32_t max) {

	if (id >= m_max_receivers || !m_receivers[id] || max < 1)
		return -1;
	m_receivers[id]->m_max_conns = max;
	return 0;
}

// Set the type for a receiver
int CReceivers::SetType(u_int32_t id, receiver_enum_t type) {

	if (id >= m_max_receivers || !m_receivers[id] || type == REC_UNDEFINED)
		return -1;
	return m_receivers[id]->SetType(type);
}
