/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */


//#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
//#include <limits.h>
#include <string.h>
#ifdef HAVE_MALLOC
#include <malloc.h>
#else
#include <stdlib.h>
#endif
#include <stdlib.h>
//#include <sys/types.h>
//#include <math.h>
//#include <string.h>

//#include "cairo/cairo.h"
//#include "video_mixer.h"
//#include "video_text.h"
//#include "video_scaler.h"
//#include "add_by.h"
//#include "video_image.h"
#include "command.h"
#include "video_feed.h"


CCommand::CCommand(CVideoMixer* pVideoMixer)
{
	m_pVideoMixer = pVideoMixer;
	m_command_head = NULL;
	m_last_command = NULL;
	m_command_count = 0;
	m_pTimedCommands = NULL;
	m_verbose = false;
}

CCommand::~CCommand()
{
	command_head_t* p = m_command_head;
	m_command_head = NULL;
	m_last_command = NULL;
	m_command_count = 0;
	if (p) DeleteCommandHead(p, p->left, p->right);
}

int CCommand::ParseCommand(CController* pController, struct controller_type* ctr,
	const char* str)
{
	//if (!m_pVideoMixer) return 0;

	// command at [+]<time> <command ........>
	if (!strncasecmp (str, "at ", strlen("at "))) {
		return set_command_at(ctr, pController, str+3);
	// command afterend <command name>
	} else if (!strncasecmp (str, "afterend ", strlen("afterend "))) {
		return set_command_afterend(ctr, pController, str+8);
	// command create <command name>
	} else if (!strncasecmp (str, "create ", strlen("create "))) {
		return set_command_create(ctr, pController, str+7);
	// command push <command name> <command>
	} else if (!strncasecmp (str, "push ", strlen("push "))) {
		return set_command_push(ctr, pController, str+5);
	// command pointer atline [ <command name> [ <line> ]]
	} else if (!strncasecmp (str, "pointer atline ", 15)) {
		return set_command_pointer_atline(ctr, pController, str+15);
	// command addatline <command name> <line number> <command ........>
	} else if (!strncasecmp (str, "addatline ", strlen ("addatline "))) {
		return set_command_addatline(ctr, pController, str+10);
	// command deleteline <command name> <line number>
	} else if (!strncasecmp (str, "deleteline ", strlen ("deleteline "))) {
		return set_command_deleteline(ctr, pController, str+11);
	// command delete <command name>
	} else if (!strncasecmp (str, "delete ", strlen ("delete "))) {
		return set_command_delete(ctr, pController, str+7);
	// command list [<command name>]
	} else if (!strncasecmp (str, "list ", strlen ("list "))) {
		return set_command_list(ctr, pController, str+5);
	// command pop <command name> [<count>]
	} else if (!strncasecmp (str, "pop ", strlen ("pop "))) {
		return set_command_pop(ctr, pController, str+4);
	// command restart <command name>
	} else if (!strncasecmp (str, "restart ", strlen("restart "))) {
		return set_command_restart(ctr, pController, str+8);
	// command verbose
	} else if (!strncasecmp (str, "verbose ", strlen ("verbose "))) {
		m_verbose = !m_verbose; return 0;
	// command help
	} else if (!strncasecmp (str, "help ", strlen ("help "))) {
		return set_command_help(ctr, pController, str+5);
	} else return 1;
	return 0;
}

int CCommand::set_command_help(struct controller_type* ctr, CController* pController, const char* str)
{
	if (!pController || !str) return -1;
	pController->controller_write_msg (ctr, "MSG: Commands:\n"
			"MSG:  command addatline <command name> <line number> <command>\n"
			"MSG:  command afterend <command name>\n"
			"MSG:  command at [+][[hh:]mm:]ss[.nnnn] <command ...>\n"
			"MSG:  command create [<name>]\n"
			"MSG:  command delete <name>\n"
			"MSG:  command deleteline <command name> <line number>\n"
			"MSG:  command end\n"
			"MSG:  command list [<command name>]\n"
			"MSG:  command pointer atline [ <command name> [ <line> ]]\n"
			"MSG:  command pop <command name> [<number of lines>]\n"
			"MSG:  command push <command name> <command>\n"
			"MSG:  command restart <name>\n"
			"MSG:  command help    // This help listing\n"
			"MSG:  command list [<command name>]\n"
			"MSG:\n"
			);
	return 0;
}


