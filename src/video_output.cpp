/*
 * (c) 2013 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */


#include <stdio.h>
#ifndef WIN32
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>
#else
#include <io.h>
#endif

#include <sys/types.h>
#include <string.h>

#ifdef HAVE_MMAN
#include <sys/mman.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "snowmix_util.h"
#include "video_output.h"

#ifdef WIN32
#include "windows_util.h"
#endif
//#define system_frame_no m_pVideoMixer->m_pController->m_frame_seq_no

static int LoopCaller(void* p)
{
	CVideoOutput* pVideoOutput = (CVideoOutput*) p;
	if (!p) {
		fprintf(stderr, "CVideoOutput LoopCaller called with NULL "
			"pointer. Exiting\n");
		exit(1);
	}
	if (pVideoOutput->Verbose()) fprintf(stderr, "LoopCaller thread started. Mode %u\n",
		pVideoOutput->OutputMode());
	while (pVideoOutput->OutputSocketSet()) {
		if (pVideoOutput->OutputMode() == 1) pVideoOutput->MainLoopSimple();
		else if (pVideoOutput->OutputMode() == 2) pVideoOutput->MainLoopTimed();
		else if (pVideoOutput->OutputMode() == 3) pVideoOutput->MainLoopTimed2();
		else pVideoOutput->MainLoop();
	}
	if (pVideoOutput->Verbose()) fprintf(stderr, "LoopCaller thread stopping\n");
	return 0;
}

CVideoOutput::CVideoOutput(CVideoMixer* pVideoMixer)
{
	m_verbose = 1;
	if (!(m_pVideoMixer = pVideoMixer))
		fprintf(stderr, "Warning : pVideoMixer set to NULL for CVideoOutput\n");
	if (!(m_pOutputMutex = SDL_CreateMutex()))
		fprintf(stderr, "Warning : Mutex creation failed for CVideoOutput\n");
	if (!(m_pOutputSem = SDL_CreateSemaphore(0)))
		fprintf(stderr, "Warning : Semaphore creation for CVideoOutput\n");
	m_system_socket_set	= false;
	m_pOutput_memory_list	= NULL;
	m_pOutput_memory_base	= NULL;
	m_pFrames_list		= NULL;
	m_pFrames_tail_list	= NULL;
	m_output_memory_size	= 0;
	m_output_memory_handle_name[0] = '\0';
	m_output_memory_handle	= -1;
	m_output_master_socket	= -1;
	m_output_client_socket	= -1;
	m_pOutput_master_name	= NULL;
	m_buffer_delay		= 1;
	m_output_mode		= 0;
	m_buffer_allocate	= 20;
	m_output_freeze		= 0;
	m_output_repeated	= 0;
	m_got_no_free_buffers	= false;
	m_pLoopThread = NULL;
	m_pAverage_loop		= NULL;

	m_buffer_count		= 0;		// Buffers with framed in queue for output
	m_con_seq_no		= 0;		// Connection sequence number
	m_buffers_sent		= 0;		// Numbers of buffers sent
	m_buffers_ack		= 0;		// Number of buffers acknowledge
	m_timed_loops		= 0;		// Number of looped in timed loop
	m_copied_frames		= 0;		// Number of frames copied out
	m_pFrame_period		= NULL;
	if (!(m_pVideoMixer && m_pOutputMutex && m_pOutputSem)) {
		fprintf(stderr, "VideoMixer or mutex/semaphore not present for CVideoOutput\n");
		exit(1);
	}
}

char * CVideoOutput::Query(const char* query, bool tcllist)
{
	int len;
	u_int32_t type;
	char *retstr = NULL;

	if (!query) return NULL;
	while (isspace(*query)) query++;
	if (!(strncasecmp("info ", query, 5))) {
		query += 5; type = 0;
	}
	else if (!(strncasecmp("status ", query, 7))) {
		query += 7; type = 1;
	}
	else if (!(strncasecmp("ext", query, 3))) {
		query += 4; type = 2;
		while ((*query) && !isspace(*query)) query++;
	}
	else if (!(strncasecmp("syntax", query, 6))) {
		return strdup("output ((info | status) [ format ]) | syntax");
	} else return 0;

	while (isspace(*query)) query++;
	if (!strncasecmp(query, "format", 6)) {
		if (type == 0) {
			// output_id	: 0 (for now. maybe more later)
			// sock_name	: Name of socket
			// sock_master	: FD of master socket
			// sock_client	: FD of client socket
			// shm_name	: Name of Shared memory
			// out_geo	: Geometry of Output
			// out_mode	: Output Mode
			// out_bufs	: Number of frame buffers for output
			// freeze_fl	: Freeze set or not?
			// delay_buf	: Numbers of ready mixed frame buffers to hold before sending out
			
			retstr = tcllist ?
				strdup("output_id sock_name sock_master sock_client shm_name out_geo out_mode out_bufs freeze_fl delay_buf") :
				strdup("output info <output_id> <sock_name> <sock_master> <sock_client> <shm_name> <out_geo> <out_mode> <out_bufs> <freeze_fl> <delay_buf>");
		}
		else if (type == 1) {
			// output_id	: 0 (for now. maybe more later)
			// con_seq_no	: Connection sequence number
			// frame_seq_no	: Frame sequence number
			// outfr_in_use : Frames currently in use.
			// no_free_set	: 0 or 1
			// tf_loops	: Tmed frame loops
			// copy_fr	: Copied frames
			// repeat_frs	: Repeated frames
			// out_ready	: Mixed frames ready for output
			// out_sent	: Mixed Frames sent out
			// out_ack	: Mixed frames sent out and acknowledged and returned free for use.
			// period_fr	: Frame Period for timed loops.

			retstr = tcllist ?
				strdup("output_id con_seq_no frame_seq_no outfr_in_use no_free_set tf_loops copy_fr repeat_frs out_ready out_sent out_ack period_fr") :
				strdup("output status <output_id> <con_seq_no> <frame_seq_no> <outfr_in_use> <no_free_set> <tf_loops> <copy_fr> <repeat_frs> <out_ready> <out_sent> <out_ack> <period_fr>");
		}
		else if (type == 2) {
			// output_id	: 0 (for now. maybe more later)
			// cur_periods	: list of current frame periods
			// min_periods	: Minimum period for the last 40 frames
			// ave_periods	: Average period for the last 40 frames
			// max_periods	: Max period for the last 40 frames
			// frame_info	: Frame info
	
			retstr = tcllist ?
				strdup("output_id output_id min_periods ave_periods max_periods last_3") :
				strdup("output extended <output_id> <output_id> <min_periods> <ave_periods> <max_periods> <last_3>");
		} else return NULL;
		if (!retstr) goto malloc_failed;
		return retstr;
	}
	else if (!(*query)) {
		if (type == 0) {
			// output info output_id sock_name sock_master sock_client shm_name out_geo out_mode out_bufs freeze delay_buf
			// output info 1234567890 L 1234567890 1234567890 L 1 1234567890x1234567890 1234567890 1234567890 1234567890
			//          1         2         3         4         5         6         7         8         9
			// 12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567

			len = 107 + (m_pOutput_master_name ? strlen(m_pOutput_master_name) : 1) +
				strlen(m_output_memory_handle_name) + 1;

			// Allocate space for string
			retstr = (char*) malloc(len);
			if (!(retstr)) goto malloc_failed;
			//*retstr = '\0';

			sprintf(retstr, (tcllist ?
				"{0 {%s} %d %d {%s} {%u %u} %u %u %u %u}" : "output info 0 %s %d %d %s %ux%u %u %u %u %u\n"),
				m_pOutput_master_name ? m_pOutput_master_name : "-",
				m_output_master_socket,
				m_output_client_socket,
				*m_output_memory_handle_name ?  m_output_memory_handle_name : "-",
				m_pVideoMixer->GetSystemWidth(), m_pVideoMixer->GetSystemHeight(),
				m_output_mode,
				m_buffer_allocate,
				m_output_freeze,
				m_buffer_delay);
		}
		else if (type == 1) {
			// output_id con_seq_no frame_seq_no outfr_in_use no_free_set tf_loops copy_fr repeat_frs out_ready out_sent out_ack period_fr") :
			// output status 1234567890 1234567890 1234567890 1234567890 1 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890
			//          1         2         3         4         5         6         7         8         9         0         1         2         3
			// 123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678

			// system status 1234567890 0 1234567890.123 1234567890.123 1234567890 1234567890 1,2,3....
			// 12345678901234567890123456789012345678901234567890123456789012345678901234567890
			len = 138;

			// Allocate space for string
			retstr = (char*) malloc(len);
			if (!(retstr)) goto malloc_failed;
			//*retstr = '\0';

			u_int32_t count, inuse;
			SDL_LockMutex(m_pOutputMutex);
	  		  InUseCount(&count, &inuse, true);
	  		  u_int32_t frame_period = m_pFrame_period ? 1000*time2double((*m_pFrame_period)) : 0.0;
			  sprintf(retstr, (tcllist ?
				"{0 %u %u %u %s %u %u %u %u %u %u %u}" : "output status 0 %u %u %u %s %u %u %u %u %u %u %u\n"),
				m_con_seq_no,				// con_seq_no
				SYSTEM_FRAME_NO,			// frame_seq_no
				inuse,					// outfr_in_use
				m_got_no_free_buffers ? "1" : "0",	// no_free_set
				m_timed_loops,				// tf_loops
				m_copied_frames,			// copy_fr
	  			m_output_repeated,			// repeat_frs
				m_buffer_count,				// out_ready
				m_buffers_sent,				// out_sent
				m_buffers_ack,				// out_ack
				frame_period);				// period_fr

			SDL_UnlockMutex(m_pOutputMutex);
		}
		else if (type == 2) {
			// output_id	: 0 (for now. maybe more later)
			// cur_periods	: list of current frame periods
			// min_periods	: Minimum period for the last 40 frames
			// ave_periods	: Average period for the last 40 frames
			// max_periods	: Max period for the last 40 frames
			// frame_info	: Frame info
			// output_id output_id min_periods ave_periods max_periods last_3
			// output extended <output_id> <min_periods> <ave_periods> <max_periods> <last_3>
			// 12345678901234567890123456789012345678901234567890123456789012345678901234567
			// output extended 1234567890 12345.1 12345.1 12345.1 12345.1,12345.1,12345.1

			len = 77;
			// Allocate space for string
			retstr = (char*) malloc(len);
			if (!(retstr)) goto malloc_failed;
			//*retstr = '\0';

			SDL_LockMutex(m_pOutputMutex);
			  sprintf(retstr, (tcllist ?
				"{0 %.1lf %.1lf %.1lf {%.1lf %.1lf %.1lf}}" : "output extended 0 %.1lf %.1lf %.1lf  %.1lf,%.1lf,%.1lf\n"),
				1000*m_pAverage_loop->Min(),
				1000*m_pAverage_loop->Average(),
				1000*m_pAverage_loop->Max(),
				1000 * m_pAverage_loop->Last(),
				1000 * m_pAverage_loop->Last(1),
				1000 * m_pAverage_loop->Last(2));

			SDL_UnlockMutex(m_pOutputMutex);
		}
		else return NULL;
	}
	return retstr;

malloc_failed:
	fprintf(stderr, "CVideoOutput::Query failed to allocate memory\n");
	return NULL;
}

