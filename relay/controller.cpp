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
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
//#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
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
#include "controller.h"
#include "receivers.h"
#include "senders.h"
#include "tsanalyzer.h"

// set fd to nonblocking. Return true if failed
bool set_fd_nonblocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0) return false;
	return (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0 ? true : false);
};

CController::CController(int read_fd, int write_fd, u_int32_t verbose) {
	m_read_fd	= read_fd;
	m_write_fd	= write_fd;
	m_got_bytes	= 0;
	m_prev		= NULL;
	m_next		= NULL;
	m_linebuf[0]	= '\0';
	m_close_controller = false;
	m_pBuffers	= NULL;
	m_ipstr[0]	= '\0';
	m_verbose	= verbose;
	if (m_verbose) fprintf(stderr, "Adding controller for read %d write %d\n", m_read_fd, m_write_fd);
}

CController::~CController() {
	if (m_verbose) fprintf(stderr, "Closing controller for read %d write %d\n", m_read_fd, m_write_fd);
	if (m_read_fd > 0) close(m_read_fd);
	if (m_write_fd > 1) close(m_write_fd);
	while (m_pBuffers) {
		output_buffer_t* pNext = m_pBuffers->next;
		free (m_pBuffers);
		m_pBuffers = pNext;
	}

}

int CController::ReadData() {
	int n = read(m_read_fd, &m_linebuf[m_got_bytes],
		sizeof(m_linebuf) - m_got_bytes - 1);
	if (n > 0) {
		m_got_bytes += n;
		m_linebuf[m_got_bytes] = '\0';	// Make sure its terminated
	}
//fprintf(stderr, "Read %d bytes on %d\n", n, m_read_fd);
	return n;
}

char* CController::GetLine() {
	// Check that we have bytes
	if (!m_got_bytes) return NULL;

	// Check that we have a line with a newline
	char* str = index(&m_linebuf[0], '\n');
//fprintf(stderr, "GetLine got_bytes %d has newline %s\n", m_got_bytes, str ? "yes" : "no");
	if (!str) return NULL;		// Wait for more data
	int len = 1 + (str - &m_linebuf[0]);

	// Duplicate until and inclusive newline
	if (!(str = strndup(&m_linebuf[0], len))) {
		fprintf(stderr, "Failed to duplicate line for CController\n");
		return NULL;
	}
	// Add a space instead of newline
	str[len-1] = ' ';

	// Copy rest of bytes to position 0 in m_linebuf
	int copy_bytes = m_got_bytes - len;
	for (int i=0; i < copy_bytes; i++) {
		m_linebuf[i] = m_linebuf[len+i];
	}
	m_linebuf[copy_bytes] = '\0';
	m_got_bytes -= len;

	return str;
}

// Write a formatted data set to ctr. Buffer data if fd_write is not
// ready to write data. Write to stderr, if fd_write is -1
int CController::WriteMsg(const char* format, ...) {

	va_list			ap;
	char			buffer[2000];
	int			n;

	va_start(ap, format);
	n = vsnprintf(buffer, sizeof(buffer), format, ap);
	va_end(ap);

	return WriteData((u_int8_t*)buffer, n, 2);
}

int CController::WriteData(u_int8_t* buffer, int32_t len, int alt_fd) {
	int			fd, written = 0;
	output_buffer_t*	pBuffers;

	// If write_fd < 0 and alt_fd < 0, then fail
	if (m_write_fd < 0 && alt_fd < 0) return 0;

	// First we check if we should try to write. If there is no buffer,
	// already queed in a list, we try to write
	if (!m_pBuffers) {
		if (m_write_fd < 0) fd = alt_fd; // No output. Write to stderr
		else fd = m_write_fd;
		written = write(fd, buffer, len);
		if (written < 0) {
			perror("Writing failed for WriteMsg");
			written = 0;
		}
	}

	// Then we see if there is data to be written later
	if (written < len) {
		if (!(pBuffers = (output_buffer_t*)
			malloc(sizeof(output_buffer_t) + len - written))) {
			fprintf(stderr, "Failed to allocate buffer for WriteMsg\n");
			return 0;
		}
		pBuffers->data = ((u_int8_t*)pBuffers) + sizeof(output_buffer_t);
		pBuffers->len = len - written;
		pBuffers->next = NULL;
		memcpy(pBuffers->data, &buffer[written], len - written);
		if (!m_pBuffers) {
			m_pBuffers = pBuffers;
			return len;
		}
		output_buffer_t* p = m_pBuffers;
		while (p->next) p = p->next;
		p->next = pBuffers;
	}
	
	return len;
}


