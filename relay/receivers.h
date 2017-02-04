/*
 * (c) 2014 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */

#ifndef __RECEIVERS_H__
#define __RECEIVERS_H__

#include <sys/types.h>
//#include <stdint.h>
//#include <netinet/in.h>

//#include "receivers.h"
#include "controller.h"
#include "tsanalyzer.h"

enum receiver_enum_t {
	REC_UNDEFINED,
	REC_UDP,
	REC_MULTICAST,
	REC_TCPCLIENT,
	REC_TCPSERVER,
	REC_TCPSERVERCON
};

#define ReceiverType2String(a) ( a == REC_UNDEFINED ? "undefined" : \
	a == REC_UDP ? "udp" :				\
	  a == REC_MULTICAST ? "multicast" :		\
	    a == REC_TCPCLIENT ? "tcpclient" :		\
	      a == REC_TCPSERVER ? "tcpserver" :	\
	        a == REC_TCPSERVERCON ? "tcpconnect" :	\
	          "undefined" )

class CQueue;
class CReceivers;

struct slave_list_t {
	u_int32_t		id;
	struct slave_list_t*	next;
};

class CReceiver {
  public:
	CReceiver(CReceivers* pReceivers, u_int32_t id, const char* name);
	~CReceiver();
	const char*	GetName() { return m_name; }
	int		GetPort() { return m_port; }
	int		GetMTU() { return m_mtu; }
	int		GetHost() { return m_host; }
	state_enum_t	GetState() { return m_state; }
	receiver_enum_t	GetType() { return m_type; }
	u_int32_t	GetID() { return m_id; }
	CQueue*		GetQueue() { return m_pQueue; }
	u_int8_t*	GetBuffer() { return m_buffer; }
	u_int32_t	GetMasterID() { return m_master_id; }
	void		SetMasterID(u_int32_t master_id) { m_master_id = master_id; }
	int		SetName(const char* name);
	int		SetPort(u_int32_t port);
	int		SetSend(u_int8_t* buffer, int32_t n);
	int		SetMTU(u_int32_t mtu);
	int		SetHost(u_int32_t host);
	int		SetType(receiver_enum_t type);
	void		SetQueue(CQueue* pQueue);
	slave_list_t*	GetSlaveList() { return m_pSlaveList; };
	int		Start();
	int		Stop();
	int		Connected(bool connected);
	int		ReadData(u_int32_t verbose, struct timeval* pTimeNow);
	void		UpdateState();
	//int		SetTsAnalyzer(u_int32_t start);
	//int		TsAnalyzerList(CController* pCtr);

	int		AddSlave(u_int32_t slave_id, bool add = true, u_int32_t verbose = 0);

	analyzer_list_t*	m_pAnalyzerList;
	int			AddAnalyzer(CAnalyzer* pAnalyzer);
	int			RemoveAnalyzer(CAnalyzer* pAnalyzer);

	// Public variables for receiver
	CReceiver*	m_receiver_list;// List of receivers for TCPSERVER
	CController*	m_pCtr;		// Controller for I/O
	host_allow_t*	m_host_allow;	// list of allowed senders
	u_int32_t	m_conns;	// receiver connections
	u_int32_t	m_max_conns;	// receiver max connections
	CQueue*		m_pQueue;	// Master Queue for receiver

	// Receiver stats
	struct timeval	m_start_time;	// Time the receiver was started;
	struct timeval	m_stop_time;	// Time the receiver was stopped
	u_int32_t	m_times_started;// Number of times started
	u_int32_t	m_bytes_received;	// Number of bytes received
	u_int32_t	m_bytes_dropped;	// Number of bytes dropped

  private:
	state_enum_t	AdjustState();

