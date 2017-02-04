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
//#include <stdio.h>
#include <ctype.h>
//#include <limits.h>
//#include <string.h>
#ifdef HAVE_MALLOC
#include <malloc.h>
#else
#include <stdlib.h>
#endif
//#include <stdlib.h>
//#include <sys/types.h>
//#include <math.h>
//#include <string.h>

#include "snowmix.h"
#include "tcl_interface.h"
#include "command.h"

//#include "cairo/cairo.h"
//#include "video_mixer.h"
#include "video_text.h"
//#include "add_by.h"
#include "video_image.h"
#include "video_feed.h"

//#include "virtual_feed.h"

class CVideoMixer;

//#define DEBUG 1

#ifdef HAVE_TCL
void SnowmixResultDelete(char* str) {
fprintf(stderr, "TCL C\n");
}

static int Snowmix(ClientData clientData, Tcl_Interp
	*interp, int argc, const char **argv)
{
	CTclInterface* pTclInterface = (CTclInterface*) clientData;

	if (!pTclInterface || !pTclInterface->m_pVideoMixer || argc < 2) {
		fprintf(stderr, "Snowmix called from Tcl with error : %s\n",
			!pTclInterface ? "TclInterface is NULL" :
			  !pTclInterface->m_pVideoMixer ? "m_pVideoMixer NULL" : "No argument");
		return TCL_ERROR;
	}
	CVideoMixer* pVideoMixer = pTclInterface->m_pVideoMixer;

	// Find sum of length of rest of erguments
	int len = 1;
	int i = 1;
	while (i < argc) {
		len += strlen(argv[i]);
		if (*argv[i++]) len += 1;
	}

	// Allocate memory to build a buffer for holding command(s)
	char* command = (char*) malloc(len+1);
	char* p = command;
	if (!command) {
		fprintf(stderr, "Snowmix parse with error : No memory\n");
		return TCL_ERROR;
	}
	//*p = '\0';

	// Fill buffer with argument(s)
	i = 1;
	*p = '\0';
	while (i < argc) {
		//while (isspace(*p)) p++;
		strcpy(p,argv[i++]);
		while (*p) p++;
		*p++ = ' ';
		*p = '\0';
	}

	// parse option
	if (!(strncasecmp("parse", command, 5))) {
		bool silent;
		p = command + 5;
		if ((*p == 's' || *p == 'S') && *(p+1) == ' ') { // parses = silent
			p += 2;
			silent = true;
		} else if (*p == ' ') {
			while (isspace(*p)) p++;
			if (!(strncasecmp("silent ", p, 7))) {
				p += 7;
				silent = true;
			} else silent = false;
		} else {	// parse error
			fprintf(stderr, "Parse error in tcl command snowmix <%s>\n", p);
			goto goto_error;
		}
		while (isspace(*p)) p++;

		// Are there any argument?
		if (!(*p)) {
			fprintf(stderr, "Snowmix parse called from Tcl with error : No command\n");
			goto goto_error;
		}

		// We have arguments. Is there a controller for parsing?
		if (!pVideoMixer->m_pController) {
			fprintf(stderr, "Snowmix parse called from Tcl with error : No controller\n");
			goto goto_error;
		}

		//  Split commands separated by newline and send them to controller parser
		char* buffer = p;
		while (buffer) {
			p = index(buffer, '\n');
			char tmp;
			if (p) {
				*p++ = ' ';
				tmp = *p;
				*p = '\0';
			} else {
				// line did not end in newline. Rest of buffer is one command line
				// We can add space plus nul because we allocated extra space
				p = buffer + strlen(buffer);
				tmp = '\0';
			}
			pVideoMixer->m_pController->controller_parse_command(
				pVideoMixer, silent ? NULL : pTclInterface->m_ctr, buffer);
			if (tmp) {
				*p = tmp;
				buffer = p;
			}
			else buffer = NULL;
		}

	}
	// snowmix info text ( string | font | place | move | backgr | linpat )
	//	( format | ids | maxid | nextavail | <id_list> )
	// snowmix info feed ( geometry | status | extended )
	//      ( format | ids | maxid | nextavail | <id_list> )
	// snowmix info image ( load | place ) ( format | ids | maxid | nextavail | <id_list> )
	// snowmix info ( vfeed | virtual feed ) ( format | ids | maxid | nextavail | <id_list> )
	// snowmix info shape [<id>]
	// snowmix info shape place [<id>]
	// snowmix info audio feed ( info | status | extended ) ( format | ids | maxid | nextavail | <id_list> )
	// snowmix info audio mixer (info | status | extended | source info | source status | source extended ) (format | ids | maxid | nextavail | <id_list> )
	// snowmix info audio feed ( info | status | extended ) ( format | ids | maxid | nextavail | <id_list> )
	// snowmix info system ( info | status | maxplaces ) [ format ]
	else if (!(strncasecmp("info ", command, 5))) {

		p = command + 5;
		while (isspace(*p)) p++;

		// Are there any argument?
		if (!(*p)) {
			fprintf(stderr, "Snowmix info called from Tcl with error : No arguments\n");
			goto goto_error;
		}

	        if (!(strncasecmp("text ", p, 5))) {
			p += 5;
			while (isspace(*p)) p++;

			// Check for class pVideotext
			if (!pVideoMixer->m_pTextItems) {
				fprintf(stderr, "Snowmix info called from Tcl with error : No CTextItems\n");
				goto goto_error;
			}
			if (!(p = pVideoMixer->m_pTextItems->Query(p, true)))
				goto error_info_malformed_query;
		}
	        else if (!(strncasecmp("feed ", p, 5))) {
			p += 5;
			while (isspace(*p)) p++;

			// Check for class CVideoFeed
			if (!pVideoMixer->m_pVideoFeed) {
				fprintf(stderr, "Snowmix info called from Tcl with error : No CVideoFeed\n");
				goto goto_error;
			}
			if (!(p = pVideoMixer->m_pVideoFeed->Query(p, true)))
				goto error_info_malformed_query;
		}
	        else if (!(strncasecmp("vfeed ", p, 6)) || !(strncasecmp("virtual feed ", p, 13))) {
			if (p[1] == 'f' || p[1] == 'F') p += 6;
			else p += 13;
			while (isspace(*p)) p++;

			// Check for class CVirtualFeed
			if (!pVideoMixer->m_pVirtualFeed) {
				fprintf(stderr, "Snowmix info called from Tcl with error : No CVirtualFeed\n");
				goto goto_error;
			}
			while (isspace(*p)) p++;
			if (!(p = pVideoMixer->m_pVirtualFeed->Query(p, true)))
				goto error_info_malformed_query;
		}
	        else if (!(strncasecmp("shape ", p, 6))) {
			p += 6;
			while (isspace(*p)) p++;

			// Check for class pVideoShape
			if (!pVideoMixer->m_pVideoShape) {
				fprintf(stderr, "Snowmix info called from Tcl with error : No CVideoShape\n");
				goto goto_error;
			}
			if (!(p = pVideoMixer->m_pVideoShape->Query(p, true)))
				goto error_info_malformed_query;
		}
	        else if (!(strncasecmp("image ", p, 6))) {
			p += 6;
			while (isspace(*p)) p++;

			// Check for class pVideoImage
			if (!pVideoMixer->m_pVideoImage) {
				fprintf(stderr, "Snowmix info called from Tcl with error : No CVideoImage\n");
				goto goto_error;
			}
			if (!(p = pVideoMixer->m_pVideoImage->Query(p, true)))
				goto error_info_malformed_query;
		}
	        else if (!(strncasecmp("command ", p, 8))) {
			p += 8;
			while (isspace(*p)) p++;

			// Check for class pVideoImage
			if (!pVideoMixer->m_pController && !pVideoMixer->m_pController->m_pCommand) {
				fprintf(stderr, "Snowmix info called from Tcl with error : No CController or CCommand\n");
				goto goto_error;
			}
			if (!(p = pVideoMixer->m_pController->m_pCommand->Query(p, true)))
				goto error_info_malformed_query;
		}
	        else if (!(strncasecmp("audio ", p, 6))) {
			p += 6;
			while (isspace(*p)) p++;

			// Check for audio feed
	        	if (!(strncasecmp("feed ", p, 5))) {
				p += 5;
				while (isspace(*p)) p++;
				// Check for class pAudioFeed
				if (!pVideoMixer->m_pAudioFeed) {
					fprintf(stderr, "Snowmix info called from Tcl with error : No CAudioFeed\n");
					goto goto_error;
				}
				if (!(p = pVideoMixer->m_pAudioFeed->Query(p, true)))
					goto error_info_malformed_query;
			}
			// Check for audio mixer
	        	else if (!(strncasecmp("mixer ", p, 6))) {
				p += 6;
				while (isspace(*p)) p++;
				// Check for class pAudioMixer
				if (!pVideoMixer->m_pAudioMixer) {
					fprintf(stderr, "Snowmix info called from Tcl with error : No CAudioMixer\n");
					goto goto_error;
				}
				if (!(p = pVideoMixer->m_pAudioMixer->Query(p, true)))
					goto error_info_malformed_query;
			}
			// Check for audio sink
	        	else if (!(strncasecmp("sink ", p, 5))) {
				p += 5;
				while (isspace(*p)) p++;
				// Check for class pAudioSink
				if (!pVideoMixer->m_pAudioSink) {
					fprintf(stderr, "Snowmix info called from Tcl with error : No CAudioSink\n");
					goto goto_error;
				}
				if (!(p = pVideoMixer->m_pAudioSink->Query(p, true)))
					goto error_info_malformed_query;
			} else goto error_info_malformed_query;
		}
	        else if (!(strncasecmp("output ", p, 7))) {
			p += 7;
			while (isspace(*p)) p++;

			if (!(p = pVideoMixer->m_pVideoOutput->Query(p, true)))
				goto error_info_malformed_query;
		}
	        else if (!(strncasecmp("system ", p, 7))) {
			p += 7;
			while (isspace(*p)) p++;

			if (!(p = pVideoMixer->Query(p, true)))
				goto error_info_malformed_query;
		} else goto goto_error;
		if (p) {
			Tcl_SetResult(interp, p, TCL_VOLATILE);
			free (p);
		} else goto goto_error;
		
	}
	else if (!(strncasecmp("message ", command, 8))) {
		p = command + 8;
		while (isspace(*p)) p++;
		// We have arguments. Is there a controller for parsing?
		if (!pVideoMixer->m_pController) {
			fprintf(stderr, "Snowmix parse called from Tcl with error : No controller\n");
			goto goto_error;
		}
		pVideoMixer->m_pController->controller_write_msg(pTclInterface->m_ctr, "MSG: %s\n", p);
	} else goto goto_error;

	if (command) free(command);
	return TCL_OK;

error_info_malformed_query:
	fprintf(stderr, "Snowmix info called from Tcl with error : malformed query\n");
goto_error:
	if (command) free(command);
	return TCL_ERROR;
}