int CVideoOutput::ListStatus(struct controller_type* ctr, const char* str)
{
	struct output_memory_list_type *buf;
	u_int32_t output_repeated, buffer_count, count, inuse, con_seq_no, buffers_sent, buffers_ack, timed_loops, copied_frames;
	int n=0;
	bool got_no_free_buffers;
	double last, last1, last2, average, max, min, frame_period;
	last = last1 = last2 = average = max = 0.0;
	min = max = average = 0;
	
	if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return -1;
	char* bstr = (char*) malloc(m_buffer_allocate*6+1);
	if (bstr) *bstr = '\0';

	SDL_LockMutex(m_pOutputMutex);
	  got_no_free_buffers = m_got_no_free_buffers;
	  output_repeated = m_output_repeated;
	  buffer_count = m_buffer_count;
	  InUseCount(&count, &inuse, true);
	  con_seq_no = m_con_seq_no;			// Connection sequence number
	  buffers_sent = m_buffers_sent;		// Numbers of buffers sent
	  buffers_ack = m_buffers_ack;			// Number of buffers acknowledge
	  timed_loops = m_timed_loops;			// Number of loops in timed loop
	  copied_frames = m_copied_frames;		// Number of copied frames
	  frame_period = m_pFrame_period ? time2double((*m_pFrame_period)) : 0.0;		// Current frame period for running Timed loop
	  if (bstr) for (buf = m_pOutput_memory_list, n=0; buf != NULL; buf = buf->next) {
		//sprintf(bstr+n*5, " %02d:%u ", n, buf->use);
		sprintf(bstr+n*3, " %02u", buf->use);
		n++;
	  }
	  if (m_pAverage_loop) {
		average = m_pAverage_loop->Average();
		max = m_pAverage_loop->Max();
		min = m_pAverage_loop->Min();
		last = m_pAverage_loop->Last();
		last1 = m_pAverage_loop->Last(1);
		last2 = m_pAverage_loop->Last(2);
	  }
	SDL_UnlockMutex(m_pOutputMutex);
	  if (m_pVideoMixer && m_pVideoMixer->m_pController) m_pVideoMixer->m_pController->controller_write_msg (ctr,
		STATUS"system output status\n"
		STATUS"Connection Seq. no.  = %u\n"
		STATUS"Frame number         = %u\n"
		STATUS"Timed frame loops    = %u frames\n"
		STATUS"Copied frames        = %u frames\n"
		STATUS"Frames dif/sent/ack  = %u (%u/%u)\n"
		STATUS"Ready for output     = %u frames\n"
		STATUS"Inuse                = %u of %u frame buffers\n"
		STATUS"Repeated status      = %u frames\n"
		STATUS"Timed loop frame p.  = %.1lf ms\n"
		STATUS"Got no free buffers  = %s\n"
		STATUS"Last 3 frame periods = %.1lf %.1lf %.1lf ms\n"
		STATUS"Frame p. min/ave/max = %.1lf %.1lf %.1lf ms\n"
		STATUS"Frame info           =%s\n"
		STATUS"\n",

		con_seq_no,
		SYSTEM_FRAME_NO,
		timed_loops,
		copied_frames,
		buffers_sent-buffers_ack, buffers_sent, buffers_ack,
		buffer_count,
		inuse, count,
		output_repeated,
		1000*frame_period,
		got_no_free_buffers ? "yes" : "no",
		1000*last, 1000*last1, 1000*last2,
		1000*min, 1000*average, 1000*max,
		bstr ? bstr : "");

	if (bstr) free(bstr);

	return 0;
}

int CVideoOutput::ListInfo(struct controller_type* ctr, const char* str)
{
	if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return -1;
	m_pVideoMixer->m_pController->controller_write_msg (ctr,
		STATUS"system output info\n"
		STATUS"  Verbose           = %u\n"
		STATUS"  Socket name       = %s\n"
		STATUS"  Socket master     = %d\n"
		STATUS"  Socket client     = %d\n"
		STATUS"  Memory handle     = %s\n"
		STATUS"  Memory size       = %u (%u blocks, block size %u)\n"
		STATUS"  Output mode       = %u (%s)\n"
		STATUS"  Freeze            = %s (%u frames, %.2lf secs)\n"
		STATUS"  Delay             = %u frames (%.1lf ms)\n"
		STATUS"\n",
		m_verbose,
		m_pOutput_master_name ? m_pOutput_master_name : "",
		m_output_master_socket,
		m_output_client_socket,
		m_output_memory_handle_name,
		m_output_memory_size, m_buffer_allocate, m_pVideoMixer->m_block_size,
		m_output_mode, m_output_mode == 0 ? "standard, no thread" :
		  m_output_mode == 1 ? "standard, but threaded" :
		    m_output_mode == 2 ? "Fixed rate and threaded" :
		      m_output_mode == 3 ? "Fixed rate2 and threaded" : "unknown",
		m_output_freeze ? "yes" : "no", m_output_freeze,
		  m_pVideoMixer->m_frame_rate > 0.0 ?
		    m_output_freeze*1.0/m_pVideoMixer->m_frame_rate : 0.0,
		m_buffer_delay, m_pVideoMixer->m_frame_rate > 0.0 ?
		  m_buffer_delay * 1000.0 / m_pVideoMixer->m_frame_rate : 0.0);
	return 0;
}

