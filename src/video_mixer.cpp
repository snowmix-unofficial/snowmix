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
//#include <limits.h>
#include <string.h>
//#include <malloc.h>
#include <errno.h>
#include <sys/types.h>

#include <sys/stat.h>
#include <fcntl.h>

#ifdef WIN32
#include <winsock.h>
#include "windows_util.h"
#else
#include <sys/socket.h>
#include <sys/utsname.h>
#endif


#include <math.h>
#ifdef HAVE_OSMESA
# include "GL/osmesa.h"
# ifdef HAVE_MACOSX
#  include <OpenGL/OpenGL.h>
#  ifdef HAVE_GLUT
#   include <GLUT/glut.h>
#  endif
# else
#  ifdef HAVE_GLUT
#   include <GL/glut.h>
#  endif
# endif
//#include <GL/gl.h>
//#include <GL/glu.h>
//#include "GL/glu.h"
//#include <GL/glu_mangle.h>
//#include "gl_wrap.h"
#endif

#include "snowmix.h"
#include "video_mixer.h"
#include "video_text.h"
//#include "video_scaler.h"
#include "video_image.h"
//#include "add_by.h"
#include "command.h"
#include "snowmix_util.h"

#if defined (HAVE_OSMESA) || defined (HAVE_GLX)
#include "opengl_video.h"
#endif


CVideoMixer::CVideoMixer() {
	// Windows does not have utsname
#ifndef WIN32
	struct utsname sysname;
	if (!uname(&sysname)) {
		m_mixer_name = strdup(sysname.nodename);
//fprintf(stderr, "Node name %s\n", m_mixer_name);
	} else {
		perror("CVideoMixer failed to get node name\n");
		m_mixer_name = NULL;
	}
#else
	m_mixer_name = strdup("Windows");
#endif
	m_geometry_width	= 0;
	m_geometry_height	= 0;
	memset(m_pixel_format, 0, 5);
	m_video_type		= VIDEO_NONE;
	m_pixel_bytes		= 0;
	m_frame_rate		= 0;
	m_block_size		= 0;
	m_verbose		= 0;
	//m_system_monitor	= false;
	m_pMonitor		= NULL;
	//m_pVideoScaler	= NULL;
	m_pCairoGraphic		= NULL;
	m_pTextItems		= NULL;
	m_output_stack[0]	= 0;
	for (int i=1 ; i<MAX_STACK; i++ ) m_output_stack[i] = -1;
	m_pController		= NULL;
	gettimeofday(&m_start,NULL);
	m_start_master.tv_sec	= m_start_master.tv_usec = 0;
	m_time_last.tv_sec	= m_time_last.tv_usec = 0;
	m_mixer_duration.tv_sec	= m_mixer_duration.tv_usec = 0;
	m_output_called 	= 0;
	m_output_missed 	= 0;
	m_pVideoImage		= NULL;
	m_pVideoFeed		= NULL;
	m_pVideoShape		= NULL;
	m_pAudioFeed		= NULL;
	m_pAudioMixer		= NULL;
	m_pAudioSink		= NULL;
	m_pVirtualFeed		= NULL;
	m_overlay		= NULL;
	m_command_pre		= NULL;
	m_command_finish 	= NULL;
	m_write_png_file 	= NULL;
	m_pTclInterface 	= NULL;
	m_pVideoOutput		= NULL;
#if defined (HAVE_OSMESA) || defined (HAVE_GLX)
	m_pOpenglVideo		= new COpenglVideo(this);
#endif
	m_search_path_size 	= 8;
	if (!(m_search_path = (char**) calloc(m_search_path_size + 1, sizeof(char*)))) {
		fprintf(stderr, "Warning: Failed to allocate array for search path.\n");
		m_search_path_size = 0;
	}
}

CVideoMixer::~CVideoMixer() {

	if (m_pMonitor) delete m_pMonitor; m_pMonitor = NULL;
	//if (m_pVideoScaler) delete m_pVideoScaler; m_pVideoScaler = NULL;
#if defined (HAVE_OSMESA) || defined (HAVE_GLX)
	if (m_pOpenglVideo) delete m_pOpenglVideo; m_pOpenglVideo = NULL;
#endif
	if (m_pCairoGraphic) delete m_pCairoGraphic; m_pCairoGraphic = NULL;
	if (m_pTextItems) delete m_pTextItems; m_pTextItems = NULL;
	if (m_pAudioFeed) delete m_pAudioFeed; m_pAudioFeed = NULL;
	if (m_pVideoFeed) delete m_pVideoFeed; m_pVideoFeed = NULL;
	if (m_pVideoImage) delete m_pVideoImage; m_pVideoImage = NULL;
	if (m_pVirtualFeed) delete m_pVirtualFeed; m_pVirtualFeed = NULL;
	if (m_pVideoOutput) delete m_pVideoOutput; m_pVideoOutput = NULL;
	if (m_pController) delete m_pController; m_pController = NULL;
	if (m_mixer_name) free(m_mixer_name);
	if (m_command_pre) free(m_command_pre);
	if (m_command_finish) free(m_command_finish);
	if (m_search_path) {
		for (int i=0 ; i < m_search_path_size; i++) {
			if (m_search_path[i]) free(m_search_path[i]);
		}
	}
}

u_int32_t CVideoMixer::GetSystemWidth() {
	return m_geometry_width;
}
u_int32_t CVideoMixer::GetSystemHeight() {
	return m_geometry_height;
}
void CVideoMixer::SystemGeometry(u_int32_t *pWidth, u_int32_t *pHeight) {
	if (pWidth) *pWidth = m_geometry_width;
	if (pHeight) *pHeight = m_geometry_height;
}


bool CVideoMixer::AddSearchPath(char* path) {
	if (!m_search_path || !path || !(*path)) return true;
	for (int i=0 ; i < m_search_path_size; i++) {
		if (!m_search_path[i]) {
			m_search_path[i] = path;
			return false;
		}
	}
	// Failed to insert path.
	fprintf(stderr, "CvideoMixer search path maximum reached. Increase m_search_path in source code if you want a larger search path maximum.\n");
	return true;
}
bool CVideoMixer::RemoveSearchPath(int index) {
	if (!m_search_path || index >= m_search_path_size || !m_search_path[index]) return true;
	free(m_search_path[index]);
	m_search_path[index] = NULL;
	for (int i=index ; i < m_search_path_size-1; i++) {
		m_search_path[index] = m_search_path[index+1];
	}
	return false;
}

const char* CVideoMixer::GetSearchPath(int index) {
	if (!m_search_path || index >= m_search_path_size) return NULL;
	return m_search_path[index];
}


int CVideoMixer::OpenFile(const char* file_name, int mode, int perm, char** open_name) {
	if (!file_name || !(*file_name)) return -1;
	if (*file_name == '/') {
		if (m_verbose > 1) fprintf(stderr, "Image load opening absolut file name %s\n", file_name);
		if (perm > -1) return open(file_name, mode, perm);
		else return open(file_name, mode);
	}

	char *longname;
	const char *path;
	int fd = -1, index = 0;
	int len = strlen(file_name);
	while ((path = GetSearchPath(index++))) {
		longname = (char*) malloc(strlen(path)+1+len+1);
		if (!longname) {
			fprintf(stderr, "Warning. Failed to allocate memory for longname.\n");
			return -1;
		}
		sprintf(longname, "%s/%s", path, file_name);
		TrimFileName(longname);
if (m_verbose > 1) fprintf(stderr, "Trying to open file %s\n", longname);
		if ((fd = (perm < 0 ? open(longname, mode) : open(longname, mode, perm))) > -1) {
			if (open_name) *open_name = longname;
			else free(longname);
			return fd;
		}
		free (longname);
	}
	return fd;
}


