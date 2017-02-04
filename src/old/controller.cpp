/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Scott		pscott@mail.dk
 *		 Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */

/************************************************************************
*									*
*	Command interface.						*
*									*
************************************************************************/

#ifndef WIN32
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/utsname.h>
#else
#include <winsock.h>
#include <io.h>
#include "windows_util.h"
#endif
#include <stdio.h>
//#include <stdlib.h>
//#include <stdint.h>
//#include <stdarg.h>
//#include <ctype.h>
//#include <sys/select.h>
//#include <sys/time.h>
#include <sys/types.h>
//#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>




#include "snowmix.h"
#include "video_mixer.h"
#include "video_text.h"
#include "audio_feed.h"
#include "audio_sink.h"
#include "video_shape.h"
#include "video_mixer.h"
#include "video_feed.h"
#include "virtual_feed.h"
#include "tcl_interface.h"
#include "command.h"
#include "snowmix_util.h"
#include "controller.h"
#if defined (HAVE_OSMESA) || defined (HAVE_GLX)
#include "opengl_video.h"
#endif


/* Function to delete trailing white space chars on a string. */
void trim_string (char *str)
{
	char	*cp;
	if (!str) return;
	int len = strlen(str);

	if (len <= 0) return;

	cp = str + len - 1;
	while (cp >= str && *cp <= ' ' && *cp > 0) {
	//while (cp >= str && *cp <= ' ') {
		*cp = 0;			/* Move the terminator back. */
		cp--;
	}
}

/* Initialize the controller subsystem. */
CController::CController(CVideoMixer* pVideoMixer, char *init_file)
{
	struct controller_type	*ctr;
	m_pController_list = NULL;
	m_controller_listener = -1;
	m_host_allow = NULL;

	//m_pFeed_list = NULL;
        m_pSystem_feed = NULL;
	m_monitor_status = false;
	m_ini_file_read = false;
	m_longname = NULL;
	//m_pMonitor = NULL;

	if (!pVideoMixer) {
		fprintf(stderr, "Error: CController was created with pVideoMixer set to NULL\n");
		exit(1);
	}

	/* Create the default controler on stdin/stdout. */
	ctr = (controller_type*) calloc (1, sizeof (*ctr));
	ctr->read_fd = 0;
	ctr->write_fd = 1;
	ctr->ini_fd = -1;
	// ctr->output_buffers = NULL;		// Already done by calloc
	// ctr->prev = ctr->next = NULL;
	// ctr->feed_id = ctr->sink_id = 0;
	list_add_head (m_pController_list, ctr);

	/* Say hellow to the console controller. */
	controller_write_msg (ctr, "%s", BANNER);

	if (init_file != NULL) {
		controller_write_msg (ctr, "Copyright Peter Maersk-Moller 2012-2015.\n");
		if (pVideoMixer) {
			const char* path;
			int index = 0;
			controller_write_msg (ctr, "Search Path : ");
			while ((path = pVideoMixer->GetSearchPath(index))) {
				controller_write_msg (ctr, "%s%s",
					index++ ? ", " : "", path);
			}
			controller_write_msg (ctr, "\n");
		}

		/* Setup a controller entity to read the init file. */
		ctr = (controller_type*) calloc (1, sizeof (*ctr));

		//ctr->read_fd = open (init_file, O_RDONLY);
		ctr->read_fd = pVideoMixer->OpenFile(init_file, O_RDONLY, -1, &(m_longname));
		if (ctr->read_fd < 0) {
			fprintf (stderr, "Unable to open init file: \"%s\": %s\n",
				init_file, strerror (errno));
			fprintf(stderr, "Snowmix require an ini file to start.\n"
				"Read more about Snowmix on http://snowmix.sourceforge.net/index.html "
				"and http://snowmix.sourceforge.net/Examples/basic.html/\n");
			ControllerListDeleteElement(ctr,false);	// free (ctr);
			exit(0);
		}
		if (m_verbose) fprintf(stderr, "Controller opened file %s for reading.\n", m_longname);
		ctr->write_fd = -1;
		ctr->ini_fd = ctr->read_fd;
		// ctr->output_buffers = NULL;		// Already done by calloc
		// ctr->prev = ctr->next = NULL;
		// ctr->feed_id = ctr->sink_id = 0;	// Already done by calloc

		list_add_head (m_pController_list, ctr);
	}
	m_command_name				= NULL;
	m_pCommand				= NULL;
	m_verbose				= false;
	m_maxplaces_text			= MAX_STRINGS;
        m_maxplaces_font			= MAX_FONTS;
        m_maxplaces_images			= MAX_IMAGES;
        m_maxplaces_text_places 		= MAX_TEXT_PLACES;
        m_maxplaces_image_places 		= MAX_IMAGE_PLACES;
        m_maxplaces_shapes 			= MAX_SHAPES;
        m_maxplaces_shape_places 		= MAX_SHAPE_PLACES;
        m_maxplaces_patterns	 		= MAX_PATTERNS;
        m_maxplaces_audio_feeds			= MAX_AUDIO_FEEDS;
        m_maxplaces_audio_mixers		= MAX_AUDIO_MIXERS;
        m_maxplaces_audio_sinks			= MAX_AUDIO_SINKS;
        m_maxplaces_video_feeds			= MAX_VIDEO_FEEDS;
        m_maxplaces_virtual_feeds		= MAX_VIRTUAL_FEEDS;
	m_frame_seq_no				= 0;
	m_lagged_frames				= 0;
	m_broadcast				= false;
	m_control_port				= -1;
	m_control_connections			= 2;		// The ini file and stdin/stdout/stderr.
								// Closing the inifile will decrement the count
}

CController::~CController() {
	while (m_host_allow) {
		host_allow_t* p = m_host_allow->next;
		free(m_host_allow);
		m_host_allow = p;
	}
	if (m_longname) free(m_longname);
}


int CController::WriteData(struct controller_type *ctr, u_int8_t* buffer, int32_t len, int alt_fd) {
	int			fd, written = 0;
	output_buffer_t*	output_buffer;

	// If write_fd < 0 and alt_fd < 0, then fail
	if (!ctr || (ctr->write_fd < 0 && alt_fd < 0)) return 0;

	// First we check if we should try to write. If there is no buffer,
	// already queed in a list, we try to write
	if (!ctr->output_buffers) {
		if (ctr->write_fd < 0) fd = alt_fd; // No output. Write to stderr
		else fd = ctr->write_fd;
		written = write(fd, buffer, len);
		if (written < 0) {
			perror("Writing failed for WriteMsg");
			written = 0;
		}
	}

	// Then we see if there is data to be written later
	if (written < len) {
		if (!(output_buffer = (output_buffer_t*)
			malloc(sizeof(output_buffer_t) + len - written))) {
			fprintf(stderr, "Failed to allocate buffer for WriteMsg\n");
			return 0;
		}
		output_buffer->data = ((u_int8_t*)output_buffer) + sizeof(output_buffer_t);
		output_buffer->len = len - written;
		output_buffer->next = NULL;
		memcpy(output_buffer->data, &buffer[written], len - written);
		if (!ctr->output_buffers) {
			ctr->output_buffers = output_buffer;
			return len;
		}
		output_buffer_t* p = ctr->output_buffers;
		while (p->next) p = p->next;
		p->next = output_buffer;
	}
	
	return len;
}
/* Write a message to a controller (or all controllers). */
int CController::controller_write_msg (struct controller_type *ctr, const char *format, ...)
{
	va_list	ap;
	char	buffer [3000];
	int	n;

	va_start (ap, format);
	n = vsnprintf (buffer, sizeof (buffer), format, ap);
	va_end (ap);


	if (ctr) return WriteData(ctr, (u_int8_t*)buffer, n, 2);
	else {
		/* Write to all controllers. */
		if (m_broadcast) for (ctr = m_pController_list; ctr != NULL; ctr = ctr->next) {
			if (!ctr->is_audio) WriteData(ctr, (u_int8_t*)buffer, n, 2);;
		}
	}

	return n;
}

char* CController::SetCommandName(const char* name)
{
	if (m_command_name) free(m_command_name);
	if (name) {
		while (isspace(*name)) name++;
		m_command_name = strdup(name);
		if (m_command_name) trim_string(m_command_name);
		return m_command_name;
	}
	return NULL;
}


