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
//#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
//#include <errno.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <fcntl.h>
#include <string.h>
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
#include "tsanalyzer.h"

// Function to delete trailing whitespaces
void trim_string(char* str) {
	char* cp;
	if (!str) return;
	int len = strlen(str);
	cp = str + len - 1;
	while (cp >= str && *cp <= ' ' && *cp > 0) {
		*cp = 0;
		cp--;
	}
}


// Set system host allow
// Returns : 0=OK, 1=SystemError, -1=SyntaxError
int SetHostAllow(CController *pCtr, const char* ci, host_allow_t** host_allow_list, const char* prepend, bool newline)
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
		if (newline && pCtr) pCtr->WriteMsg("MSG:\n");
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
// Returns true if host is not allowed
bool HostNotAllowed (u_int32_t ip, host_allow_t* host_allow_list)
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

int main(int argc, char *argv[])
{
	CRelayControl* pRelayControl = new CRelayControl(argc > 1 ? argv[1] : NULL);
	if (!pRelayControl) {
		fprintf(stderr, "Unable to create relay control\n");
		exit(1);
	}
	pRelayControl->MainLoop();
	delete pRelayControl;
	return 0;
}