// SetGeometry()
// Sets m_video_type, m_pixel_bytes, m_pixel_format, m_block_size
// WARNING. pixelformat must be nul terminated.
int CVideoMixer::SetGeometry(const u_int32_t width, const u_int32_t height, const char* pixelformat) {
	u_int32_t w_mod = 0;
	u_int32_t h_mod = 0;

	if (!width || !height || !pixelformat) return -1;
	m_pixel_bytes = 0;		// For safe keeping

	//pixelformat[4]= '\0'; Wee need to ensure this, as we print it if we have errors.
	if (pixelformat[4]) {
		fprintf(stderr, "pixelformat was not nulterminated on 5th position\n");
		return -1;
	}

	// Cycle through allowable formats
	if (strcmp (pixelformat, "ARGB") == 0) {
		m_pixel_bytes = 4;
		m_video_type = VIDEO_ARGB;
		w_mod = 1;
		h_mod = 1;
	} else if (strcmp (pixelformat, "BGRA") == 0) {
		m_pixel_bytes = 4;
		m_video_type = VIDEO_ARGB;
		w_mod = 1;
		h_mod = 1;
	} else if (strcmp (pixelformat, "YUY2") == 0) {
		m_pixel_bytes = 2;
		m_video_type = VIDEO_ARGB;
		w_mod = 2;
		h_mod = 1;
	} else if (strcmp (pixelformat, "I420") == 0) {
		fprintf(stderr, "Unsupported pixelformat <%s> as we can not "
			"yet handle 1.5 bytes per pixel\n", pixelformat);
		return -1;
	} else {
		fprintf(stderr, "Unsupported pixelformat <%s>\n", pixelformat);
		return -1;
	}

	// Check for valid widths and heights
	if ((width % w_mod) != 0) {
		fprintf(stderr, "Video width must be divisable by %u.\n", w_mod);
		m_pixel_bytes = 0;
		m_video_type = VIDEO_NONE;
		return -1;
	}
	if ((height % h_mod) != 0) {
		fprintf(stderr, "Video height must be divisable by %u.\n", w_mod);
		m_pixel_bytes = 0;
		m_video_type = VIDEO_NONE;
		return -1;
	}

//	if (m_pVideoScaler) delete m_pVideoScaler;
//	m_pVideoScaler = new CVideoScaler(width, height, m_video_type);
//	if (!m_pVideoScaler) {
//		fprintf(stderr, "Faild to get video scaler for video mixer.\n");
//		m_pixel_bytes = 0;
//		m_video_type = VIDEO_NONE;
//		return -1;
//	}
	strncpy (m_pixel_format, pixelformat, sizeof (m_pixel_format) - 1);
	m_block_size = m_pixel_bytes * width * height;
	m_geometry_width = width;
	m_geometry_height = height;
	return 0;
}