CRelayControl::CRelayControl(const char* inifile) {
	m_control_ok = false;
	m_controller_list = NULL;
	m_control_connections = 0;
	m_controller_listener = -1;
	m_controller_port = 0;
	m_host_allow = NULL;
	CController* pCtr = CreateAndAddController(0, -1, "stdin/out/err");
	if (!pCtr) {
		fprintf(stderr, "Failed to create controller for main control\n");
		exit(1);;
	}
	pCtr->WriteMsg("%s", BANNER);
	if (inifile) {
		int fd = open(inifile, O_RDONLY);
		if (fd < 0) {
			fprintf(stderr, "Unable to open ini file \"%s\": %s\n",
				inifile, strerror(errno));
		} else {
			pCtr = CreateAndAddController(fd, -1, "file");
			if (!pCtr) {
				fprintf(stderr, "Failed to create controller for reading inifile %s\n", inifile);
				close(fd);
			}
		}
	}
	m_control_ok = true;
	m_pReceivers = new CReceivers(40);
	m_pSenders = new CSenders(50, m_pReceivers);
	m_pAnalyzers = new CAnalyzers(this, 10, 1);
	if (!m_pReceivers) {
		fprintf(stderr, "Failed to create receivers for CRelayControl\n");
		m_control_ok = false;
	}
	if (!m_pSenders) {
		fprintf(stderr, "Failed to create senders for CRelayControl\n");
		m_control_ok = false;
	}
	if (!m_pAnalyzers) {
		fprintf(stderr, "Failed to create analyzers for CRelayControl\n");
		m_control_ok = false;
	}
	
}

CRelayControl::~CRelayControl() {
	CController* pCtr = m_controller_list;
	while (pCtr) {
		CController* pCtrNext = pCtr->m_next;
		delete pCtr;
		pCtr = pCtrNext;
	}
	while (m_host_allow) {
		host_allow_t* next = m_host_allow->next;
		free(m_host_allow);
		m_host_allow = next;
	}
	if (m_pReceivers) delete m_pReceivers;
	if (m_pSenders) delete m_pSenders;
	if (m_pAnalyzers) delete m_pAnalyzers;
}

int CRelayControl::AddAnalyzer2Source(source_type_t source_type, u_int32_t source_id, CAnalyzer* pAnalyzer, bool add) {
	if (source_type == SOURCE_RECEIVER && m_pReceivers) {
		if (add) return m_pReceivers->AddAnalyzer(source_id, pAnalyzer);
		else return m_pReceivers->RemoveAnalyzer(source_id, pAnalyzer);
/*	} else if (source_type == SOURCE_SENDER && m_pSenders) {
		if (add) return m_pSenders->AddAnalyzer(source_id, pAnalyzer);
		else return m_pSenders->RemoveAnalyzer(source_id, pAnalyzer);
*/
	} else if (source_type == SOURCE_ANALYZER && m_pAnalyzers) {
		if (add) return m_pAnalyzers->AddAnalyzer(source_id, pAnalyzer);
		else return m_pAnalyzers->RemoveAnalyzer(source_id, pAnalyzer);
	} else return -1;
}

