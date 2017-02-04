/*
 * (c) 2014 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */

#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__

#include <sys/types.h>
//#include <stdint.h>
#include <sys/select.h>
#include <netinet/in.h>

#include "receivers.h"
#include "relay.h"


struct output_buffer_t {
	u_int8_t*		data;
	u_int32_t		len;
	struct output_buffer_t*	next;
};

bool set_fd_nonblocking(int fd);

class CReceivers;
class CSenders;
class CAnalyzers;
class CAnalyzer;

class CController {
  public:
	CController(int read_fd = -1, int write_fd = -1, u_int32_t verbose = 0);
	~CController();
	int		WriteMsg(const char* format, ...);
	int		WriteData(u_int8_t* buffer, int32_t len, int alt_fd = -1);
	int		ReadData();
	void		WriteBuffers();
	char*		GetLine();

	CController*	m_prev;	// For linked list of controllers
	CController*	m_next;	// For linked list of controllers
	int		m_read_fd;	// File desc for receiving commands
	int		m_write_fd;	// File descriptor for transmitting
	bool		m_close_controller;	// True if ctr is to be closed
	char		m_ipstr[INET_ADDRSTRLEN];
	output_buffer_t*	m_pBuffers;
  private:
	char	m_linebuf[2*MAX_MTU_SIZE];	// Buffer for receiving commands
	int	m_got_bytes;	// Number of bytes already in the buffer
	u_int32_t	m_verbose;

};

class CRelayControl {
  public:
	CRelayControl(const char*);
	~CRelayControl();
	int		MainLoop();
	bool		CRelayControlInitialized() { return m_control_ok; };
	char*		GetHostAllowList(unsigned int* len, host_allow_t* host_allow_list);
	//bool		HostNotAlloweD(u_int32_t ip, host_allow_t* host_allow_list);
	int 		ListHelp(CController* pCtr, const char* str);
	//int		SetHostAlloW(CController* pCtr, const char* str, host_allow_t** host_allow_list, const char* prepend);
	bool		SourceExist(source_type_t source_type, u_int32_t source_id);
	int		AddAnalyzer2Source(source_type_t source_type, u_int32_t source_id, CAnalyzer* pAnalyzer, bool add);

  private:
	int		ParseCommand(CController *pCtr, char* line);
	CController*	CreateAndAddController(int read_fd = -1, int write_fd = -1, const char* ipstr = NULL);
	void		DeleteAndRemoveController(CController* pCtr);
	void		CheckReadWrite(fd_set* read_fds, fd_set* write_fds);
	int		SetSystemPort(CController* pCtr, const char* str);
	int		SetFds (int maxfd, fd_set *read_fds, fd_set *write_fds);
	bool		m_control_ok;		// Control initialize to run
						// Set to 0 to terminate loop

	CController*	m_controller_list;	// Linked list of created controllers
	CReceivers*	m_pReceivers;
	CSenders*	m_pSenders;
	CAnalyzers*	m_pAnalyzers;

	u_int16_t	m_controller_port;	// Port number for listen
	int		m_controller_listener;	// Fd of listener for ctr conns.
	int		m_control_connections;	// Number of control connections
	host_allow_t*	m_host_allow;		// List of allowed hosts and nets
};

#endif	// CONTROLLER_H