// system
// Query type 0=info 1=status 2=maxplaces 3=extended
// format	: format 
// data		: data
char* CVideoMixer::Query(const char* query, bool tcllist)
{
	int len;
	u_int32_t type;
	char *retstr = NULL;
	//const char *str;

	if (!query) return NULL;
	while (isspace(*query)) query++;

	if (!(strncasecmp("info ", query, 5))) {
		query += 5; type = 0;
	}
	else if (!(strncasecmp("status ", query, 7))) {
		query += 7; type = 1;
	}
//	else if (!(strncasecmp("extended ", query, 9))) {
//		query += 9; type = 4;
//	}
	else if (!(strncasecmp("maxplaces ", query, 10))) {
		query += 10; type = 2;
	}
	else if (!(strncasecmp("overlay ", query, 8))) {
		query += 8; type = 3;
	}
       	else if (!(strncasecmp("syntax", query, 6))) {
//		return strdup("system (info | status | extended | maxplaces | overlay | syntax) [ format ]");
		return strdup("system (info | status | maxplaces | overlay | syntax) [ format ]");
	} else return NULL;


	while (isspace(*query)) query++;
	if (!strncasecmp(query, "format", 6)) {
		char* str = NULL;
		if (type == 0) {
			// info - geometry pixel_format pixel_bytes frame_rate block_size host_allow version system_name
			if (tcllist) str = strdup("geometry pixel_format pixel_bytes frame_rate block_size host_allow version system_name");
			else str = strdup("system info <geometry> <pixel_format> <pixel_bytes> <frame_rate> <block_size> <host_allow> <version> <system_name>");

		}
		else if (type == 1) {
			// status
			//	frame_seq_no	: Current Frame Number
			//	client_fd	: Current Output Socket client fd. -1 if none is connected
			//	in_use		: Output buffers in use and total buffers in_use:total
			//	no_free_set	: 1 if no_free_buffer_flag is set, otherwise 0

			// status - frame_seq_no in_use out_ready
			// status - frame_seq_no monitor start start_master time_last mixer_duration output_called output_missed stack
			if (tcllist) str = strdup("frame_seq_no monitor start start_master time_last mixer_duration output_called output_missed stack");
			else str = strdup("system status <frame_seq_no> <monitor> <start> <start_master> <time_last> <mixer_duration> <output_called> <output_missed stack>");
		}
		else if (type == 2) {
			// maxplaces - strings fonts placed_text loaded_images placed_images shapes placed_shapes shape_patterns audio_feeds audio_mixers audio_sink video_feeds virtual_feeds
			if (tcllist) str = strdup("strings fonts placed_text loaded_images placed_images shapes placed_shapes shape_patterns audio_feeds audio_mixers audio_sink video_feeds virtual_feeds");
			else str = strdup("maxplaces - <strings> <fonts> <placed_text> <loaded_images> <placed_images> <shapes> <placed_shapes> <shape_patterns> <audio_feeds> <audio_mixers> <audio_sink> <video_feeds> <virtual_feeds>");
		}
		else if (type == 3) {
			// overlay - overlay_pre overlay_post
			if (tcllist) str = strdup("overlay_pre overlay_finish");
			else str = strdup("overlay <overlay_pre> <overlay_post>");
		}
		else if (type == 4) {

			// THIS PART IS NOT ENABLED. Needs reconfiguration for other variables . It is a copy from "status"
			// status - frame_seq_no monitor start start_master time_last mixer_duration output_called output_missed stack
			if (tcllist) str = strdup("frame_seq_no monitor start start_master time_last mixer_duration output_called output_missed stack");
			else str = strdup("system status <frame_seq_no> <monitor> <start> <start_master> <time_last> <mixer_duration> <output_called> <output_missed stack>");
		}
		if (!str) goto malloc_failed;
		return str;
	}
	else if (!(*query)) {
		if (type == 0) {
			// info - geometry pixel_format pixel_bytes frame_rate block_size host_allow version system_name
			// system info 1234567890x1234567890 BGRA 12 1234.1234567890 1234567890 { } { }
			//         12  +10       +2 +10       +5  +3 +16             +11        +3  +3  +1
			len = 90 + (m_mixer_name ? strlen(m_mixer_name) : 0);

			// Get host allow list and find its length
			unsigned int allowlistlen = 0;
			char* allow_list = m_pController ? m_pController->GetHostAllowList(&allowlistlen) : NULL;
			len += (allow_list ? allowlistlen : 12);	// If we have no allow_list, 127.0.0.1 is allowed
			// Check to se if we need to replace ' ' with ','
			if (allow_list && !(tcllist)) {
				char* tmpstr = allow_list;
				while (*tmpstr) {
					if (isspace(*tmpstr)) *tmpstr = ',';
					tmpstr++;
				}
			}

			// Allocate space for string
			retstr = (char*) malloc(len);
			if (!(retstr)) goto malloc_failed;
			*retstr = '\0';

			sprintf(retstr, (tcllist ?
				"{%u %u} %s %u %.9lf %u {%s} %s {%s}" : "system info %ux%u %s %u %.9lf %u %s %s <%s>\n"),
				m_geometry_width, m_geometry_height, m_pixel_format, m_pixel_bytes,
				m_frame_rate, m_block_size,
				allow_list ? allow_list : "127.0.0.1",
				SNOWMIX_VERSION,
				m_mixer_name ? m_mixer_name : "");
			if (allow_list) free(allow_list);
		}
		else if (type == 1) {
			// status - frame_seq_no monitor start start_master time_last mixer_duration output_called output_missed stack
			// system status 1234567890 0 1234567890.123 1234567890.123 1234567890 1234567890 1,2,3....
			// 12345678901234567890123456789012345678901234567890123456789012345678901234567890
			len = 80 + 4*MAX_STACK;

			// Allocate space for string
			retstr = (char*) malloc(len);
			if (!(retstr)) goto malloc_failed;
			*retstr = '\0';
			char* pstr = retstr;

#ifdef HAVE_MACOSX	// suseconds_t is int on OS X 10.9
#define TIMEFORMAT "%ld.%03d"
#else                               // suseconds_t is long int on Linux
#define TIMEFORMAT "%ld.%03lu"
#endif


			sprintf(pstr, "%s%u %u "TIMEFORMAT" "TIMEFORMAT" "TIMEFORMAT" "TIMEFORMAT" %u %u %s",
				(tcllist ? "" : "system status "),
				m_pController ? m_pController->m_frame_seq_no : 0,
				m_pController ? (m_pController->m_monitor_status ? 1 : 0) : 0,
				m_start.tv_sec, m_start.tv_usec/1000, 
				m_start_master.tv_sec, m_start_master.tv_usec/1000, 
				m_time_last.tv_sec, m_time_last.tv_usec/1000, 
				m_mixer_duration.tv_sec, m_mixer_duration.tv_usec/1000,
				m_output_called, m_output_missed,
				tcllist ? "{" : "");
			while (*pstr) pstr++;
			for (int x = 0; x < MAX_STACK; x++) {
                		if (m_output_stack[x] > -1) {
					sprintf(pstr, "%d%s", x, tcllist ? " " : ",");
					while (*pstr) pstr++;
				}
			}
			if (tcllist && *(pstr-1) == ' ') pstr--;
			if (!tcllist && *(pstr-1) == ',') pstr--;
			sprintf(pstr, "%s", tcllist ? "}" : "\n");

		}
		else if (type == 2) {
			retstr = (char*) malloc(10+13*11);
			if (!(retstr)) goto malloc_failed;
			// maxplaces - strings fonts placed_text loaded_images placed_images shapes placed_shapes shape_patterns audio_feeds audio_mixers audio_sink video_feeds virtual_feeds
			sprintf(retstr,
				"%s%u %u %u %u %u %u %u %u %u %u %u %u %u%s",
				tcllist ? "" : "maxplaces ",
				m_pTextItems ? m_pTextItems->MaxStrings() : 0,
				m_pTextItems ? m_pTextItems-> MaxFonts() : 0,
				m_pTextItems ? m_pTextItems-> MaxPlaces() : 0,
				m_pVideoImage ? m_pVideoImage->MaxImages() : 0,
				m_pVideoImage ? m_pVideoImage->MaxImagesPlace() : 0,
				m_pVideoShape ? m_pVideoShape->MaxShapes() : 0,
				m_pVideoShape ? m_pVideoShape->MaxPlaces() : 0,
				m_pVideoShape ? m_pVideoShape->MaxPatterns() : 0,
				m_pAudioFeed ? m_pAudioFeed->MaxFeeds() : 0,
				m_pAudioMixer ? m_pAudioMixer->MaxMixers() : 0,
				m_pAudioSink ? m_pAudioSink->MaxSinks() : 0,
				m_pVideoFeed ? m_pVideoFeed->MaxFeeds() : 0,
				m_pVirtualFeed ? m_pVirtualFeed->MaxFeeds() : 0,
				tcllist ? "" : "\n");
		}
		else if (type == 3) {
			// overlay_pre overlay_post
			// overlay <overlay_pre> <overlay_post>
			// overlay <> <>
			// 123456789012345
			int len = 15 + (m_command_pre ? strlen(m_command_pre) : 0) +
				(m_command_finish ? strlen(m_command_finish) : 0) ;
			retstr = (char*) malloc(len);
			if (!(retstr)) goto malloc_failed;
			sprintf(retstr, "%s%s%s%s %s%s%s",
				tcllist ? "" : "overlay ",
				tcllist ? "{" : "<",
				m_command_pre ? m_command_pre : "",
				tcllist ? "}" : ">",
				tcllist ? "{" : "<",
				m_command_finish ? m_command_finish : "",
				tcllist ? "}" : "\n");
		}
		else return NULL;
	}
	return retstr;

malloc_failed:
	fprintf(stderr, "CVirtualFeed::Query failed to allocate memory\n");
	return NULL;
}