/* Parse received commands. */
void CController::controller_parse_command (CVideoMixer* pVideoMixer, struct controller_type *ctr, char *line)
{
	char	*co = line;
	char	*ci = line;
	int	space_last;
	int	x;
	if (!pVideoMixer) {
		fprintf(stderr, "pVideoMixer was NULL for controller_parse_command.\n");
		return;
	}
//fprintf(stderr, "PARSE <%s>\n", line);

	/* Loose any initial whitespace and prune up the command. */
	while (*ci != 0 && *ci <= ' ' && *ci > 0) 	ci++;
	space_last = 0;
	while (*ci != 0) {
		if (*ci <= ' ' && *ci > 0 && space_last) {
			/* Skip this extra space. */
			ci++;
		} else if (*ci <= ' ' && *ci >= 0) {
		//} else if (*ci <= ' ') {
			/* Add a space to the output. */
			*co = ' ';
			co++;
			ci++;
			space_last = 1;
		} else {
			/* Add the char. */
			*co = *ci;
			co++;
			ci++;
			space_last = 0;
		}
	}
	*co = 0;	/* Terminate the string properly. */
	ci = line;

/*
	{
		// Loose any initial whitespace and prune up the command.
		while (*ci2 != 0 && *ci2 <= ' ' && *ci2 > 0) 	ci2++;
		space_last = 0;
		while (*ci2 != 0) {
			if (*ci2 <= ' ' && *ci2 > 0 && space_last) {
				// Skip this extra space.
				ci2++;
			} else if (*ci2 <= ' ' && *ci2 >= 0) {
			//} else if (*ci2 <= ' ') {
				// Add a space to the output.
				*co2 = ' ';
				co2++;
				ci2++;
				space_last = 1;
			} else {
				// Add the char.
				*co2 = *ci2;
				co2++;
				ci2++;
				space_last = 0;
			}
		}
		*co2 = 0;	// Terminate the string properly.
		ci2 = line2;
		if (strcmp(line,line2)) fprintf(stderr, "Stringdif <%s> != <%s>\n", line,line2);
		free (line2);
	}
*/
	{
		int n = strlen(ci);
		if (n && (*(ci+n-1) != ' ')) fprintf(stderr, "Line without end space <%s>\n", line);
	}

	/* Terminate the line on a '#' char - that is for comments. */
	if (*ci == '#') {
		*ci = 0;
	}

	//co = index (ci, '#');
	//if (co != NULL) {
	//	*co = 0;
	//}

	// ctr is NULL if we are executing a command
	if (ctr && m_command_name) {
		if (!strncasecmp (ci, "command end ",
			strlen ("command end "))) {
			if (m_verbose) controller_write_msg(ctr, "MSG: command"
				" end <%s>\n", m_command_name);
                        bool not_tcl = true;
                        int len = strlen(m_command_name);
                        if (len > 3) {
				not_tcl = strncasecmp(m_command_name+len-4,".tcl",4) ? true : false;
                        }
			if (not_tcl && m_pCommand && m_command_name)
				m_pCommand->CommandCheck(m_command_name);
			if (m_command_name) free(m_command_name);
			m_command_name = NULL;
		} else {
			//fprintf(stderr, "command add <%s>\n", ci);
			if (!m_pCommand) m_pCommand = new CCommand(pVideoMixer);
			if (m_pCommand) m_pCommand->AddCommandByName(
				m_command_name, ci);
		}
		return;
	}
else if (!strncasecmp (ci, "check ", 6)) {
	char* p = strdup(ci+6);
	TrimFileName(p);
	fprintf(stderr, "Trim(%s) => (%s)\n", ci+6, p);
	free(p);
	return;
}

	// command .....
	else if (!strncasecmp (ci, "command ", 8)) {
		if (!m_pCommand) m_pCommand = new CCommand(pVideoMixer);
		if (!m_pCommand) {
			controller_write_msg (ctr, "MSG: Failed to "
				"initialize CCommand\n");
			return;
		}
		int n = m_pCommand->ParseCommand(this, ctr, ci+8);
		if (!n) return;
		if (n<1) goto wrong_param_no;
		goto invalid;
	}
	// tcl ....
	else if (!strncasecmp (ci, "tcl ", 4)) {
		char* str = ci + 4;
		if (!strncasecmp (str, "reset ", 6)) {
			if (!pVideoMixer->m_pTclInterface) goto invalid;
			delete pVideoMixer->m_pTclInterface;
			pVideoMixer->m_pTclInterface = NULL;
		}
		if (!pVideoMixer->m_pTclInterface) {
			pVideoMixer->m_pTclInterface =
			new CTclInterface(pVideoMixer);
			if (pVideoMixer->m_pTclInterface) {
				if (!pVideoMixer->m_pTclInterface->InterpreterCreated()) {
					delete pVideoMixer->m_pTclInterface;
					pVideoMixer->m_pTclInterface = NULL;
				}
			}
		}
		if (!pVideoMixer->m_pTclInterface) {
			controller_write_msg (ctr, "MSG: Failed to "
				"initialize CTclInterface\n");
			return;
		}
		int n = pVideoMixer->m_pTclInterface->ParseCommand(ctr, str);
		if (!n) return;
		if (n<1) goto wrong_param_no;
		goto wrong_param_no;	// Fixme. Actual something else failed
	}
	// virtual feed ....
	else if (!strncasecmp(ci, "vfeed ", 6) || !strncasecmp (ci, "virtual feed ", 13)) {
		if (!pVideoMixer->m_pVirtualFeed) pVideoMixer->m_pVirtualFeed =
			 new CVirtualFeed(pVideoMixer, m_maxplaces_virtual_feeds);
		if (!pVideoMixer->m_pVirtualFeed) {
			controller_write_msg (ctr,
				"MSG: Failed to initialize CVirtualFeed\n");
			return;
		}
		int n = pVideoMixer->m_pVirtualFeed->ParseCommand(ctr, ci + (*(ci+5) == ' ' ? 6 : 13));
		if (!n) return;
		if (n<1) goto wrong_param_no;
		goto wrong_param_no;	// Fixme. Actual something else failed

	}
	else if (!strncasecmp (ci, "system ", 7)) {
		char* str = ci+7;
		while (isspace(*str)) str++;

		// Check for system info
		if (!strncasecmp (str, "info ", 5)) {
			int n = list_system_info(pVideoMixer, ctr, str+5);
			if (!n) return;
			if (n<1) goto wrong_param_no;
			goto invalid;
		// Check system name
		} else if (!strncasecmp (str, "name ", 5)) {
			str = str+5;
			while (isspace(*str)) str++;
			if (!(*str)) {
				controller_write_msg (ctr, STATUS" system name = %s\n",
					pVideoMixer->m_mixer_name ? pVideoMixer->m_mixer_name : "");
			} else {
				if (pVideoMixer->m_mixer_name) free(pVideoMixer->m_mixer_name);
#ifndef WIN32
				struct utsname sysname;
				sysname.nodename[0] = '\0';
				uname(&sysname);
				int len = 4 + strlen(sysname.nodename) + strlen(str);
#else
				int len = 4 + 7 + strlen(str); // "Windows" = 7
#endif				
				if ((pVideoMixer->m_mixer_name = (char*) calloc(1, len))) {
#ifndef WIN32
					sprintf(pVideoMixer->m_mixer_name, "%s : %s", sysname.nodename, str);
#else
					sprintf(pVideoMixer->m_mixer_name, "Windows : %s", str);
#endif
					trim_string(pVideoMixer->m_mixer_name);
				} else goto invalid;
			}
		// Check for system geometry
		} else if (!strncasecmp (str, "geometry ", 9)) {
			int	w, h;
			char	format [100];
			str = str + 9;

			// We need to initalize CVideoFeeds first to set geometry for feed 0
			if (!pVideoMixer->m_pVideoFeed)
				pVideoMixer->m_pVideoFeed =
					new CVideoFeeds(pVideoMixer, m_maxplaces_video_feeds);
			if (!pVideoMixer->m_pVideoFeed) {
				fprintf(stderr, "pVideoFeed failed to initialize before "
					"setting geometry.\n");
				goto invalid;
			}
			while (isspace(*str)) str++;
			if (!(*str)) {
				controller_write_msg (ctr, STATUS" system geometry = %ux%u %s\n",
					pVideoMixer->GetSystemWidth(),
					pVideoMixer->GetSystemHeight(),
					pVideoMixer->m_pixel_format);
			} else {
				x = sscanf (str, "%d %d %s", &w, &h, &format [0]);
				if (x != 3) goto wrong_param_no;
				x = feed_init_basic (pVideoMixer, w, h, format);
				if (x < 0) goto invalid;
			}
		}
		else if (!strncasecmp (str, "frame rate ", 11)) {
			double r;
			str = str+11;
			while (isspace(*str)) str++;
			if (!(*str)) {
				controller_write_msg (ctr, STATUS" system frame rate = %.4lf\n",
					pVideoMixer->m_frame_rate);
			} else {
				x = sscanf (str , "%lf", &r);
				if (x != 1) goto wrong_param_no;
				if (r < MIN_SYSTEM_FRAME_RATE ||
					r > MAX_SYSTEM_FRAME_RATE) goto invalid;
				pVideoMixer->m_frame_rate = r;
			}
		}
		else if (!strncasecmp (str, "socket ", strlen ("socket "))) {
			if (!pVideoMixer->GetSystemWidth() ||
				!pVideoMixer->m_frame_rate) goto setup_geo_frame_first;
			if (!pVideoMixer->m_pVideoOutput) goto invalid;
			x = pVideoMixer->m_pVideoOutput->InitOutput (ctr, str + strlen ("socket "));
			if (!x) return;
			if (x<1) goto wrong_param_no;
			else goto invalid;
		}
		else if (!strncasecmp (str, "control port ", strlen ("control port "))) {
			x = set_system_controller_port(ctr, str+13);
			if (!x) return;
			else if (x<0) goto wrong_param_no;
			else goto invalid;
		}
		else if (!strncasecmp (str, "host allow ", 11)) {
			int x = host_allow(ctr, str+11);
			if (x < 0) goto wrong_param_no;
			else if (x > 0) goto invalid;
		}
		else if (!strncasecmp (str, "output ", 7)) {
			if (!pVideoMixer->m_pVideoOutput) goto invalid;
			x = pVideoMixer->m_pVideoOutput->ParseCommand(ctr, str+7);
			if (x < 0) goto wrong_param_no;
			else if (x > 0) goto invalid;
		}
		else goto invalid;
	}
	// audio ...
	else if (!strncasecmp (ci, "audio ", strlen ("audio "))) {
		char* str = ci + 6;
		while (isspace(*str)) str++;
		if (!strncasecmp (str, "feed ", 5)) {
			if (!pVideoMixer->m_pAudioFeed) pVideoMixer->m_pAudioFeed =
			 	new CAudioFeed(pVideoMixer, m_maxplaces_audio_feeds);
			if (!pVideoMixer->m_pAudioFeed) {
				controller_write_msg (ctr,
					"MSG: Failed to initialize CAudioFeed\n");
				return;
			}
			int n = pVideoMixer->m_pAudioFeed->ParseCommand(ctr, str+5);
			if (n == 0) return;
			if (n<1) goto wrong_param_no;
			goto invalid;
		}
		else if (!strncasecmp (str, "sink ", 5)) {
			if (!pVideoMixer->m_pAudioSink) pVideoMixer->m_pAudioSink =
			 	new CAudioSink(pVideoMixer, m_maxplaces_audio_sinks);
			if (!pVideoMixer->m_pAudioSink) {
				controller_write_msg (ctr,
					"MSG: Failed to initialize CAudioSink\n");
				return;
			}
			int n = pVideoMixer->m_pAudioSink->ParseCommand(ctr, str+5);
			if (!n) return;
			if (n<1) goto wrong_param_no;
			goto invalid;
		}
		else if (!strncasecmp (str, "mixer ", 6)) {
			if (!pVideoMixer->m_pAudioMixer) pVideoMixer->m_pAudioMixer =
			 	new CAudioMixer(pVideoMixer, m_maxplaces_audio_mixers);
			if (!pVideoMixer->m_pAudioMixer) {
				controller_write_msg (ctr,
					"MSG: Failed to initialize CAudioMixer\n");
				return;
			}
			int n = pVideoMixer->m_pAudioMixer->ParseCommand(ctr, str+6);
			if (!n) return;
			if (n<1) goto wrong_param_no;
			goto invalid;
		} else goto invalid;

	}
	// feed
	else if (!strncasecmp (ci, "feed ", 5)) {
		if (!pVideoMixer->m_pVideoFeed) pVideoMixer->m_pVideoFeed =
			 new CVideoFeeds(pVideoMixer, m_maxplaces_video_feeds);
		if (!pVideoMixer->m_pVideoFeed) {
			controller_write_msg (ctr,
				"MSG: Failed to initialize CVideoFeeds\n");
			return;
		}
		int n = pVideoMixer->m_pVideoFeed->ParseCommand(ctr, ci+5);
		if (!n) return;
		if (n<1) goto wrong_param_no;
		goto invalid;

	}
	// text ...
	else if (!strncasecmp (ci, "text ", 5)) {
		if (!pVideoMixer->m_pTextItems)
			pVideoMixer->m_pTextItems =
				new CTextItems(pVideoMixer,
					m_maxplaces_text, m_maxplaces_font,
					m_maxplaces_text_places);
		if (pVideoMixer->m_pTextItems) {
			int n = pVideoMixer->m_pTextItems->ParseCommand(this, ctr, ci+5);
			if (!n) return;
			else if (n < 0) goto wrong_param_no;
			goto invalid;
		} else controller_write_msg (ctr,
			"MSG : Failed to create class TextItem.\n");
		return;
	}
	// image ...
	else if (!strncasecmp (ci, "image ", strlen ("image "))) {
		if (!pVideoMixer->m_pVideoImage) {
			pVideoMixer->m_pVideoImage = new CVideoImage( pVideoMixer,
				m_maxplaces_images, m_maxplaces_image_places);
			if (!pVideoMixer->m_pVideoImage) {
				controller_write_msg (ctr,
					"MSG : Unabale to create class CVideoImage.\n");
				goto invalid;
			}
		}
		int n = pVideoMixer->m_pVideoImage->ParseCommand(this, ctr, ci+6);
		if (!n) return;
		else if (n < 0) goto wrong_param_no;
		goto invalid;
	}
	else if (!strncasecmp (ci, "shape ", 6)) {
		if (!pVideoMixer->m_pVideoShape) {
			pVideoMixer->m_pVideoShape = new CVideoShape( pVideoMixer,
				m_maxplaces_shapes, m_maxplaces_shape_places);
			if (!pVideoMixer->m_pVideoShape) {
				controller_write_msg (ctr,
					"MSG : Unable to create class CVideoShape.\n");
				goto invalid;
			}
		}
		int n = pVideoMixer->m_pVideoShape->ParseCommand(ctr, ci+6);
		if (!n) return;
		else if (n < 0) goto wrong_param_no;
		goto invalid;
	}
#if defined (HAVE_OSMESA) || defined (HAVE_GLX)
	else if (!strncasecmp(ci, "gl ", 3) || !strncasecmp (ci, "glshape ", 8)) {
		if (!pVideoMixer->m_pOpenglVideo) {
			pVideoMixer->m_pOpenglVideo = new COpenglVideo(pVideoMixer);
				//, m_maxplaces_shapes, m_maxplaces_shape_places
			if (!pVideoMixer->m_pOpenglVideo) {
				controller_write_msg (ctr,
					"MSG : Unable to create class COpenglVideo.\n");
				goto invalid;
			}
		}
		int n = pVideoMixer->m_pOpenglVideo->ParseCommand(ctr, ci+((*(ci+2) == ' ') ? 3 : 8));
		if (!n) return;
		else if (n < 0) goto wrong_param_no;
		goto invalid;
	}
#endif	// #if defined (HAVE_OSMESA) || defined (HAVE_GLX)
	else if (!strncasecmp (ci, "overlay pre ", 12)) {
		char* str = ci + 12;
		while (isspace(*str)) str++;
		if (!*str) {
			controller_write_msg (ctr, STATUS" overlay pre : %s\n",
				pVideoMixer->m_command_pre ? pVideoMixer->m_command_pre : "");
		} else {
			if (pVideoMixer->m_command_pre) free(pVideoMixer->m_command_pre);
			pVideoMixer->m_command_pre = NULL;
			trim_string(str);
			if (*str) pVideoMixer->m_command_pre = strdup(str);
			//else goto wrong_param_no;
		}
	}
	else if (!strncasecmp (ci, "overlay finish ", 15)) {
		char* str = ci + 15;
		while (isspace(*str)) str++;
		if (!*str) {
			controller_write_msg (ctr, STATUS" overlay finish : %s\n",
				pVideoMixer->m_command_finish ? pVideoMixer->m_command_finish : "");
		} else {
			if (pVideoMixer->m_command_finish) free(pVideoMixer->m_command_finish);
			pVideoMixer->m_command_finish = NULL;
			trim_string(str);
			if (*str) pVideoMixer->m_command_finish = strdup(str);
			//else goto wrong_param_no;
		}
	}
	else if (!strncasecmp (ci, "png write ", strlen ("png write "))) {
		if (pVideoMixer->m_write_png_file) free(pVideoMixer->m_write_png_file);
		pVideoMixer->m_write_png_file = NULL;
		char* str = ci + strlen ("png write ");
		while (isspace(*str)) str++;
		trim_string(str);
		if (*str) pVideoMixer->m_write_png_file = strdup(str);
		else goto wrong_param_no;
	}
	else if (!strncasecmp (ci, "monitor ", strlen ("monitor "))) {
		if (strlen(ci) == strlen("monitor ")) list_monitor (ctr);
		else if (!strncasecmp (ci, "monitor on ", strlen ("monitor on ")))
			set_monitor_on (pVideoMixer, ctr);
		else if (!strncasecmp (ci, "monitor off ", strlen ("monitor off ")))
			set_monitor_off (pVideoMixer, ctr);
		else  goto invalid;
	}

	else if (!strncasecmp (ci, "select ", strlen ("select "))) {
		int	no;

		x = sscanf (ci + strlen ("select "), "%d", &no);
		if (x != 1) goto wrong_param_no;
		if (!pVideoMixer || !pVideoMixer->m_pVideoFeed) {
			controller_write_msg (ctr, "MSG: pVideoMixer and pVideoFeed must "
				"be set before 'select ' can be used.\n");
			goto invalid;
		}
		pVideoMixer->m_pVideoFeed->output_select_feed (ctr, no);
	}
	else if (!strncasecmp (ci, "stack ", 6)) {
		char* str = ci+6;
		while (isspace(*str)) str++;
		if (!strncasecmp (str, "list ", 5)) {
			controller_write_msg (ctr, MESSAGE);
			for (int x = 0; x < MAX_STACK; x++) {
				if (pVideoMixer->m_output_stack[x] > -1)
					controller_write_msg (ctr, " %d", pVideoMixer->m_output_stack[x]);
			}
			controller_write_msg (ctr, "\n");
		} else {
			output_set_stack (pVideoMixer, ctr, str);
		}
	}

	else if (!strncasecmp (ci, "stat ", strlen ("stat "))) {
		/* Dump a feed/output stack status. */
		if (!pVideoMixer->m_pVideoFeed) {
			fprintf(stderr, "Can not stat feeds as pVideoFeed is NULL\n");
			goto invalid;
		}
		struct feed_type *feed = pVideoMixer->m_pVideoFeed->GetFeedList();

		controller_write_msg (ctr, "STAT: ID  Name                  State               Mode     Cutout       Size     Offset  Stack Layer\n");
		for ( ; feed != NULL; feed = feed->next) {
			char	*state;
			char	cut [20];
			char	layer [20];

			switch (feed->state) {
				case SETUP:        state = (char*) "Setup";        break;
				case PENDING:      state = (char*) "Pending";      break;
				case DISCONNECTED: state = (char*) "Disconnected"; break;
				case RUNNING:      state = (char*) "Running";      break;
				case STALLED:      state = (char*) "Stalled";      break;
				default:      state = NULL;      break;
			}
			if (feed->width == pVideoMixer->GetSystemWidth() &&
				feed->height == pVideoMixer->GetSystemHeight()) {
				strcpy (cut, "Fullscreen");
			} else {
				strcpy (cut, "Cutout");
			}

			strcpy (layer, "--");
			for (x = 0; x < MAX_STACK; x++) {
				if (pVideoMixer->m_output_stack [x] == feed->id) {
					sprintf (layer, "%d", x);
					break;
				}
			}

			controller_write_msg (ctr, "STAT: %2d  <%-20s>  %-12s  "
				"%10s  %4dx%-4d  %4dx%-4d  %4dx%-4d  %2d:%-2d %s\n",
						   feed->id, feed->feed_name ? feed->feed_name : "",
						   state, cut,
						   feed->cut_start_column, feed->cut_start_row,
						   feed->cut_columns, feed->cut_rows,
						   feed->shift_columns, feed->shift_rows,
						   feed->scale_1, feed->scale_2,
						   layer);
		}
		controller_write_msg (ctr, "STAT: --\n");	/* End of table marker. */
	}
	else if (!strncasecmp (ci, "quit ", strlen ("quit "))) {
		exit_program = 1;
fprintf(stderr, "shutting down snowmix\n");
	}

	// Set or list maxplaces
	else if (!strncasecmp (ci, "maxplaces ", strlen ("maxplaces "))) {
		int n = set_maxplaces (ctr, ci );
		if (n < 0) goto wrong_param_no;
		else if (n > 0) goto invalid;
		else return;
		if (n != 1) goto wrong_param_no;
		 else goto invalid;
		return;


	// write out message
	} else if (!strncasecmp (ci, "message ", 8)) {
		//fprintf(stderr, MESSAGE"<%u><%s>\n", m_frame_seq_no, ci + strlen ("message "));
		controller_write_msg(ctr, "MSG: %s\n", ci+8);

	// write out message with frame number
	} else if (!strncasecmp (ci, "messagef ", 9)) {
		controller_write_msg(ctr, "MSG: <%u> %s\n", m_frame_seq_no, ci+9);

	// Toggle verbosity
	} else if (!strncasecmp (ci, "verbose ", strlen ("verbose "))) {
		m_verbose = !m_verbose;
		if (m_verbose) controller_write_msg(ctr, "MSG: verbose is on\n");

	// include <file name>
	} else if (!strncasecmp (ci, "include ", strlen ("include "))) {
		trim_string(ci+8);
		if (ctr->longname) {
			free(ctr->longname);
			ctr->longname = NULL;
		}
		//int read_fd = open (ci+8, O_RDONLY);
		int read_fd = pVideoMixer->OpenFile(ci+8, O_RDONLY, -1, &(ctr->longname));
		if (read_fd < 0) {
			fprintf (stderr, "Unable to open include file: \"%s\": %s\n",
				ci+8, strerror (errno));
			goto invalid;
		}
		if (m_verbose) fprintf(stderr, "Controller opened file %s for reading.\n", m_longname);
		off_t file_size = lseek(read_fd, 0, SEEK_END);
		if (file_size < 0 || lseek(read_fd, 0, SEEK_SET) < 0) {
			fprintf (stderr, "Unable to seek in include file: \"%s\": %s\n",
				ctr->longname ? ctr->longname : ci+8, strerror (errno));
			close (read_fd);
			goto invalid;
		}
		char* buffer = (char*) malloc((size_t) file_size + 2);
		char* buffer_to_free = buffer;
		if (!buffer) {
			fprintf (stderr, "Unable to allocate memory for reading include file: \"%s\"\n",ci+8);
			close(read_fd);
			goto invalid;
		}
		int bytes_read = 0;
		while (bytes_read < file_size) {
			int n = read(read_fd, buffer+bytes_read, file_size - bytes_read);
			if (n<0) break;
			bytes_read += n;
		}
		close(read_fd);
		if (bytes_read != file_size) {
			fprintf(stderr, "Failed to read all the bytes of the include file: \"%s\"\n",ci+8);
			free(buffer_to_free);
			goto invalid;
		}
		*(buffer+bytes_read) = '\0';
		char* ci = buffer;
		char* p = NULL;
		while ((p = index(buffer, '\n')) != NULL) {
			if (*p) {
				*p = ' ';
				p++;
				char tmp = *p;
				*p = '\0';
				controller_parse_command (pVideoMixer, ctr, buffer);
				*p = tmp;
			} else {
				fprintf(stderr, "Unexpected nul while reading include file %s\n", ci+8);
				break;
			}
			buffer = p;
		}
		free (buffer_to_free);
	
	// require version
	} else if (!strncasecmp (ci, "require version ",
		strlen ("require version "))) {
		int n = require_version(ctr, ci);
		if (n > 0) exit(1);

	// List Help
//fprintf(stderr, " - Parsing <%s> near help\n", ci);
	} else if (!strncasecmp (ci, "h ", strlen ("h ")) ||
	    !strncasecmp (ci, "help ", strlen ("help "))) {
		if (list_help(ctr, ci)) goto invalid;

	} else if (!strncasecmp (ci, "about ", 6)) {
		controller_write_msg(ctr, "MSG: Snowmix version "SNOWMIX_VERSION"\n"
			"MSG: by "SNOWMIX_AUTHOR".\n"
			"MSG: All rights reserved.\n"
			"MSG:\n");
		return;
	// Command not found. Search in the command list
	} else if (m_pCommand) {
		trim_string(ci);
		if (m_pCommand->CommandExists(ci)) {
			if (ParseCommandByName (pVideoMixer, ctr, ci)) goto invalid;
			return;
		} else {
			/* Only complain if it contains anything but space chars. */
			for (co = ci; *co != 0; co++) {
				if (*co > ' ') {
					if (ctr) controller_write_msg (ctr,
						"MSG: Unknown command: \"%s\"\n", ci);
					break;
				}
			}
		}
	} else {

		/* Only conplain if it contains anything but space chars. */
		for (co = ci; *co != 0; co++) {
			if (*co > ' ') {
				if (ctr) controller_write_msg (ctr,
					"MSG: Unknown command: \"%s\"\n", ci);
				break;
			}
		}
	}
	return;


setup_geo_frame_first:
	controller_write_msg (ctr, "MSG: Setup geometry and frame rate first\n");
	return;

// > 0
invalid:
	controller_write_msg (ctr, "MSG: Invalid parameters: \"%s\"\n", ci);
	return;
// < 0
wrong_param_no:
	controller_write_msg (ctr, "MSG: Invalid number of parameters: \"%s\"\n", ci);
	return;
}