CVideoOutput::~CVideoOutput()
{
	// Indicate that the socket will close
	m_system_socket_set = false;
	// And signal loop thread to take notice
	SDL_SemPost(m_pOutputSem);
	// Give loop thread a chance to close down. Wait a frame period
	int delay = m_pVideoMixer && m_pVideoMixer->m_frame_rate > 1.0 ? ((int)(1000 / m_pVideoMixer->m_frame_rate)) : 1000;
	SDL_Delay(delay);
	// If it isn't dead, then kill it
	if (m_pLoopThread) SDL_KillThread(m_pLoopThread);
	SDL_LockMutex(m_pOutputMutex);
	if (m_pOutput_master_name) free(m_pOutput_master_name);
	if (m_output_client_socket > -1) close(m_output_client_socket);
	if (m_output_master_socket > -1) close(m_output_master_socket);
	if (m_output_memory_handle > -1) {
#ifndef WIN32
		if (shm_unlink(m_output_memory_handle_name)) {
#else
#pragma message("We need a shm_unlink() replacment for Windows")
		if (0) {
#endif
			perror("shm_unlink error for output handle: ");
		} else fprintf(stderr, "Output Shared memory unlinked\n");
	}
	if (m_pOutputSem) SDL_DestroySemaphore(m_pOutputSem);
	if (m_pOutputMutex) SDL_DestroyMutex(m_pOutputMutex);
	if (m_pAverage_loop) delete m_pAverage_loop;
}

int CVideoOutput::ParseCommand(struct controller_type* ctr, const char* str)
{
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) return -1;

	// system output status
	if (!strncasecmp (str, "status ", 7)) {
		return ListStatus(ctr, str+7);
	}
	// system output info
	else if (!strncasecmp (str, "info ", 5)) {
		return ListInfo(ctr, str+5);
	}
	// system output mode <mode>
	else if (!strncasecmp (str, "mode ", 5)) {
			u_int32_t mode = 0;
			n = sscanf(str+5, "%u", &mode);
			return n == 1 ? SetMode(mode) : -1;
	}
	// system output delay <delay>
	else if (!strncasecmp (str, "delay ", 6)) {
			u_int32_t delay = 1;
			n = sscanf(str+6, "%u", &delay);
			return n == 1 ? SetDelay(delay) : -1;
	}
	// system output buffers <number buffers>
	else if (!strncasecmp (str, "buffers ", 8)) {
			u_int32_t buffers = 1;
			n = sscanf(str+8, "%u", &buffers);
			return n == 1 ? SetBuffers(buffers) : -1;
	}
	// system output freeze <number frames to freeze>
	else if (!strncasecmp (str, "freeze ", 7)) {
		u_int32_t buffers = 1;
		n = sscanf(str+7, "%u", &buffers);
		return n == 1 ? SetFreeze(buffers) : -1;
	}
	// system output reset
	else if (!strncasecmp (str, "reset ", 6)) {
		OutputReset();
		return 0;
	}
	return 1;
}

int CVideoOutput::MainLoopSimple() {
	struct output_memory_list_type* buf = NULL;
	int n = 0;

	// Run loop until told not to (m_system_socket_set false)
	while (m_system_socket_set) {

		// Wait for a buffer to be ready
		if ((n = SDL_SemWait(m_pOutputSem))) {
			fprintf(stderr, "CVideoOutput Semaphore failure\n");
			goto failure;
		}

		// Check to see if we should bail out
		if (!m_system_socket_set) return 0;

		// Lock the region and grab the buffer;
		SDL_LockMutex(m_pOutputMutex);

		  // Check the delay parameter
		  if (m_buffer_delay > m_buffer_count) {
		    SDL_UnlockMutex(m_pOutputMutex);
		    continue;
		  }
		  // We are ok to grab a buffer.
		  list_get_head_withframe(m_pFrames_list, m_pFrames_tail_list, buf);
		  if (!buf) {
			fprintf(stderr, "CVideoOutput Semaphore signaled a buffer, "
				"but none were ready\n");
			goto failure;
		  }
		  m_buffer_count--;
		SDL_UnlockMutex(m_pOutputMutex);
		if ((n = SendOutBuffer(buf))) {
			fprintf(stderr, "Send failure for output buffer\n");
		}
	}

	return 0;
failure:
	SDL_Delay(500);
	return -1;
}


u_int32_t  CVideoOutput::FramesRepeated(bool clear)
{	u_int32_t count = 0;
	SDL_LockMutex(m_pOutputMutex);
		count = m_output_repeated;
		if (clear) m_output_repeated = 0;
	SDL_UnlockMutex(m_pOutputMutex);
	return count;
}