static void SnowmixDelete(ClientData clientData) {
//fprintf(stderr, "TCL A\n");
}
#endif

CTclInterface::CTclInterface(CVideoMixer* pVideoMixer)
{
	m_pVideoMixer = pVideoMixer;
	//int res = 0;
#ifdef HAVE_TCL
	//Tcl_FindExecutable(argv0);
	m_pInterp = Tcl_CreateInterp();
	if (Tcl_Init(m_pInterp) != TCL_OK) {
		Tcl_DeleteInterp(m_pInterp);
		m_pInterp = NULL;
		fprintf(stderr, "Tcl Interpreter Initialization Failed\n");
	} else fprintf(stderr, "Tcl Interpreter Initialized\n");
	if (m_pInterp) {
		Tcl_CreateCommand(m_pInterp, "snowmix", Snowmix,
			(ClientData) this, SnowmixDelete);
		Tcl_CreateCommand(m_pInterp, "Snowmix", Snowmix,
			(ClientData) this, SnowmixDelete);
	}
#endif
	m_ctr = NULL;
	//res = TCL_OK;
}

CTclInterface::~CTclInterface()
{
#ifdef HAVE_TCL
fprintf(stderr, "Delete Tcl Interpreter\n");
	if (m_pInterp) {
		Tcl_DeleteInterp(m_pInterp);
		//Tcl_Finalize();
	}
	m_pInterp = NULL;
#endif
}