// Run main poller until exit. Returns 1 upon error, otherwise 0.
int CVideoMixer::MainMixerLoop()
{
	struct timeval	next_frame = {0, 0};
	bool ini_file_read = false;

	if (!m_pController) {
		fprintf(stderr, "m_pController was NULL for MainMixerLoop\n");
		return 1;
	}
	if (!m_pVideoFeed) {
		fprintf(stderr, "m_pVideoFeed was NULL for MainMixerLoop\n");
		return 1;
	}
	if (!m_pVideoOutput) {
		fprintf(stderr, "m_pVideoOutput was NULL for MainMixerLoop\n");
		return 1;
	}

	/* Start doing some work. */
	while (!exit_program) {
		struct timeval	timeout;
		fd_set		read_fds;
		fd_set		write_fds;
		int		x;
		int		maxfd = 0;

		/* Setup for select. */
		FD_ZERO (&read_fds);
		FD_ZERO (&write_fds);

		maxfd = m_pVideoFeed->SetFds(maxfd, &read_fds, &write_fds);
		maxfd = m_pVideoOutput->SetFds(maxfd, &read_fds, &write_fds);
		maxfd = m_pController->SetFds (maxfd, &read_fds, &write_fds);

		/* Calculate the timeout value. */
		{
			struct timeval now;

			gettimeofday (&now, NULL);
			timersub (&next_frame, &now, &timeout);

			if (timeout.tv_sec < 0 || timeout.tv_usec < 0) {
				timeout.tv_sec  = 0;	/* No negative values here. */
				timeout.tv_usec = (typeof(timeout.tv_usec))(m_frame_rate > 0.0 ?
					1.0/m_frame_rate : 50000);
			}
		}

		x = select (maxfd + 1, &read_fds, &write_fds, NULL, &timeout);
		if (x < 0) {
			if (errno == EINTR) {
				continue;	/* Just try again. */
			}
			perror ("Select error");
			return 1;
		}

		if (x > 0) {
			/* Invoke the service functions. */
			m_pVideoFeed->CheckRead(&read_fds, m_block_size);
			m_pVideoOutput->CheckRead(&read_fds);
			m_pController->CheckReadWrite(this, &read_fds, &write_fds);
		}

		if (m_frame_rate > 0) {
			struct timeval now;
			gettimeofday (&now, NULL);
			if (timercmp (&now, &next_frame, >)) {
				/* Time to produce some more output. */
				struct timeval	delta, new_time;

				if (m_pController->m_ini_file_read) {
					if (ini_file_read != m_pController->m_ini_file_read) {
						fprintf(stderr, "Initial ini file read\n");
						ini_file_read = m_pController->m_ini_file_read;
					}
					output_producer (m_pController->m_pSystem_feed, now);
				}

				/* Check if the frame time is too much behind schedule. */
				delta.tv_sec  = 0;
				delta.tv_usec = 500000;		// Half a second
				new_time = next_frame;

				timeradd (&next_frame, &delta, &new_time);
				if (timercmp (&now, &new_time, >)) {
					struct timeval delta_time;
					timersub(&now, &new_time, &delta_time);
					/* Ups Lagging - Reset the next_frame time to now. */
					next_frame = now;

					if (0 && m_pController->m_frame_seq_no) {
						fprintf(stderr, "Frame lagging frame %u : %ld ms\n",
							m_pController->m_frame_seq_no,
							(long)(1000*delta_time.tv_sec+delta_time.tv_usec/1000));
						m_pController->m_lagged_frames++;
					}
				}

				/* Calculate the next frame time. */
				delta.tv_sec  = 0;
				delta.tv_usec = (typeof(delta.tv_usec))(1000000 / m_frame_rate);
				timeradd (&next_frame, &delta, &next_frame);

				/* Maintain the feeds in sync with the output. */
				m_pVideoFeed->Timertick (m_block_size, m_pVideoFeed->GetFeedList());
			}
		}
	}

	return 0;
}

// Run main poller until exit. Returns 1 upon error, otherwise 0.
int CVideoMixer::MainMixerLoop2()
{
	//struct timeval	next_frame = {0, 0};
	struct timeval time_now, time_frame, next_time, timeout;
	bool ini_file_read = false;
	bool late = false;

fprintf(stderr, "MainMixerLoop 2\n");

	if (!m_pController) {
		fprintf(stderr, "m_pController was NULL for MainMixerLoop\n");
		return 1;
	}
	if (!m_pVideoFeed) {
		fprintf(stderr, "m_pVideoFeed was NULL for MainMixerLoop\n");
		return 1;
	}
	if (!m_pVideoOutput) {
		fprintf(stderr, "m_pVideoOutput was NULL for MainMixerLoop\n");
		return 1;
	}

	// We don't know the frame rate so we set the period to 10ms for now
	time_frame.tv_sec = 0;
	time_frame.tv_usec = 10000;		// 100fps

	// and initially the timeout is the initial frame period
	timeout = time_frame;

	// Now we need to set the time for the next frame
	gettimeofday(&time_now, NULL);
	timeradd(&time_now, &time_frame, &next_time);

	bool check_io = true;

	// Main loop
	while (!exit_program) {

		// Setup timeout and call select
		gettimeofday(&time_now, NULL);

		// We need to service I/O at least once per main loop run
		if (check_io || timercmp(&next_time, &time_now, >)) {
			fd_set		read_fds;
			fd_set		write_fds;
			int		maxfd = 0;
			int		n;

			if (timercmp(&next_time, &time_now, >)) {
				// calculate timeout
				timersub(&next_time, &time_now, &timeout);
if (timeout.tv_sec < 0 || timeout.tv_usec < 0)
fprintf(stderr, "Frame %u : negative time\n", m_pController->m_frame_seq_no);
			} else {
				// we are late, but needs to call select at least once
				timeout.tv_sec = 0;
				timeout.tv_usec = 2000;
				struct timeval time_late;
				timersub(&time_now, &next_time, &time_late);
				if (timercmp(&time_late, &time_frame, >)) late = true;
fprintf(stderr, "Frame %u : Late I/O check. Skipping frame : %s\n", m_pController->m_frame_seq_no, late ? "yes" : "no");
			}

			// Setup for select for video feed control pipe, output control pipe and the controller connections
			FD_ZERO (&read_fds);
			FD_ZERO (&write_fds);
			maxfd = m_pVideoFeed->SetFds(maxfd, &read_fds, &write_fds);
			maxfd = m_pVideoOutput->SetFds(maxfd, &read_fds, &write_fds);
			maxfd = m_pController->SetFds (maxfd, &read_fds, &write_fds);

			// Now call select with timeout and wait for it to return to perhaps service I/O
			n = select (maxfd + 1, &read_fds, &write_fds, NULL, &timeout);		// timeout may be changed

			// We mark that we have called select
			check_io = false;

			// now we need to check the result of select
			if (n < 0) {
				if (errno == EINTR) continue; 	// Just try again.
				perror ("Select error");
				return 1;
			} else if (n > 0) {
				// Select indicated I/O to service. Invoke the service functions.
				m_pVideoFeed->CheckRead(&read_fds, m_block_size);
				m_pVideoOutput->CheckRead(&read_fds);
				m_pController->CheckReadWrite (this, &read_fds, &write_fds);
			}
			// Check to see if we should loop or continue down to generate a frame
			gettimeofday(&time_now, NULL);
			if (timercmp(&next_time, &time_now, >)) continue;
		}

		// Now we check if the initial ini file was read as we will
		// not start generate frames before it has been read completely
		if (m_pController->m_ini_file_read) {

			// Ini file has been read. Is this the first time?
			if (ini_file_read != m_pController->m_ini_file_read) {
				// This is the first time we are here.
				fprintf(stderr, "Initial ini file read\n");
				ini_file_read = m_pController->m_ini_file_read;

				// Now we need to set the frame period
				if (m_frame_rate <= 1 || m_frame_rate > 999 ) {		// max 999 fps
					fprintf(stderr, "Frame rate at %.3lf is to high or negative. Stopping.\n", m_frame_rate);
					exit(1);
				}
				time_frame.tv_sec = (typeof(time_frame.tv_sec))(1.0 /  m_frame_rate);
				time_frame.tv_usec = (typeof(time_frame.tv_usec))(1000000.0*((1.0/m_frame_rate)-time_frame.tv_sec));
			}
	
			// Now call output mixer
			output_producer (m_pController->m_pSystem_feed, time_now, late);
		}

		// Update feeds
		m_pVideoFeed->Timertick (m_block_size, m_pVideoFeed->GetFeedList());

		// Set time for next frame
		timeradd(&next_time, &time_frame, &next_time);

		check_io = true;
		late = false;
	}

	return 0;
}