int CVideoOutput::MainLoopTimed() {
	struct output_memory_list_type* buf = NULL;
	struct output_memory_list_type* last_buf = NULL;
	struct timeval time_now, next_time, time_frame, time_frame_short, time_frame_long, time_delta;
	int n = 0;
	u_int32_t buffers_sent = 0;

	// First we do some setting up
	if (m_pVideoMixer->m_frame_rate < 0.1 || m_pVideoMixer->m_frame_rate > 999.0) {
		fprintf(stderr, "Frame rate %.4lf out of range. Defaulting to simple "
			"threaded loop\n", m_pVideoMixer->m_frame_rate);
		return MainLoopSimple();
	}

	// set time interval between frames.
	double period = 1.0/m_pVideoMixer->m_frame_rate;
	//time_frame.tv_sec = time_frame_short.tv_sec = 1.0 /  m_pVideoMixer->m_frame_rate;
	//time_frame.tv_usec = 1000000.0*((1.0/m_pVideoMixer->m_frame_rate)-time_frame.tv_sec);
	double2time(period, time_frame);
	double2time(0.98*period, time_frame_short);
	//time_frame_short.tv_usec = 980000.0*((1.0/m_pVideoMixer->m_frame_rate)-time_frame.tv_sec);
	double2time(1.02*period, time_frame_long);
	m_pFrame_period = &time_frame;

	// First we need to wait for 
	// Run loop until told not to (m_system_socket_set false)
	bool delay_wait = true;
	while (delay_wait && m_system_socket_set) {

		// Wait for a buffer to be ready
		if ((n = SDL_SemWait(m_pOutputSem))) {
			fprintf(stderr, "CVideoOutput Semaphore failure\n");
			goto failure;
		}
		if (m_verbose) fprintf(stderr, "CVideoOutput Timed loop thread detected first frame\n");

		// Check to see if we should bail out
		if (!m_system_socket_set) return 0;

		// Lock the region and grab the buffer;
		SDL_LockMutex(m_pOutputMutex);

		  // Check the delay parameter
		  if (m_buffer_delay <= m_buffer_count) delay_wait = false;

		SDL_UnlockMutex(m_pOutputMutex);
	}
	if (m_verbose) fprintf(stderr, "CVideoOutput Timed thread delay requirement = %u frames.\n",
		m_buffer_delay);

	// We have met the delay criteria and at least one buffer is ready for us.

	gettimeofday(&time_now,NULL);
	timeradd(&time_now, &time_frame, &next_time);
	// However we took the semaphore signal, so we add another one.
	SDL_SemPost(m_pOutputSem);

	// Now we will run the timed loop
	while (m_system_socket_set) {

		// Lock region and perhaps grab a buffer
		SDL_LockMutex(m_pOutputMutex);

		  // Check that we honour the delay
		  if (m_buffer_delay <= m_buffer_count) {

			// If we have more than m_buffer_delay buffers waiting ready for output,
			// we will speed up, else we maintain normal frame rate
			if (m_buffer_count > m_buffer_delay) {
				m_pFrame_period = &time_frame_short;
				if (m_verbose) fprintf(stderr,
					"Frame %u - Time short, delay %u, count %u\n",
					SYSTEM_FRAME_NO, m_buffer_delay, m_buffer_count);
			} else m_pFrame_period = &time_frame;

	  		// We are ok to grab a buffer.
	  		list_get_head_withframe(m_pFrames_list, m_pFrames_tail_list, buf);

			if (buf) {
				SDL_SemTryWait(m_pOutputSem);		// We do this to keep score of the semaphore posts
				m_buffer_count--;			// We got a buffer and decrement the counter
				if (buf->use < 1) fprintf(stderr, "Unexpected Timed loop got a new buffer with use < 1. Loop %u use %u\n", m_timed_loops, buf->use);

			} else {
				// Unexpected. We should not come here.
				fprintf(stderr, "Unexpected no buffer available for output\n");

				// We mitigate this by reusing the last one.
				buf = last_buf;
			}

		  // we do not have enough buffers and must reuse the old one
		  } else {
			buf = last_buf;
		  }

		  // Check if should freeze (repeat last frame) output
		  if (m_output_freeze) {
			// Decrement freeze count
			m_output_freeze--;
			// If we got a new buffer, we will throw it away
			if (buf != last_buf) {
				if (buf->use != 1) fprintf(stderr, "Unexpected. While freezing, new buffer had use != 1. %u", buf->use);
				buf->use = 0;
				buf = last_buf;
			}
		  }

		  // Now we check if we are using the old one or got a new one.
		  if (buf == last_buf) {

			// We are reusing the old buffer
			m_output_repeated++;	// This will be reset, when we use a new buffer ready
	  		m_copied_frames++;	// Number of frames copied out

		  } else {

			// We got a new buffer and can decrease the counter of the old one
			// First run will have last_but == NULL.
			if (last_buf) {
				 if (last_buf->use >0) last_buf->use--;
				else fprintf(stderr, "Unexpected last_buf use was less than 1 for loop %u\n", m_timed_loops);
			}
		  }

		  // We should have a buf, but lets check to be safe
		  if (!buf) {
			fprintf(stderr, "Unexpected buf == NULL in MainLoopTimed\n");
			continue;
		  }
		
		  // We increase the use of the buffer by one extra, so it would not
		  // be released when used by the shmsrc receiver and received by OutputAckBuffer.
		  // That way it stays allocated (until we deallocate it) in case we need it again.
		  buf->use++;
		  if (buf->use < 2) fprintf(stderr, "Unexpected buf-use < 2. %u\n", buf->use);

		  // Check to see if we are about to run out of buffers or should go slower
		  if (m_buffers_sent-m_buffers_ack > 2) {
			double period = (1.0 + ((m_buffers_sent-m_buffers_ack) / ((double)m_buffer_allocate)))/ m_pVideoMixer->m_frame_rate;
			double2time(period, time_frame_long);
			m_pFrame_period = &time_frame_long;
		  }

		SDL_UnlockMutex(m_pOutputMutex);

// if (buf == last_buf) fprintf(stderr, "Ready to send buffer. buf is %s. buf %s last use %d\n", buf ? "ptr" : "nill", buf == last_buf ? "==" : "!=", buf ? buf->use : 0);


		// Now if we have a buffer, we will send it
	  	if (buf) {
			if ((n = SendOutBuffer(buf))) {
				fprintf(stderr, "Send failure for output buffer\n");
				goto failure;
			}
			buffers_sent++;
		} else {
			// Can't do much about it.
			fprintf(stderr, "No output buffer to send\n");
		}
		last_buf = buf;

		// Now we find the time now
		struct timeval last_time = time_now;
		gettimeofday(&time_now, NULL);

		if (m_pAverage_loop) {
			// and see how long it took
			timersub(&time_now, &last_time, &time_delta);
			m_pAverage_loop->Add(time2double(time_delta));
//fprintf(stderr, "Adding %.3lf\n", time2double(time_delta));
		}

		// Compare it for the time for the next frame
		timersub(&next_time, &time_now, &time_delta);

		// We then update next_time by adding a frame period. Either the frame time or the short one.
		timeradd(&next_time, m_pFrame_period, &next_time);

		// Then we sleep if the time_delta is positive
		if (time_delta.tv_usec > 0 && time_delta.tv_sec >= 0) {
#ifndef WIN32
			struct timespec sleep_time;
			sleep_time.tv_sec = time_delta.tv_sec;
			sleep_time.tv_nsec = time_delta.tv_usec * 1000;
			nanosleep(&sleep_time, NULL);
#else
#pragma message ("We need a nanosleep() replacment for Windows. The Sleep() is too crude")
			Sleep(time_delta.tv_sec / 1000 + time_delta.tv_usec * 1000);
#endif
		} else {
			double delta = time2double(time_delta);
			u_int32_t buffer_count, count, inuse;
			InUseCount(&count, &inuse);
			SDL_LockMutex(m_pOutputMutex);
			  buffer_count = m_buffer_count;
			SDL_UnlockMutex(m_pOutputMutex);
			fprintf(stderr, "Frame %u - Unexpected output time_delta <= 0. "
#ifdef HAVE_MACOSX                  // suseconds_t is int on OS X 10.9
				"delta= %ld.%06d sec. Timenow = %ld.%06d sec\n",
#else                               // suseconds_t is long int on Linux
                    "delta= %ld.%06ld sec. Timenow = %ld.%06ld sec\n",
#endif
				SYSTEM_FRAME_NO,  time_delta.tv_sec, time_delta.tv_usec,
				time_now.tv_sec, time_now.tv_usec);
			fprintf(stderr, " - Buffers count=%u Inuse=%u Ready4output=%u Send=%u\n",
				count, inuse, buffer_count, buffers_sent);
			if (delta < -1.0) {
				fprintf(stderr, " - Advancing next_time to now + frame period\n");
				timeradd(&time_now, &time_frame, &next_time);
			}
		}
	  	m_timed_loops++;		// Number of looped in timed loop
	}
	SDL_LockMutex(m_pOutputMutex);
	  m_pFrame_period = NULL;
	SDL_UnlockMutex(m_pOutputMutex);
	return 0;
failure:
	SDL_Delay(100);
	return -1;
}

