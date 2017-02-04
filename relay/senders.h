/*
 * (c) 2014 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */

#ifndef __SENDERS_H__
#define __SENDERS_H__

#include <sys/types.h>
//#include <stdint.h>
//#include <netinet/in.h>

#include "controller.h"
#include "receivers.h"
#include "relay.h"

enum sender_enum_t {
	SEND_UNDEFINED,
	SEND_UDP,
	SEND_MULTICAST,
	SEND_TCPCLIENT,
	SEND_TCPSERVER,
	SEND_TCPSERVERCON
};

#define SenderType2String(a) ( a == SEND_UNDEFINED ? "undefined" : \
	a == SEND_UDP ? "udp" :				\
	  a == SEND_MULTICAST ? "multicast" :		\
	    a == SEND_TCPCLIENT ? "tcpclient" :		\
	      a == SEND_TCPSERVER ? "tcpserver" :	\
	        a == SEND_TCPSERVERCON ? "tcpconnect" :	\
	          "undefined" )

class CQueue;
class CSenders;

class CSender {
  public:
	CSender(CSenders* pSenders, u_int32_t id, const char* name);
	~CSender();
	void		UpdateState();
	const char*	GetName() { return m_name; }
	int		GetPort() { return m_port; }
	int		GetMTU() { return m_mtu; }
	int		GetHost() { return m_host; }
	state_enum_t	GetState() { return m_state; }
	sender_enum_t	GetType() { return m_type; }
	u_int32_t	GetID() { return m_id; }
        u_int32_t	GetMasterID() { return m_master_id; }
        void		SetMasterID(u_int32_t master_id) { m_master_id = master_id; }
	CQueue*		GetQueue() { return m_pQueue; }
	u_int8_t*	GetBuffer() { return m_buffer; }
	int		SetPort(u_int32_t port);
	int		SetSend(u_int8_t* buffer, int32_t n);
	int		SetMTU(u_int32_t mtu);
	int		SetHost(u_int32_t host);
	int		SetSource(source_type_t source_type,
				u_int32_t source_id);
	int		SetType(sender_enum_t type);
	void		SetQueue(CQueue* pQueue);
	slave_list_t*	GetSlaveList() { return m_pSlaveList; };
	int		Start();
	int		Stop();
	int		Connected(bool connected);
	int		ReadData(u_int32_t verbose, struct timeval* pTimeNow);
	int		WriteQueueData();	// Write Queued data if any
	int		AddSlave(u_int32_t slave_id, bool add = true, u_int32_t verbose = 0);

	// Public variables for sender
	CSender*	m_sender_list;	// List of senders for TCPSERVER
	CController*	m_pCtr;		// Controller for I/O
	host_allow_t*	m_host_allow;	// list of allowed senders
	u_int32_t	m_conns;	// sender connections
	u_int32_t	m_max_conns;	// sender max connections
	CQueue*		m_pQueue;	// Master Queue for sender
	source_type_t	m_source_type;	// Type of source
	u_int32_t	m_source_id;	// ID of source

	// Sender stats
	struct timeval	m_start_time;	// Time the sender was started;
	struct timeval	m_stop_time;	// Time the sender was stopped
	u_int32_t	m_times_started;// Number of times started
	u_int32_t	m_bytes_sent;	// Number of bytes sender
	u_int32_t	m_bytes_dropped;	// Number of bytes dropped
	u_int32_t	m_bytes_sent_last;	// Number of bytes sender

  private:
	state_enum_t	AdjustState();

	// Sender variables
	u_int32_t	m_id;		// Sender ID
	char*		m_name;		// Sender name
	int32_t		m_port;		// Sender port
	u_int32_t	m_mtu;		// Expected MTU
	u_int32_t	m_host;		// Host interface
	state_enum_t	m_state;	// State of sender
	int		m_sockfd;	// sender socket fd
	struct sockaddr_in	m_addr;	// sender socket addr
	struct ip_mreq	m_mreq;		// sender multicast join structure
	bool		m_reuseaddr;	// sender socket option reuse addr
	bool		m_reuseport;	// sender socket option reuse port
	u_int8_t*	m_buffer;	// buffer for read
	sender_enum_t	m_type;		// Type of sender

        CSenders*	m_pSenders;	// Link back to CSenders class
        u_int32_t	m_master_id;	// ID of master. Sender is master if it is its own id
        slave_list_t*	m_pSlaveList;	// Master will have a list of slaves if any. Slaves may or may not
};

class CSenders {
  public:
	CSenders(u_int32_t max_id, CReceivers* pReceivers);
	~CSenders();
	bool		SenderExist(u_int32_t id);
	int		ParseCommand(CController* pCtr, const char* str);
	int		SetFds(int max_fd, fd_set* read_fds, fd_set* write_fds);
	void		CheckReadWrite(fd_set *read_fds, fd_set *write_fds,
				struct timeval* timenow);
	int		SenderCheckReadWrite(CSender* pSender,
				fd_set *read_fds, fd_set *write_fds,
				struct timeval* pTimeNow);
	CQueue*		AddQueue(u_int32_t id, CQueue* pQueue = NULL);
	bool		RemoveQueue(u_int32_t id, CQueue* pQueue);
	void		UpdateState();
	int		Start(u_int32_t receiver_id);
	int		Stop(u_int32_t receiver_id);

  private:
	int		AddSlave(u_int32_t master_id, u_int32_t slave_id, bool add);
	int		AcceptNewConnection(u_int32_t id);

	// Functions for parsing commands
	int		add_sender(CController* pCtr, const char* str);
	int		startstop_sender(CController* pCtr, const char* str, bool start);
	int		set_send(CController* pCtr, const char* str);
	int		set_port(CController* pCtr, const char* str);
	int		set_conns(CController* pCtr, const char* str);
	int		set_source(CController* pCtr, const char* str);
	int		set_mtu(CController* pCtr, const char* str);
	int		set_type(CController* pCtr, const char* str);
	int		set_master(CController* pCtr, const char* str);
	int		set_host(CController* pCtr, const char* str);
	int		list_info(CController* pCtr, const char* str);
	int		list_status(CController* pCtr, const char* str);
	int		list_help(CController* pCtr, const char* str);
	int		set_host_allow(CController* pCtr, const char* str);
	int		set_verbose(CController* pCtr, const char* str);

	int		SetSend(u_int32_t id, u_int8_t* buffer, int32_t n);
	int		SetPort(u_int32_t id, u_int32_t port);
	int		SetHost(u_int32_t id, u_int32_t host);
	int		SetSource(u_int32_t id, source_type_t source_type,
				u_int32_t source_id);
	int		SetMTU(u_int32_t id, u_int32_t mtu);
	int		SetMaxConnections(u_int32_t id, u_int32_t mtu);
	int		SetType(u_int32_t id, sender_enum_t type);

	// Functions for configure setting 
	int		DeleteSender(u_int32_t id);
	int		CreateSender(u_int32_t id, const char* name);


	// Variables for CSenders
	u_int32_t	m_max_senders;		// Max number of senders
	u_int32_t	m_count_senders;	// Count number of senders
	CSender**	m_senders;		// Array of senders
	CController*	m_controller_list;	// List of senders;
	u_int32_t	m_verbose;		// Setting verbose level
	CReceivers*	m_pReceivers;		// Pointer to receivers
};

#endif	// __SENDERS_H__