/* Produce output data. */
/* This function is invoked at the frame rate. */
int CVideoMixer::output_producer (struct feed_type* system_feed, struct timeval time_now, bool late)
{
	//struct feed_type		*feeds [MAX_STACK];
	int				feed_list[MAX_STACK];
	int				feed_no;
	struct output_memory_list_type	*buf;
	int				x;

	u_int32_t			width = 0;
	u_int32_t			height = 0;

	m_output_called++;
	struct timeval time;
	//gettimeofday(&time_now, NULL);
	if (m_output_called == 1) {
		m_start_master = time_now;
		m_time_last = m_start_master;
		m_output_missed = 0;
	} else {
		timersub(&time_now,&m_start_master,&time);
		//double since_start = time2double(time);
		timersub(&time_now,&m_time_last,&time);
		double since_last = time2double(time);
		double frame_period = 1.0 / m_frame_rate;
		//double mixer_duration = time2double(m_mixer_duration);
		if (since_last/frame_period >= 2.0) {
			fprintf(stderr, "FRAME %u:%u Period Delays %.1lf frame(s)\n",
				m_pController->m_frame_seq_no,
				m_output_called,
				since_last/frame_period);
		}

	}
	m_time_last.tv_sec = time_now.tv_sec;
	m_time_last.tv_usec = time_now.tv_usec;

	u_int32_t repeated = m_pVideoOutput->FramesRepeated(true);
if (repeated) fprintf(stderr, "Frame %u - Frame repeated = %u\n", m_pController->m_frame_seq_no, repeated);
	for (unsigned int i = 0; i <= repeated; i++) {
		if (m_pController && m_command_pre)
			m_pController->ParseCommandByName(this, NULL, m_command_pre);
		if (m_pTextItems) m_pTextItems->UpdateMoveAll();
	}
	if (m_pAudioFeed) m_pAudioFeed->Update();
	if (m_pAudioMixer) m_pAudioMixer->Update();
	//if (m_pAudioSink) m_pAudioSink->Update();
	if (m_pAudioSink) m_pAudioSink->NewUpdate(&time_now);

	// Run timed commands, if any
	if (m_pController && m_pController->m_pCommand) {
		m_pController->m_pCommand->RunTimedCommands(time_now);
	}

	if (!m_pVideoOutput || m_pVideoOutput->OutputClientSocket() < 0) {
		return 0;		/* No receiver/consumer yet. Do nothing. */
	}

	// Now if we are late or to many buffers ready for output we return
	if (late || m_pVideoOutput->ReadBuffersAhead() > 1) return 0;

	/* Check if any members of the output stack have disappeared. */
	feed_no = 0;
	for (x = 0; x < MAX_STACK; x++) {
		if (m_output_stack [x] < 0) break;
		if (!m_pVideoFeed) {
			fprintf(stderr, "pVideoFeed for output producer was NULL\n");
			return -1;
		}
//		feeds [feed_no] = m_pVideoFeed->FindFeed (m_output_stack [x]);
		feed_list[feed_no] = m_output_stack [x];
		if (m_pVideoFeed->FeedExist(m_output_stack [x])) feed_no++;

//		if (feeds [feed_no] != NULL) {
//			feed_no++;
//		}
	}

	// Check if the first feed exist and if it is fullscreen feed.
	// If it isn't, then display the system feed instead
	if (feed_no > 0) {
		if (!m_pVideoFeed->FeedGeometry(feed_list[0], &width, &height) ||
			width != m_geometry_width || height != m_geometry_height) {
			feed_no = 0;
			feed_list[feed_no++] = 0;	// feed 0 is the system feed.
		}
	}
//	if (feed_no > 0 && (feeds [0]->width != m_geometry_width || feeds [0]->height != m_geometry_height)) {
//		/* The first feed is NOT a full-screen feed. Just display the system feed. */
//		feed_no = 0;
//		feeds [feed_no++] = system_feed;
//	}

	// Check to see if there are feed to display, if not, then display system feed
	if (feed_no == 0) {
		/* All selected feeds have disappeared. */
//		feeds [feed_no++] = system_feed;
		feed_list[feed_no++] = 0;
	}

	// if the first feed does not exist, then system is not initialized yet
	if (!m_pVideoFeed->FeedExist(feed_list[0])) return -1;

//	if (feeds [0] == NULL) {
//		/* No feed available. The system is not yet initialized. Nothing to deliver. */
//		return -1;
//	}

	// Not able to do it without copying the frame.  This is due to a bug in
	// gst-plugins/bad/sys/shm/shmpipe.c line 931 where self->shm_area->id
	// should have been shm_area->id. In other words it will always acknowledge
	// buffers using the newest area_id recorded.
	buf = m_pVideoOutput ? m_pVideoOutput->OutputGetFreeBlock() : NULL;
	if (buf == NULL) {
		/* Out of memory. Not able to deliver a frame. Skip the timeslot. */
		return -1;
	}

	// Set Overlay (active video frame) from GStreamer shmsrc
	if (!(m_overlay = m_pVideoOutput ? m_pVideoOutput->FrameAddress(buf) : NULL)) {
		// No overlay
		return -1;
	}

	// The first frame in the stack must be a fullscreen feed. We will copy that
	// to be the bottom layer of the output frame
	u_int8_t* src_buf =  m_pVideoFeed->GetFeedFrame(feed_list[0]);
	if (src_buf) {
		memcpy (m_overlay, src_buf, m_block_size);
//fprintf(stderr, "Memcpy\n");
		u_int8_t* p = src_buf+3;
		for (u_int32_t i=0; i < m_geometry_height ; i++) {
		  for (u_int32_t j=0; j < m_geometry_width ; j++) {
		    if (*p != 255) {
		      fprintf(stderr, "Baseframe alpha not 1 at %u,%u = %hhu\n", j, i, *p);
		      i = m_geometry_height;
		      break;
		    }
                    p += 4;
		  }
		}

	}
	else {
		memset (m_overlay, 0x00, m_block_size);
	}

	// Create the Cairo video surface using the current output frame
	if (!(m_pCairoGraphic = new CCairoGraphic(m_geometry_width, m_geometry_height,
		m_overlay))) {
		// Failed to create the Cairo context. Out of memory. We fail silently
		return -1;
	}

	// Overlay each feed onto frame. We have already copied the first feed in the list as the bottom layer
	for (x = 1; x < feed_no; x++) {
		BasicFeedOverlay(feed_list[x]);
	}
	
	if (m_pController) m_pController->m_frame_seq_no++;
/*
	if (m_pController->m_frame_seq_no > 0 && m_pController->m_frame_seq_no % 6 == 0) {
		struct timeval time_now, time;
		gettimeofday(&time_now, NULL);
		timersub(&time_now, &m_start_master, &time);
		int32_t frames = time.tv_sec*m_frame_rate + time.tv_usec*m_frame_rate/1000000 + 1;
		fprintf(stderr, "Frames at %5.2f : %s %d (calc %d produced %u\n",
			time.tv_sec + time.tv_usec/1000000.0,
			frames != m_pController->m_frame_seq_no ? "missing" : "ok",
			frames - m_pController->m_frame_seq_no, frames, m_pController->m_frame_seq_no);
		
	}
*/
	if (m_pTextItems || m_pVideoImage || m_pVideoShape) {

		if (m_pCairoGraphic) {
			if (m_pController && m_command_finish)
				m_pController->ParseCommandByName(this, NULL, m_command_finish);

		}

		if (m_pVideoShape) m_pVideoShape->Update();
		if (m_write_png_file && m_pCairoGraphic) {
			m_pCairoGraphic->WriteFilePNG(m_pCairoGraphic->m_pSurface,
				m_write_png_file);
			free(m_write_png_file);
			m_write_png_file = NULL;
		}
	}

	// We are finish rendering and can delete the Cairo context
	if (m_pCairoGraphic) {
		delete m_pCairoGraphic;
		m_pCairoGraphic = NULL;
	}

	// Check if we should output video to monitor output
	if (m_pController && m_pController->m_monitor_status && m_pMonitor) {
		m_pMonitor->ShowDisplay(m_overlay, 0);
	}


#if defined (HAVE_OSMESA) || defined (HAVE_GLX)
	if (m_pOpenglVideo) {
#ifdef HAVE_GLX
		m_pOpenglVideo->SwapBuffers(m_overlay);
#endif	// HAVE_GLX
		m_pOpenglVideo->InvalidateCurrent();
	}
#endif	// #if defined (HAVE_OSMESA) || defined (HAVE_GLX)

	// Set pointer to active video to NULL to prevent other part from accessing it
	m_overlay = NULL;

	// send rendered buffer overlay to Output
	if (m_pVideoOutput) {
		if (m_pVideoOutput->OutputBufferReady4Output(buf))
			fprintf(stderr, "Failed to send ready buffer overlay to Output\n");
	} else {
		fprintf(stderr, "No Output to send rendered overlay to.\n");
	}
	struct timeval timenow;
	gettimeofday(&timenow, NULL);
	timersub(&timenow,&m_time_last,&m_mixer_duration);

	return 0;
}