int CVideoOutput::MainLoopTimed2() {
	struct output_memory_list_type* buf = NULL;
	struct output_memory_list_type* last_buf = NULL;
	struct timeval time_now;	// Current time
	struct timeval next_time;	// Time for next frame
	struct timeval time_frame;	// The time for a single frame period.
	struct timeval time_last;	// The last time we sent a frame out.

	struct timeval time_frame_short;
	struct timeval time_frame_long;
	struct timeval time_delta;
struct timeval start_time;
	int n = 0;
	u_int32_t buffers_sent = 0;

	// First we do some setting up
	if (m_pVideoMixer->m_frame_rate < 0.1 || m_pVideoMixer->m_frame_rate > 999.0) {
		fprintf(stderr, "Frame rate %.4lf out of range. Defaulting to simple "
			"threaded loop\n", m_pVideoMixer->m_frame_rate);
		return MainLoopSimple();
	}

	// set time interval between frames.
	double period = 1.0/m_pVideoMixer->m_frame_rate;
	//time_frame.tv_sec = time_frame_short.tv_sec = 1.0 /  m_pVideoMixer->m_frame_rate;
	//time_frame.tv_usec = 1000000.0*((1.0/m_pVideoMixer->m_frame_rate)-time_frame.tv_sec);
	double2time(period, time_frame);
	double2time(0.98*period, time_frame_short);
	double2time(1.02*period, time_frame_long);
	m_pFrame_period = &time_frame;

	// First we need to wait for 
	// Run loop until told not to (m_system_socket_set false)
	bool delay_wait = true;
	while (delay_wait && m_system_socket_set) {

		// Wait for a buffer to be ready
		if ((n = SDL_SemWait(m_pOutputSem))) {
			fprintf(stderr, "CVideoOutput Semaphore failure\n");
			goto failure;
		}
		if (m_verbose) fprintf(stderr, "CVideoOutput Timed loop thread detected first frame\n");

		// Check to see if we should bail out
		if (!m_system_socket_set) return 0;

		// Lock the region and grab the buffer;
		SDL_LockMutex(m_pOutputMutex);

		  // Check the delay parameter
		  if (m_buffer_delay <= m_buffer_count) delay_wait = false;

		SDL_UnlockMutex(m_pOutputMutex);
	}
	if (m_verbose) fprintf(stderr, "CVideoOutput Timed2 thread require at least %u frame%s to start.\n",
		m_buffer_delay, m_buffer_delay > 1 ? "s" : "");

	// We have met the delay criteria and at least one buffer is ready for us.

	// First we get the time now and calculate when the next frame after this frame has to be output.
	gettimeofday(&time_now,NULL);
	timeradd(&time_now, &time_frame, &next_time);

	// For accounting we set last_time to time_now-time_frame
	timersub(&time_now, &time_frame, &time_last);

	// However we took the semaphore signal, so we add another one.
	SDL_SemPost(m_pOutputSem);
start_time = time_now;
m_verbose = 2;
	// Now we will run the timed loop
	while (m_system_socket_set) {
bool verbose = false;
gettimeofday(&time_now,NULL);

struct timeval time_delta1, time_delta2;
timersub(&time_now, &start_time, &time_delta1);
timersub(&next_time, &time_now, &time_delta2);
		// Lock region and perhaps grab a buffer
		SDL_LockMutex(m_pOutputMutex);
if (verbose) fprintf(stderr, "Frame %04u : %03ld.%03d, next %ld.%03d\n - region locked. buffer count %u\n",
	SYSTEM_FRAME_NO, time_delta1.tv_sec, time_delta1.tv_usec/1000, time_delta2.tv_sec, time_delta2.tv_usec/1000, m_buffer_count);

		  // Check that we honour the delay. If we have to few buffers ready, we have to
		  // reuse an old one.
		  if (m_buffer_delay <= m_buffer_count) {
if (verbose) fprintf(stderr, "   - Using a new buffer\n");

			// If we have more than m_buffer_delay buffers waiting ready for output,
			// we will speed up, else we maintain normal frame rate
			if (m_buffer_count > m_buffer_delay) {
				//m_pFrame_period = &time_frame_short;
//				if (m_verbose) fprintf(stderr,
//					"Frame %u - Time short, delay %u, count %u\n",
//					SYSTEM_FRAME_NO, m_buffer_delay, m_buffer_count);
			} // else m_pFrame_period = &time_frame;

	  		// We are ok to grab a buffer.
	  		list_get_head_withframe(m_pFrames_list, m_pFrames_tail_list, buf);

			if (buf) {
				SDL_SemTryWait(m_pOutputSem);		// We do this to keep score of the semaphore posts
				m_buffer_count--;			// We got a buffer and decrement the counter
				if (buf->use < 1) fprintf(stderr, "Unexpected Timed loop got a new buffer with use < 1. Loop %u use %u\n", m_timed_loops, buf->use);

			} else {
				// Unexpected. We should not come here.
				fprintf(stderr, "Unexpected no buffer available for output\n");

				// We mitigate this by reusing the last one.
				buf = last_buf;
			}

		  // we do not have enough buffers and must reuse the old one
		  } else {
if (verbose) fprintf(stderr, "   - Reusing old buffer. buffer %s\n", last_buf ? "ptr" : "null");
			buf = last_buf;
		  }

		  // Check if should freeze (repeat last frame) output
		  if (m_output_freeze) {
if (verbose) fprintf(stderr, "   - freeze %u\n", m_output_freeze);
			// Decrement freeze count
			m_output_freeze--;
			// If we got a new buffer, we will throw it away
			if (buf != last_buf) {
				if (buf->use != 1) fprintf(stderr, "Unexpected. While freezing, new buffer had use != 1. %u", buf->use);
				buf->use = 0;
				buf = last_buf;
			}
		  }
else if (verbose) fprintf(stderr, "   - no freeze\n");

		  // Now we check if we are using the old one or got a new one.
		  if (buf == last_buf) {

			// We are reusing the old buffer
			m_output_repeated++;	// This will be reset, when we use a new buffer ready
	  		m_copied_frames++;	// Number of frames copied out
if (verbose) fprintf(stderr, "   - we detected we are reusing a buffer. repeat %u copied %u\n", m_output_repeated, m_copied_frames);

		  } else {
if (verbose) fprintf(stderr, "   - we detected we got a new buffer\n");

			// We got a new buffer and can decrease the counter of the old one
			// First run will have last_buf == NULL.
			if (last_buf) {
if (verbose) fprintf(stderr, "   - we detected we got a new buffer and will decrement use counter on last one. last use %u\n",last_buf->use);

				 if (last_buf->use >0) last_buf->use--;
				else fprintf(stderr, "Unexpected last_buf use was less than 1 for loop %u\n", m_timed_loops);
			}
else fprintf(stderr, "   - no last_buf. Normal for first one\n");
		  }

		  // We should have a buf, but lets check to be safe
		  if (!buf) {
			fprintf(stderr, "Possible error. Unexpected buf == NULL in MainLoopTimed2\n");
			continue;
		  }
		
		  // We increase the use of the buffer by one extra, so it would not
		  // be released when used by the shmsrc receiver and received by OutputAckBuffer.
		  // That way it stays allocated (until we deallocate it) in case we need it again.
		  buf->use++;
		  if (buf->use < 2) fprintf(stderr, "Unexpected buf-use < 2. %u\n", buf->use);
if (verbose) fprintf(stderr, "   - buf->use is %u\n", buf->use);

		  // Check to see if we are about to run out of buffers or should go slower
		  if (m_buffers_sent-m_buffers_ack > 2) {
if (verbose) fprintf(stderr, "   - We detected we should go slower .... except we shouldn't\n");
			double period = (1.0 + ((m_buffers_sent-m_buffers_ack) / ((double)m_buffer_allocate)))/ m_pVideoMixer->m_frame_rate;
			double2time(period, time_frame_long);
//			m_pFrame_period = &time_frame_long;
		  }

		SDL_UnlockMutex(m_pOutputMutex);
if (verbose) fprintf(stderr, " - region unlocked\n");

		// Now if we have a buffer, we will send it
	  	if (buf) {
//			fprintf(stderr, " - Sending out buffer\n");
			if ((n = SendOutBuffer(buf))) {
				fprintf(stderr, "Send failure for output buffer\n");
				goto failure;
			}
			buffers_sent++;
		} else {
			// Can't do much about it.
			fprintf(stderr, " - No output buffer to send\n");
		}
		last_buf = buf;

		// Now we find the time now
//		struct timeval last_time = time_now;
		gettimeofday(&time_now, NULL);
if (verbose) fprintf(stderr, " - Getting time now\n");

		// Accounting, if enabled
struct timeval actual;
timersub(&time_now,&time_last, &actual);
		if (m_pAverage_loop) {
			// and see how long it took
			timersub(&time_now, &time_last, &time_delta);
			m_pAverage_loop->Add(time2double(time_delta));
if (verbose) fprintf(stderr, " - Adding %.3lf\n", time2double(time_delta));
			time_last = time_now;
		}

		// Compare it for the time for the next frame
		timersub(&next_time, &time_now, &time_delta);

		if (verbose) fprintf(stderr, " - timenow "
#ifdef HAVE_MACOSX                  // suseconds_t is int on OS X 10.9
			"%ld.%03d period %ld.%03d actual %ld.%03d nexttime %ld.%03d Time until next frame %ld.%03d\n",
//				"delta= %ld.%06d sec. Timenow = %ld.%06d sec\n",
#else                               // suseconds_t is long int on Linux
			"%ld.%03ld period %ld.%03ld actual %ld.%03ld nexttime %ld.%03ld Time until next frame %ld.%03ld\n",
//                   "delta= %ld.%06ld sec. Timenow = %ld.%06ld sec\n",
#endif
	time_now.tv_sec, time_now.tv_usec/1000,
	m_pFrame_period->tv_sec, m_pFrame_period->tv_usec/1000,
	actual.tv_sec, actual.tv_usec/1000,
	next_time.tv_sec, next_time.tv_usec/1000,
	time_delta.tv_sec,time_delta.tv_usec/1000);

		// We then update next_time by adding a frame period. Either the frame time or the short one.
		timeradd(&next_time, m_pFrame_period, &next_time);

		// Then we sleep if the time_delta is positive
		if (time_delta.tv_usec > 0 && time_delta.tv_sec >= 0) {
#ifndef WIN32
			while  (time_delta.tv_usec > 0 && time_delta.tv_sec >= 0) {
				struct timespec sleep_time;
				sleep_time.tv_sec = time_delta.tv_sec;
				sleep_time.tv_nsec = time_delta.tv_usec * 1000;
if (verbose) fprintf(stderr, " - We will sleep for %ld.%06ld secs\n", sleep_time.tv_sec, sleep_time.tv_nsec/1000);
				if (nanosleep(&sleep_time, NULL)) {
					if (m_verbose > 1) fprintf(stderr, "nanosleep was interrupted. recalculating sleep time\n");
					// nanosleep was interrupted. We need to calculate new sleep time
					gettimeofday(&time_now, NULL);
					timersub(&next_time, &time_now, &time_delta);
				} else break;
			}
#else
#pragma message ("We need a nanosleep() replacment for Windows. The Sleep() is too crude")
			Sleep(time_delta.tv_sec / 1000 + time_delta.tv_usec * 1000);
#endif
		} else {
fprintf(stderr, " - no sleep required. We must be falling behind\n");
			double delta = time2double(time_delta);
			u_int32_t buffer_count, count, inuse;
			InUseCount(&count, &inuse);
			SDL_LockMutex(m_pOutputMutex);
			  buffer_count = m_buffer_count;
			SDL_UnlockMutex(m_pOutputMutex);
			fprintf(stderr, "Frame %u - Unexpected output time_delta <= 0. "
#ifdef HAVE_MACOSX                  // suseconds_t is int on OS X 10.9
				"delta= %ld.%06d sec. Timenow = %ld.%06d sec\n",
#else                               // suseconds_t is long int on Linux
                    "delta= %ld.%06ld sec. Timenow = %ld.%06ld sec\n",
#endif
				SYSTEM_FRAME_NO,  time_delta.tv_sec, time_delta.tv_usec,
				time_now.tv_sec, time_now.tv_usec);
			fprintf(stderr, " - Buffers count=%u Inuse=%u Ready4output=%u Send=%u\n",
				count, inuse, buffer_count, buffers_sent);
			if (delta < -1.0) {
				fprintf(stderr, " - Advancing next_time to now + frame period\n");
				timeradd(&time_now, &time_frame, &next_time);
			}
		}
	  	m_timed_loops++;		// Number of loops in timed loop
	}
	SDL_LockMutex(m_pOutputMutex);
	  m_pFrame_period = NULL;
	SDL_UnlockMutex(m_pOutputMutex);
	return 0;
failure:
	SDL_Delay(100);
	return -1;
}