/*
int EvalFile (char *fileName)
{
    return Tcl_EvalFile(interp, fileName);
}
*/


int CTclInterface::ParseCommand(struct controller_type* ctr, const char* str)
{
	if (!m_pVideoMixer) return 0;
#ifdef HAVE_TCL
	if (!m_pInterp) return 0;
//fprintf(stderr, "Tcl Parse command <%s>\n", str);

	if (!strncasecmp (str, "eval ", 5)) {
		if (set_tcl_eval(ctr, str+5)) return -1;
	} else if (!strncasecmp (str, "exec ", 5)) {
		if (set_tcl_exec(ctr, str+5)) return -1;
	} else if (!strncasecmp (str, "help ", 5)) {
		if (list_help(ctr, str+5)) return -1;
	} else if (!strncasecmp (str, "reset ", 5)) {
		return 0;
	} else return 1;
#endif
	return 0;
}

int CTclInterface::list_help(struct controller_type* ctr, const char* str)
{
	if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
	char* audiofeed = m_pVideoMixer->m_pAudioFeed ? m_pVideoMixer->m_pAudioFeed->Query("syntax") : NULL;
	char* audiomixer = m_pVideoMixer->m_pAudioMixer ? m_pVideoMixer->m_pAudioMixer->Query("syntax") : NULL;
	char* command = m_pVideoMixer->m_pController->m_pCommand ? m_pVideoMixer->m_pController->m_pCommand->Query("syntax") : NULL;
	char* feed = m_pVideoMixer->m_pVideoFeed ? m_pVideoMixer->m_pVideoFeed->Query("syntax") : NULL;
	char* vfeed = m_pVideoMixer->m_pVirtualFeed ? m_pVideoMixer->m_pVirtualFeed->Query("syntax") : NULL;
	char* image = m_pVideoMixer->m_pVideoImage ? m_pVideoMixer->m_pVideoImage->Query("syntax") : NULL;
	char* text = m_pVideoMixer->m_pTextItems ? m_pVideoMixer->m_pTextItems->Query("syntax") : NULL;
	char* shape = m_pVideoMixer->m_pVideoShape ? m_pVideoMixer->m_pVideoShape->Query("syntax") : NULL;
	char* system = m_pVideoMixer->Query("syntax");
	char* output = m_pVideoMixer->m_pVideoOutput->Query("syntax");

	m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Commands:\n"
		MESSAGE"  tcl eval <tcl command>	// evaluate tcl command\n"
		MESSAGE"  tcl exec <command name>	// execute tcl script by command name\n"
		MESSAGE"  tcl reset			// delete interpreter and initialize new\n"
		MESSAGE"  tcl help			// list this help for tcl commands.\n"
		"%s%s%s"				// snowmix info audio feed, audio sink
		"%s%s%s"				// snowmix info audio mixer
		"%s%s%s"				// snowmix info command
		"%s%s%s"				// snowmix info feed
		"%s%s%s"				// snowmix info image
		"%s%s%s"				// snowmix info output
		"%s%s%s"				// snowmix info text
		"%s%s%s"				// snowmix info virtual feed
		"%s%s%s"				// snowmix info shape
		"%s%s%s"				// snowmix info system
//		MESSAGE"  tcl eval snowmix info shape ( syntax | ( ( info | list | place | move ) "
//			"( format | ids | maxid | nextavail | <id_list>) )\n"
//		MESSAGE"  tcl eval snowmix info system (info | status | maxplaces | overlay | syntax) [ format ]\n"
		MESSAGE"  tcl eval snowmix parse <snowmix command>\n"
		MESSAGE"\n",
		audiofeed ? MESSAGE"  tcl eval snowmix info " : "",
		audiofeed ? audiofeed : "",
		audiofeed ? "\n" : "		// audio feed not loaded\n",
		audiomixer ? MESSAGE"  tcl eval snowmix info " : "",
		audiomixer ? audiomixer : "",
		audiomixer ? "\n" : "		// audio mixer not loaded\n",
		command ? MESSAGE"  tcl eval snowmix info " : "",
		command ? command : "",
		command ? "\n" : "	// command not loaded\n",
		feed ? MESSAGE"  tcl eval snowmix info " : "",
		feed ? feed : "",
		feed ? "\n" : "		// feed not loaded\n",
		image ? MESSAGE"  tcl eval snowmix info " : "",
		image ? image : "",
		image ? "\n" : "	// image not loaded\n",
		output ? MESSAGE"  tcl eval snowmix info " : "",
		output ? output : "",
		output ? "\n" : "	// output not loaded\n",
		text ? MESSAGE"  tcl eval snowmix info " : "",
		text ? text : "",
		text ? "\n" : "		// text not loaded\n",
		vfeed ? MESSAGE"  tcl eval snowmix info " : "",
		vfeed ? vfeed : "",
		vfeed ? "\n" : "		// virtual feed not loaded\n",
		shape ? MESSAGE"  tcl eval snowmix info " : "",
		shape ? shape : "",
		shape ? "\n" : "		// shape not loaded\n",
		system ? MESSAGE"  tcl eval snowmix info " : "",
		system ? system : "",
		system ? "\n" : "		// system mixer not loaded\n"
	);
	if (audiofeed) free (audiofeed);
	if (audiomixer) free (audiomixer);
	if (command) free (command);
	if (feed) free (feed);
	if (image) free (image);
	if (text) free (text);
	if (vfeed) free (vfeed);
	if (shape) free (shape);
	if (system) free (system);
	if (output) free (output);
	return 0;
}