int CVideoMixer::BasicFeedOverlay(u_int32_t feed_no)
{
	u_int32_t width, height;			// Geometry of feed
	u_int32_t shift_columns, shift_rows;		// Offset on mixed frame
	u_int32_t cut_start_column, cut_start_row;	// Offset in feed
	u_int32_t cut_columns, cut_rows;		// width and height of clip of feed
	u_int32_t par_w, par_h;				// Pixel aspect ratio for feed
	u_int32_t scale_1, scale_2;			// Scale of feed
	u_int8_t* src_buf;				// Pointer to feed video frame
	double rotation = 0.0;				// Rotation
	double alpha;					// Alpha value
	cairo_filter_t filter;				// Cairo filter for Overlay

	//u_int32_t offset_x, offset_y;
	u_int32_t clip_height, clip_width;
	//u_int32_t clip_offset_x, clip_offset_y;
	double scale_x, scale_y;
	scale_x = scale_y = 1.0;

	// Check that we have the CVideoFeed class initialized.
	if (!m_pVideoFeed || !m_pCairoGraphic) return -1;

	// Get the feed video frame
	src_buf = m_pVideoFeed->GetFeedFrame(feed_no, &width, &height);
	if (!src_buf || !width || !height) return -1;

	// Get Feed details
	m_pVideoFeed->FeedGeometryDetails(feed_no, &shift_columns, &shift_rows,
		&cut_start_column, &cut_start_row, &cut_columns, &cut_rows,
		&par_w, &par_h, &scale_1, &scale_2, &alpha, &filter);

	// Check that we have valid scale values
	if (scale_1 < 1 || scale_2 < 1) return -1;

	// Check that we have valid PAR values
	if (par_w < 1 || par_h < 1) return -1;

	// See if we need to scale
	if (scale_1 != scale_2) {
		scale_x = scale_y = ((double) scale_1) / ((double) scale_2);
	}

	// See if we non square pixels
	if (par_w != par_h) {
		scale_x = scale_x * par_w / par_h;
	}

	// Calculate clip width and height
	if (cut_columns + cut_start_column > width) clip_width = width - cut_start_column;
	else clip_width = cut_columns;
	if (cut_rows + cut_start_row > height) clip_height = height - cut_start_row;
	else clip_height = cut_rows;

	/*
	fprintf(stderr, "BasicFeedOverlay feed %u wxh %ux%u, offset %u,%u, clip %ux%u, "
		"clipoff %u,%u, scale %.3lf,%.3lf\n", feed_no, width, height,
		shift_columns, shift_rows, clip_width, clip_height,
		cut_start_column, cut_start_row, scale_x, scale_y);
	*/

	m_pCairoGraphic->OverlayFrame(src_buf,
		width, height,				// Geometry feed
		shift_columns, shift_rows,		// Offset on mixer frame
		0, 0,					// Offset for source
		clip_width, clip_height,		// Feed clip geometry
		cut_start_column, cut_start_row,	// Offset in clip
		(u_int32_t) SNOWMIX_ALIGN_TOP | SNOWMIX_ALIGN_LEFT, // Alignment
		scale_x, scale_y, rotation, alpha, filter);

	return 0;
}

// overlay image (<id> | <id>..<id> | all | end | <id>..end) [ (<id> | <id>..<id> | all | end | <id>..end) ] ....
int CVideoMixer::OverlayImage(const char *str)
{
	if (!str || !m_overlay || !m_pVideoImage) return 1;
	while (isspace(*str)) str++;
	if (!(*str)) return 1;
	int from, to;
	from = to = 0;
	while (m_pVideoImage && *str) {
		if (!strncmp(str,"all",3)) {
			from = 0;
			to = m_pVideoImage->MaxImages();
			while (*str) str++;
		} else if (!strncmp(str,"end",3)) {
			to = m_pVideoImage->MaxImages();
			while (*str) str++;
		} else {
			int n=sscanf(str, "%u", &from);
			if (n != 1) return 1;
			while (isdigit(*str)) str++;
			n=sscanf(str, "..%u", &to);
			if (n != 1) to = from;
			else {
				str += 2;
			}
			while (isdigit(*str)) str++;
			while (isspace(*str)) str++;
		}
		if (from > to) return 1;
		while (from <= to) {
			// PMM
			if (m_pVideoImage && m_pVideoImage->GetImagePlaced(from)) {
				if (m_pCairoGraphic) {
					m_pCairoGraphic->OverlayImage(this, from);
				}
				//m_pVideoImage->OverlayImage(from, m_overlay,
					//m_geometry_width, m_geometry_height);
			}
			from++;
		}
	}
	return 0;
}