int CVideoOutput::MainLoop() {
	struct output_memory_list_type* buf = NULL;
	//struct output_memory_list_type* last_buf = NULL;
	//struct shm_commandbuf_type	cb;
	//int frame_period = 1000 / m_pVideoMixer->m_frame_rate;
	struct timeval time_now, frame_time, short_frame_time, next_time, last_time, time;
	int n = 0;

	// Set frame time (frame period)
	if (!m_pVideoMixer || m_pVideoMixer->m_frame_rate <= 0.0) {
		fprintf(stderr, "%s. Exiting snowmix.\n", m_pVideoMixer ?
			"Frame rate <= 0" : "m_pVideoMixer not defined in CVideoOutput");
		exit(1);
	}
	frame_time.tv_sec = (typeof(frame_time.tv_sec))(1.0 /  m_pVideoMixer->m_frame_rate);
	frame_time.tv_usec = (typeof(frame_time.tv_usec))(1000000.0*((1.0 / m_pVideoMixer->m_frame_rate) - frame_time.tv_sec));
	if (frame_time.tv_usec < 0) frame_time.tv_usec = 0;
	short_frame_time = frame_time;
	short_frame_time.tv_usec = (typeof(short_frame_time.tv_usec))(short_frame_time.tv_usec * 0.98);
#ifdef HAVE_MACOSX          // suseconds_t is integer for OS X
	fprintf(stderr, "Loop Frame period %ld.%06d secs fps %.3lf period %.3lf\n",
#else                       // suseconds_t is long int for Linux
    fprintf(stderr, "Loop Frame period %ld.%06ld secs fps %.3lf period %.3lf\n",
#endif
            frame_time.tv_sec, frame_time.tv_usec,
            m_pVideoMixer->m_frame_rate, 1.0/m_pVideoMixer->m_frame_rate);
	
	// First we wait for two frames to be ready
	while (true) {
		if ((n = SDL_SemWait(m_pOutputSem)))
			fprintf(stderr, "CVideoOutput SemWait returned error\n");
		if (!m_system_socket_set) return 0;

		// Lock the mutex and check if we have two or more frames ready
		SDL_LockMutex(m_pOutputMutex);
		if (m_pFrames_list != m_pFrames_tail_list) {
			list_get_head_withframe(m_pFrames_list, m_pFrames_tail_list, buf);
			// We have a buffer. Get time now and calculate time for next frame
			m_buffer_count--;
			gettimeofday(&time_now, NULL);
			timeradd(&time_now, &frame_time, &next_time);
			last_time = time_now;
		}
		SDL_UnlockMutex(m_pOutputMutex);
		if (buf) break;
	}

	bool show_time = false;
	// We had two or more frames ready in the list and buf is pointing at the first
	while (m_system_socket_set) {

		// Check we have a buffer
		if (!buf) {
//			SDL_LockMutex(m_pOutputMutex);
//			buf = last_buf;
//			fprintf(stderr, "Reusing old buffer. %s use %d\n",
//				buf->use == 0 ? "WARNING" : "" , buf->use);
//			buf->use++;
//			SDL_UnlockMutex(m_pOutputMutex);
		} ;
		//if (m_buffer_count >3) fprintf(stderr, "Frame %u - Count %d\n", SYSTEM_FRAME_NO, m_buffer_count);

		SendOutBuffer(buf);
		//last_buf = buf;

		// Now lets find out how much time to wait
		gettimeofday(&time_now, NULL);
		if (timercmp(&time_now,&next_time,<)) {
			if (m_buffer_count < 3) {
				timersub(&next_time, &time_now, &time);
				timeradd(&next_time, &frame_time, &next_time);
			} else {
				timersub(&next_time, &time_now, &time);
				timeradd(&next_time, &short_frame_time, &next_time);
				time.tv_usec = (typeof(time.tv_usec))(time.tv_usec * 0.99);
			}
			struct timespec sleep_time;
			sleep_time.tv_sec = time.tv_sec;
			sleep_time.tv_nsec = time.tv_usec * 1000;
if (show_time) {
	struct timeval since_last;
	timersub(&time_now, &last_time, &since_last);
#ifdef HAVE_MACOSX
	fprintf(stderr, "Frame %u - Queue %u delta time %ld.%06d Sleep %ld.%09ld\n",
#else
    fprintf(stderr, "Frame %u - Queue %u delta time %ld.%06ld Sleep %ld.%09ld\n",
#endif
		SYSTEM_FRAME_NO, m_buffer_count, since_last.tv_sec, since_last.tv_usec,
		sleep_time.tv_sec, sleep_time.tv_nsec);
}
#ifndef WIN32
			nanosleep(&sleep_time, NULL);
#else
#pragma message ("We need a nanosleep() replacment for Windows. The Sleep() is too crude")
			Sleep((DWORD)(sleep_time.tv_sec / 1000 + sleep_time.tv_nsec * 1000));
#endif
		} else {
			fprintf(stderr, "Frame %u - Loop skipping wait\n", SYSTEM_FRAME_NO);
		}
last_time = time_now;

		// Lock list, get a buffer and unlock list
		SDL_LockMutex(m_pOutputMutex);
		list_get_head_withframe(m_pFrames_list, m_pFrames_tail_list, buf);
		if (buf) m_buffer_count--;
		SDL_UnlockMutex(m_pOutputMutex);
		if (buf) SDL_SemTryWait(m_pOutputSem);
		else {
			if (m_output_client_socket < 0) return 0;;
			fprintf(stderr, "Frame %u - Failed to get a frame in time %d\n",
				SYSTEM_FRAME_NO, m_buffer_count);
			show_time = true;
			continue;
		}
	}
	return 0;
}


/* Initialize the output system. */
int CVideoOutput::InitOutput(struct controller_type *ctr, char *path)
{
	int	no = 0;

	if (!m_pVideoMixer || !m_pVideoMixer->m_pController ||
		!m_pOutputMutex || !path) return -1;
	while (isspace(*path)) path++;
	if (!(*path)) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS" system socket = %s\n", m_pOutput_master_name ? m_pOutput_master_name : "");
		return 0;
	}
	if (m_system_socket_set) return -1;

	if (*path != '/') {
		fprintf(stderr, "Warning: system socket path must be absolute by starting with a '/'\n");
		return -1;
	}

	// Create a shared memory region for talking to the output receivers.
	// Space for buffers (should be more than enough).
	m_output_memory_size = m_buffer_allocate * m_pVideoMixer->m_block_size;

	// Create the shared memory handle.
	do {
#ifndef WIN32
		pid_t	pid = getpid ();

#ifdef HAVE_MACOSX
		sprintf (m_output_memory_handle_name, "/shmpipe.%d.%d", pid, no);
		m_output_memory_handle = shm_open (m_output_memory_handle_name,
				O_RDWR | O_CREAT | O_EXCL,  S_IRUSR | S_IWUSR | S_IRGRP);
#else
		sprintf (m_output_memory_handle_name, "/shmpipe.%5d.%5d", pid, no);
		//sprintf (m_output_memory_handle_name, "/snowmix_shmpipe.%d.%d", pid, no);
		m_output_memory_handle =
			shm_open (m_output_memory_handle_name,
				O_RDWR | O_CREAT | O_TRUNC | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP);
#endif
#else
#pragma message("We need a shm_open() replacement for Windows")
#endif
		if (m_output_memory_handle < 0 && errno == EEXIST) {
			fprintf(stderr, "Warning. Shared memory %s already existed.\n",
				m_output_memory_handle_name);
			no++;
		}
	} while (m_output_memory_handle < 0 && errno == EEXIST);
	if (m_output_memory_handle < 0) {
		fprintf (stderr, "Unable to open shared memory handle: name %s. Error : %s\n",
			m_output_memory_handle_name, strerror (errno));
		exit (1);
	}
#ifndef WIN32
	if (ftruncate (m_output_memory_handle, m_output_memory_size) < 0) {
#else
#pragma message("We need a ftruncate() replacemanet for Windows")
	if (0) {
#endif
		perror ("Unable to resize shm area.");
		exit (1);
	}

#ifndef WIN32
	m_pOutput_memory_base = (uint8_t*) mmap (NULL,
		m_output_memory_size, PROT_READ | PROT_WRITE, MAP_SHARED,
		m_output_memory_handle, 0);
#else
#pragma message ("We need a mmap() replacement for Windows")
	m_pOutput_memory_base = (typeof(m_pOutput_memory_base))(MAP_FAILED);
#endif
	if (m_pOutput_memory_base == MAP_FAILED) {
		perror ("Unable to map area into memory.");
		exit (1);
	}

	/* Clear the area. */
	memset (m_pOutput_memory_base, 0x00, m_output_memory_size);

	/* Assign individual buffer pointers to the area. */
	{
		struct output_memory_list_type	*buf;
		int				offset;

		for (offset = 0; offset < m_output_memory_size; offset += m_pVideoMixer->m_block_size) {
			//buf = (output_memory_list_type*) calloc (1, sizeof (*buf));
			buf = (output_memory_list_type*) calloc (1, sizeof (output_memory_list_type));
			buf->offset    = offset;
			buf->use       = 0;
			buf->next_withframe = NULL;

			list_add_tail (m_pOutput_memory_list, buf);
		}
	}

	/* Create the output socket. */
	trim_string (path);
	m_pOutput_master_name = strdup (path);

	unlink (m_pOutput_master_name);
	m_output_master_socket = socket (AF_UNIX, SOCK_STREAM, 0);
	if (m_output_master_socket < 0) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"MSG: Output master socket create error: %s", strerror (errno));
		m_system_socket_set = false;
		return -1;
	}

	/* Bind it. */
	{
		struct sockaddr_un addr;

		memset (&addr, 0, sizeof (addr));
		addr.sun_family = AF_UNIX;
		strncpy (addr.sun_path, m_pOutput_master_name, sizeof (addr.sun_path) - 1);
		if (bind (m_output_master_socket, (struct sockaddr *)&addr, sizeof (addr)) < 0) {
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				"MSG: Output master socket bind error: %s", strerror (errno));
			m_system_socket_set = false;
			close(m_output_master_socket);
			m_output_master_socket = -1;
			return -1;
		}
	}

	/* Prepare to receive new connections. */
	if (listen (m_output_master_socket, 1) < 0) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"MSG: Output master socket listen error: %s", strerror (errno));
		m_system_socket_set = false;
		close(m_output_master_socket);
		m_output_master_socket = -1;
		return -1;
	}

	m_system_socket_set = true;

	// See if we should run threaded
	if (m_output_mode) {
		if (!(m_pLoopThread = SDL_CreateThread(LoopCaller, this))) {
			fprintf(stderr, "Failed to create LoopCaller thread\n");
			m_system_socket_set = false;
			close(m_output_master_socket);
			m_output_master_socket = -1;
			return -1;
		}
	}
	m_pAverage_loop = new CAverage(50);
	gettimeofday(&m_last_time, NULL);

	return 0;
}