// Return list in allocated memory of allowed hosts and networks
// Memory should be freed after use. Format is xxx.xxx.xxx.xxx/xx,xxx.xxx.........
// And we allocate one more byte than we use, so the string can be expanded with
// a newline after it is returned.
char* CController::GetHostAllowList(unsigned int* len)
{
	int n = 0;
	host_allow_t* ph;
	char* str = NULL;
	char* retstr;
	if (!(ph = m_host_allow)) return NULL;
	while (ph) { n++; ph = ph->next; }
	if (len) *len = 19*n;
	if (!(str = (char*) malloc(19*n))) {
		fprintf(stderr, "Failed to allocate memory in CController::GetHostAllowList()\n");
		return NULL;
	}
	*str = '\0';
	retstr = str;
	ph = m_host_allow;
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

int CController::list_system_info(CVideoMixer* pVideoMixer, struct controller_type* ctr, const char* str)
{
	if (!ctr || !pVideoMixer) return 1;
	u_int32_t outbuf_count = 0;
	u_int32_t inuse = 0;
	if (pVideoMixer->m_pVideoOutput)
		pVideoMixer->m_pVideoOutput->InUseCount(&outbuf_count, &inuse);
	char* host_allow = GetHostAllowList();
	
	controller_write_msg (ctr, STATUS" System info\n"
		STATUS" Snowmix version      : "SNOWMIX_VERSION"\n"
		STATUS" System name          : %s\n"
		STATUS" Ini file             : %s\n"
		STATUS" System geometry      : %ux%u pixels\n"
		STATUS" Pixel format         : %s\n"
		STATUS" Bytes per pixel      : %u\n"
		STATUS" Frame rate           : %.3lf\n"
		STATUS" Block size mmap      : %u\n"
		STATUS" Frame sequence no.   : %u\n"
		STATUS" Lagged frames        : %u\n"
		STATUS" Open Control conns.  : %d\n"
		STATUS" Control port number  : %d\n"
		STATUS" Host allow           : %s\n",
		pVideoMixer->m_mixer_name ? pVideoMixer->m_mixer_name : "",
		m_longname ? m_longname : "no ini file loaded",
		pVideoMixer->GetSystemWidth(),
		pVideoMixer->GetSystemHeight(),
		pVideoMixer->m_pixel_format,
		pVideoMixer->m_pixel_bytes,
		pVideoMixer->m_frame_rate,
		pVideoMixer->m_block_size,
		m_frame_seq_no,
		m_lagged_frames,
		m_control_connections,
		m_control_port,
		host_allow ? host_allow : "undefined - only 127.0.0.1");
	if (host_allow) free(host_allow);
/*
	host_allow_t* ph = m_host_allow;
	while (ph) {
		u_int32_t mask = ph->ip_to - ph->ip_from;
		u_int32_t count = 32;
		while (mask) {
			mask = mask >> 1;
			count--;
		}
		controller_write_msg (ctr, " %u.%u.%u.%u/%u",
			(ph->ip_from >> 24),
			(0xff0000 & ph->ip_from) >> 16,
			(0xff00 & ph->ip_from) >> 8,
			ph->ip_from & 0xff, count);
		ph = ph->next;
	}
*/

	controller_write_msg (ctr,
		STATUS" Ctr verbose          : %s\n"
		STATUS" Ctr broadcast        : %s\n"
		STATUS" Output ctr sock name : %s\n"
		STATUS" Output ctr fd        : master %d client %d\n"
		STATUS" Output settings      : mode %u delay %u\n"
		STATUS" Output frames inuse  : %u of %u\n"
		STATUS" Output shm name      : %s\n"
		STATUS" Video feeds          : %s\n"
		STATUS" Virtual video feeds  : %s\n"
		STATUS" Video text           : %s\n"
		STATUS" Video image          : %s\n"
		STATUS" Video shapes         : %s\n"
		STATUS" Command interface    : %s\n"
		STATUS" Audio feeds          : %s\n"
		STATUS" Audio mixers         : %s\n"
		STATUS" Audio sinks          : %s\n"
		STATUS" TCL interpreter      : %s\n"
		STATUS" Stack                :",
		m_verbose ? "yes" : "no",
		m_broadcast ? "yes" : "no",
		pVideoMixer->m_pVideoOutput &&
			pVideoMixer->m_pVideoOutput->OutputMasterName() ?
			pVideoMixer->m_pVideoOutput->OutputMasterName() : "",
		pVideoMixer->m_pVideoOutput ?
		  pVideoMixer->m_pVideoOutput->OutputMasterSocket() : -1,
		pVideoMixer->m_pVideoOutput ?
		  pVideoMixer->m_pVideoOutput->OutputClientSocket() : -1,
		pVideoMixer->m_pVideoOutput ?
		  pVideoMixer->m_pVideoOutput->OutputMode() : 0,
		pVideoMixer->m_pVideoOutput ?
		  pVideoMixer->m_pVideoOutput->OutputDelay() : 1,
		inuse,
		outbuf_count,
		pVideoMixer->m_pVideoOutput &&
			pVideoMixer->m_pVideoOutput->OutputMemoryHandleName() ?
			pVideoMixer->m_pVideoOutput->OutputMemoryHandleName() : "",
		pVideoMixer->m_pVideoFeed ? "loaded" : "no",
		pVideoMixer->m_pVirtualFeed ? "loaded" : "no",
		pVideoMixer->m_pTextItems ? "loaded" : "no",
		pVideoMixer->m_pVideoImage ? "loaded" : "no",
		pVideoMixer->m_pVideoShape ? "loaded" : "no",
		m_pCommand ? "loaded" : "no",
		pVideoMixer->m_pAudioFeed ? "loaded" : "no",
		pVideoMixer->m_pAudioMixer ? "loaded" : "no",
		pVideoMixer->m_pAudioSink ? "loaded" : "no",
		pVideoMixer->m_pTclInterface ? "loaded" : "no"

	);
	for (int x = 0; x < MAX_STACK; x++) {
		if (pVideoMixer->m_output_stack[x] > -1)
			controller_write_msg (ctr, " %d", pVideoMixer->m_output_stack[x]);
	}
	controller_write_msg (ctr, "\n"STATUS"\n");
	return 0;
}

int CController::ParseCommandByName (CVideoMixer* pVideoMixer, controller_type* ctr, char* commandname)
{
	if (m_pCommand && m_pCommand->CommandExists(commandname)) {
		char* str = NULL;
		while ((str = m_pCommand->GetNextCommandLine(commandname))) {
			while (isspace(*str)) str++;
			if (!strncasecmp(str, "loop ", 5)) {
				//m_pCommand->RestartCommandPointer(commandname);
				break;
			}
			if (!strncasecmp(str, "next ", 5) || !strncasecmp(str, "nextframe ", 10)) break;
			if (pVideoMixer) {
				char* command = strdup(str);
/*
				if (!strncasecmp(str, "overlay feed ", 13)) {
					if (pVideoMixer)
						pVideoMixer->OverlayFeed(str + 13);
				} else if (!strncasecmp(str, "overlay text ", 13)) {
					if (pVideoMixer)
						pVideoMixer->OverlayText(str + 13,
							pVideoMixer->m_pCairoGraphic);
				} else if (!strncasecmp(str, "overlay image ", 14)) {
					if (pVideoMixer)
						pVideoMixer->OverlayImage(str + 14);
				} else if (!strncasecmp(str, "cairooverlay feed ", 18)) {
					if (pVideoMixer)
						pVideoMixer->CairoOverlayFeed(
							pVideoMixer->m_pCairoGraphic,
								str + 18);
				} else if (!strncasecmp(str, "overlay virtual feed ",
					21)) {
					if (pVideoMixer && pVideoMixer->m_pVirtualFeed)
						pVideoMixer->m_pVirtualFeed->OverlayFeed(
							pVideoMixer->m_pCairoGraphic,
								str + 21);
				} else {
*/
					int n = strlen(commandname);
					if (strncmp(command, commandname, n) == 0 &&
						*(command + n) == ' ') {
						fprintf(stderr, "Recursive call In ParseCommandByName is forbidden. <%s> calling <%s>\n", commandname, command);
						free (command);
						break;
					}
					controller_parse_command (pVideoMixer,
						ctr, command);		// Can we safely parse ctr being not NULL ??????
						//NULL, command);	// We used as a test earlier to prevent loop
									// but the need for that appear to no longer
									// exist

//				}
				free (command);
			}
		}
	} else return -1;
	return 0;
}
/* Populate rfds and wrfd for the controller subsystem. */
int CController::SetFds (int maxfd, fd_set *rfds, fd_set *wfds)
{
	struct controller_type	*ctr;

	for (ctr = m_pController_list; ctr != NULL; ctr = ctr->next) {

		// Check that we can write and have data to write
		if (ctr->write_fd > -1 && ctr->output_buffers) {
			// Add its descriptor to the bitmap.
			FD_SET (ctr->write_fd, wfds);
			if (ctr->write_fd > maxfd) {
				maxfd = ctr->write_fd;
			}
		}

		// Now check if we can not read from controller
		if (ctr->read_fd < 0) continue;		// Not reading.

		// Add its descriptor to the bitmap.
		FD_SET (ctr->read_fd, rfds);
		if (ctr->read_fd > maxfd) {
			maxfd = ctr->read_fd;
		}
	}

	/* Add the master socket to the list. */
	if (m_controller_listener >= 0) {
		FD_SET (m_controller_listener, rfds);
		if (m_controller_listener > maxfd) {
			maxfd = m_controller_listener;
		}
	}

	return maxfd;
}


/* Check if there is anything to read or write on a controller socket. */
void CController::CheckReadWrite(CVideoMixer* pVideoMixer, fd_set *rfds, fd_set *wfds)
{
	struct controller_type	*ctr, *next_ctr;

	if (!pVideoMixer) {
		fprintf(stderr, "pVideoMixer for controller_check_read was NULL.\n");
		return;
	}

	// Check the list of known controllers.
	for (ctr = m_pController_list, next_ctr = ctr ? ctr->next : NULL;
	     ctr != NULL;
	     ctr = next_ctr, next_ctr = ctr ? ctr->next : NULL) {
		int	x, written;
		char	*ci, *co;

		// First we check write
		if (ctr->write_fd > -1 && ctr->output_buffers && !ctr->is_audio && !ctr->feed_id) {
			// We have data to write
			while (ctr->output_buffers) {
				// Check if buffer is empty and discard if it is and continue with next buffer
				if (!ctr->output_buffers->data || !ctr->output_buffers->len) {
					output_buffer_t* p = ctr->output_buffers->next;
					free (ctr->output_buffers);
					ctr->output_buffers = p;
					continue;
				}
				// Write current buffer
				written = write(ctr->write_fd, ctr->output_buffers->data, ctr->output_buffers->len);

				// If it will block or it is closed, written will less than 1
				if (written < 1) break;		// We can not write any more

				// If we wrote the whole buffer, we can set it's len to zero and continue loop
				if (written == ctr->output_buffers->len) {
					ctr->output_buffers->len = 0;
					continue;
				}

				// Written was less than what was in the buffer
				u_int8_t* data = ((u_int8_t*)ctr->output_buffers)+sizeof(output_buffer_t);
				ctr->output_buffers->len -= written;	// Bytes left to write
				memmove(data, ctr->output_buffers->data + written, ctr->output_buffers->len);
				ctr->output_buffers->data = data;

				// As we wrote less than we wanted, next write will block, so we break loop
				break;
			}
		}

		// Then we check for read
		if (ctr->read_fd < 0) continue;			// Not able to read.
		if (!FD_ISSET (ctr->read_fd, rfds)) continue;	// No data for this one.

		// Check if the ctr is used for streaming audio
		if (ctr->is_audio && ctr->feed_id) {
			// Get state
			// Check that we have a pointer to the audio feed class
			if (pVideoMixer->m_pAudioFeed) {

				feed_state_enum state = pVideoMixer->m_pAudioFeed->GetState(ctr->feed_id);
				if (state == RUNNING || state == PENDING || state == STALLED) {
					u_int32_t size;
					audio_buffer_t* buf = pVideoMixer->m_pAudioFeed->
						GetEmptyBuffer(ctr->feed_id, &size);
					if (buf) {
						buf->len = x = read(ctr->read_fd, buf->data,
							size);
						if (x > 0) pVideoMixer->m_pAudioFeed->
							AddAudioToFeed(ctr->feed_id, buf);
						else free(buf);
					// else we need to close the connection
					} else x = -1;
				} else {
					fprintf(stderr, "Error. Controller is set to audio feed, "
						"but the feed is in state %s. Closing controller for fd %d,%d\n",
						feed_state_string(state), ctr->read_fd, ctr->write_fd);
					x = -1;
				}

			} else {
				fprintf(stderr, "Error. Controllor set to audio feed, "
					"but we have no CAudioFeed.\n");
				// We set x = -1. This should close the connection
				x = -1;
			}
		// Otherwise this is a normal control channel
		} else {

			/* Read the new data from the socket. */
			x = read (ctr->read_fd, &ctr->linebuf [ctr->got_bytes],
				sizeof (ctr->linebuf) - ctr->got_bytes - 1);
			//fprintf(stderr, "Got %d bytes - ctr\n", x);

		}
		if (x <= 0) {
			// EOF. Close this controller.

			// Check if this is the initial ini file
			if (ctr->read_fd > -1 && ctr->read_fd == ctr->ini_fd)
				m_ini_file_read = true;

/* This is done in ControllerListDeleteElement
			// Remove controller from list
			list_del_element (m_pController_list, ctr);
			close (ctr->read_fd);
			if (ctr->write_fd >= 0) {
				close (ctr->write_fd);
			}
*/

			if (ctr->feed_id && pVideoMixer->m_pAudioFeed) {
				fprintf(stderr, "Closing ctr for audio feed %d\n",ctr->feed_id);
				pVideoMixer->m_pAudioFeed->StopFeed(ctr->feed_id);
			}
			if (ctr->sink_id && pVideoMixer->m_pAudioSink) {
				fprintf(stderr, "Closing ctr for audio sink %u\n", ctr->sink_id);
				pVideoMixer->m_pAudioSink->StopSink(ctr->sink_id);
			}
			ControllerListDeleteElement(ctr, true);	// free (ctr);

			m_control_connections--;
			continue;
		}
		if (ctr->is_audio) continue;

		ctr->got_bytes += x;
		ctr->linebuf [ctr->got_bytes] = 0;	/* Make sure it is properly terminated. */

		/* Check if the buffer contains a full line. */
		while ((ci = index (ctr->linebuf, '\n')) != NULL) {
			char	line [1024];

			/* Yes. Copy the line to a new buffer. */
			ci = ctr->linebuf;
			co = line;
			while (*ci != 0) {
				*co = *ci;
				co++;
				if (*ci == '\n') break;
				ci++;
			}
			*co = 0;
			controller_parse_command (pVideoMixer, ctr, line);
			if (ctr->close_controller) break;

			/* Shift the rest of the buffer down to the beginning and try again. */
			ci++;
			co = ctr->linebuf;
			while (*ci != 0) {
				*(co++) = *(ci++);
			}
			*co = 0;	/* Terminate the result. */
		}
		if (ctr->close_controller) {
fprintf(stderr, "Closing Controller\n");
			ControllerListDeleteElement(ctr, true);	// free (ctr);
/*
			list_del_element (m_pController_list, ctr);
			close (ctr->read_fd);
			if (ctr->write_fd >= 0) {
				close (ctr->write_fd);
			}
			free (ctr);
*/
fprintf(stderr, "Controller Closed\n");
			continue;
		}

		/* Prepare for the next read. */
		ctr->got_bytes = strlen (ctr->linebuf);
	}

	if (m_controller_listener >= 0 && FD_ISSET (m_controller_listener, rfds)) {
		/* New connection. Create a new controller element. */
		struct sockaddr_in addr;
		socklen_t len = sizeof(addr);
		ctr = (controller_type*) calloc (1, sizeof (*ctr));
		ctr->read_fd = accept (m_controller_listener, (struct sockaddr*) &addr, &len);

		// Now check the connecting address. Assume IPV4
		// FIXME : Rewrite to include IPv6
		if (addr.sin_family != AF_INET) {
			fprintf(stderr, "Only IPv4 supported yet\n");
			close(ctr->read_fd);
			ctr->read_fd = -1;
		} else {
			char	ipstr[INET_ADDRSTRLEN];
			u_int32_t ip = ntohl(*((u_int32_t*)&addr.sin_addr));
			if (!m_host_allow) {
				// Only ip = 127.0.0.1 is allowed then
				if (ip != 0x7f000001) {
					close(ctr->read_fd);
					ctr->write_fd = ctr->read_fd = -1;
				}
				if (m_verbose) {
					inet_ntop(AF_INET, &(addr.sin_addr), ipstr, INET_ADDRSTRLEN);
					fprintf(stderr, "Connection. No host_allow. Using default. "
						"Host %s allowed ? %s\n", ipstr,
						ctr->read_fd > -1 ? "yes" : "no");
				}
			} else {
				if (host_not_allow(ip)) {
					close(ctr->read_fd);
					ctr->write_fd = ctr->read_fd = -1;
				}
				// FIXME. Use inet_ntop to print address.
				// See http://www.beej.us/guide/bgnet/output/html/multipage/inet_ntopman.html
				if (m_verbose) {
					inet_ntop(AF_INET, &(addr.sin_addr), ipstr, INET_ADDRSTRLEN);
					fprintf(stderr, "Connection. Host %s allowed ? %s\n",
						ipstr, ctr->read_fd > -1 ? "yes" : "no");
				}
			}
		}
		if (ctr->read_fd >= 0) {
			char	msg [100];

			ctr->write_fd = ctr->read_fd;
			ctr->got_bytes = 0;
			ctr->is_audio = false;
			ctr->feed_id = 0;
			ctr->sink_id = 0;
			// ctr->output_buffers = NULL;		// Already done by calloc
			ctr->close_controller = false;

			/* Mark the writer socket as nonblocking. */
			set_fd_nonblocking(ctr->read_fd);

			list_add_head (m_pController_list, ctr);

			/* Say hellow to the new controller. */
			sprintf (msg, "%s", BANNER);
			ssize_t l = strlen(msg);
			do {
				ssize_t n = write (ctr->write_fd, msg, l);
				if (n < 0) {
					perror("Failed to write to controllor\n");
				}
				l -= n;
			} while (l > 0);
			m_control_connections++;
		} else {
			/* Failed on accept. */
			ControllerListDeleteElement(ctr, false);	// free (ctr);
		}
	}
}

void CController::ControllerListDeleteElement (struct controller_type* ctr,  bool list_delete)
{
	if (!ctr) return;
//fprintf(stderr, "Deleting Controller\n");
	if (list_delete) {
		list_del_element (m_pController_list, ctr);
	}
	if (ctr->read_fd > -1) close (ctr->read_fd);
	if (ctr->write_fd >= 0) close (ctr->write_fd);
	while (ctr->output_buffers) {
		output_buffer_t* p = ctr->output_buffers->next;
		free (ctr->output_buffers);
		ctr->output_buffers = p;
	}
	if (ctr->longname) free(ctr->longname);
	free (ctr);
}

/* Find a feed by its id number. */
/*
struct feed_type* CController::find_feed (int id)
{
        struct feed_type        *feed;

        for (feed = m_pFeed_list; feed != NULL; feed = feed->next) {
                if (feed->id == id) {
                        return feed;
                }
        }
        return NULL;
}
*/

// require version <version>
int CController::require_version (struct controller_type *ctr, char* ci )
{
	if (!ci || !(*ci)) return 1;
	u_int32_t snow_major1, snow_major2, snow_minor;
	u_int32_t req_major1, req_major2, req_minor;
	req_minor = 0;
	int n = sscanf(SNOWMIX_VERSION,"%u.%u.%u", &snow_major1, &snow_major2,
		&snow_minor);
	if (n != 3) return 1;
	int j = sscanf(ci, "require version %u.%u.%u", &req_major1, &req_major2,
		&req_minor);
	if (j < 2) return -1;
	if (req_major1 > snow_major1 || (req_major1 == snow_major1 && ((req_major2 > snow_major2) ||
		(req_major2 == snow_major2 && req_minor > snow_minor)))) {
		controller_write_msg (ctr, "This configuration require Snowmix version "
			"%u.%u.n where n must be at least %u. ", req_major1, req_major2,
			req_minor);
		controller_write_msg (ctr, "Found Snowmix version %s\n", SNOWMIX_VERSION);
		return 1;
	}
	if (snow_major1 > req_major1 || ((snow_major1 == req_major1) && ((snow_major2 > req_major2) ||
		(snow_major2 == req_major2 && snow_minor > req_minor)))) {
		controller_write_msg (ctr, "WARNING. This configuration require Snowmix version "
			"%u.%u.%u. ", req_major1, req_major2, req_minor);
		controller_write_msg (ctr, "Found Snowmix version %s\n", SNOWMIX_VERSION);
		return 0;
	}
	if (req_major1 == snow_major1 && req_major2 == snow_major2 &&
		req_minor <= snow_minor) {
		if (m_verbose) controller_write_msg (ctr, "Snowmix version %s "
			"satisfy requirement for version %u.%u.%u\n",
			SNOWMIX_VERSION, req_major1, req_major2, req_minor);
		return 0;
	}
	controller_write_msg (ctr, "This configuration require Snowmix version "
		"%u.%u.n where n must be at least %u. ", req_major1, req_major2,
		req_minor);
	controller_write_msg (ctr, "Found Snowmix version %s\n", SNOWMIX_VERSION);
	return 1;
}


// Set system control port
int CController::set_system_controller_port (struct controller_type *ctr, char* ci )
{
	int	port;
	if (!ci) return 1;
	while (isspace(*ci)) ci++;
	int x = sscanf (ci, "%d", &port);
	if (x != 1 || port < 0 || port > 65535) return -1;
	if (m_controller_listener >= 0) {
		controller_write_msg (ctr, "MSG: Listener have already been setup\n");
	} else {
		/* Setup the controller listener socket. */

		m_controller_listener = socket (AF_INET, SOCK_STREAM, 0);
		if (m_controller_listener < 0) {
			perror ("Unable to create controller listener");
			exit (1);
		}

#ifndef WIN32
		int	yes = 1;
		if (setsockopt (m_controller_listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (yes)) < 0) {
#else
		bool	yes = true;
		if (setsockopt(m_controller_listener, SOL_SOCKET, SO_REUSEADDR, (char*)(&yes), sizeof(yes)) < 0) {
#endif
			perror ("setsockopt on m_controller_listener");
			exit (1);
		}

		{
			struct sockaddr_in addr;

			memset (&addr, 0, sizeof (addr));
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = INADDR_ANY;
				addr.sin_port = htons (port);
			if (bind (m_controller_listener, (struct sockaddr *)&addr, sizeof (addr)) < 0) {
				perror ("bind on m_controller_listener");
				exit (1);
			}
		}

		if (listen (m_controller_listener, MAX_LISTEN_BACKLOG) < 0) {
			perror ("listen on m_controller_listener");
			exit (1);
		}
		m_control_port = port;
	}
	return 0;
}

// List or set maxplaces
int CController::set_maxplaces (struct controller_type *ctr, char* ci )
{
	if (strlen(ci) == strlen("maxplaces ")) {
		controller_write_msg (ctr,
			"MSG: Max text strings     : %u\n"
			"MSG: Max font definitions : %u\n"
			"MSG: Max text places      : %u\n"
			"MSG: Max loaded images    : %u\n"
			"MSG: Max placed images    : %u\n"
			"MSG: Max shapes           : %u\n"
			"MSG: Max placed shapes    : %u\n"
			"MSG: Max shape patterns   : %u\n"
			"MSG: Max audio feeds      : %u\n"
			"MSG: Max audio mixers     : %u\n"
			"MSG: Max audio sinks      : %u\n"
			"MSG: Max video feeds      : %u\n"
			"MSG: Max virtual feeds    : %u\n"
			"MSG:\n",
			m_maxplaces_text, m_maxplaces_font, m_maxplaces_text_places,
			m_maxplaces_images, m_maxplaces_image_places,
			m_maxplaces_shapes, m_maxplaces_shape_places,
			m_maxplaces_patterns,
			m_maxplaces_audio_feeds,
			m_maxplaces_audio_mixers,
			m_maxplaces_audio_sinks,
			m_maxplaces_video_feeds,
			m_maxplaces_virtual_feeds);
		return 0;
	}
	char* s = ci + strlen("maxplaces ");
	if (!strncasecmp (s, "strings ", strlen ("strings "))) {
		int n = sscanf(s, "strings %u", &m_maxplaces_text);
		if (n != 1) return -1;
	} else if (!strncasecmp (s, "fonts ", strlen ("fonts "))) {
		int n = sscanf(s, "fonts %u", &m_maxplaces_font);
		if (n != 1) return -1;
	} else if (!strncasecmp (s, "texts ", strlen ("texts "))) {
		int n = sscanf(s, "texts %u", &m_maxplaces_text_places);
		if (n != 1) return -1;
	} else if (!strncasecmp (s, "images ", strlen ("images "))) {
		int n = sscanf(s, "images %u", &m_maxplaces_images);
		if (n != 1) return -1;
	} else if (!strncasecmp (s, "imageplaces ", strlen ("imageplaces "))) {
		int n = sscanf(s, "imageplaces %u", &m_maxplaces_image_places);
		if (n != 1) return -1;
	} else if (!strncasecmp (s, "shapes ", strlen ("shapes "))) {
		int n = sscanf(s, "shapes %u", &m_maxplaces_shapes);
		if (n != 1) return -1;
	} else if (!strncasecmp (s, "shapeplaces ", strlen ("shapeplaces "))) {
		int n = sscanf(s, "shapeplaces %u", &m_maxplaces_shape_places);
		if (n != 1) return -1;
	} else if (!strncasecmp (s, "shape patterns ", strlen ("shape patterns "))) {
		int n = sscanf(s, "shape patterns %u", &m_maxplaces_patterns);
		if (n != 1) return -1;
	} else if (!strncasecmp (s, "audio feeds ", strlen ("audio feeds "))) {
		int n = sscanf(s, "audio feeds %u", &m_maxplaces_audio_feeds);
		if (n != 1) return -1;
	} else if (!strncasecmp (s, "audio mixers ", strlen ("audio mixers "))) {
		int n = sscanf(s, "audio mixers %u", &m_maxplaces_audio_mixers);
		if (n != 1) return -1;
	} else if (!strncasecmp (s, "audio sinks ", strlen ("audio sinks "))) {
		int n = sscanf(s, "audio sinks %u", &m_maxplaces_audio_sinks);
		if (n != 1) return -1;
	} else if (!strncasecmp (s, "video feeds ", strlen ("video feeds "))) {
		int n = sscanf(s, "video feeds %u", &m_maxplaces_video_feeds);
		if (n != 1) return -1;
	} else if (!strncasecmp (s, "virtual feeds ", strlen ("virtual feeds "))) {
		int n = sscanf(s, "virtual feeds %u", &m_maxplaces_virtual_feeds);
		if (n != 1) return -1;
	} else return 1;
	return 0;
}


bool CController::host_not_allow (u_int32_t ip)
{
	if (!m_host_allow) return true;
	host_allow_t* p = m_host_allow;

	// m_host_allow is a sorted list with lowest range first
	while (p) {
		if (p->ip_from > ip) break;
		if (p->ip_to >= ip) return false; // it is allowed
		p = p->next;
	}
	return true; // it is not allowed
}

int CController::host_allow (struct controller_type *ctr, const char* ci)
{
	u_int32_t ip_from, ip_to, a, b, c, d, e;
	if (!ci) return -1;
	e = 33;
	//ip_to = ip_from = 0;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		host_allow_t* ph = m_host_allow;
		while (ph) {
			u_int32_t mask = ph->ip_to - ph->ip_from;
			u_int32_t count = 32;
			while (mask) {
				mask = mask >> 1;
				count--;
			}
			controller_write_msg (ctr,
				STATUS" host allow %u.%u.%u.%u/%u\n",
				(ph->ip_from >> 24),
				(0xff0000 & ph->ip_from) >> 16,
				(0xff00 & ph->ip_from) >> 8,
				ph->ip_from & 0xff, count);
			ph = ph->next;
		}
		controller_write_msg (ctr,
			STATUS"\n");
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
	if (!m_host_allow) m_host_allow = p;

	// yes we have
	else {
		// should we insert it as the first
		if (ip_from < m_host_allow->ip_from) {
			p->next = m_host_allow;
			m_host_allow = p;
		} else {
			// we have to go through the list
			host_allow_t* ph = m_host_allow;
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
	return (*ci ? host_allow(ctr, ci) : 0);
}


int CController::list_help (struct controller_type *ctr, char* ci)
{
	if (!ci) return 1;
	//if (!ctr || !ci) return 1;
	if (!strcasecmp(ci, "help ")) {
		controller_write_msg (ctr,
			"MSG: Commands:\n"
			"MSG:  maxplaces [(strings | fonts | texts | images | "
				"imageplaces | shapes | shapeplaces | "
				"video feeds | virtual feeds | audio feeds | "
				"audio mixers | audio sinks) <number>\n"
			"MSG:  message <message>\n"
			"MSG:  messagef <message>\n"
			"MSG:  monitor [on | off]\n"
			"MSG:  overlay finish [<command name>]\n"
			"MSG:  overlay pre [<command name>]\n"
			"MSG:  png write <file name>\n"
			"MSG:  select <feed no>\n"
			"MSG:  stack [ list | <feed no> ....]  // no arguments empties stack\n"
			"MSG:  stat\n"
			"MSG:  system frame rate [<frames/s>]\n"
		 	"MSG:  system geometry <width> <height> <fourcc>\n"
			"MSG:  system info\n"
			"MSG:  system host allow [(<network> | <host>)[(<network> | <host>)[ ...]]]\n"
			"MSG:  system name [<system name>]\n"
			"MSG:  system output buffers <buffer count>\n"
			"MSG:  system output delay <frames>\n"
			"MSG:  system output freeze <number frames to freeze>\n"
			"MSG:  system output info\n"
			"MSG:  system output mode <mode>\n"
			"MSG:  system output reset\n"
			"MSG:  system output status\n"
			"MSG:  system socket [<output socket name>]\n"
			"MSG:  verbose\n"
			"MSG:  help  // prints this list\n"
			"MSG:  audio feed help  // to see additional help on audio feed\n"
			"MSG:  audio mixer help  // to see additional help on audio feed\n"
			"MSG:  audio sink help  // to see additional help on audio feed\n"
			"MSG:  command help  // to see additional help on command)\n"
			"MSG:  feed help  // to see additional help on feed\n"
#if defined (HAVE_OSMESA) || defined (HAVE_GLX)
			"MSG:  glshape help  // to see additional help on glshape\n"
			"MSG:  glshape place help  // to see additional help on glshape\n"
#endif
			"MSG:  image help  // to see additional help on image\n"
			"MSG:  shape help  // (to see additional help on text)\n"
			"MSG:  shape place help  // (to see additional help on text)\n"
			"MSG:  text help  // (to see additional help on text)\n"
			"MSG:  tcl help  // (to see additional help on tcl)\n"
			"MSG:  virtual feed help   // to see additional help on virtual feed\n"
			"MSG:  quit   // Quit snowmix. This clean up shm alloacted in /run/shm/\n"
			"MSG:  \n"
			);
	} else return -1;
	return 0;
}

/* list monitor status */
int CController::list_monitor (struct controller_type *ctr)
{
	if (m_monitor_status) controller_write_msg (ctr, "Monitor is on.\n");
	else controller_write_msg (ctr, "Monitor is off.\n");
	return 0;
}

/* set monitor on */
int CController::set_monitor_on (CVideoMixer* pVideoMixer, struct controller_type *ctr)
{
	if (!pVideoMixer) {
		fprintf(stderr, "pVideoMixer was NULL for set_monitor_on\n");
		return -1;
	}
	if (!m_monitor_status) {
		if (pVideoMixer->m_video_type == VIDEO_NONE) {
			controller_write_msg (ctr, "Monitor can not be turned on as video type not defined.\n");
			return -1;
		}
		if (pVideoMixer->m_pMonitor) delete pVideoMixer->m_pMonitor;
		pVideoMixer->m_pMonitor = new CMonitor_SDL(pVideoMixer->GetSystemWidth(), pVideoMixer->GetSystemHeight(), pVideoMixer->m_video_type);
		if (!pVideoMixer->m_pMonitor) {
			controller_write_msg (ctr, "Failed to get monitor class. Monitor set to off\n");
			return -1;
		} else {
			m_monitor_status = true;
			controller_write_msg (ctr, "Monitor status set to on.\n");
		}
	} else {
		controller_write_msg (ctr, "Monitor status was already on. Turn it off to change.\n");
	}
	return 0;
}

/* set monitor off */
int CController::set_monitor_off (CVideoMixer* pVideoMixer, struct controller_type *ctr)
{
	if (m_monitor_status) {
		m_monitor_status = false;
		if (pVideoMixer->m_pMonitor) delete pVideoMixer->m_pMonitor;
		pVideoMixer->m_pMonitor = NULL;
		controller_write_msg (ctr, "Monitor status set to off.\n");
	} else {
		controller_write_msg (ctr, "Monitor was off. Turn it on to change.\n");
	}
	return 0;
}


/* Setup the output stack. */
int CController::output_set_stack (CVideoMixer* pVideoMixer, struct controller_type *ctr, char *str)
{
	int	new_stack [MAX_STACK];
	int	slot;
	int	x;
	int	id;
	char	stack_str [MAX_STACK * 5];

	if (!pVideoMixer) {
		fprintf(stderr, "pVideoMixer was NULL in output_select_stack\n");
		return -1;
	}
	if (!pVideoMixer->m_pVideoFeed) {
		fprintf(stderr, "m_pVideoFeed for pVideoMixer was NULL in "
			"output_select_stack\n");
		return -1;
	}

	/* Clear the buffer. */
	for (slot = 0; slot < MAX_STACK; slot++) {
		new_stack [slot] = -1;
	}
	strcpy (stack_str, "");

	/* Parse the input. */
	for (slot = 0; slot < MAX_STACK; slot++) {
		struct feed_type	*feed;

		/* Advance *str past any whitespace. */
		while (*str == ' ') str++;

		x = sscanf (str, "%d", &id);
		if (x != 1) break;

		/* Check if the feed is valid. */
		feed = pVideoMixer->m_pVideoFeed->FindFeed (id);
		if (feed == NULL) {
			controller_write_msg (ctr, "MSG: Unknown feed: %d\n", id);
			return -1;
		}
		if (slot == 0 && (feed->width != pVideoMixer->GetSystemWidth() || feed->height != pVideoMixer->GetSystemHeight())) {
			controller_write_msg (ctr, MESSAGE" The bottom layer is not a fullscreen feed\n");
			return -1;
		}
		new_stack [slot] = id;
		sprintf (stack_str + strlen (stack_str), " %d", id);

		/* Advance *str past the value. */
		while (*str >= '0' && *str <= '9') str++;
	}

	if (new_stack [0] == -1) {
		/* Empty stack. */
		controller_write_msg (ctr, "MSG: Empty stackup.\n");
		return -1;
	}

	memcpy (pVideoMixer->m_output_stack, new_stack, sizeof (pVideoMixer->m_output_stack));

	// Boadcast disabled /* Let everyone know that a new stack have been selected. */
	controller_write_msg (ctr, MESSAGE"Stack%s\n", stack_str);

	return 0;
}

/* Initialize the feed subsystem. */
int CController::feed_init_basic (CVideoMixer* pVideoMixer, const int width, const int height, const char *pixelformat)
{
	//int	w_mod;
	//int	h_mod;
	//VideoType video_type = VIDEO_NONE;

	if (!pVideoMixer) {
		fprintf(stderr, "pVideoMixer for feed_init_basic was NULL.\n");
		return -1;
	}

	if (pVideoMixer->SetGeometry(width, height, pixelformat)) {
		fprintf(stderr, "Failed to set geometry %ux%u for VideoMixer pixelformat "
			"<%s>\n", width, height, pixelformat);
		return -1;
	}


	/* Create the system feed. */
	m_pSystem_feed = (feed_type*) calloc (1, sizeof (*m_pSystem_feed));
	m_pSystem_feed->previous_state   = SETUP;
	m_pSystem_feed->state            = STALLED;		/* Is always marked stalled. */
	m_pSystem_feed->id               = 0;
	m_pSystem_feed->feed_name        = (char*) "Internal";
	m_pSystem_feed->oneshot          = 0;
	m_pSystem_feed->is_live          = 0;
	m_pSystem_feed->control_socket   = -1;	/* Does not have a control socket. */
	m_pSystem_feed->width            = width;
	m_pSystem_feed->height           = height;
	m_pSystem_feed->cut_start_column = 0;
	m_pSystem_feed->cut_start_row    = 0;
	m_pSystem_feed->cut_columns      = width;
	m_pSystem_feed->cut_rows         = height;
	m_pSystem_feed->shift_columns    = 0;
	m_pSystem_feed->shift_rows       = 0;
	m_pSystem_feed->par_w            = 1;
	m_pSystem_feed->par_h            = 1;
	m_pSystem_feed->scale_1          = 1;
	m_pSystem_feed->scale_2          = 1;

	// We will allocate the dead image, fill it with black and set alpha to 1
	m_pSystem_feed->dead_img_buffer  = (u_int8_t*) calloc(1,width*height*4); // We only supports BGRA
	if (m_pSystem_feed->dead_img_buffer) {
		u_int8_t* p = m_pSystem_feed->dead_img_buffer + 3;
		for (int32_t i = 0; i < width*height; i++) {
			*p = 255;
			p += 4;
		}
	} else fprintf(stderr, "Failed to allocate dead image frame for system feed 0\n");
			
		


	/* Add the m_pSystem_feed to the list of feeds. */
	if (!pVideoMixer->m_pVideoFeed) {
		fprintf(stderr, "pVideoFeed == NULL in feed_init_basic.\n");
		return -1;
	}
	struct feed_type* pFeedList = pVideoMixer->m_pVideoFeed->GetFeedList();
	if (pFeedList) {
		//fprintf(stderr, "Adding to feed list\n");
		list_add_tail (pFeedList, m_pSystem_feed);
	}
	else {
		//fprintf(stderr, "Adding to empty feed list\n");
		pVideoMixer->m_pVideoFeed->SetFeedList(m_pSystem_feed);
	}

	return 0;
}