// overlay text (<id> | <id>..<id> | all | end | <id>..end) [ (<id> | <id>..<id> | all | end | <id>..end) ] ....
int CVideoMixer::OverlayText(const char *str, CCairoGraphic* pCairoGraphic)
{
	if (!str || !m_overlay) return 1;
	while (isspace(*str)) str++;
	while (*str) {
		int from, to;
		from=0;
		if (!strncmp(str,"all",3)) {
			from = 0;
			to = m_pTextItems->MaxPlaces();
			while (*str) str++;
		} else {
			int n=sscanf(str, "%u", &from);
			if (n != 1) return 1;
			while (isdigit(*str)) str++;
			n=sscanf(str, "..%u", &to);
			if (n != 1) {
				if (!strncmp(str,"..end",5)) {
					to = m_pTextItems->MaxPlaces();
					while (*str) str++;
				} else to = from;
			} else {
				str += 2;
			}
			while (isdigit(*str)) str++;
			while (isspace(*str)) str++;
		}
		if (from > to) return 1;
		while (from <= to) {
			pCairoGraphic->OverlayText(this, from);
			//text_item_t* pListItem = m_pTextItems->GetTextPlace(from);
			//	if (pListItem) pCairoGraphic->OverlayText(this, pListItem);
			from++;
		}
	}
	return 0;
}

// overlay virtual feed feed_no
// overlay virtual feed (<id> | <id>..<id> | all | end | <id>..end) [ (<id> | <id>..<id> | all | end | <id>..end) ] ....
/*
int CVideoMixer::OverlayVirtualFeed(CCairoGraphic* pCairoGraphic, const char* str)
{
	if (!str || !m_overlay) return 1;
	while (isspace(*str)) str++;
	if (!(*str)) return 1;
	//int from, to;
	//from = to = 0;
	u_int32_t maxid = m_pVirtualFeed->MaxFeeds();
	while (*str) {
		u_int32_t from, to;
		const char* nextstr = GetIdRange(str, &from, &to, 0, maxid ? maxid-1 : 0);
		if (!nextstr) break;
		while (isspace(*nextstr)) nextstr++;
		str = nextstr;
*/
/*
		if (!strncmp(str,"all",3)) {
			from = 0;
			to = m_pVirtualFeed->MaxFeeds();
			while (*str) str++;
		} else if (!strncmp(str,"end",3)) {
			to = m_pVirtualFeed->MaxFeeds();
			while (*str) str++;
		} else {
			int n=sscanf(str, "%u", &from);
			if (n != 1) return 1;
			while (isdigit(*str)) str++;
			n=sscanf(str, "..%u", &to);
			if (n != 1) to = from;
			else {
				str += 2;
			}
			while (isdigit(*str)) str++;
			while (isspace(*str)) str++;
		}
*/
/*
		if (from > to) return 1;
		while (from <= to) {
			int32_t x, y, offset_x, offset_y;
			u_int32_t vir_feed_no, w, h, src_x, src_y, width, height;
			double scale_x, scale_y, rotation, alpha;
			cairo_filter_t filter = CAIRO_FILTER_FAST;
			vir_feed_no = from;
			int i = m_pVirtualFeed->GetFeedCoordinates(vir_feed_no, &x, &y);
			int j = m_pVirtualFeed->GetFeedClip(vir_feed_no, &src_x, &src_y, &w, &h);
			int k = m_pVirtualFeed->GetFeedScale(vir_feed_no, &scale_x, &scale_y);
			int l = m_pVirtualFeed->GetFeedRotation(vir_feed_no, &rotation);
			int m = m_pVirtualFeed->GetFeedAlpha(vir_feed_no, &alpha);
			int n = m_pVirtualFeed->GetFeedGeometry(vir_feed_no, &width, &height);
			int p = m_pVirtualFeed->GetFeedFilter(vir_feed_no, &filter);
			int q = m_pVirtualFeed->GetFeedOffset(vir_feed_no, &offset_x, &offset_y);
			transform_matrix_t* pMatrix = m_pVirtualFeed->GetFeedMatrix(vir_feed_no);

			if (i || j || k || l || m || n || p || q) return 1;

			virtual_feed_enum_t feed_type = m_pVirtualFeed->GetFeedSourceType(vir_feed_no);
			u_int8_t* src_buf = NULL;
			int real_feed_no = m_pVirtualFeed->GetFeedSourceId(vir_feed_no);
			if (feed_type == VIRTUAL_FEED_FEED) {
				if (!m_pVideoFeed) {
					fprintf(stderr, "pVideoFeed was NULL for OverlayVirtualFeed\n");
					return 1;
				}
				src_buf = m_pVideoFeed->GetFeedFrame(real_feed_no);
			} else if (feed_type == VIRTUAL_FEED_IMAGE) {
				if (!m_pVideoImage) {
					fprintf(stderr, "pVideoImage was NULL for OverlayVirtualFeed\n");
					return 1;
				}
				image_item_t* pImage = m_pVideoImage->GetImageItem(real_feed_no);
				if (pImage) src_buf = pImage->pImageData;
			}
			if (!src_buf) return 1;
//			if (pFeed->fifo_depth <= 0) {
//		       		// Nothing in the fifo. Show the idle image.
//		       		src_buf = pFeed->dead_img_buffer;
//			} else {
//		       		// Pick the first one from the fifo.
//		       		src_buf = pFeed->frame_fifo [0].buffer->area_list->buffer_base +
//			       		pFeed->frame_fifo [0].buffer->offset;
//			}
			if (pCairoGraphic) {
//fprintf(stderr, "Overlay virtual feed %u - %u\n", vir_feed_no, real_feed_no);
		 		pCairoGraphic->OverlayFrame(src_buf, width, height,
		       			x, y, 0, 0, w, h, src_x, src_y, (u_int32_t) 0, scale_x , scale_y, rotation, alpha, filter, pMatrix);
		 		m_pVirtualFeed->UpdateMove(vir_feed_no);
			}
//			pFeed->dequeued = 1;	// Set the dequeued flag on the feed.
			from++;
		}
	}
	return 0;
}
*/


// cairooverlay feed feed_no x y w h src_x src_y scale_x scale_y rotate alpha
int CVideoMixer::CairoOverlayFeed(CCairoGraphic* pCairoGraphic, char* str)
{
	if (!str || !pCairoGraphic) return 1;
	while (isspace(*str)) str++;
	u_int32_t feed_no, x, y, w, h, src_x, src_y, width, height;
	double scale_x, scale_y, rotate, alpha;
	int n = sscanf(str, "%u %u %u %u %u %u %u %lf %lf %lf %lf", &feed_no, &x, &y, &w, &h,
		&src_x, &src_y, &scale_x, &scale_y, &rotate, &alpha);
	if (!m_pVideoFeed) {
		fprintf(stderr, "pVideoFeed was NULL for CairoOverlayFeed\n");
		return 1;
	}
//	struct feed_type* pFeed = m_pVideoFeed->FindFeed (feed_no);
	u_int8_t* src_buf = m_pVideoFeed->GetFeedFrame(feed_no, &width, &height);
	if (src_buf) {
//		u_int8_t* src_buf = NULL;
//		if (pFeed->fifo_depth <= 0) {
//			/* Nothing in the fifo. Show the idle image. */
//			src_buf = pFeed->dead_img_buffer;
//		} else {
//			/* Pick the first one from the fifo. */
//			src_buf = pFeed->frame_fifo [0].buffer->area_list->buffer_base +
//				pFeed->frame_fifo [0].buffer->offset;
//		}
		
		if (pCairoGraphic) {

			if (n == 11) pCairoGraphic->OverlayFrame(src_buf, width, height,
				x, y, 0, 0, w, h, src_x, src_y, (u_int32_t) 0, scale_x , scale_y, rotate, alpha);
			else if (n == 9) pCairoGraphic->OverlayFrame(src_buf, width, height,
				x, y, 0, 0, w, h, src_x, src_y, (u_int32_t) 0, scale_x , scale_y);
			else if (n == 7) pCairoGraphic->OverlayFrame(src_buf, width, height,
				x, y, 0, 0, w, h, src_x, src_y);
			else return 1;

		} else return 1;
	} else return 1;
	return 0;
}