// Evaluate expression
int CTclInterface::set_tcl_eval(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (*str == '\0') return -1;
	int res = 0;
	m_ctr = ctr;
#ifdef HAVE_TCL
#ifdef DEBUG
	fprintf(stderr, "TCL eval <%s>\n", str);
#endif
        res = Tcl_Eval(m_pInterp, str);
/*        const char* str_res = */ Tcl_GetStringResult(m_pInterp);
/*
fprintf(stderr, "TCL <%s>\n", str_res);
	char* p = strdup(str_res);
	char* p_to_free = p;
	int n = strlen(p) + 2;
	char* pnl = (char*) malloc(n);
*/
	Tcl_ResetResult(m_pInterp);
	Tcl_FreeResult(m_pInterp);
/*
        if (res == TCL_OK && pnl) {
#ifdef DEBUG
		fprintf(stderr, "TCL got <%s>\n", p);
#endif
		while (*p && *p != '\n') p++;
		if (*p == '\n') {
			p++;
			while (*p) {
				char* p2 = p;
				while (*p && *p != '\n') p++;
				if (*p == '\n') *p++ = '\0';
#ifdef DEBUG
fprintf(stderr, "PARSING <%s> ctr is %s\n", p2, ctr ? "ptr" : "null");
#endif
				sprintf(pnl, "%s \n", p2);
				m_pVideoMixer->m_pController->controller_parse_command(
					m_pVideoMixer, ctr, pnl);
			}
		} // else fprintf(stderr, "TCL, no command to parse\n");
	} else {
#ifdef DEBUG
		fprintf(stderr, "TCL error. No result to parse. Script was <%s>\n<%s>\n", str, p_to_free ? p_to_free : "null");
#endif
	}
	if (pnl) free(pnl);
	if (p_to_free) free(p_to_free);
*/
#endif
	m_ctr = NULL;
	return res;
}

// Evaluate command
int CTclInterface::set_tcl_exec(struct controller_type* ctr, const char* str)
{
	//char* line = NULL;

	if (!str) return -1;
	while (isspace(*str)) str++;
	if (*str == '\0') return -1;
	if (!m_pVideoMixer || !m_pVideoMixer->m_pController ||
		!m_pVideoMixer->m_pController->m_pCommand) return -1;
	CCommand* pCommand = m_pVideoMixer->m_pController->m_pCommand;
	char* script_name = strdup(str);
	if (!script_name) return -1;
	char* p = script_name;
	while (*p) p++;
	p--;
	while (isspace(*p)) { *p = '\0'; p--; }
	char* script = pCommand->GetWholeCommand(script_name);
	if (script_name) free(script_name);	// Need to free up script_name copied with strdup.
	if (set_tcl_eval(ctr, script)) {
		if (script) free(script);
		return -1;
	}
	return 0;
}