	// Receiver variables
	u_int32_t	m_id;		// Receiver ID
	char*		m_name;		// Receiver name
	int32_t		m_port;		// Receiver port
	u_int32_t	m_mtu;		// Expected MTU
	u_int32_t	m_host;		// Host interface
	state_enum_t	m_state;	// State of receiver
	int		m_sockfd;	// receiver socket fd
	struct sockaddr_in	addr;	// receiver socket addr
	struct ip_mreq	m_mreq;		// receiver multicast join structure
	bool		m_reuseaddr;	// receiver socket option reuse addr
	bool		m_reuseport;	// receiver socket option reuse port
	u_int8_t*	m_buffer;	// buffer for read
	receiver_enum_t	m_type;		// Type of receiver
	u_int32_t	m_bytes_received_last;	// Number of bytes received at last update

	CReceivers*	m_pReceivers;	// Link back to CReceivers class
	u_int32_t	m_master_id;	// ID of master. Receiver is master if it is its own id
	slave_list_t*	m_pSlaveList;	// Master will have a list of slaves. Slaves will not
};

class CReceivers {
  public:
	CReceivers(u_int32_t max_id);
	~CReceivers();
	bool		ReceiverExist(u_int32_t);
	int		ParseCommand(CController* pCtr, const char* str);
	int		SetFds(int max_fd, fd_set* read_fds, fd_set* write_fds);
	void		CheckReadWrite(fd_set *read_fds, fd_set *write_fds,
				struct timeval* timenow);
	int		ReceiverCheckReadWrite(CReceiver* pReceiver,
				fd_set *read_fds, fd_set *write_fds,
				struct timeval* pTimeNow);
	CQueue*		AddQueue(u_int32_t id, CQueue* pQueue = NULL);
	bool		RemoveQueue(u_int32_t id, CQueue* pQueue);
	void		UpdateState();

	int			AddAnalyzer(u_int32_t receiver_id, CAnalyzer* pAnalyzer);
	int			RemoveAnalyzer(u_int32_t receiver_id, CAnalyzer* pAnalyzer);
	int		Start(u_int32_t receiver_id);
	int		Stop(u_int32_t receiver_id);
  private:
	int		AddSlave(u_int32_t master_id, u_int32_t slave_id, bool add);
	int		AcceptNewConnection(u_int32_t id);

	// Functions for parsing commands
	int		add_receiver(CController* pCtr, const char* str);
	int		startstop_receiver(CController* pCtr, const char* str, bool start);
	int		set_send(CController* pCtr, const char* str);
	int		set_port(CController* pCtr, const char* str);
	int		set_conns(CController* pCtr, const char* str);
	int		set_mtu(CController* pCtr, const char* str);
	int		set_name(CController* pCtr, const char* str);
	int		set_type(CController* pCtr, const char* str);
	int		set_host(CController* pCtr, const char* str);
	int		list_info(CController* pCtr, const char* str);
	int		list_status(CController* pCtr, const char* str);
	int		list_help(CController* pCtr, const char* str);
	int		set_host_allow(CController* pCtr, const char* str);
	int		set_master(CController* pCtr, const char* str);
	int		set_verbose(CController* pCtr, const char* str);

	int		SetSend(u_int32_t id, u_int8_t* buffer, int32_t n);
	int		SetPort(u_int32_t id, u_int32_t port);
	int		SetName(u_int32_t id, const char* name);
	int		SetHost(u_int32_t id, u_int32_t host);
	int		SetMTU(u_int32_t id, u_int32_t mtu);
	int		SetMaxConnections(u_int32_t id, u_int32_t mtu);
	int		SetType(u_int32_t id, receiver_enum_t type);

	// Functions for configure setting 
	int		DeleteReceiver(u_int32_t id);
	int		CreateReceiver(u_int32_t id, const char* name);

	// Analyzers
//	int		SetTsAnalyzer(u_int32_t id, u_int32_t start);
//	int		TsAnalyzerList(u_int32_t id, CController* pCtr);


	u_int32_t	m_max_receivers;	// Max number of receivers
	u_int32_t	m_count_receivers;	// Count number of receivers
	CReceiver**	m_receivers;		// Array of receivers
	CController*	m_controller_list; // List of receivers;
	u_int32_t	m_verbose;		// Setting verbose level
};

#endif	// __RECEIVERS_H__