// overlay feed <feed id> <col> <row> <feed col> <feed row> <cut cols> <cut rows> [<scale 1> <scale 2> [<par_w> <par_h> [ c ]]]
int CVideoMixer::OverlayFeed(const char *str)
{
	u_int32_t width, height;			// Geometry of feed
	u_int32_t shift_columns, shift_rows;		// Offset on mixed frame
	u_int32_t cut_start_column, cut_start_row;	// Offset in feed
	u_int32_t cut_columns, cut_rows;		// width and height of clip of feed
	u_int32_t par_w, par_h;				// Pixel aspect ratio for feed
	u_int32_t scale_1, scale_2;			// Scale of feed
	u_int32_t feed_no;				// Feed id
	u_int8_t* src_buf;				// Pointer to feed video frame
	u_int32_t clip_height, clip_width;
	double scale_x, scale_y;
	scale_x = scale_y = 1.0;
	if (!str || !m_overlay) return 1;
	while (isspace(*str)) str++;
	char c = 'c';
	int n = sscanf(str, "%u %u %u %u %u %u %u %u %u %u %u %c", &feed_no,
		&shift_columns, &shift_rows, &cut_start_column,
		&cut_start_row, &cut_columns, &cut_rows, &scale_1, &scale_2,
		&par_w, &par_h, &c);
	if (n == 7) {
		scale_1 = 1;
		scale_2 = 1;
		par_w = 1;
		par_h = 1;
	} else if (n == 9) {
		par_w = 1;
		par_h = 1;
	} else if ((n != 11 && n != 12)) return -1;

	// Check we have the CVideoFeed class
	if (!m_pVideoFeed) {
		fprintf(stderr, "pVideoFeed was NULL for OverlayFeed\n");
		return -1;
	}

	// Get the feed video frame
	src_buf = m_pVideoFeed->GetFeedFrame(feed_no, &width, &height);
	if (!src_buf || !width || !height) return -1;

	// Check that we have valid scale values
	if (scale_1 < 1 || scale_2 < 1) return -1;

	// Check that we have valid PAR values
	if (par_w < 1 || par_h < 1) return -1;

	// See if we need to scale
	if (scale_1 != scale_2) {
		scale_x = scale_y = ((double) scale_1) / ((double) scale_2);
	}

	// See if we non square pixels
	if (par_w != par_h) {
		scale_x = scale_x * par_w / par_h;
	}

	// Calculate clip width and height
	if (cut_columns + cut_start_column > width) clip_width = width - cut_start_column;
	else clip_width = cut_columns;
	if (cut_rows + cut_start_row > height) clip_height = height - cut_start_row;
	else clip_height = cut_rows;

	m_pCairoGraphic->OverlayFrame(src_buf,
		width, height,				// Geometry feed
		0, 0,					// Offset of source
		shift_columns, shift_rows,		// Offset on mixer frame
		clip_width, clip_height,		// Feed clip geometry
		cut_start_column, cut_start_row,	// Offset in clip
		(u_int32_t)0, 				// Align
		scale_x, scale_y);

/*
	struct feed_type* pFeed = m_pVideoFeed->FindFeed (feed_no);
	if (!pFeed) return 1;
	if (scale_1 == scale_2) scale_1 = scale_2 = 1;
	while ((!(scale_1 & 1)) && (!(scale_2 & 1))) { scale_1 >>= 1; scale_2 >>= 1; }
	if (((scale_2/scale_1)*scale_1) == scale_2) { scale_2 /= scale_1 ; scale_1 = 1; }
	if (scale_1 > 1 || scale_2 > 1) {

		if (!(m_pVideoScaler)) {
			fprintf(stderr, "Overlay feed %d has no VideoScaler. Skipping overlay\n",
				feed_no);
			return 1;
		}
		// Check that we max cut width and height of feed.
		if (cut_cols > pFeed->width) { cut_cols = pFeed->width; feed_col = 0; }
		if (cut_rows > pFeed->height) { cut_rows = pFeed->height; feed_row = 0; }

		// Now get avilable cols/rows on base given col/row placement
		u_int32_t available_cols = m_geometry_width - col;
		u_int32_t available_rows = m_geometry_height - row;

		// Now check that we are allowed to produce the cut
		if (cut_cols*scale_1/scale_2 > available_cols) cut_cols = available_cols*scale_2/scale_1;
		if (cut_rows*scale_1/scale_2 > available_rows) cut_rows = available_rows*scale_2/scale_1;

		// Now check if we want to center
		if (c == 'c' || c == 'C') {
			feed_col = (pFeed->width - cut_cols)>>1;
			feed_row = (pFeed->height - cut_rows)>>1;
		}

		// We want cut_rows to be divisable by scale_2
//		cut_rows /= feed->scale_2;
//		cut_rows *= feed->scale_2;

		// Truncate to even start pixel
//		cut_start_column &= 0xfffe;
		u_int8_t* src_buf = NULL;
		if (pFeed->fifo_depth <= 0) {
			// Nothing in the fifo. Show the idle image.
			src_buf = pFeed->dead_img_buffer;
		} else {
			// Pick the first one from the fifo.
			src_buf = pFeed->frame_fifo [0].buffer->area_list->buffer_base +
				pFeed->frame_fifo [0].buffer->offset;
		}
*/

//#define VideoScale(scaler) m_pVideoScaler->scaler(	\
//						m_overlay, 	/* Overlay Frame start */	\
//						src_buf, 	/* Feed Frame start */		\
//						col,	 	/* Overlay Start Column */	\
//						row,	 	/* Overlay Start row */		\
//						pFeed->width, 	/* Feed width */		\
//						pFeed->height, 	/* Feed height */		\
//						feed_col, 	/* Feed cut start col */	\
//						feed_row, 	/* Feed cut start row */	\
//						cut_cols, 	/* Feed cut columns */		\
//						cut_rows)	/* Feed cut rows  */
//		if (scale_1 == 1 && scale_2 == 1) VideoScale(Overlay);
//		else if (scale_1 == 1 && scale_2 == 2) VideoScale(Overlay2);
//		else if (scale_1 == 1 && scale_2 == 3) VideoScale(Overlay3);
//		else if (scale_1 == 1 && scale_2 == 4) VideoScale(Overlay4);
//		else if (scale_1 == 1 && scale_2 == 4) VideoScale(Overlay5);
//		else if (scale_1 == 2 && scale_2 == 3) VideoScale(Overlay2_3);
//		else if (scale_1 == 2 && scale_2 == 5) VideoScale(Overlay2_5);
//
//		// else we cant scale;
//	}

	return 0;
}