bool CRelayControl::SourceExist(source_type_t source_type, u_int32_t source_id) {
	if (source_type == SOURCE_RECEIVER && m_pReceivers) return m_pReceivers->ReceiverExist(source_id);
	else if (source_type == SOURCE_SENDER && m_pSenders) return m_pSenders->SenderExist(source_id);
	else if (source_type == SOURCE_ANALYZER && m_pAnalyzers) return m_pAnalyzers->AnalyzerExist(source_id);
	else return false;
}

CController* CRelayControl::CreateAndAddController(int read_fd, int write_fd, const char* ipstr) {
	CController* pCtr = new CController(read_fd, write_fd, 1);
	if (!pCtr) return NULL;
	list_add_head(m_controller_list, pCtr);
	m_control_connections++;
	if (ipstr) strncpy(pCtr->m_ipstr, ipstr, INET_ADDRSTRLEN-1);
	return pCtr;
}

void CRelayControl::DeleteAndRemoveController(CController* pCtr) {
	CController* p = m_controller_list;
	if (!pCtr || !p) return;
	while (p) {
		if (p == pCtr) {
			list_del_element(m_controller_list, pCtr);
			delete(pCtr);
			m_control_connections--;
			break;
		}
		p = p->m_next;
	}
	return;
}



int CRelayControl::SetFds(int max_fd, fd_set* read_fds, fd_set* write_fds) {
	CController* pCtr;
	for (pCtr = m_controller_list; pCtr != NULL ; pCtr = pCtr->m_next) {
//fprintf(stderr, "Settings fds for read %d write %d\n", pCtr->m_read_fd, pCtr->m_write_fd);
		if (pCtr->m_read_fd > -1) {
			// Add descriptor to read bitmap
			FD_SET(pCtr->m_read_fd, read_fds);
			if (pCtr->m_read_fd > max_fd) max_fd = pCtr->m_read_fd;
		} // else not reading
		if (pCtr->m_write_fd > -1 && pCtr->m_pBuffers) {
			// Data to write. Add descriptor to write bitmap
			FD_SET(pCtr->m_write_fd, write_fds);
			if (pCtr->m_write_fd > max_fd) max_fd = pCtr->m_write_fd;
		} // else not writing
	}
	// Add the master socket to the read list
	if (m_controller_listener > -1) {
		FD_SET(m_controller_listener, read_fds);
		if (m_controller_listener > max_fd)
			max_fd = m_controller_listener;
	}
//fprintf(stderr, "Returning from SetFds with max fd %d\n",max_fd);
	return max_fd;
}

// Return list in allocated memory of allowed hosts and networks
// Memory should be freed after use.
// Format is xxx.xxx.xxx.xxx/xx,xxx.xxx.........
// And we allocate one more byte than we use, so the string can be expanded with
// a newline after it is returned.
char* CRelayControl::GetHostAllowList(unsigned int* len, host_allow_t* host_allow_list) {
	int n = 0;
	host_allow_t* ph;
	char* str = NULL;
	char* retstr;
	if (!(ph = host_allow_list)) return NULL;
	while (ph) { n++; ph = ph->next; }
	if (len) *len = 19*n;
	if (!(str = (char*) malloc(19*n))) {
		fprintf(stderr, "Failed to allocate memory in "
			"CRelayControl::GetHostAllowList()\n");
		return NULL;
	}
	*str = '\0';
	retstr = str;
	ph = host_allow_list;
	while (ph) {
		u_int32_t mask = ph->ip_to - ph->ip_from;
		u_int32_t count = 32;
		while (mask) {
			mask = mask >> 1;
			count--;
		}
		sprintf(str, "%u.%u.%u.%u/%u%s",
			(ph->ip_from >> 24),
			(0xff0000 & ph->ip_from) >> 16,
			(0xff00 & ph->ip_from) >> 8,
			ph->ip_from & 0xff, count,
			ph->next ? " " : "");
		ph = ph->next;
		while (*str) str++;
	}
	return retstr;
}