u_int8_t* CVideoOutput::FrameAddress(struct output_memory_list_type* buf)
{
	return !buf || !m_pOutput_memory_base ? NULL : m_pOutput_memory_base + buf->offset;
}


// Note:
// Not able to do it without copying the frame.  This is due to a bug in
// gst-plugins/bad/sys/shm/shmpipe.c line 931 where self->shm_area->id
// should have been shm_area->id. In other words it will always acknowledge
// buffers using the newest area_id recorded.

// Return a descriptor for a free frame buffer.
struct output_memory_list_type* CVideoOutput::OutputGetFreeBlock()
{
	struct output_memory_list_type		*buf;
	bool got_no_free_cleared = false;
	int n=0;
	SDL_LockMutex(m_pOutputMutex);

	// Check if we should pause in using new buffers
	if (m_got_no_free_buffers) {
		u_int32_t count, inuse;
		InUseCount(&count, &inuse);
		if (inuse<<1 > count) {
			SDL_UnlockMutex(m_pOutputMutex);
			return NULL;
		}
	        m_got_no_free_buffers = false;
		got_no_free_cleared = true;
	}
	for (buf = m_pOutput_memory_list; buf != NULL; buf = buf->next) {
		if (buf->use == 0) {
			buf->use++;	/* Increase use count and return the descriptor pointer. */
			SDL_UnlockMutex(m_pOutputMutex);
			if (got_no_free_cleared) fprintf(stderr, "Frame %u : CVideoOutput Cleared "
				"m_got_no_free_buffers flag\n", SYSTEM_FRAME_NO);
			return buf;
		}
		n++;
	}
	if (!m_got_no_free_buffers) {
		m_got_no_free_buffers = true;
		SDL_UnlockMutex(m_pOutputMutex);
		fprintf (stderr, "Frame %u : CVideoOutput out of shm buffers. "
			"Setting m_got_no_free_buffers flag.\n", SYSTEM_FRAME_NO);
	} else SDL_UnlockMutex(m_pOutputMutex);
	//OutputInfo();
	return NULL;
}


/* Release the use count for a memory buffer. */
void CVideoOutput::OutputAckBuffer (int area_id, unsigned long offset)
{
	struct output_memory_list_type	*buf;

	SDL_LockMutex(m_pOutputMutex);
	for (buf = m_pOutput_memory_list; buf != NULL; buf = buf->next) {
		if (buf->offset == offset) {
			if (0) {
				fprintf (stderr, "ACK Buffer. Client  ID: %d  block: %4lu "
					" use: %d  offset: %9lu\n", area_id,
					offset/m_pVideoMixer->m_block_size, buf->use, offset);
			}
			buf->use--;		/* Found you. */
			m_buffers_ack++;
			SDL_UnlockMutex(m_pOutputMutex);
			return;
		}
	}
	SDL_UnlockMutex(m_pOutputMutex);

	fprintf (stderr, "Invalid free by output_ack_buffer. offset: %lu\n", offset);
	return;
}


/* Reset the output stream. */
void CVideoOutput::OutputReset ()
{
	struct output_memory_list_type	*buf;

	SDL_LockMutex(m_pOutputMutex);
	  close (m_output_client_socket);
	  m_output_client_socket = -1;

	  /* Clear the use count on all elements in the output_memory_list. */
	  for (buf = m_pOutput_memory_list; buf != NULL; buf = buf->next) {
		buf->use = 0;
	  }
	  m_got_no_free_buffers = false;	// We now have all buffers free
	  m_buffer_count	= 0;		// Buffers with framed in queue for output
	  m_buffers_sent	= 0;		// Numbers of buffers sent
	  m_buffers_ack		= 0;		// Number of buffers acknowledge
	  m_timed_loops		= 0;		// Number of looped in timed loop
	  m_copied_frames	= 0;		// Number of frames copied out
	SDL_UnlockMutex(m_pOutputMutex);
}

void CVideoOutput::InUseCount(u_int32_t* buf_count, u_int32_t* inuse, bool locked)
{
	struct output_memory_list_type	  *buf;
	*buf_count = 0;
	*inuse = 0;
	if (!locked) SDL_LockMutex(m_pOutputMutex);
	for (buf = m_pOutput_memory_list; buf != NULL;
		buf = buf->next) {
		(*buf_count)++;
		if (buf->use) (*inuse)++;
	}
	if (!locked) SDL_UnlockMutex(m_pOutputMutex);
}