// snowmix info command ( names | list | at | syntax ) [ format | <name> ]
//
// format	: format 
// ids		: list of IDs
// maxid	: max number of ID
// nextavail	: Next available ID
// id or ids	: listing each ID
char* CCommand::Query(const char* query, bool tcllist)
{
	u_int32_t type;
	char* retstr = NULL;
	char* pstr = NULL;
	char* qstr = NULL;

	if (!query) return NULL;
	while (isspace(*query)) query++;
	if (!(strncasecmp("names ", query, 6))) {
		query += 6; type = 0;
	}
       	else if (!(strncasecmp("list ", query, 4))) {
		query += 4; type = 1;
	}
       	else if (!(strncasecmp("at ", query, 3))) {
		query += 3; type = 2;
	}
	else if (!(strncasecmp("syntax", query, 6))) {
		return strdup("command ( names | list | at | syntax ) [ format | <name> ]");
	} else return NULL;

	// We skip whitespaces
	while (isspace(*query)) query++;

	// Do we have a "format" command ?
	if (!strncasecmp(query, "format", 6)) {
		char* str = NULL;
		if (type == 0) {
			if (tcllist) str = strdup("name line_count pointer");
			else str = strdup("command <name> <line_count> <pointer>");
		}
		else if (type == 1) {
			// name {{ <line 1> } { <line 2> } ...}
			// command <name> <line 1> \n <line 2> \n \n
			if (tcllist) str = strdup("name newline line1 newline line2 newline ...");
			else str = strdup("command <name> <line 1> <newline> <line 2> ... <newline> <newline>");
		}
		else if (type == 2) {
			if (tcllist) str = strdup("time name");
			else str = strdup("command at <time> <name>");
		}
		if (!str) fprintf(stderr, "CCommand::Query failed to allocate memory\n");
		return str;
	}

	// snowmix info command ( names | list | at | syntax ) [ format | <name> ]
	// We have either a 'names' or a 'list' or an 'at' command

	int len = 2;
	// We need to estimate memory needed for returning result
	if (type == 0) {		// names
		const char* str = query;
		// {name 1234567890 1234567890} {...
		// command name name 1234567890 1234567890
		// 1234567890123    45678901234567890123456
		len += strlen(str);	// if we have names, we need space for them
		if (*str) {		// we should name a specific command
			while (*str) {  // We don't to to find it now, just count it
				while ((*str) && !isspace(*str)) str++;
				while (isspace(*str)) str++;
				len += 36;
			}
		} else {	// no specific name(s) we need to list all
			len += (36*CommandCount() + CommandNamesLength(NULL));
		}
	} else if (type == 1) {		// list
	// snowmix info command list [ <name> ]
		// name lines \n <line 1> \n <line 2> ...
		// command <name> <lines> \n <line 1> \n <line 2> \n
		// command list NAME 1234567890\n ......\n
		// 1234567890123    456780123456

		// Do we have any names at all?
		if (!(*query)) qstr = GetCommandNames();
		else qstr = strdup(query);
		if (!qstr) goto failed_memory;
		char* freestr = qstr;

		// Now we have a list (might be empty) of names to walk through
		while (*qstr) {
			char* nstr = qstr;
			while (*nstr && !(isspace(*nstr))) nstr++;
			if (*nstr) *nstr++ = '\0';
			while (isspace(*nstr)) nstr++;

			// qstr will now point at a nul terminated name
			// and nstr points at the next name if any
			char* command = GetWholeCommand(qstr);
			if (command) {
				len += (26 + strlen(command));
				free(command);
			} else {
				fprintf(stderr, "CCommand::Got nothing for %s\n", command);
			}
			qstr = nstr;
		}
		if (freestr) free(freestr);
	} else if (type == 2) {		// at
		timed_command_t* p = m_pTimedCommands;
		while (p) {
			// command at <time> <name>
			// command at 1234567890.123456 command
			// 12345678901234567890123456789       0
			len += (30 + p->len);
			p = p->next;
		}
	} else return NULL;
	

fprintf(stderr, "We have allocated %u bytes\n", len);
	// Now we allocate memory for the return string
	pstr = retstr = (char*) malloc(len);
	if (!retstr) {
		fprintf(stderr, "CCommand::Query failed to allocate memory\n");
		return NULL;
	}
	*retstr = '\0';

	// For type 0 we print name line_count, and pointer number
	// We either print for specific names or for all names.
	if (type == 0) {		// names

		// Do we have any names at all?
		if (!(*query)) qstr = GetCommandNames();
		else qstr = strdup(query);
		if (!qstr) goto failed_memory;
		char* freestr = qstr;

		// Now we have a list (might be empty) of names to walk through
		while (*qstr) {
			char* nstr = qstr;
			while (*nstr && !(isspace(*nstr))) nstr++;
			if (*nstr) *nstr++ = '\0';
			while (isspace(*nstr)) nstr++;

			// qstr will now point at a nul terminated name
			// and nstr points at the next name if any
			
			u_int32_t lines, pointer;
			if (CommandLinesAndPointer(qstr, &lines, &pointer)) {
				sprintf(pstr,"%s%s %u %u%s",
					(tcllist ? "{" : "command name "),
					qstr, lines, pointer,
					(tcllist ? "} " : "\n"));
				while (*pstr) pstr++;
			} else {
				fprintf(stderr, "CCommand Query name %s not found\n", qstr);
			}
			qstr = nstr;
		}
		if (freestr) free (freestr);
		return retstr;

	} else if (type == 1) {
		// We need to return list (content) of commands
		// Do we have any names at all?
		if (!(*query)) qstr = GetCommandNames();
		else qstr = strdup(query);
		if (!qstr) goto failed_memory;
		char* freestr = qstr;

		// Now we have a list (might be empty) of names to walk through
		while (*qstr) {
			char* nstr = qstr;
			while (*nstr && !(isspace(*nstr))) nstr++;
			if (*nstr) *nstr++ = '\0';
			while (isspace(*nstr)) nstr++;

			// qstr will now point at a nul terminated name
			// and nstr points at the next name if any
			
			u_int32_t lines;
			if (CommandLinesAndPointer(qstr, &lines, NULL)) {
				char* command = GetWholeCommand(qstr);
				sprintf(pstr,"%s%s %u\n%s\n",
					(tcllist ? "" : "command list "),
					qstr, lines,
					command ? command : "");
				while (*pstr) pstr++;
				if (command) free(command);
			} else {
				fprintf(stderr, "CCommand Query name %s not found\n", qstr);
			}
			qstr = nstr;
		}
		//sprintf(pstr,"\n");
		if (freestr) free (freestr);
		return retstr;

	} else if (type == 2) {
		timed_command_t* p = m_pTimedCommands;
		while (p) {
			double time = p->time.tv_sec +(((double) p->time.tv_usec)/1000000.0);
			sprintf(pstr, "%s%.3lf %s%s%s",
				tcllist ? "{" : "command at",
				time,
				tcllist ? "{" : "",
				p->command,
				tcllist ? "}} " : "\n");
			while (*pstr) pstr++;
			p = p->next;
		}
		return retstr;
	} else return NULL;
		
	return NULL;
failed_memory:
	fprintf(stderr, "CCommand::Query failed to allocate memory\n");
//failed:
	//if (str) free(str);
	if (qstr) free(qstr);
	return NULL;
}