/*
// Returns true if host is not allowed
bool CRelayControl::HostNotAlloweD (u_int32_t ip, host_allow_t* host_allow_list)
{
	if (!host_allow_list) return true;
	host_allow_t* p = host_allow_list;

	// host_allow_list is a sorted list with lowest range first
	while (p) {
		if (p->ip_from > ip) break;
		if (p->ip_to >= ip) return false; // it is allowed
		p = p->next;
	}
	return true; // it is not allowed
}
*/

// Set system host allow
// Returns : 0=OK, 1=SystemError, -1=SyntaxError
/*
int CRelayControl::SetHostAlloW(CController *pCtr, const char* ci, host_allow_t** host_allow_list, const char* prepend)
{
	u_int32_t ip_from, ip_to, a, b, c, d, e;
	if (!ci) return -1;
	if (!host_allow_list) return 1;
	e = 33;
	//ip_to = ip_from = 0;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		host_allow_t* ph = *host_allow_list;
		while (ph) {
			u_int32_t mask = ph->ip_to - ph->ip_from;
			u_int32_t count = 32;
			while (mask) {
				mask = mask >> 1;
				count--;
			}
			if (pCtr) pCtr->WriteMsg("MSG: %s %u.%u.%u.%u/%u\n",
				prepend,
				(ph->ip_from >> 24),
				(0xff0000 & ph->ip_from) >> 16,
				(0xff00 & ph->ip_from) >> 8,
				ph->ip_from & 0xff, count);
			ph = ph->next;
		}
		if (pCtr) pCtr->WriteMsg("MSG:\n");
		return 0;
	}
	if (sscanf(ci, "%u.%u.%u.%u/%u", &a, &b, &c, &d, &e) == 5) {
		if (a > 255 || b > 255 || c > 255 || d > 255 || e > 32)
			return -1;
		ip_from = (a << 24) | (b << 16) | (c << 8) | d;
		ip_to = ip_from | (0xffffffff >> e);
	}
	else if (sscanf(ci, "%u.%u.%u.%u", &a, &b, &c, &d) == 4) {
		if (a > 255 || b > 255 || c > 255 || d > 255)
			return -1;
		ip_to = ip_from = (a << 24) | (b << 16) | (c << 8) | d;
	} else return -1;
	while (isdigit(*ci)) ci++; ci++; // a
	while (isdigit(*ci)) ci++; ci++; // b
	while (isdigit(*ci)) ci++; ci++; // c
	while (isdigit(*ci)) ci++;       // d
	if (e < 33) {
		ci++;
		while (isdigit(*ci)) ci++;       // e
	}
		
	host_allow_t* p = (host_allow_t*) calloc(sizeof(host_allow_t),1);
	if (!p) return 1;
	p->ip_from = ip_from;
	p->ip_to = ip_to;
	p->next = NULL;

	// Do we have any on the list at all
	if (!(*host_allow_list)) *host_allow_list = p;

	// yes we have
	else {
		// should we insert it as the first
		if (ip_from < (*host_allow_list)->ip_from) {
			p->next = *host_allow_list;
			*host_allow_list = p;
		} else {
			// we have to go through the list
			host_allow_t* ph = *host_allow_list;
			while (ph) {
				// have we reach the end of the list next
				if (!ph->next) {
					ph->next = p;
					break;
				} else {
					if (ip_from < ph->next->ip_from) {
						p->next = ph->next;
						ph->next = p;
						break;
					}
				}
				ph = ph->next;
			}
		}
	}
	while (isspace(*ci)) ci++;
	return (*ci ? SetHostAllow(pCtr, ci, host_allow_list, prepend) : 0);
}
*/