/* Check if there is anything to do on a output socket. */
void CVideoOutput::CheckRead (fd_set *rfds)
{
	if (m_output_client_socket >= 0 && FD_ISSET (m_output_client_socket, rfds)) {
		/* Got data. */
		struct shm_commandbuf_type	cb;
		int				x;

		x = read (m_output_client_socket, &cb, sizeof (cb));
		if (x < 0) {
			// EOF or socket disconnect.
			// close (m_output_client_socket);
			// m_output_client_socket = -1;
			// OutputReset will close connection.
			OutputReset();
		} else if (x > 0) {
			switch (cb.type) {
				case 4:	{	/* COMMAND_ACK_BUFFER. */
					if (0)
					fprintf (stderr, "ACK Buffer. Client  ID: %d  block: %4lu  offset: %9lu\n",
						 cb.area_id,
						 cb.payload.ack_buffer.offset/m_pVideoMixer->m_block_size,
						 cb.payload.ack_buffer.offset);

					/* Release the buffer. */
					OutputAckBuffer(cb.area_id, cb.payload.ack_buffer.offset);

					break;
				}

				case 1:		/* COMMAND_NEW_SHM_AREA. */
				case 2:		/* COMMAND_CLOSE_SHM_AREA. */
				case 3: 	/* COMMAND_NEW_BUFFER. */
				default:
					fprintf (stderr, "Unknown/unexpected packet. Client  Code: %d\n",
						 cb.type);
					break;
			}
		}
	}

	if (m_output_master_socket >= 0 && FD_ISSET (m_output_master_socket, rfds)) {
		/* Got a new connection. */
		m_output_client_socket = accept (m_output_master_socket, NULL, NULL);
		if (m_output_client_socket >= 0) {

			// Set the socket to non blocking
			if (set_fd_nonblocking(m_output_client_socket)) {
				perror ("failed to set system output control client socket to non blocking");
				exit(1);
			}
// MacOSX does not have MSG_NOSIGNAL. send() can throw an EPIPE error and terminate snowmix.
// To prevent this, the SO_NOSIGPIPE has been set for socket. Then EPIPE is returned if connection
// is remotely broken.
#ifndef MSG_NOSIGNAL
			int yes = 1;
			if (setsockopt (m_output_client_socket, SOL_SOCKET, SO_NOSIGPIPE, &yes, sizeof (yes)) < 0) {
				perror("Warning. Failed to set SO_NOSIGPIPE for socket");
			}
#endif
			gettimeofday(&m_pVideoMixer->m_start_master, NULL);
			/* Initialize it. */
			uint8_t				buffer [200];
			struct shm_commandbuf_type	*cb = (shm_commandbuf_type*)&buffer;
			int				x;

			/* Inform the client about which shared memory area to map. */
			memset (&buffer, 0, sizeof (buffer));
			cb->type = 1;
			cb->area_id = 1;
			cb->payload.new_shm_area.size = m_output_memory_size;
			cb->payload.new_shm_area.path_size = strlen (m_output_memory_handle_name) + 1;
			strcpy ((char *)&buffer [sizeof (*cb)], m_output_memory_handle_name);

			x = send (m_output_client_socket,
#ifdef WIN32
				(const char*)
#endif
				  (&buffer), sizeof (*cb) + strlen (m_output_memory_handle_name) + 1,
#ifdef MSG_NOSIGNAL
				  MSG_NOSIGNAL
#else
				  0
#endif
				);
			if (x > 0) {
				//dump_cb (m_pVideoMixer->m_block_size, "Mixer", "Client", cb, sizeof (*cb) + strlen (m_output_memory_handle_name) + 1);
			} else {
				/* Output disappeared. */
				OutputReset();
			}
			m_con_seq_no++;
		}
	}
}

int CVideoOutput::SetMode(u_int32_t mode) {
	if (m_system_socket_set) return 1;
	if (mode > 3) return -1;
	m_output_mode = mode;
	return 0;
}

int CVideoOutput::SetDelay(u_int32_t delay) {
	if (!delay) return -1;
	m_buffer_delay = delay;
	return 0;
}

int CVideoOutput::SetBuffers(u_int32_t buffers) {
	if (m_system_socket_set) return 1;
	if (buffers < 10 || buffers > 1000) return -1;
	m_buffer_allocate = buffers;
	return 0;
}

int CVideoOutput::SetFreeze(u_int32_t buffers) {
	//if (m_system_socket_set) return 1;
	m_output_freeze = buffers;
	return 0;
}

int CVideoOutput::ReadBuffersAhead()
{
	SDL_LockMutex(m_pOutputMutex);
	int n = m_buffer_count - m_buffer_delay;
	SDL_UnlockMutex(m_pOutputMutex);
	return n;
}

int CVideoOutput::OutputBufferReady4Output(struct output_memory_list_type* buf)
{
	if (!buf) return -1;

	// Check for direct output
	if (!m_output_mode) return SendOutBuffer(buf);
	//u_int32_t count, inuse;

	// Output mode > 0
	// Lock region
	SDL_LockMutex(m_pOutputMutex);
	  list_add_tail_withframe(m_pFrames_list, m_pFrames_tail_list, buf);
	  m_buffer_count++;
//fprintf(stderr, "SemPost buffer count %u\n", m_buffer_count);
	SDL_UnlockMutex(m_pOutputMutex);
	SDL_SemPost(m_pOutputSem);
	return 0;

}

/* Populate rfds and wrfd for the output subsystem. */
int CVideoOutput::SetFds (int maxfd, fd_set *rfds, fd_set *wfds)
{
	SDL_LockMutex(m_pOutputMutex);
	if (m_output_client_socket >= 0) {
		/* Got a client connection. Only care about that. */
		FD_SET (m_output_client_socket, rfds);
		if (m_output_client_socket > maxfd) {
			maxfd = m_output_client_socket;
		}
	} else {
		if (m_output_master_socket >= 0) {
			/* Wait for new connections. */
			FD_SET (m_output_master_socket, rfds);
			if (m_output_master_socket > maxfd) {
				maxfd = m_output_master_socket;
			}
		}
	}
	SDL_UnlockMutex(m_pOutputMutex);

	return maxfd;
}

int CVideoOutput::SendOutBuffer(struct output_memory_list_type* buf) {
	struct shm_commandbuf_type	cb;
	int n;
	if (!buf) return -1;
	if (m_pAverage_loop && m_output_mode < 2) {
		struct timeval time_now, time_delta;
		gettimeofday(&time_now, NULL);
		timersub(&time_now, &m_last_time, &time_delta);
//fprintf(stderr, "Time %.1lf\n", time2double(time_delta));
 		m_pAverage_loop->Add(time2double(time_delta));
		m_last_time = time_now;
	}

	//u_int32_t count, inuse;
	// Setup and transmit the buffer reference to the output socket.
	memset (&cb, 0, sizeof (cb));
	cb.type = 3;
	cb.area_id = 1;
	cb.payload.buffer.offset = buf->offset;
	cb.payload.buffer.bsize  = m_pVideoMixer->m_block_size;
	if ((n = send (m_output_client_socket,
#ifdef WIN32
		(const char*)
#endif
		(&cb), sizeof (cb),
#ifdef MSG_NOSIGNAL
		MSG_NOSIGNAL
#else
		0
#endif
		)) == sizeof(cb)) {
//fprintf(stderr, "Frame %u sent %d bytes\n", m_buffers_sent, n);
		if (0) { dump_cb (m_pVideoMixer->m_block_size, "Mixer", "Client",
				&cb, sizeof (cb));
		}
		m_buffers_sent++;
//if (m_buffers_sent < 50) fprintf(stderr, "F%04u Buffer sent\n", m_buffers_sent);
		return 0;
	}
//fprintf(stderr, " - Frame %u sent %d bytes\n", m_buffers_sent, n);
	if (n < 0) {
		// Check for EAGAIN or EWOULDBLOCK
		if (errno == EAGAIN) {
			fprintf(stderr, "output control socket returned EAGAIN. Dropping frame\n");
			buf->use--;
		} else if (errno == EWOULDBLOCK) {
			fprintf(stderr, "output control socket returned EWOULDBLOCK. Dropping frame\n");
			buf->use--;
		} else if (errno == EPIPE) {
			fprintf(stderr, "Output pipe connection broken. Resetting socket\n");
			OutputReset();
		} else {
			int en = errno;
			perror("Send returned error\n");
			// Client disappeared.
			fprintf (stderr, "Output receiver dissapered. errno %d."
				"Reset of Output buffer TX\n", en);
	
			// reset all buffers including current
			OutputReset();
			return 1;
		}
		return 0;
	} else {
		fprintf(stderr, "Failed to signal frame ready on system socket. "
			"Needed to send %d bytes, but sent %d bytes\n",
			(int) sizeof(cb), n);
	}
//fprintf(stderr, "CVideoOutput SendOutBuffer failed to send all cb\n");
	return -1;
}