// command at [+]<time> <command ........>
// time =  [[hh:]mm:]nn[.nnnnn] 
int CCommand::set_command_at(struct controller_type* ctr, CController* pController, const char* str)
{
	if (!pController || !str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		timed_command_t* p = m_pTimedCommands;
		struct timeval time_now;
		if (gettimeofday(&time_now, NULL) < 0) {
			fprintf(stderr, "Failed to get time of day\n");
			return 1;
		}
		struct timeval diftime;
		while (p) {
			timersub(&(p->time), &time_now, &diftime);
			double time = diftime.tv_sec +(((double) diftime.tv_usec)/1000000.0);
			pController->controller_write_msg (ctr,
				"MSG: command in %6.1lf seconds <%s>\n", time, p->command);
			p = p->next;
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	}
	bool relative = false;
	int n;
	u_int32_t hours = 0;
	u_int32_t minutes = 0;
	double seconds = 0.0;
	struct timeval time;
	if (*str == '+') {
		relative = true;
		str++;
	}
	while (isspace(*str)) str++;
	if ((n = sscanf(str, "%u:%u:%lf", &hours, &minutes, &seconds)) == 3) {
		while (isdigit(*str)) str++;
		str++;
		while (isdigit(*str)) str++;
		str++;
		while (isdigit(*str)) str++;
		if (*str == '.') str++;
		while (isdigit(*str)) str++;
	} else if ((n = sscanf(str, "%u:%lf", &minutes, &seconds)) == 2) {
		while (isdigit(*str)) str++;
		str++;
		while (isdigit(*str)) str++;
		if (*str == '.') str++;
		while (isdigit(*str)) str++;
	} else if ((n = sscanf(str, "%lf", &seconds)) == 1) {
		while (isdigit(*str)) str++;
		if (*str == '.') str++;
		while (isdigit(*str)) str++;
	} else return -1;		// time syntax incorrect
	while (isspace(*str)) str++;
	if (!(*str)) return -1;		// No command to execute
	time.tv_usec = (typeof(time.tv_usec))seconds;
	time.tv_usec = (typeof(time.tv_usec))((seconds - time.tv_usec)*1000000.0);
	time.tv_sec = (typeof(time.tv_sec))seconds;
	AddTimedCommand(str, &time, relative);
	return 0;
}


int CCommand::set_command_create(struct controller_type* ctr, CController* pController, const char* str)
{
	if (!pController || !str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		fprintf(stderr, "empty command name for 'command create'\n");
		return -1;
	}
	char* name = pController->SetCommandName(str);
	if (pController->m_verbose) pController->controller_write_msg (ctr,
		"MSG: Command mode start name <%s>\n", name);
	return 0;
}

// command restart <command name>
int CCommand::set_command_restart(struct controller_type* ctr, CController* pController, const char* str)
{
	int val = -1;
	if (!pController || !str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		fprintf(stderr, "empty command name for 'command restart'\n");
		return val;
	}
	char* name = strdup(str);
	trim_string(name);
	if (CommandExists(name)) {
		RestartCommandPointer(name);
		val = 0;
	} else val = 1;
	free(name);
	return val;
}

// command delete <command name>
int CCommand::set_command_delete(struct controller_type* ctr, CController* pController, const char* str)
{
	int val = -1;
	if (!pController || !str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		fprintf(stderr, "empty command name for 'command delete'\n");
		return val;
	}
	char* name = strdup(str);
	trim_string(name);
	if (CommandExists(name)) {
		DeleteCommandByName(name);
		val = 0;
	} else val = 1;
	free(name);
	return val;
}

// command pop <command name> [<number of lines>]
int CCommand::set_command_pop(struct controller_type* ctr, CController* pController, const char* str)
{
	int val = -1;
	if (!pController || !str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		fprintf(stderr, "empty command name\n");
		return val;
	}
	int lines = 1;
	char* name = strdup(str);
	char* s2 = name;
	while (*s2 && !isspace(*s2)) s2++;
	if (*s2) {
		*s2++ = '\0';
		while (isspace(*s2)) s2++;
		sscanf(s2, "%u", &lines);
	}
	if (m_verbose) fprintf(stderr, "Pop %u lines of command <%s>\n", lines, name);
	if (CommandExists(name) && lines > 0) {
		PopCommandByName(name, lines);
		val = CommandCheck(name);
	} else val = 1;
	free(name);
	return val;
}

// command addatline <command name> <line number> <command>
int CCommand::set_command_addatline(struct controller_type* ctr, CController* pController, const char* str)
{
	int val = -1;
	if (!pController || !str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		fprintf(stderr, "empty command name\n");
		return val;
	}
	int line;
	char* name = strdup(str);
	char* s2 = name;
	while (*s2 && !isspace(*s2)) s2++;
	if (*s2) {
		*s2++ = '\0';
		if (CommandExists(name)) {
			while (isspace(*s2)) s2++;
			int n = sscanf(s2, "%u", &line);
			if (n == 1) {
				while (isdigit(*s2)) s2++;
				while (isspace(*s2)) s2++;
				AddCommandToListAtLine(FindCommand(name), line, s2);
				val = CommandCheck(name);
			}
		}
	}
	free(name);
	return val;
}

// command push <command name> <command>
int CCommand::set_command_push(struct controller_type* ctr, CController* pController, const char* str)
{
	int val = -1;
	if (!pController || !str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		fprintf(stderr, "empty command name\n");
		return val;
	}
	char* name = strdup(str);
	char* s2 = name;
	while (*s2 && !isspace(*s2)) s2++;
	if (*s2) {
		*s2++ = '\0';
		if (CommandExists(name)) {
			while (isspace(*s2)) s2++;
			if (*s2) {
				command_head_t* pHead = FindCommand(name);
				if (pHead) {
					AddCommandByName(name, s2, pHead);
					val = CommandCheck(name);
				}
			}
		}
	}
	free(name);
	return val;
}

void CCommand::WriteCommandPointer(struct controller_type* ctr, command_head_t* pHead, bool recursive)
{
	if (!pHead) return;
	if (recursive) WriteCommandPointer(ctr, pHead->left);
	int32_t pointer = 0;
	command_t* p = pHead->head;
	if (pHead->next_command) {
		while (p) {
			pointer++;
			if (p == pHead->next_command) break;	// found
			p = p->next;
		}
		if (!p) pointer = -1;
	} else pointer = -2;
	if (!pHead->deleted) m_pVideoMixer->m_pController->controller_write_msg (ctr,
		"MSG: %s lines %u pointer %d\n",
		pHead->name, pHead->command_counter, pointer);
	if (recursive) WriteCommandPointer(ctr, pHead->right);
}


// command pointer atline <command name> <line>
int CCommand::set_command_pointer_atline(struct controller_type* ctr, CController* pController, const char* str)
{
	if (!pController || !str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {		// print out all commands pointer status
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return -1;
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"MSG: command pointer status\n");
		WriteCommandPointer(ctr, m_command_head);
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	}

	// Copy the name and put a nul at the end of it
	char* name = strdup(str);
	char* p2 = name;
	if (!name) return 1;
	while (*p2 && !isspace(*p2)) p2++;
	*p2 = '\0';

	// Walk past the name
	while (*str && !isspace(*str)) str++;
	while (isspace(*str)) str++;

	// Now if we have no more than the name, we print it
	if (!(*str)) {
		command_head_t* p = FindCommand(name);
		if (!p || !m_pVideoMixer || !m_pVideoMixer->m_pController) {
            free(name);
            return -1;
        }
		// m_pVideoMixer->m_pController->controller_write_msg (ctr,
			// "MSG: command pointer status\n");
		WriteCommandPointer(ctr, p, false);
		// m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG:\n");
		free(name);
        return 0;
	}

	// We have more and expect a line number

	u_int32_t line;
	if (sscanf(str, "%u", &line) != 1) {
        free(name);
        return -1;
    }
    int n = SetPointer4Command(name, line);
	free (name);
	return n;
}

int CCommand::SetPointer4Command(const char* name, u_int32_t line)
{
	if (!name || !(*name) || !line) return -1;
	command_head_t* pHead = FindCommand(name);
	if (!pHead) return -1;
	u_int32_t i = 1;
	command_t* p = pHead->head;
	while (p) {
		if (i == line) {
			pHead->next_command = p;
			break;
		}
		i++;
		p = p->next;
	}
	return (p ? 0 : -1);
}

// command deleteline <command name> <line number>
int CCommand::set_command_deleteline(struct controller_type* ctr, CController* pController, const char* str)
{
	int val = -1;
	if (!pController || !str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		fprintf(stderr, "empty command name\n");
		return val;
	}
	int line;
	char* name = strdup(str);
	char* s2 = name;
	while (*s2 && !isspace(*s2)) s2++;
	if (*s2) {
		*s2++ = '\0';
		if (CommandExists(name)) {
			while (isspace(*s2)) s2++;
			int n = sscanf(s2, "%u", &line);
			if (n == 1) {
				DeleteLineCommandByName(name, line);
				val = CommandCheck(name);
			}
		}
	}
	free(name);
	return val;
}

// command afterend <command name>
int CCommand::set_command_afterend(struct controller_type* ctr, CController* pController, const char* str)
{
	int val = -1;
	if (!pController || !str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		fprintf(stderr, "empty command name\n");
		return val;
	}
	char* name = strdup(str);
	trim_string(name);
	if (CommandExists(name)) {
		SetCommandPointerAfterEnd(name);
		val = 0;
	} else val = 1;
	free(name);
	return val;
}

// command list [<command name>]
int CCommand::set_command_list(struct controller_type* ctr, CController* pController, const char* str)
{
	if (!pController || !str) return -1;
	while (isspace(*str)) str++;
	if (*str) {
		char* name = strdup(str);
		trim_string(name);
		if (!CommandExists(name)) return 1;
		int n = 1;
		char* line;
		command_head_t* pHead = FindCommand(name);
		pController->controller_write_msg (ctr, "MSG: Command %s has %u lines "
			"of a total of %u bytes nulterminating included\n",
			name, pHead->command_counter, pHead->len);
		while ((line = GetCommandLine(name, n++))) {
			pController->controller_write_msg (ctr,
					"MSG: %3d: %s\n", n-1, line);
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		free(name);
	} else {
		char* s = GetCommandNames();
		if (s) {
			char* p = s;
			char* p2 = p;
			while (p) {
				while (*p2 && !isspace(*p2)) p2++;
				if (isspace(*p2)) {
					*p2 = '\0';
					p2++;
				} else p2=NULL;
				pController->controller_write_msg (ctr, "MSG: %s\n", p);
				p = p2;
			}
			free (s);
		}
	}
	return 0;
}

void CCommand::RunTimedCommands(struct timeval time_now)
{	//struct timeval time_now;
	if (!m_pTimedCommands) return;
/*
	if (gettimeofday(&time_now, NULL) < 0) {
		fprintf(stderr, "Failed to get time of day in RunTimedCommands.\n");
		return;
	}
*/
	timed_command_t* p = m_pTimedCommands;
	timed_command_t* prev = NULL;
	while (p) {
		if (!timercmp(&time_now, &(p->time), <)) {
			// Execute command
			if (m_pVideoMixer && m_pVideoMixer->m_pController) {
				m_pVideoMixer->m_pController->controller_parse_command(m_pVideoMixer, NULL, p->command);
			}

			if (!prev) {
				m_pTimedCommands = p->next;
				free(p);
				p = m_pTimedCommands;
			} else {
				prev->next = p->next;
				free(p);
				p = prev->next;
			}
			continue;
		}
		p = p->next;
	}
}

// Add timed command to list of timed commands
void CCommand::AddTimedCommand(const char* command, struct timeval* time, bool relative)
{
	struct timeval time_now;
	if (!command || !time) return;
	if (gettimeofday(&time_now, NULL) < 0) {
		fprintf(stderr, "Failed to get time of day in AddTimedCommand.\n");
		return;
	}
	if (!relative && !timercmp(time, &time_now, >)) {
		fprintf(stderr, "Time had already elapsed.\n");
		return;		// Time has already elapsed
	}
	timed_command_t *p = (timed_command_t*) malloc(sizeof(timed_command_t) + strlen(command) + 1);
	if (!p) return;		// No memory. We return silently.
	if (relative) timeradd(&time_now, time, &(p->time));
	else {
		p->time.tv_sec = time->tv_sec;
		p->time.tv_usec = time->tv_usec;
	}
	p->next = 0;
	p->command = ((char*)p)+sizeof(timed_command_t);
	strcpy(p->command, command);
	p->len = strlen(p->command);
	trim_string(p->command);
	if (!m_pTimedCommands || timercmp(&(m_pTimedCommands->time), &(p->time), >)) {
		p->next = m_pTimedCommands;
		m_pTimedCommands = p;
		return;
	}
	timed_command_t* pH = m_pTimedCommands;
	while (pH) {
		if (!pH->next || timercmp(&(pH->next->time), &(p->time), >)) {
			p->next = pH->next;
			pH->next = p;
			break;
		}
		pH = pH->next;
	}
	return;
}
static int CommandNameLength(command_head_t* p, command_head_t* left, command_head_t* right)
{
	int len = 0;
	if (!p) return len;
	if (left) len += CommandNameLength(left, left->left, left->right);
	if (right) len += CommandNameLength(right, right->left, right->right);
	if (p->name) len+= (strlen(p->name)+1);
	return len;
}

static char* PrintCommandName(char* str, command_head_t* p)
{
	if (!p) return str;
	str = PrintCommandName(str,p->left);
	str = PrintCommandName(str,p->right);
	if (str && !p->deleted) {
		strcpy(str, p->name);
		while (*str) str++;
		*str++ = ' ';
		*str = '\0';
	}
	return str;
}

int CCommand::CommandCheck(const char* name) {
	int val = -1;
	if (m_verbose)
		fprintf(stderr, "CCommandCheck <%s>\n", name ? name : "NULL");
	if (name) {
		command_head_t* pHead = FindCommand(name);
		if (pHead && pHead->head) {
			val = Parse4IfElseEndif(pHead->head, NULL);
			if (val) {
				pHead->next_command = NULL;
				return val;
			}
			command_t* pCommand;
			for (pCommand = pHead->head; pCommand ; pCommand = pCommand->next) {
				char* label = pCommand->command;
				if (!(*label)) continue;
				while (*label && isspace(*label)) label++;
				if (strncasecmp(label, "goto ", 5)) continue;

				// goto found.
				label += 5;
				while (*label && isspace(*label)) label++;
			// fprintf(stderr, "GOTO looking for label <%s>\n", label);
				command_t* pC;	// = pHead->head;
				for (pC = pHead->head; pC ; pC = pC->next) {
					char* p2 = pC->command;
					if (!(*p2)) continue;
					while (*p2 && isspace(*p2)) p2++;
					if (strncasecmp(p2, "label ", 6)) continue;
					p2 += 6;
					while (*p2 && isspace(*p2)) p2++;
			//fprintf(stderr, "Searching label <%s> <%s>\n", p2, label);
					if (strncasecmp(p2, label, strlen(label))) continue;
			//	fprintf(stderr, "Found label <%s> <%s>\n", p2, label);
					break;
				}
				if (!(pCommand->pEndif = pC)) {

					// Uhhh, we didn't find the label.
					pHead->next_command = NULL;
					val = -1;
					break;
				}
			}
		} else val = 0;		// command is empty. Hardly an error.
	}
	return val;
}

char* CCommand::GetCommandNames() {
	char* str = NULL;
	int len = CommandNameLength(m_command_head,
		m_command_head ? m_command_head->left : NULL,
		m_command_head ? m_command_head->right : NULL);
	//fprintf(stderr, "String to hold all names are %d bytes\n", len);
	if (len) {
		str = (char*) calloc(len+1,1);
		*str = '\0';
		PrintCommandName(str, m_command_head);
	}
	return str;
}

// Create a new command holder (command head)
command_head_t* CCommand::NewCommandHead(const char* name)
{
	int len = 0;
	// Check that we have a name and that it is longer than 0
	if (!name || !(len=strlen(name))) return NULL;

	// Allocate holder for head plus name and implicitly set deleted, head, tail, left and right to NULL
	command_head_t* p = (command_head_t*) calloc(sizeof(command_head_t)+len+1,1);
	if (p) {
		p->name = ((char*)p)+sizeof(command_head_t);
		p->head = p->tail = NULL;
		p->left = p->right = NULL;
		p->deleted = false;
		// and copy name to name place in holder
		p->next_command = NULL;
		p->command_counter = 0;
		strcpy(p->name,name);
	}
	return p;
}

void CCommand::RestartCommandPointer(const char* name) {
	command_head_t* pHead = FindCommand(name);
	if (!pHead) return;
	pHead->next_command = pHead->head;
	command_t* p = pHead->head;
	while (p) {
		//if (strstr(p->command, "loop ")) p->frame_counter = -1;
		p->frame_counter = -1;
		p = p->next;
		
	}
}

void CCommand::SetCommandPointerAfterEnd(const char* name) {
	command_head_t* pHead = FindCommand(name);
	if (!pHead) return;
	pHead->next_command = NULL;
}

char* CCommand::GetNextCommandLine(const char* name) {
	command_head_t* pHead = FindCommand(name);
	if (!pHead || pHead->deleted) return NULL;
	if (!pHead->next_command || !pHead->next_command->command) return NULL;
	char* p = pHead->next_command->command;
	//command_t* pCommand = pHead->next_command;
	int32_t* pFrameCounter = &(pHead->next_command->frame_counter);
	char* command = p;
	while (isspace(*p)) p++;
	//if (!strncasecmp(p, "loop ", 5)) pHead->next_command = pHead->head;
	if (!strncasecmp(p, "loop ", 5)) {

		// If we reach 0, then we skip to next command
		if (*pFrameCounter == 0) {
			*pFrameCounter = -1;
			pHead->next_command = pHead->next_command->next;
			return GetNextCommandLine(name);
		} else {
			// If counter > 0, we decrement counter and return command (will be restarted)
			if (*pFrameCounter > 0) {
				(*pFrameCounter)--;
				pHead->next_command = pHead->head;
			} else { // frame_counter < 0 ; lets check if there is an argument
				if (sscanf(p+5, "%u", pFrameCounter) == 1) {
					// we have initialized the frame counter
					if (*pFrameCounter == 0) {
						//*pFrameCounter = -1;
						pHead->next_command = pHead->next_command->next;
						command = GetNextCommandLine(name);
					} else {
						if (*pFrameCounter >= 1) {
							pHead->next_command = pHead->head;
						}
					}
					(*pFrameCounter)--;
				} else {
					// We have an unconditional loop and only
					// need to return the loop command. It will then
					// be restarted. However to be sure, we also do it here.
					pHead->next_command = pHead->head;
				}
			}
			return command;
		}
	} else if (!strncasecmp(p, "next ", 5) || !strncasecmp(p, "nextframe ", 10)) {
//fprintf(stderr, "next <%d>\n", *pFrameCounter);
		if (*pFrameCounter == 0) {
			*pFrameCounter = -1;
			pHead->next_command = pHead->next_command->next;
			command = GetNextCommandLine(name);
		} else {
			if (*pFrameCounter > 0) {
				(*pFrameCounter)--;
			} else { // frame_counter < 0 ; lets check if there is an argument
				while (*p && !isspace(*p)) p++;
				while (*p && isspace(*p)) p++;
				if (sscanf(p, "%u", pFrameCounter) == 1) {
					// we have initialized the frame counter
					if (*pFrameCounter == 0) {
						*pFrameCounter = -1;
						pHead->next_command = pHead->next_command->next;
						command = GetNextCommandLine(name);
					} else (*pFrameCounter)--; 
				// Else we have an unconditional next. Always wait a frame
				} else pHead->next_command = pHead->next_command->next;
			}
		}
	} else if (!strncasecmp(p, "else ", 5)) {
		if ((pHead->next_command = pHead->next_command->pEndif)) {
			pHead->next_command = pHead->next_command->next;
			return GetNextCommandLine(name);
		}
		fprintf(stderr, "Error. No matching endif for else <%s>\n", name);
		return NULL;
	} else if (!strncasecmp(p, "if ", 3)) {
		p = p + 3;
		while (*p && isspace(*p)) p++;
		int n = ParseIfCommand(p);
		if (n == 0) {
			// Condition was true Jumping to next line
			pHead->next_command = pHead->next_command->next;
			return GetNextCommandLine(name);
		} else if (n > 0) {
			// Condition was false. Jumping to either else or endif
			if (pHead->next_command->pElse) {
				pHead->next_command = pHead->next_command->pElse->next;
				//pHead->next_command = pHead->next_command->next;
				return GetNextCommandLine(name);
			} else if (pHead->next_command->pEndif) {
				pHead->next_command = pHead->next_command->pEndif->next;
				//pHead->next_command = pHead->next_command->next;
				return GetNextCommandLine(name);
			} else {
				fprintf(stderr, "Error in command <%s>. No else-endif match defined for if\n", name);
			}
		} else {
			// Parsing condition failed
			pHead->next_command = NULL;
			fprintf(stderr, "Condition <%s> for <%s> could not be parsed correctly %d.\n", name, command, n);
			command = NULL;
		}

	// CommandCheck() will parse command looking for labels if needed by goto statements.
	// Otherwise labels are ignored. So here we just skip to next command
	} else if (!strncasecmp(p, "label ", 6)) {
		pHead->next_command = pHead->next_command->next;
		return GetNextCommandLine(name);

	// jump address was already parsed by CommandCheck() and stored in pEndif
	} else if (!strncasecmp(p, "goto ", 5)) {
		pHead->next_command = pHead->next_command->pEndif;
		return GetNextCommandLine(name);

	// We do not want the include commandto be used in a command because it can block
	} else if (!strncasecmp(p, "include ", 8)) {
		pHead->next_command = pHead->next_command->next;
		return GetNextCommandLine(name);

	// Otherwise we just advance the program pointer and return the current command.
	} else pHead->next_command = pHead->next_command->next;
/*
	if (!strncasecmp(command, "delete ", 7)) {
		DeleteCommandByName(name);
		command = NULL;
	}
*/
	return command;
}

int CCommand::ParseIfCommand(const char* condition)
{
//fprintf(stderr, "Parse command <%s>\n", condition ? condition : "NULL");
	int val = -1;
	bool negate = false;
	if (!(condition)) return val;
	while (*condition && isspace(*condition)) condition++;
	if (!(*condition)) return val-1;
	if (*condition == '!') {
		negate = true;
		condition++;
		while (*condition && isspace(*condition)) condition++;
	}
	if (!strncasecmp(condition, "exist", 5)) {
		condition += 5;
		while (*condition && isspace(*condition)) condition++;
		if (*condition != '(') return val-3;
		condition++;
		while (*condition && isspace(*condition)) condition++;
		if (!strncasecmp(condition, "command", 7)) {
			condition += 7;
			while (*condition && isspace(*condition)) condition++;
			if (*condition != ',') return val-4;
			condition++;
			while (*condition && isspace(*condition)) condition++;
			char* name = strdup(condition);
			if (!name) return val-5;
			char* p = name;
			while (*p && !isspace(*p) && *p != ')') p++;
			*p = '\0';
			condition += strlen (name);
			while (*condition && isspace(*condition)) condition++;
			if (*condition != ')') {
				free(name);
				return val-6;
			}
			condition++;
			if (CommandExists(name)) val = negate ? 1 : 0;
			else val = negate ? 0 : 1;
			free(name);
		} else if (!strncasecmp(condition, "image", 5)) {
			u_int32_t image_id;
			if (!m_pVideoMixer) return val-4;
			condition += 5;
			if (*condition != ',') return val-5;
			condition++;
			while (*condition && isspace(*condition)) condition++;
			if ((sscanf(condition, "%u", &image_id) != 1)) return val-6;
			while (isdigit(*condition)) condition++;
			while (*condition && isspace(*condition)) condition++;
			if (*condition != ')') return val-7;
			condition++;
			if (m_pVideoMixer->m_pVideoImage && m_pVideoMixer->m_pVideoImage->GetImageItem(image_id))
				val = negate ? 1 : 0;
			else val = negate ? 0 : 1;
		} else return val-7;
	}
	// feedstate(2,STALLED) (SETUP,PENDING,STALLED,DISCONNECTED,RUNNING)
	// prevstate(2,STALLED) (SETUP,PENDING,STALLED,DISCONNECTED,RUNNING)
	else if (!strncasecmp(condition, "feedstate", 9) || !strncasecmp(condition, "prevstate", 9)) {
		bool prev = (*condition == 'p' || *condition == 'P');
		if (!m_pVideoMixer || !m_pVideoMixer->m_pVideoFeed) {
			fprintf(stderr, "Error. pVideoMixer or pVideoFeed == NULL in ParseIfCommand\n");
			return val-2;
		}
		condition += 9;
		while (*condition && isspace(*condition)) condition++;
		if (*condition != '(') return val-3;
		condition++;
		while (*condition && isspace(*condition)) condition++;
		int feed_no;
		int n = sscanf(condition,"%u", &feed_no);
		if (n != 1) return val-4;
		while (*condition && isdigit(*condition)) condition++;
		while (*condition && isspace(*condition)) condition++;
		if (*condition != ',') return val-5;
		condition++;
		while (*condition && isspace(*condition)) condition++;
		feed_state_enum test_state;
		if (!strncasecmp(condition, "RUNNING", 7)) {
			test_state = RUNNING;
			condition += 7;
		} else if (!strncasecmp(condition, "DISCONNECTED", 12)) {
			test_state = DISCONNECTED;
			condition += 12;
		} else if (!strncasecmp(condition, "STALLED", 7)) {
			test_state = STALLED;
			condition += 7;
		} else if (!strncasecmp(condition, "PENDING", 7)) {
			test_state = PENDING;
			condition += 7;
		} else if (!strncasecmp(condition, "SETUP", 5)) {
			test_state = SETUP;
			condition += 5;
		} else return val-6;
		feed_state_enum* state = prev ?
			m_pVideoMixer->m_pVideoFeed->PreviousFeedState(feed_no) :
			m_pVideoMixer->m_pVideoFeed->FeedState(feed_no);
		if (!state) return val-7;
		while (*condition && isspace(*condition)) condition++;
		if (*condition != ')') return val;
		condition++;
		if (*state == test_state) val = negate ? 1 : 0;
		else val = negate ? 0 : 1;
	} else {
		if (*condition == '1') val = negate ? 1 : 0;
		else if (*condition == '0') val = negate ? 0 : 1;
	}
	if (val < 0) return val;
	while (*condition && isspace(*condition)) condition++;
	if (!(*condition)) {
		return val;
	}
	if (!strncasecmp(condition, "&&", 2)) {
		if (val == 1) return val;	// If first is false we will always have false with &&
		condition += 2;
		int val2 = ParseIfCommand(condition);
		if (val2 < 0) return val;
		if (val2 == 0) return 0;
		else return 1;
	} else if (!strncasecmp(condition, "||", 2)) {
		if (val == 0) return val;	// If first is true we will always have true with ||
		condition += 2;
		int val2 = ParseIfCommand(condition);
		if (val2 < 0) return val;
		if (val2 == 0) return 0;
		else return 1;
	} else return -1;
	return val;
}


char* CCommand::GetCommandLine(const char* name, u_int32_t line) {
//fprintf(stderr, "GetCommandLine <%s>\n", name);
	command_head_t* pHead = FindCommand(name);
	u_int32_t l = 1;
	if (!pHead || pHead->deleted) return NULL;

	// Start of command list
	command_t* p = pHead->head;
	while (p) {
		if (line == l) return p->command;
		p = p->next;
		l++;
	}
	return NULL;
}

int CCommand::CommandPointerAtLine(const char* name, u_int32_t line)
{
	u_int32_t i = 1;
	command_head_t* pHead = FindCommand(name);
	if (!pHead || !line) return -1;
	command_t* p = pHead->head;
	while (p) {
		if (i == line) {
			pHead->next_command = p;
			break;
		}
		i++;
		p = p->next;
	}
	return (p ? 0 : 1);
}

bool CCommand::CommandExists(const char* name)
{
	command_head_t* pHead = FindCommand(name);
	return (pHead && !pHead->deleted ? true : false);
}

bool CCommand::CommandLinesAndPointer(const char* name, u_int32_t* lines, u_int32_t* pointer)
{
	command_head_t* pHead = FindCommand(name);
	if (!pHead) return false;
	if (lines) *lines = pHead->command_counter;
	if (pointer) {
		*pointer = 0;
		command_t* p = pHead->head;
		while (p) {
			(*pointer)++;
			if (p == pHead->next_command) break;	// found
			p = p->next;
		}

	}
	return true;
}

u_int32_t CCommand::CommandNamesLength(command_head_t* pHead)
{
	if (!pHead) pHead = m_command_head;
	if (!pHead) return 0;
	u_int32_t len = 0;
	if (pHead->left) len = CommandNamesLength(pHead->left);
	if (pHead->name && !(*pHead->name)) len += strlen(pHead->name);
	if (pHead->right) len += CommandNamesLength(pHead->right);
	return len;
}


/*
int CCommand::CommandLength(const char* name)
{
	if (!name || !(*name)) return -1;
	command_head_t* p = FindCommand(name);
	if (p) return p->len;
	else return -1;
}
*/

char* CCommand::GetWholeCommand(const char* name)
{
	if (!name || !(*name)) return NULL;
	command_head_t* pHead = FindCommand(name);
	if (!pHead) return NULL;
//fprintf(stderr, "Allocating %u bytes\n", pHead->len);
	char* command = (char*) malloc(pHead->len + 1);
	if (!command) return NULL;		// No memory, return silently
	char* str = command;
	command_t* p = pHead->head;
	int len = pHead->len + 1;
	while (p) {
		if (p->command) {
			int i = strlen(p->command);
			len -= (i+1);
			if (i>0) {
				strcpy(str, p->command);
				if (p->next) *(str+i) = '\n';
				str += (i+1);
			}
			else fprintf(stderr, "Oops. Command longer than it outght to be\n");
		}
		p = p->next;
	}
	//fprintf(stderr, "Got Script <\n%s\n>\n", command);
	return command;
}

command_head_t* CCommand::FindCommand(const char* name)
{
	command_head_t* p = m_command_head;
	int cmp = 0;
	if (!name || !strlen(name)) return NULL;

	// Lets check if we benefit from caching last name
	if (m_last_command && !strcmp(m_last_command->name,name))
		return m_last_command->deleted ? NULL : m_last_command;

	// Ok, we didn't benefit from caching so we have to search
	while (p) {
		if ((cmp = strcmp(p->name,name)) < 0) p = p->left;
		else if (cmp > 0) p = p->right;
		else break;
	}
	if (p) m_last_command = p;
	return p ? (p->deleted ? NULL : p) : NULL;
}

bool CCommand::AddCommandByName(const char* name, const char* command,
	command_head_t* pCommandHead)
{

	command_head_t* p = pCommandHead ? pCommandHead : m_command_head;
	int cmp = 0;
	if (!name || !command || !(*name) || !(*command)) return false;

	// Check if this is the first command at all
	if (!m_command_head) {
		m_command_head = p = pCommandHead ? pCommandHead :
			NewCommandHead(name);
	} else while (p) {
		if ((cmp = strcmp(p->name,name)) < 0) {
			if (!p->left) {
				p->left = pCommandHead ? pCommandHead :
					NewCommandHead(name);
				p = p->left;
				break;
			} else p = p->left;
		} else if (cmp > 0) {
			if (!p->right) {
				p->right = pCommandHead ? pCommandHead :
					NewCommandHead(name);
				p = p->right;
				break;
			} else p = p->right;
		} else break;
	}
	if (p && !cmp && !pCommandHead) m_command_count++;

	if (p) {
		AddCommandToList(p, command);
		m_last_command = p;
	}
	p->deleted = false;
	return p ? true : false;
}

// Search for if, else, endif.
// If pIf is NULL, then search for 'if'
// If pIf is not NULL, search for 'else' and 'endif' for 'pIf'
int CCommand::Parse4IfElseEndif(command_t* pCommand, command_t* pIf)
{
	//if (m_verbose)
	//	fprintf(stderr, "Parse4IfElseEndif <%s>\n", pCommand ? pCommand->command : "NULL");
	int val = -1;
	if (!pCommand) return val;
	if (!pIf) {
		while (pCommand) {
			char* str = pCommand->command;
			if (str) {
				while (isspace(*str)) str++;
				if (strncasecmp(str,"if ", 3) == 0) {
					pCommand->pElse = NULL;
					pCommand->pEndif = NULL;
					if (m_verbose)
						fprintf(stderr, " - Parse4IfElseEndif <%s>\n", str);
					val = Parse4IfElseEndif(pCommand->next, pCommand);
					// if val, we had an error
					if (val) {
						fprintf(stderr, " - Parse4IfElseEndif ERROR for <%s>\n", str);
						return val;
					}
					if (m_verbose)
						fprintf(stderr, " - Parse4IfElseEndif if <%s> "
							"else <%s> endif <%s>\n", str,
							pCommand->pElse ? pCommand->pElse->command :
								"NULL",
							pCommand->pEndif ? pCommand->pEndif->command :
								"NULL");

					// If we found 'endif' we can jump to that
					if (pCommand->pEndif) {
						if (pCommand->pElse) pCommand->pElse->pEndif = pCommand->pEndif;
						pCommand = pCommand->pEndif;
					}
				}
			}
			pCommand = pCommand->next;
		}
		val = 0;
	} else {
		if (m_verbose)
			fprintf(stderr, " - Parse4IfElseEndif Search for else/endif <%s>\n", pIf->command);
		while (pCommand) {
			char* str = pCommand->command;
			if (str) {
				while (isspace(*str)) str++;
				if (strncasecmp(str,"if ", 3) == 0) {
					pCommand->pElse = NULL;
					pCommand->pEndif = NULL;
					if (m_verbose)
						fprintf(stderr, " - Parse4IfElseEndif new IF <%s> <%s>\n", pIf->command, pCommand->command);
					val = Parse4IfElseEndif(pCommand->next, pCommand);
					if (val) {
						fprintf(stderr, " - Parse4IfElseEndif ERROR for <%s>\n", str);
						return val;
					}
					// If we found 'endif' we can jump to that
					if (pCommand->pEndif) {
						if (pCommand->pElse) pCommand->pElse->pEndif = pCommand->pEndif;
						pCommand = pCommand->pEndif;
						val = -1;	// Now go search for other endif
					}
				} else if (strncasecmp(str,"else ", 5) == 0) {
					if (m_verbose)
						fprintf(stderr, " - Parse4IfElseEndif found else for <%s> <%s>\n", pIf->command, pCommand->command);
					pIf->pElse = pCommand;
					// Found else, now look for endif
				} else if (strncasecmp(str,"endif ", 6) == 0) {
					if (m_verbose)
						fprintf(stderr, " - Parse4IfElseEndif found endif for <%s> <%s>\n", pIf->command, pCommand->command);
					pIf->pEndif = pCommand;
					// we found endif and can return;
					return 0;
				}
			}
			pCommand = pCommand->next;
		}
			
	}
	return val;
}


void CCommand::AddCommandToListAtLine(command_head_t* pHead, u_int32_t line, const char* command)
{
	if (!pHead || !command) return;
	u_int32_t len;
	command_t* p = NewCommandLine(command, &len);
	pHead->command_counter++;
	if (line ==1 || !pHead->head) {
		p->next = pHead->head;
		pHead->head = p;
		if (!pHead->tail) pHead->tail = p;
	} else {
		command_t* p2 = pHead->head;
		u_int32_t n = 1;
		while (p2) {
			if (!p2->next || n+1 == line) {
				p->next = p2->next;
				p2->next = p;
				if (!p->next) pHead->tail = p;
				break;
			}
			n++;
			p2 = p2->next;
		}
	}
	pHead->len += (len+1);
}

command_t* CCommand::NewCommandLine(const char* command, u_int32_t* slen)
{
	if (!command) return NULL;
	int len = strlen(command);
	if (slen) *slen = len;
	command_t* p = (command_t*) calloc(sizeof(command_t)+ len + 1, 1);
	if (!p) return NULL;
	p->command = ((char*) p) + sizeof(command_t);
	strcpy(p->command, command);
	p->frame_counter = -1;
	p->pElse = NULL;
	p->pEndif = NULL;
	p->next = NULL;
	return p;
}

// Add command to end of list of command holder (command head)
void CCommand::AddCommandToList(command_head_t* pCommand_head, const char* command)
{
	if (!pCommand_head || !command) return;
/*
	int len = strlen(command);
	command_t* p = (command_t*) calloc(sizeof(command_t)+ len + 1, 1);
	if (!p) return;
	p->command = ((char*) p) + sizeof(command_t);
	strcpy(p->command, command);
	p->frame_counter = -1;
	p->pElse = NULL;
	p->pEndif = NULL;
	p->next = NULL;
*/
	u_int32_t len;
	command_t* p = NewCommandLine(command, &len);
	pCommand_head->command_counter++;
	if (!pCommand_head->next_command) pCommand_head->next_command = p;
	if (pCommand_head->tail) pCommand_head->tail->next = p;
	pCommand_head->tail = p;
	pCommand_head->len += (len+1);
	if (!pCommand_head->head) pCommand_head->head = p;
}

void CCommand::DeleteLineCommandByName(const char* name, u_int32_t line)
{
	if (!name || !(*name) || !line) return;
	command_head_t* p = FindCommand(name);
	if (!p) return;
	unsigned int line_count = 1;
	if (line == 1) {
		command_t* pCommand = p->head;
		if (p->head) {
			p->head = p->head->next;
			(p->command_counter)--;
			if (pCommand) p->len -= (strlen(pCommand->command)+1);
		}
		if (p->tail == pCommand) p->tail = p->head;
		if (pCommand) {
			if (p->next_command == pCommand) p->next_command = pCommand->next;
			free (pCommand);
		}
		return;
	}
	command_t* pList = p->head;
	while (pList) {
		if (line_count + 1 == line) {
			if (pList->next) {
				command_t* pCommand = pList->next;
				pList->next = pList->next->next;
				(p->command_counter)--;
				p->len -= (strlen(pCommand->command)+1);
				if (p->tail == pCommand) {
					if (pCommand->next) p->tail = pCommand->next;
					else p->tail = pList;
				}
				if (p->next_command == pCommand) p->next_command = pCommand->next;
				free(pCommand);
			}
		}
		pList = pList->next;
		line_count++;
	}
}

void CCommand::PopCommandByName(const char* name, u_int32_t lines)
{
	//int cmp = 0;
	if (!name || !(*name)) return;
	command_head_t* p = FindCommand(name);
	if (!p) return;
	if (m_verbose) fprintf(stderr, "Pop command. Command <%s> has %u lines\n", name, p->command_counter);
	while (lines--) {
		if (p->command_counter)
			DeleteLineCommandByName(name,p->command_counter);
	}
	return;
}

void CCommand::DeleteCommandByName(const char* name)
{
	//int cmp = 0;
	if (!name || !(*name)) return;
	command_head_t* p = FindCommand(name);
	if (p) {
		p->deleted = true;
		command_t* pList = p->head;
		while (pList) {
			command_t* tmp = pList->next;
			free(pList);
			pList = tmp;
		}
		p->head = p->tail = NULL;
		p->len = 0;
	}
	return;
}

void CCommand::DeleteCommandHead(command_head_t* pCommand_head, command_head_t* left, command_head_t* right)
{
	if (!pCommand_head) return;
	if (left) DeleteCommandHead(left, left->left, left->right);
	if (right) DeleteCommandHead(right, right->left, right->right);
	while (pCommand_head->head) {
		command_t* tmp = pCommand_head->head->next;
		free(pCommand_head->head);
		pCommand_head->head = tmp;
	}
	free(pCommand_head);
}