// Set system control port
// Returns : 0=OK, 1=SystemError, -1=SyntaxError
int CRelayControl::SetSystemPort(CController* pCtr, const char* str) {
	int port, yes = 1;
	struct sockaddr_in addr;

	if (!str) return -1;

	while (isspace(*str)) str++;
	if (!(*str)) {
		if (!pCtr) return 1;
		pCtr->WriteMsg("MSG: system control port %hu %sactive\n",
			m_controller_port,
			m_controller_listener > -1 ? "" : "in");
		return 0;
	}

	// Check to see if port is already listening
	if (m_controller_listener > -1) {
		fprintf(stderr, "Controller listener has already been setup\n");
		return -1;
	}
	// Get the port number
	while (isspace(*str)) str++;
	int n = sscanf(str, "%d", &port);
	if (n != 1 || port < 0 || port > 65535) return -1;
	
	// Setup the socket for listening
	if ((m_controller_listener = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Unable to create controller listener");
		exit(1);
	}
	if (setsockopt (m_controller_listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (yes)) < 0) {
		perror ("setsockopt failed on m_controller_listener");
		exit(1);
	}
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	// Bind the port
	if (bind(m_controller_listener, (struct sockaddr*)&addr,
		sizeof(addr)) < 0) {
		perror("Failed to bind on m_controller_listener");
		exit(1);
	}

	// Listen on the port
	if (listen(m_controller_listener, MAX_LISTEN_BACKLOG) < 0) {
		perror("Failed to listen on m_controller_listener");
		exit(1);
	}

	// We succeeded in setting up the listener for controller connections
	m_controller_port = port;
	return 0;
}

int CRelayControl::ListHelp(CController* pCtr, const char* str) {
	if (!pCtr) return 1;
	pCtr->WriteMsg("MSG: Commands:\n"
		"MSG: system host allow [ <ip address> | <ip network>/<number> [ ... ]]\n"
		"MSG: system control port [ <port> ]\n"
		"MSG: help\n"
		"MSG: quit\n"
		"MSG:\n");
	return 0;
}

void CController::WriteBuffers() {
	while (m_pBuffers) {
		if (m_pBuffers->len == 0) {
			struct output_buffer_t* pNext = m_pBuffers->next;
			free(m_pBuffers);
			m_pBuffers = pNext;
		}
		int n = write(m_write_fd, m_pBuffers->data, m_pBuffers->len);
		if (n == 0) break;	// Can not write (anymore right now)
		if (n < 0) {
			perror("Failed to write to controller");
			break;
		}
		m_pBuffers->len -= n;
		m_pBuffers->data += n;
		if (m_pBuffers->len == 0) {
			struct output_buffer_t* pNext = m_pBuffers->next;
			free(m_pBuffers);
			m_pBuffers = pNext;
		}
	}
}

void CRelayControl::CheckReadWrite(fd_set *read_fds, fd_set *write_fds) {
	CController *pCtr, *pCtrNext;
	int n;
	char* str;

	// Go through list of know controllers
	for (pCtr = m_controller_list, pCtrNext = pCtr ? pCtr->m_next : NULL; pCtr != NULL ; pCtr = pCtrNext , pCtrNext = pCtr ? pCtr->m_next : NULL) {

		// First we check for writes
		if (pCtr->m_write_fd > 0 &&
			FD_ISSET(pCtr->m_write_fd, write_fds)) {
			// Ready for writing
			pCtr->WriteBuffers();
/*
			while (pCtr->m_pBuffers) {
				if (pCtr->m_pBuffers->len == 0) {
					struct output_buffer_t* pNext =
						pCtr->m_pBuffers->next;
					free(pCtr->m_pBuffers);
					pCtr->m_pBuffers = pNext;
				}
				int n = write(pCtr->m_write_fd, pCtr->m_pBuffers->data, pCtr->m_pBuffers->len);
				if (n == 0) break;	// Can not write
				if (n < 0) {
					perror("Failed to write to controller");
					break;
				}
				pCtr->m_pBuffers->len -= n;
				pCtr->m_pBuffers->data += n;
				if (pCtr->m_pBuffers->len == 0) {
					struct output_buffer_t* pNext =
						pCtr->m_pBuffers->next;
					free(pCtr->m_pBuffers);
					pCtr->m_pBuffers = pNext;
				}
			}
*/

		}
		if (pCtr->m_read_fd < 0) continue;	// Not readable
		if (!FD_ISSET(pCtr->m_read_fd, read_fds))
			continue;			// No data ready

		// Now read the data from the socket
		n = pCtr->ReadData();

		// Checking for EOF
		if (n <= 0) {

			// Remove and delete controller from list
			DeleteAndRemoveController(pCtr);
			continue;
		}
		while ((str = pCtr->GetLine())) {
			ParseCommand(pCtr, str);
		}
		
	}

	// Now check for possible new incoming connection
	if (m_controller_listener > -1 &&
		FD_ISSET(m_controller_listener, read_fds)) {
		// New incoming connection
		struct sockaddr_in addr;
		char	ipstr[INET_ADDRSTRLEN];
		socklen_t len = sizeof(addr);
		int fd = accept(m_controller_listener, (struct sockaddr*) &addr, &len);
		if (fd > -1) {
			if (addr.sin_family != AF_INET) {
				fprintf(stderr, "Only IPv4 supported yet\n");
				close(fd);
				return;
			}
			u_int32_t ip = ntohl(*((u_int32_t*)&addr.sin_addr));
			if (!m_host_allow) {
				// Only ip = 127.0.0.1 is allowed then
				if (ip != 0x7f000001) {
					close(fd);
					fd = -1;
				}
			} else {
				if (HostNotAllowed(ip, m_host_allow)) {
					close(fd);
					fd = -1;
				}
			}
			inet_ntop(AF_INET, &(addr.sin_addr), ipstr, INET_ADDRSTRLEN);
			if (fd < 0) {
				fprintf(stderr, "Denying control connection from %s\n", ipstr);
				return;
			}
fprintf(stderr, "Got incoming connection from %s\n", ipstr);
			pCtr = CreateAndAddController(fd, fd, ipstr);
			if (set_fd_nonblocking(fd)) {
				fprintf(stderr, "Failed to set connection from %s to nonblocking.\n", ipstr);
			}
			pCtr->WriteMsg("Snowswitch 0.1\n");
		} else {
			fprintf(stderr, "Failed to accept incomming connection\n");
		}
	}
}

int CRelayControl::ParseCommand(CController *pCtr, char* line) {

	// If no line, we return error
	if (!line) return -1;

//fprintf(stderr, "PARSING <%s>\n", line);
	// Skip any leading whitespace
	while (isspace(*line)) line++;
	if (!(*line)) return 0;		// empty command

	// Check for comment
	if (*line == '#') return 0;	// comment

	// Replace repeated whitespace with a single space for easy parsing
	char* co = line;
	char* ci = line;
	while (*ci) {
		if (*ci && *ci >= ' ') {
			*co++ = *ci;
		} else {
			*co++ = ' ';
		}
		if (isspace(*ci)) {
			ci++;
			while (isspace(*ci)) ci++;
		} else ci++;
	}
	// Add trailing space if none for easy parsing
	if (*(co -1) != ' ') {
		*co++ = ' ';
	}
	*co = '\0';

	if (!strncasecmp(line, "system ", 7)) {
		ci = line+7;
		if (!strncasecmp(ci, "control port ", 13)) {
			int n = SetSystemPort(pCtr, ci+13);
			if (n < 0) goto command_error;
			if (n > 0) goto system_error;
			// command is ok
		}
		else if (!strncasecmp(ci, "host allow ", 11)) {
			int n = SetHostAllow(pCtr, ci+11, &m_host_allow, "system host allow");
			if (n < 0) goto command_error;
			if (n > 0) goto system_error;
			// command is ok
		} else goto syntax_error;
	} else if (!strncasecmp(line, "recv ", 5) ||
		!strncasecmp(line, "receiver ", 9)) {
		if (!m_pReceivers) goto system_error;
		int n = m_pReceivers->ParseCommand(pCtr,
			line+(line[3] == 'v' ? 5 : 9));
		if (n < 0) goto command_error;
		if (n > 0) goto system_error;
		// command is ok
	} else if (!strncasecmp(line, "send ", 5) ||
		!strncasecmp(line, "sender ", 7)) {
		if (!m_pSenders) goto system_error;
		int n = m_pSenders->ParseCommand(pCtr,
			line+(line[4] == ' ' ? 5 : 7));
		if (n < 0) goto command_error;
		if (n > 0) goto system_error;
		// command is ok
	} else if (!strncasecmp(line, "ana ", 4) ||
		!strncasecmp(line, "analyzer ", 9)) {
		if (!m_pAnalyzers) goto system_error;
		int n = m_pAnalyzers->ParseCommand(pCtr,
			line+(line[3] == ' ' ? 4 : 9));
		if (n < 0) goto command_error;
		if (n > 0) goto system_error;
		// command is ok
	}
	else if (!strncasecmp(line, "help ", 5)) {
		return ListHelp(pCtr, line+5);
	}
	else if (!strncasecmp(line, "quit ", 5)) {
		m_control_ok = false;
	} else goto syntax_error;

	return 0;
syntax_error:
	if (pCtr) pCtr->WriteMsg("syntax error for \"%s\"\n", line);
	return -1;
command_error:
	if (pCtr) pCtr->WriteMsg("command parameter error for \"%s\"\n", line);
	return -1;
system_error:
	if (pCtr) pCtr->WriteMsg("system or ressource error for \"%s\"\n", line);
	return -1;
}

int CRelayControl::MainLoop() {

	int n;
	struct timeval timeout;
	struct timeval nexttime;
	struct timeval update_interval;
	struct timeval now;
	update_interval.tv_sec = 0;
	update_interval.tv_usec = 500000;

	gettimeofday(&now, NULL);
	timeradd(&now, &update_interval, &nexttime);
	while (m_control_ok) {
		fd_set	read_fds;
		fd_set	write_fds;
		int	max_fd = 0;

		// Setup for select
		FD_ZERO(&read_fds);
		FD_ZERO(&write_fds);

		// Find max_fd for control connections
		max_fd = SetFds(max_fd, &read_fds, &write_fds);

		// and max_fd for receivers if any
		if (m_pReceivers)
			max_fd = m_pReceivers->SetFds(max_fd, &read_fds,
				&write_fds);

		// and max_fd for senders if any
		if (m_pSenders)
			max_fd = m_pSenders->SetFds(max_fd, &read_fds,
				&write_fds);

		// Calculate time-out and check for negative
		gettimeofday(&now,NULL);
		timersub(&nexttime, &now, &timeout);
		if (timeout.tv_sec < 0 || timeout.tv_usec < 0) {
//fprintf(stderr, "Negative timeout value for select\n");
			// We are late and will have zero timeout
			timeout.tv_sec = 0;
			timeout.tv_usec = 0;
		}

		// Now wait for input/output or timeout
		n = select(max_fd + 1, &read_fds, &write_fds, NULL, &timeout);

		// Check for error
		if (n < 0) {
			if (errno == EINTR) continue; // Try again
			perror("Select error on control");
			return 1;
		}

		// Check for input/output ready
		if (n > 0) {
			// Run the input/output service function(s)
			CheckReadWrite(&read_fds, &write_fds);
			// Run the input/output service function(s) for
			// receivers if any
			gettimeofday(&now, NULL);
			if (m_pReceivers)
				m_pReceivers->CheckReadWrite(&read_fds,
					&write_fds, &now);
			if (m_pSenders)
				m_pSenders->CheckReadWrite(&read_fds,
					&write_fds, &now);
		}

		// Now we check to see if we have reach the update time
		gettimeofday(&now, NULL);
		// Some system have a broken timercmp for >=, <= and !=
		if (timercmp(&now, &nexttime, <)) {
			// We have not reached nexttime and will just loop
			continue;
		}
		// We have (perhaps over) reached nexttime
		// Do update stuff
		if (m_pReceivers) m_pReceivers->UpdateState();
		if (m_pSenders) m_pSenders->UpdateState();

		// We set the time for next update
		timeradd(&nexttime, &update_interval, &nexttime);

		// And then we loop
	}
	return 0;
}
