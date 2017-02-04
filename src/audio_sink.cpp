/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */


#ifndef WIN32
#include <unistd.h>
#else
#include <io.h>
#endif

#include <errno.h>
//#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>
#ifdef HAVE_MALLOC
#include <malloc.h>
#else
#include <stdlib.h>
#endif
#include <math.h>
//#include <stdlib.h>
//#include <sys/types.h>
//#include <math.h>
//#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

//#include "cairo/cairo.h"
#include "video_mixer.h"
//#include "video_text.h"
//#include "video_scaler.h"
//#include "add_by.h"
//#include "video_image.h"

#include "snowmix.h"
#include "audio_sink.h"
#include "snowmix_util.h"

CAudioSink::CAudioSink(CVideoMixer* pVideoMixer, u_int32_t max_sinks)
{
	m_max_sinks = 0;
	m_sink_count = 0;
	m_sinks = (audio_sink_t**) calloc(
		sizeof(audio_sink_t*), max_sinks);
	//for (unsigned int i=0; i< m_max_sinks; i++) m_sinks[i] = NULL;
	if (m_sinks) {
		m_max_sinks = max_sinks;
	}
	m_pVideoMixer = pVideoMixer;
	m_verbose = 0;
	m_animation = 0;

	m_monitor_fd = -1; //open("monitor_samples", O_RDWR | O_TRUNC);
	//if (m_monitor_fd < 0) fprintf(stderr, "ERROR in opening monitor write file\n");
}

CAudioSink::~CAudioSink()
{
	if (m_sinks) {
		for (unsigned int id=0; id<m_max_sinks; id++) DeleteSink(id);
		free(m_sinks);
		m_sinks = NULL;
	}
	m_max_sinks = 0;
	if (m_monitor_fd > -1) close(m_monitor_fd);
}

// audio sink ....
int CAudioSink::ParseCommand(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) return -1;

	//if (!m_pVideoMixer) return 0;
	// audio sink status
	if (!strncasecmp (str, "status ", 7)) {
		return set_sink_status(ctr, str+7);
	}
	// audio sink info
	else if (!strncasecmp (str, "info ", 5)) {
		return list_info(ctr, str+5);
	}
	// audio sink add silence <sink id> <ms>]
	else if (!strncasecmp (str, "add silence ", 12)) {
		return set_add_silence(ctr, str+12);
	}
	// audio sink drop <sink id> <ms>]
	else if (!strncasecmp (str, "drop ", 5)) {
		return set_drop(ctr, str+5);
	}
	// audio sink add [<sink id> [<sink name>]]
	else if (!strncasecmp (str, "add ", 4)) {
		return set_sink_add(ctr, str+4);
	}
	// audio sink start [<sink id>]
	else if (!strncasecmp (str, "start ", 6)) {
		return set_sink_start(ctr, str+6);
	}
	// audio sink stop [<sink id>]
	else if (!strncasecmp (str, "stop ", 5)) {
		return set_sink_stop(ctr, str+5);
	}
	// audio sink source [(feed | mixer) <sink id> <source id>]
	else if (!strncasecmp (str, "source ", 7)) {
		return set_sink_source(ctr, str+7);
	}
	// audio sink channels [<sink id> <channels>]
	else if (!strncasecmp (str, "channels ", 9)) {
		return set_sink_channels(ctr, str+9);
	}
	// audio sink rate [<sink id> <rate>]
	else if (!strncasecmp (str, "rate ", 5)) {
		return set_sink_rate(ctr, str+5);
	}
	// audio sink format [<sink id> (8 | 16 | 32 | 64) (signed | unsigned | float) ]
	else if (!strncasecmp (str, "format ", 7)) {
		return set_sink_format(ctr, str+7);
	}
	// audio sink file <sink id> <file name>
	else if (!strncasecmp (str, "file ", 5)) {
		return set_file(ctr, str+5);
	}
	// audio sink queue maxdelay <sink id> <max delay in ms
	else if (!strncasecmp (str, "queue maxdelay ", 15)) {
		return set_queue_maxdelay(ctr, str+15);
	}
	// audio sink queue <sink id> 
	else if (!strncasecmp (str, "queue ", 6)) {
		return set_queue(ctr, str+6);
	}
	//// audio sink monitor (on | off) <sink id> 
	//else if (!strncasecmp (str, "monitor ", 8)) {
	//	return set_monitor(ctr, str+8);
	//}
	// audio sink ctr isaudio <sink id> // set control channel to write out
	else if (!strncasecmp (str, "ctr isaudio ", 12)) {
		return set_ctr_isaudio(ctr, str+12);
	}
	// audio sink volume [<sink id> <volume>] // volume = 0..MAX_VOLUME
	else if (!strncasecmp (str, "volume ", 7)) {
		return set_volume(ctr, str+7);
	}
        // audio sink move volume [<sink id> <delta volume> <steps>]
        else if (!strncasecmp (str, "move volume ", 12)) {
                return set_move_volume(ctr, str+12);
        }
	// audio sink mute [(on | off) <sink id>]
	else if (!strncasecmp (str, "mute ", 5)) {
		return set_mute(ctr, str+5);
	}
	// audio sink verbose [<verbose level>]
	else if (!strncasecmp (str, "verbose ", 8)) {
		return set_verbose(ctr, str+8);
	}
	// audio sink help
	else if (!strncasecmp (str, "help ", 5)) {
		return list_help(ctr, str+5);
	}
	return 1;
}


#define AUDIOTYPE "audio sink"
#define AUDIOCLASS "CAudioSink"
// Query type 0=info 1=status 2=extended
// format	: format 
// ids		: list of IDs
// maxid	: max number of ID
// nextavail	: Next available ID
// id or ids	: listing each ID
char* CAudioSink::Query(const char* query, bool tcllist)
{
	u_int32_t type, maxid;

	if (!query) return NULL;
	while (isspace(*query)) query++;
	if (!(strncasecmp("info ", query, 5))) {
		query += 5; type = 0;
	}
       	else if (!(strncasecmp("status ", query, 7))) {
		query += 7; type = 1;
	}
       	else if (!(strncasecmp("extended ", query, 9))) {
		query += 9; type = 2;
	}
       	else if (!(strncasecmp("source info", query, 7))) {
		query += 7; type = 3;
	}
       	else if (!(strncasecmp("syntax", query, 6))) {
		return strdup("audio (feed | sink) (info | status | extended | syntax) (format | ids | maxid | nextavail | <id_list>)");
	} else return NULL;

	while (isspace(*query)) query++;
	maxid = MaxSinks();
	if (!strncasecmp(query, "format", 6)) {
		char* str = NULL;
		if (type == 0) {
			if (tcllist) str = strdup("id rate channels signed bytespersample delay");
			else str = strdup(AUDIOTYPE" <id> <rate> <channels> <format> <delay>");
		}
		else if (type == 1) {
			if (tcllist) str = strdup("id state starttime mute volume");
			else str = strdup(AUDIOTYPE" <id> <state> <starttime> <mute> <volume>");
		}
		else if (type == 2) {
			if (tcllist) str = strdup("id seq_no received silence dropped clipped rms");
			else str = strdup(AUDIOTYPE" <id> <seq_no> <received> <silence> <dropped> <clipped> <rms>");
		}
		else if (type == 3) {
			if (tcllist) str = strdup("id no_sources sources");
			else str = strdup(AUDIOTYPE" source <id> <no_sources> <sources>");
		}
		if (!str) fprintf(stderr, AUDIOCLASS"::Query failed to allocate memory\n");
		return str;
	}
	else if (!strncasecmp(query, "maxid", 5)) {
		int len = ((typeof(len))(log10((double)maxid))) + 3;	// in case of rounding error
		char* str = (char*) malloc(len);
		if (!str) fprintf(stderr, AUDIOCLASS"::Query failed to allocate memory\n");
		else if (maxid) sprintf(str, "%u", maxid-1);
		else {
			free(str);
			str=NULL;
		}
		return str;
	}
	else if (!strncasecmp(query, "nextavail", 9)) {
		int len = ((typeof(len))(log10((double)maxid))) + 3;	// in case of rounding error
		char* str = (char*) malloc(len);
		if (!str) {
			fprintf(stderr, AUDIOCLASS"::Query failed to allocate memory\n");
			return NULL;
		}
		*str = '\0';
		for (u_int32_t i = 0; i < maxid ; i++) {
			if (m_sinks[i]) {
				sprintf(str, "%u ", i);
				break;
			}
		}
		if (str && !(*str)) {
			free(str);
			str = NULL;
		}
		return str;
	}
	else if (!strncasecmp(query, "ids", 3)) {
		int len = ((typeof(len))(0.5 + log10((double)maxid))) + 1;
		len *= maxid;
		char* str = (char*) malloc(len+1);
		if (!str) fprintf(stderr, AUDIOCLASS"::Query failed to allocate memory\n");
		else {
			*str = '\0';
			char* p = str;
			for (u_int32_t i = 0; i < maxid ; i++) {
				if (m_sinks[i]) {
					sprintf(p, "%u ", i);
					while (*p) p++;
				}
			}
		}
		return str;
	}

	const char* str = query;
	int len = 1;
	while (*str) {
		u_int32_t from, to;
		const char* nextstr = GetIdRange(str, &from, &to, 0, maxid ? maxid-1 : 0);
		if (!nextstr) return NULL;
		while (from <= to) {
			if (!(m_sinks[from])) { from++ ; continue; }
			if (type == 0) {
				// id rate channels signed bytespersample delay");
				// 1234567890123456789012345678901234567890123456789012345678901234567890
				// audio sink 1234567890 1234567890 1234567890 unsigned 16 01234567890
				len += 68;
			}
			else if (type == 1) {
				// id state starttime mute volume
				// audio sink 01234567890 DISCONNECTED 1234567890.123 unmuted 0.12345,0.12345 .... 
				// 12345678901234567890123456789012345678901234567890123456789
				len += (60 + m_sinks[from]->channels * 9);
			}
			else if (type == 2) {
				// id seq_no received silence dropped clipped rms
				// 123456789012345678901234567890123456789012345678901234567890123456
				// audio sink 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890,1234567890....
				len += (68 + m_sinks[from]->channels * 11);
			}
			else if (type == 3) {
				// id no_sources sources
				// 1234567890 1234567890 {{feed 3}}
				// audio mixer source 1234567890 1234567890 feed,3<newline>
				// 123456789012345678901234567890123456789012
				len += (42+8*1);
			} else return NULL;
			from++;
		}
		while (isspace(*nextstr)) nextstr++;
		str = nextstr;
	}
	if (len < 2) {
		fprintf(stderr, AUDIOCLASS"::Query malformed query\n");
		return NULL;
	}
	char* retstr = (char*) malloc(len);
	if (!retstr) {
		fprintf(stderr, AUDIOCLASS"::Query failed to allocate memory\n");
		return NULL;
	}

	str = query;
	char* pstr = retstr;
	*pstr = '\0';
	while (*str) {
		u_int32_t from, to;
		const char* nextstr = GetIdRange(str, &from, &to, 0, maxid ? maxid-1 : 0);
		if (!nextstr) {
            free(retstr);
            return NULL;
        }
		while (from <= to) {
			if (!m_sinks[from]) {
				from++;
				continue;
			}
			audio_sink_t* p = m_sinks[from];
			if (type == 0) {
				// id rate channels signed bytespersample delay
				sprintf(pstr, (tcllist ? "{%u %u %u %s %u %u} " : AUDIOTYPE" %u %u %u %s %u %u\n"), 
					from, p->rate, p->channels,
					p->is_signed ? "signed" : "unsigned",
					p->bytespersample, p->delay);
			}
			else if (type == 1) {
				// id state starttime mute volume......
#ifdef HAVE_MACOSX                  // suseconds_t is long int on OS X 10.9
				sprintf(pstr, "%s%u %s %ld.%06d %smuted %s", tcllist ? "{" : AUDIOTYPE,
                
#else                               // suseconds_t is unsigned int on Linux
				sprintf(pstr, "%s%u %s %ld.%06lu %smuted %s", tcllist ? "{" : AUDIOTYPE,
#endif
                        from, feed_state_string(p->state), p->start_time.tv_sec, p->start_time.tv_usec,
                        (p->mute ? "" : "un"), (tcllist ? "{" : ""));

				while (*pstr) pstr++;
				float* pvol = p->pVolume;
				if (pvol) {
					for (unsigned int n = 0 ; n < p->channels ; n++) {
						sprintf(pstr, "%.5f%s", *pvol,
							n+1 < p->channels ? (tcllist ? " " : ",") :
						  	(tcllist ? "}} " : "\n"));
						pvol += 1;
						while (*pstr) pstr++;
					}
				} else {
					sprintf(pstr, "%s", (tcllist ? "} " : "\n"));
					while (*pstr) pstr++;
				}
			}
			else if (type == 2) {
				// id seq_no received silence dropped clipped rms
				sprintf(pstr, "%s%u %u %u %u %u %u %s",
					tcllist ? "{" : AUDIOTYPE,
					from, p->buf_seq_no, p->samples_rec, p->silence, p->dropped, p->clipped,
					tcllist ? "{" : "");
				while (*pstr) pstr++;
				u_int64_t* prms = p->rms;
				if (prms) {
					for (unsigned int n = 0 ; n < p->channels ; n++) {
						sprintf(pstr, "%.1lf%s",
							(double) (sqrt((double)(*prms))/327.67),
							n+1 < p->channels ? (tcllist ? " " : ",") :
						  	(tcllist ? "}} " : "\n"));
						prms += 1;
						while (*pstr) pstr++;
					}
				} else {
					sprintf(pstr, "%s", (tcllist ? "} " : "\n"));
					while (*pstr) pstr++;
				}
			}
			else if (type == 3) {
				// id no_sources sources
				sprintf(pstr, "%s%u %u %s",
					tcllist ? "{" : "audio sink source ",
					from, 1,	// m_sinks[from]->no_sources,
					tcllist ? "{" : "");
				while (*pstr) pstr++;
				audio_sink_t* p = m_sinks[from];
				sprintf(pstr, "%s%s%s%u%s",
					tcllist ? "{" : "",
					p->source_type == 1 ? "feed" :
					  p->source_type == 1 ? "mixer" : "none",
					tcllist ? " " : ",",
					p->source_id,
					tcllist ?  "}}" : "\n");
				while (*pstr) pstr++;
			}
			while (*pstr) pstr++;
			from++;
		}
		while (isspace(*nextstr)) nextstr++;
		str = nextstr;
	}
	return retstr;
}

// list audio sink help
int CAudioSink::list_help(struct controller_type* ctr, const char* str)
{
	if (m_pVideoMixer && m_pVideoMixer->m_pController)
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Commands:\n"
			MESSAGE"audio sink add [<sink id> [<sink name>]]\n"
			MESSAGE"audio sink add silence <sink id> <ms>\n"
			MESSAGE"audio sink channels [<sink id> <channels>]\n"
			MESSAGE"audio sink ctr isaudio <sink id> "
				"// set control channel to write out audio\n"
			MESSAGE"audio sink drop <sink id> <ms>]\n"
			MESSAGE"audio sink file <sink id> <file name>\n"
			MESSAGE"audio sink format [<sink id> (8 | 16 | 24 | 32 | 64) "
				"(signed | unsigned | float) ]\n"
			MESSAGE"audio sink info\n"
			MESSAGE"audio sink move volume [<sink id> <delta volume> <delta steps>]\n"
			MESSAGE"audio sink mute [(on | off) <sink id>]\n"
			MESSAGE"audio sink queue <sink id> \n"
			MESSAGE"audio sink queue maxdelay <sink id> <max delay in ms\n"
			MESSAGE"audio sink rate [<sink id> <rate>]\n"
			MESSAGE"audio sink source [(feed | mixer) <sink id> <source id>]\n"
			MESSAGE"audio sink start [<sink id>]\n"
			MESSAGE"audio sink status\n"
			MESSAGE"audio sink stop [<sink id>]\n"
			MESSAGE"audio sink verbose [<verbose level>]\n"
			MESSAGE"audio sink volume [<sink id> <volume>] // volume = 0..MAX_VOLUME\n"
			MESSAGE"audio sink help\n"
			MESSAGE"\n");
	return 0;
}


// audio sink info
int CAudioSink::list_info(struct controller_type* ctr, const char* str)
{
	if (!ctr || !m_pVideoMixer || !m_pVideoMixer->m_pController || !m_sinks) return 1;
	m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS" audio sink info\n"
		STATUS" audio sinks       : %u\n"
		STATUS" max audio sinks   : %u\n"
		STATUS" verbose level     : %u\n",
		m_sink_count,
		m_max_sinks,
		m_verbose
		);
	if (m_sink_count) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS" audio sink id : state, rate, channels, bytespersample, "
			"signess, volume, mute, buffersize, delay, queues\n");
		for (u_int32_t id=0; id < m_max_sinks; id++) {
			if (!m_sinks[id]) continue;
			u_int32_t queue_count = 0;
			audio_queue_t* pQueue = m_sinks[id]->pAudioQueues;
			while (pQueue) {
				queue_count++;
				pQueue = pQueue->pNext;
			}
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				STATUS" - audio sink %u : %s, %u, %u, %u, %ssigned, %s", id,
				feed_state_string(m_sinks[id]->state),
				m_sinks[id]->rate,
				m_sinks[id]->channels,
				m_sinks[id]->bytespersample,
				m_sinks[id]->is_signed ? "" : "un",
				m_sinks[id]->pVolume ? "" : "0,");
                        if (m_sinks[id]->pVolume) {
                                float* p = m_sinks[id]->pVolume;
                                for (u_int32_t i=0 ; i < m_sinks[id]->channels; i++, p++)
                                        m_pVideoMixer->m_pController->controller_write_msg (ctr,
                                                "%.3f,", *p);
                        }
                        m_pVideoMixer->m_pController->controller_write_msg (ctr, " %smuted, %u, %u, %u\n",
				m_sinks[id]->mute ? "" : "un",
				m_sinks[id]->buffer_size,
				m_sinks[id]->delay,
				queue_count);
		}
	}
	m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
	return 0;
}
// audio sink verbose [<level>] // set control channel to write out
int CAudioSink::set_verbose(struct controller_type* ctr, const char* str)
{
	int n;
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (m_verbose) m_verbose = 0;
		else m_verbose = 1;
		if (m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS"audio sink verbose %s\n"STATUS"\n", m_verbose ? "on" : "off");
		return 0;
	}
	while (isspace(*str)) str++;
	n = sscanf(str, "%u", &m_verbose);
	if (n != 1) return -1;
	if (m_pVideoMixer && m_pVideoMixer->m_pController)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
		MESSAGE"audio sink verbose level set to %u for CAudioSink\n", m_verbose);
	return 0;
}

// audio sink status
int CAudioSink::set_sink_status(struct controller_type* ctr, const char* str)
{
	//u_int32_t id;
	//int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_sinks) return 1;
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS"sink_id : state samples samplespersecond avg_samplespersecond silence dropped clipped queue rms");
		for (unsigned id=0 ; id < m_max_sinks; id++) if (m_sinks[id]) {
			audio_queue_t* pQueue = m_sinks[id]->pAudioQueue;
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				"\n"STATUS"audio sink %u : %s %u %.0lf %.0lf %u %u %u %s",
				id, feed_state_string(m_sinks[id]->state),
				m_sinks[id]->samples_rec,
				m_sinks[id]->samplespersecond,
				m_sinks[id]->total_avg_sps/MAX_AVERAGE_SAMPLES,
				m_sinks[id]->silence,
				m_sinks[id]->dropped,
				m_sinks[id]->clipped,
                                pQueue ? "" : "0 ");
                        while (pQueue) {
                                m_pVideoMixer->m_pController->controller_write_msg (ctr, "%u%s",
                                        1000*pQueue->sample_count/m_sinks[id]->calcsamplespersecond,
                                        pQueue->pNext ? "," : " ");
                                pQueue = pQueue->pNext;
                        }

			for (u_int32_t i=0; i < m_sinks[id]->channels; i++) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%.1lf%s", (double) (sqrt((double)(m_sinks[id]->rms[i]))/327.67),
					i+1 < m_sinks[id]->channels ? "," : "");
			}

		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"\n"STATUS"\n");
		return 0;

	}
	return -1;
}

// audio sink ctr isaudio <sink id> // set control channel to write out
int CAudioSink::set_ctr_isaudio(struct controller_type* ctr, const char* str)
{

	u_int32_t id;
	int n;
	// make some checks
	if (!str || !ctr) return -1;
	if (!m_sinks || ctr->write_fd == -1) return 1;

	// skip whitespace and find the sink id
	while (isspace(*str)) str++;
	if (sscanf(str, "%u", &id) != 1 || id < 1 || !m_sinks[id]) return -1;
	ctr->is_audio = true;
	ctr->sink_id = id;
	m_sinks[id]->sink_fd = ctr->write_fd;
	if (m_verbose) fprintf(stderr,
		"Frame %u - controller output changed to audio sink %u. Starting audio sink %u\n",
				SYSTEM_FRAME_NO, id, id);
	if ((n = StartSink(id))) {
		if (m_verbose) fprintf(stderr, "audio sink %u failed to start sink\n", id);
		ctr->is_audio = false;
		ctr->sink_id = id;
		m_sinks[id]->sink_fd = -1;
		return n;
	}
	//gettimeofday(&(m_sinks[id]->start_time), NULL);
	//SetSinkBufferSize(id);
	return 0;
}

// audio sink queue maxdelay <sink id> <max delay>
int CAudioSink::set_queue_maxdelay(struct controller_type* ctr, const char* str)
{
	u_int32_t id, maxdelay;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		for (unsigned id=0 ; id < m_max_sinks; id++) {
			
			if (m_sinks[id] && m_sinks[id]->pAudioQueue) {
				//u_int32_t bytespersecond = m_sinks[id]->bytespersample*
				//	m_sinks[id]->channels*m_sinks[id]->rate;
				u_int32_t n_total = sizeof(m_sinks[id]->queue_samples)/sizeof(m_sinks[id]->queue_samples[0])-1;
				int32_t total = m_sinks[id]->queue_samples[n_total];
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS"audio sink %2u queue : average buffer bytes %d = %d ms\n"
					STATUS"                     : current buffer bytes %u = %u ms\n"
					STATUS"                     : buffers %u\n"
					STATUS"                     : maxdelay set to %u bytes = %u ms\n",
					id, total/n_total, (1000*total)/(n_total*m_sinks[id]->bytespersecond),
					m_sinks[id]->pAudioQueue->bytes_count,
					(1000*m_sinks[id]->pAudioQueue->bytes_count)/m_sinks[id]->bytespersecond,
					m_sinks[id]->pAudioQueue->buf_count,
					m_sinks[id]->pAudioQueue->max_bytes,
					1000 * m_sinks[id]->pAudioQueue->max_bytes / m_sinks[id]->bytespersecond);
			}
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		return 0;
	}

	// Get the sink id.
	if (sscanf(str, "%u %u", &id, &maxdelay) != 2) return -1;
	//maxdelay = maxdelay*m_sinks[id]->bytespersample*m_sinks[id]->channels*m_sinks[id]->rate/1000;
	maxdelay = maxdelay*m_sinks[id]->bytespersecond/1000;
	return SetQueueMaxDelay(id, maxdelay);
}

// audio sink queue <sink id> 
int CAudioSink::set_queue(struct controller_type* ctr, const char* str)
{
	u_int32_t id;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		for (id=0 ; id < m_max_sinks; id++) {
			
			if (m_sinks[id] && m_sinks[id]->pAudioQueue) {
				//u_int32_t bytespersecond = m_sinks[id]->bytespersample*
			//		m_sinks[id]->channels*m_sinks[id]->rate;
				u_int32_t n_total = sizeof(m_sinks[id]->queue_samples)/sizeof(m_sinks[id]->queue_samples[0])-1;
				int32_t total = m_sinks[id]->queue_samples[n_total];
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS"audio sink %2u queue : average buffer bytes %d = %d ms\n"
					STATUS"                     : current buffer bytes %u = %u ms\n"
					STATUS"                     : buffers %u\n"
					STATUS"                     : maxdelay set to %u bytes = %u ms\n",
					id, total/n_total, (1000*total)/(n_total*m_sinks[id]->bytespersecond),
					m_sinks[id]->pAudioQueue->bytes_count,
					(1000*m_sinks[id]->pAudioQueue->bytes_count)/m_sinks[id]->bytespersecond,
					m_sinks[id]->pAudioQueue->buf_count,
					m_sinks[id]->pAudioQueue->max_bytes,
					1000 * m_sinks[id]->pAudioQueue->max_bytes / m_sinks[id]->bytespersecond);
				/*m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS"audio sink %u queue : buffers %u bytes %u maxdelay "
					"%u ms current delay %u ms\n", id,
					m_sinks[id]->pAudioQueue->buf_count,
					m_sinks[id]->pAudioQueue->bytes_count,
					1000 * m_sinks[id]->pAudioQueue->max_bytes / m_sinks[id]->bytespersecond,
					1000 * m_sinks[id]->pAudioQueue->bytes_count / m_sinks[id]->bytespersecond);
*/
			}
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		return 0;
	}

	// Get the sink id.
	if (scanf(str, "on %u", &id) != 1) return -1;

	// Do something
	return 0;
}

/*
// audio sink monitor (on | off) <sink id> 
int CAudioSink::set_monitor(struct controller_type* ctr, const char* str)
{
	u_int32_t id;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_sinks) return 1;
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"MSG: Audio monitor\n");
		return 0;

	}
	// Get the sink id.
	if ((n = sscanf(str, "on %u", &id)) != 1) return -1;
fprintf(stderr, "Monitor on\n");
	m_sinks[id]->monitor_queue = AddQueue(id);
	if (m_sinks[id]->monitor_queue) {
		MonitorSink(id);
		SDL_PauseAudio(0);
	} else fprintf(stderr, "Failed to set audio monitor\n");
	return m_sinks[id]->monitor_queue ? 0 : 1;
}
*/

// audio sink file [<sink id> <file name>]
int CAudioSink::set_file(struct controller_type* ctr, const char* str)
{
	u_int32_t id;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_sinks) return 1;
		for (unsigned n=0 ; n < m_max_sinks; n++)
			if (m_sinks[n] && m_sinks[n]->file_name)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				STATUS"audio sink %2d file name %s\n", n, m_sinks[n]->file_name);
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		return 0;

	}
	// Get the sink id.
	if (sscanf(str, "%u", &id) != 1 || id < 1) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	if (!(*str)) return -1;
	return SetFile(id, str);
}

// audio sink move volume [<sink_id> <delta volume> <steps>]
int CAudioSink::set_move_volume(struct controller_type* ctr, const char* str)
{
	u_int32_t id, steps;
	float volume_delta;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_sinks) return 1;
		for (id=0 ; id < m_max_sinks; id++) {
			if (!m_sinks[id]) continue;
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				STATUS"audio sink %2d volume %s", id,
				m_sinks[id]->pVolume ? "" : "0");
			if (m_sinks[id]->pVolume) {
				float* p = m_sinks[id]->pVolume;
				for (u_int32_t i=0 ; i < m_sinks[id]->channels; i++, p++) {
					m_pVideoMixer->m_pController->controller_write_msg (ctr,
						"%.3f%s", *p, i+1 < m_sinks[id]->channels ? "," : "");
				}
			}
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				"%smuted delta %.4f steps %u\n",
				m_sinks[id]->mute ? " " : " un",
				m_sinks[id]->volume_delta, m_sinks[id]->volume_delta_steps);
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS"\n");
		return 0;

	}

	// Get the sink id.
	if ((n = sscanf(str, "%u %f %u", &id, &volume_delta, &steps)) == 3) {
		n = SetMoveVolume(id, volume_delta, steps);
		if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio sink %u delta volume %.4f steps %u\n",
				id, volume_delta, steps);
		return n;
	}
	else return -1;
}

// audio sink volume <sink id> <volume>
int CAudioSink::set_volume(struct controller_type* ctr, const char* str)
{
	u_int32_t id, channel;
	float volume;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_sinks) return 1;
		for (id=0 ; id < m_max_sinks; id++) if (m_sinks[id]) {
                        if (!m_sinks[id]) continue;
                        m_pVideoMixer->m_pController->controller_write_msg (ctr,
                                STATUS"audio sink %2d volume %s", id,
                                m_sinks[id]->pVolume ? "" : "0");
                        if (m_sinks[id]->pVolume) {
                                float* p = m_sinks[id]->pVolume;
                                for (u_int32_t i=0 ; i < m_sinks[id]->channels; i++, p++) {
                                        m_pVideoMixer->m_pController->controller_write_msg (ctr,
                                                "%.3f%s", *p, i+1 < m_sinks[id]->channels ? "," : "");
                                }
                        }
                        m_pVideoMixer->m_pController->controller_write_msg (ctr, "%s\n",
				m_sinks[id]->mute ? " muted" : "");
                }
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS"\n");
		return 0;

	}

	// Get the sink id.
        if ((n = sscanf(str, "%u", &id)) == 1) {
                while (*str && !isspace(*str)) str++;
                while (isspace(*str)) str++;
                const char* volume_list = str;
                channel=0;
                if (isdigit(*str) || (*str == '-' && isspace(str[1]))) {
                        while (*str) {
                                if ((*str == '-' && (isspace(str[1]) || !str[1]))) {
                                        str++;
                                        channel++;
                                        while (isspace(*str)) str++;
                                        continue;
                                }
                                if ((n = sscanf(str, "%f", &volume)) == 1) {
                                        if ((n=SetVolume(id,channel++,volume))) return n;
                                        while (*str && !isspace(*str)) str++;
                                        while (isspace(*str)) str++;
                                } else return -1;
                        }
                } else return -1;
                if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
                        m_pVideoMixer->m_pController->controller_write_msg (ctr,
                                MESSAGE"audio sink %u volume %s\n", id, volume_list);
                return 0;
        }
        else return -1;
}

// audio sink mute [(on | off) <sink id>]
int CAudioSink::set_mute(struct controller_type* ctr, const char* str)
{
	u_int32_t id;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_sinks) return 1;
		for (id=0 ; id < m_max_sinks; id++) if (m_sinks[id]) {
                        if (!m_sinks[id]) continue;
                        m_pVideoMixer->m_pController->controller_write_msg (ctr,
                                STATUS"audio sink %2d volume %s", id,
                                m_sinks[id]->pVolume ? "" : "0");
                        if (m_sinks[id]->pVolume) {
                                float* p = m_sinks[id]->pVolume;
                                for (u_int32_t i=0 ; i < m_sinks[id]->channels; i++, p++) {
                                        m_pVideoMixer->m_pController->controller_write_msg (ctr,
                                                "%.3f%s", *p, i+1 < m_sinks[id]->channels ? "," : "");
                                }
                        }
                        m_pVideoMixer->m_pController->controller_write_msg (ctr, "%s\n",
				m_sinks[id]->mute ? " muted" : "");
                }
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS"\n");
		return 0;

	}

	// Get the sink id and mute state
	if ((n = sscanf(str, "on %u", &id)) == 1) {
		n = SetMute(id, true);
		if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio sink %u mute\n", id);
		return n;
	}
	else if ((n = sscanf(str, "off %u", &id)) == 1) {
		n = SetMute(id, false);
		if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio sink %u unmute\n", id);
		return n;
	}
	else return -1;
}

// audio sink add silence <sink id> <ms>
int CAudioSink::set_add_silence(struct controller_type* ctr, const char* str)
{
	u_int32_t id, ms;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	// Get the sink id.
	if ((n = sscanf(str, "%u %u", &id, &ms)) == 2) {
		if (m_sinks && m_sinks[id] && (m_sinks[id]->state == RUNNING ||
			m_sinks[id]->state == STALLED)) {
				if (m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
					m_pVideoMixer->m_pController->controller_write_msg (ctr,
						MESSAGE"Frame %u - audio sink %u adding %u ms of silence per channel\n",
						SYSTEM_FRAME_NO, id, ms);
			m_sinks[id]->write_silence_bytes = ms * m_sinks[id]->bytespersample *
				m_sinks[id]->channels*m_sinks[id]->rate/1000;
			return 0;
		}
	}
	return -1;
}

// audio sink drop output <sink id> <ms>
int CAudioSink::set_drop(struct controller_type* ctr, const char* str)
{
	u_int32_t id, ms;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	// Get the sink id.
	if ((n = sscanf(str, "%u %u", &id, &ms)) == 2) {
		if (m_sinks && m_sinks[id] && (m_sinks[id]->state == RUNNING ||
			m_sinks[id]->state == STALLED)) {
			if (m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					MESSAGE"Frame %u - audio sink %u dropping %u ms of silence per channel\n",
					SYSTEM_FRAME_NO, id, ms);
			m_sinks[id]->drop_bytes = ms * sizeof(int32_t) *
				m_sinks[id]->channels * m_sinks[id]->rate/1000;
			return 0;
		}
	}
	return -1;
}

// audio sink source [(feed | mixer) <sink id> <source id>]
int CAudioSink::set_sink_source(struct controller_type* ctr, const char* str)
{
	u_int32_t sink_id, source_id;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_sinks) return 1;
		for (u_int32_t id = 0; id < m_max_sinks ; id++)
			if (m_sinks[id])
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS"audio sink %u source %s %u\n", id,
					m_sinks[id]->source_type == 1 ? "feed" :
					  m_sinks[id]->source_type == 2 ? "mixer" : "unknown",
					m_sinks[id]->source_id);
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS"\n");
		return 0;

	}
	int n = -1;
	// Get the sink id and source
	if (sscanf(str, "feed %u %u", &sink_id, &source_id) == 2) {
		n = SinkSource(sink_id, source_id, 1);
		if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio sink %u sourced by audio feed %u\n",
				sink_id, source_id);
	} else if (sscanf(str, "mixer %u %u", &sink_id, &source_id) == 2) {

		n = SinkSource(sink_id, source_id, 2);
		if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio sink %u sourced by audio mixer %u\n",
				sink_id, source_id);
	}
	return n;
}


// audio sink stop [<sink id>] // Stop a sink
int CAudioSink::set_sink_stop(struct controller_type* ctr, const char* str)
{
	u_int32_t id;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_sinks) return 1;
		for (id = 0; id < m_max_sinks ; id++)
			if (m_sinks[id])
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS"audio sink %u state %s\n", id,
					feed_state_string(m_sinks[id]->state));
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS"\n");
		return 0;

	}
	// Get the sink id.
	if ((n = sscanf(str, "%u", &id)) != 1) return -1;
	return StopSink(id);
}

// audio sink start [<sink id>] // Start a sink
int CAudioSink::set_sink_start(struct controller_type* ctr, const char* str)
{
	u_int32_t id;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_sinks) return 1;
		for (id = 0; id < m_max_sinks ; id++)
			if (m_sinks[id])
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS"audio sink %u state %s\n", id,
					feed_state_string(m_sinks[id]->state));
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS"\n");
		return 0;

	}
	// Get the sink id.
	if ((n = sscanf(str, "%u", &id)) != 1) return -1;
	return StartSink(id);
}

// audio sink add [<sink id> [<sink name>]] // Create/delete/list an audio sink
int CAudioSink::set_sink_add(struct controller_type* ctr, const char* str)
{
	u_int32_t id;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_sinks) return 1;
		for (id = 0; id < m_max_sinks ; id++)
			if (m_sinks[id])
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS"audio sink %u <%s>\n", id,
					m_sinks[id]->name ? m_sinks[id]->name : "");
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS"\n");
		return 0;

	}
	// Get the sink id.
	if ((n = sscanf(str, "%u", &id)) != 1) return -1;
	while (isdigit(*str)) str++; while (isspace(*str)) str++;

	// Now if eos, only id was given and we delete the sink
	if (!(*str)) {
		if ((n = DeleteSink(id))) return n;
		if (m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio sink %u deleted\n");
	} else {

		// Now we know there is more after the id and we can create the feed
		if ((n = CreateSink(id))) return n;
		m_sink_count++;
		if ((n = SetSinkName(id, str))) {
			DeleteSink(id);
			return n;
		}
		if (m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio sink %u <%s> added\n", id, m_sinks[id]->name);
	}
	return n;
}

// audio sink channels [<sink id> <channels>]] // Set number of channels for an audio sink
int CAudioSink::set_sink_channels(struct controller_type* ctr, const char* str)
{
	u_int32_t id, channels;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	// Check to see if we should list channels
	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_sinks) return 1;
		for (id = 0; id < m_max_sinks ; id++)
			if (m_sinks[id])
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS"audio sink %u channels %u\n", id,
					m_sinks[id]->channels);
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		return 0;
	}
	// Get the sink id and channels
	if ((n = sscanf(str, "%u %u", &id, &channels)) != 2) return -1;
	if (!(n = SetSinkChannels(id, channels)))
		if (m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio sink %u channels %u\n", id, channels);
	return n;
}

// audio sink rate [<sink id> <rate in Hz>]] // Set sample rate for an audio sink
int CAudioSink::set_sink_rate(struct controller_type* ctr, const char* str)
{
	u_int32_t id, rate;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	// Check to see if we should list rate
	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_sinks) return 1;
		for (id = 0; id < m_max_sinks ; id++)
			if (m_sinks[id])
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS"audio sink %u rate %u Hz\n", id,
					m_sinks[id]->rate);
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		return 0;
	}
	// Get the sink id and rate
	if ((n = sscanf(str, "%u %u", &id, &rate)) != 2) return -1;
	if (!(n = SetSinkRate(id, rate)))
		if (m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio sink %u rate %u\n", id, rate);
	return n;
}

// audio sink format [<sink id> (8 | 16 | 32 | 64) (signed | unsigned | float) ]
// Set format for audio sink
int CAudioSink::set_sink_format(struct controller_type* ctr, const char* str)
{
	u_int32_t id, bits;
	int n;
	bool is_signed = false;

	if (!str) return -1;
	while (isspace(*str)) str++;

	// Check to see if we should list rate
	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_sinks) return 1;
		for (id = 0; id < m_max_sinks ; id++)
			if (m_sinks[id])
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS"audio sink %u %s and %u bytes per "
					"sample\n", id,
					m_sinks[id]->is_signed ? "signed" : "unsigned",
					m_sinks[id]->bytespersample);
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		return 0;
	}
	// Get the sink id.
	n = sscanf(str, "%u %u", &id, &bits);
	if (n != 2) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	if (strncasecmp(str, "signed ", 8) == 0) {
		is_signed = true;
		if (bits != 8 && bits != 16 && bits != 24) return -1;
	} else if (strncasecmp(str, "unsigned ", 10) == 0) {
		is_signed = false;
		if (bits != 8 && bits != 16 && bits != 24) return -1;
	} else if (strncasecmp(str, "float ", 6) == 0) {
		is_signed = true;
		if (bits != 32 && bits != 64) return -1;
	} else return -1;
	if (!(n = SetSinkFormat(id, bits >> 3, is_signed)))
		if (m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio sink %u sample is %u bytes per "
				"sample and %s\n", id, bits >> 3, is_signed ?
				"signed" : "unsigned");
	return n;
}


int CAudioSink::CreateSink(u_int32_t id)
{
	if (id >= m_max_sinks || !m_sinks) return -1;
	audio_sink_t* p = m_sinks[id];
	if (!p) p = (audio_sink_t*) calloc(sizeof(audio_sink_t),1);
	if (!p) return -1;
	m_sinks[id]		= p;
	p->name			= NULL;
	p->id			= id;
	p->state		= SETUP;
	p->previous_state	= SETUP;
	p->channels		= 0;
	p->rate			= 0;
	p->is_signed		= true;
	p->bytespersample	= 0;
	p->bytespersecond	= 0;
	p->calcsamplespersecond	= 0;
	p->mute			= true;
	p->delay		= 0;
	p->start_time.tv_sec	= 0;
	p->start_time.tv_usec	= 0;
	p->buffer_size		= 0;
	p->pAudioQueues		= NULL;
	p->pOutQueue		= NULL;
	p->monitor_queue	= NULL;
	p->buf_seq_no		= 0;
	p->pVolume		= NULL;
	p->file_name		= NULL;
	p->sink_fd		= -1;
	p->sample_count		= 0;
	p->write_silence_bytes	= 0;
	p->drop_bytes		= 0;
	p->no_rate_limit	= true;

	p->samples_rec		= 0;
	p->silence		= 0;
	p->dropped		= 0;
	p->clipped		= 0;
	p->rms			= NULL;


	p->source_type		= 0;
	p->source_id		= 0;
	p->pAudioQueue		= NULL;

	p->update_time.tv_sec   = 0;
        p->update_time.tv_usec  = 0;
        p->samplespersecond     = 0.0;
        p->last_samples         = 0;
        p->total_avg_sps        = 0.0;
        for (p->avg_sps_index = 0; p->avg_sps_index < MAX_AVERAGE_SAMPLES; p->avg_sps_index++)
                p->avg_sps_arr[p->avg_sps_index] = 0.0;
        p->avg_sps_index        = 0;

	
	return 0;
}

// Delete a sink and allocated memory associated.
int CAudioSink::DeleteSink(u_int32_t id)
{
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id] ||
		m_sinks[id]->state == RUNNING) return -1;
	m_sink_count--;
	if (m_sinks[id]->name) free(m_sinks[id]->name);
	if (m_sinks[id]->file_name) free(m_sinks[id]->file_name);
        if (m_sinks[id]->pVolume) free(m_sinks[id]->pVolume);
        if (m_sinks[id]->rms) free(m_sinks[id]->rms);
	if (m_sinks[id]->sink_fd > -1) close(m_sinks[id]->sink_fd);
	if (m_sinks[id]) free(m_sinks[id]);
	m_sinks[id] = NULL;
	if (m_verbose) fprintf(stderr, "Deleted audio sink %u\n",id);
	return 0;
}

int CAudioSink::SetSinkName(u_int32_t id, const char* sink_name)
{
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id] || !sink_name)
		return -1;
	m_sinks[id]->name = strdup(sink_name);
	trim_string(m_sinks[id]->name);			// No memory leak.
	return m_sinks[id]->name ? 0 : -1;
}

int CAudioSink::SetSinkChannels(u_int32_t id, u_int32_t channels)
{
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id] ||
		m_sinks[id]->state == RUNNING || m_sinks[id]->state == STALLED)
		return -1;
	m_sinks[id]->channels = channels;
	if (m_sinks[id]->pVolume) free(m_sinks[id]->pVolume);
	if (m_sinks[id]->rms) free(m_sinks[id]->rms);
        m_sinks[id]->pVolume = (float*) calloc(channels,sizeof(float));
	m_sinks[id]->rms = (u_int64_t*) calloc(channels,sizeof(u_int64_t));	// No memory leak
        if (!m_sinks[id]->pVolume || !m_sinks[id]->rms) {
                if (m_sinks[id]->rms) free(m_sinks[id]->rms);
                if (m_sinks[id]->pVolume) free(m_sinks[id]->pVolume);
                m_sinks[id]->rms = NULL;
		m_sinks[id]->pVolume = NULL;
                return 1;
        }
        for (u_int32_t i=0 ; i < channels ; i++) m_sinks[id]->pVolume[i] = DEFAULT_VOLUME_FLOAT;


	SetSinkBufferSize(id);
	return 0;
}

int CAudioSink::SetSinkRate(u_int32_t id, u_int32_t rate)
{
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id] ||
		m_sinks[id]->state == RUNNING || m_sinks[id]->state == STALLED)
		return -1;
	m_sinks[id]->rate = rate;
	SetSinkBufferSize(id);
	return 0;
}

int CAudioSink::SetSinkFormat(u_int32_t id, u_int32_t bytes, bool is_signed)
{
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id] ||
		m_sinks[id]->state == RUNNING || m_sinks[id]->state == STALLED)
		return -1;
	m_sinks[id]->bytespersample = bytes;
	//m_sinks[id]->bytespersecond = bytes*m_sinks[id]->channels*m_sinks[id]->rate;
	m_sinks[id]->is_signed = is_signed;
	SetSinkBufferSize(id);
	return 0;
}

int CAudioSink::SetSinkBufferSize(u_int32_t id)
{
	// make some checks
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id]) return -1;
	if (!m_pVideoMixer || m_pVideoMixer->m_frame_rate == 0 || m_sinks[id]->channels < 1 ||
		m_sinks[id]->bytespersample < 1 || m_sinks[id]->rate < 1) {
		if (m_sinks[id]) {
			m_sinks[id]->buffer_size = 0;
			m_sinks[id]->state = SETUP;
		}
		return -1;
	}

	m_sinks[id]->calcsamplespersecond = m_sinks[id]->channels*m_sinks[id]->rate;
	m_sinks[id]->bytespersecond = m_sinks[id]->bytespersample*
		m_sinks[id]->calcsamplespersecond;
	int32_t size = (typeof(size))(m_sinks[id]->channels*m_sinks[id]->bytespersample*
		m_sinks[id]->rate/ m_pVideoMixer->m_frame_rate);
	int32_t n = 0;
	for (n=0 ; size ; n++) size >>=  1;
	m_sinks[id]->buffer_size = (1 << n);
	if (m_verbose) fprintf(stderr, "audio sink %u buffersize set to %u bytes\n",
		id, m_sinks[id]->buffer_size);
	if (m_sinks[id]->buffer_size > 0) {
		if (m_verbose && m_sinks[id]->state != READY)
			fprintf(stderr, "audio sink %u changed state from %s to %s\n",
				id, feed_state_string(m_sinks[id]->state),
				feed_state_string(READY));
		m_sinks[id]->state = READY;
	} else {
		if (m_sinks[id]->state != SETUP && m_verbose)
			fprintf(stderr, "audio sink %u changed state from %s to %s\n",
				id, feed_state_string(m_sinks[id]->state),
				feed_state_string(SETUP));
		m_sinks[id]->state = SETUP;
	}
	return m_sinks[id]->buffer_size;
}

audio_buffer_t* CAudioSink::GetEmptyBuffer(u_int32_t id, u_int32_t* size)
{
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id] || !m_sinks[id]->buffer_size)
		return NULL;
	audio_buffer_t* p = (audio_buffer_t*)
		malloc(sizeof(audio_buffer_t)+m_sinks[id]->buffer_size);
	if (p) {
		if (size) *size = m_sinks[id]->buffer_size;
		p->data = ((u_int8_t*)p)+sizeof(audio_buffer_t);
		p->len = 0;
		p->next = NULL;
		p->rms = NULL;
//fprintf(stderr, "Get empty buffer <%lu> data <%lu>\n", (u_int64_t) p, (u_int64_t) p->data);
	}
	return p;
}

// Set volume level for sink
int CAudioSink::SetMoveVolume(u_int32_t sink_id, float volume_delta, u_int32_t steps)
{
	if (sink_id >= m_max_sinks || !m_sinks || !m_sinks[sink_id]) return -1;
	m_sinks[sink_id]->volume_delta = volume_delta;
	m_sinks[sink_id]->volume_delta_steps = steps;
	if (m_animation < steps) m_animation = steps;
	return 0;
}

// Set volume level for sink
int CAudioSink::SetVolume(u_int32_t sink_id, u_int32_t channel, float volume)
{
        if (sink_id >= m_max_sinks || !m_sinks || !m_sinks[sink_id] ||
                channel >= m_sinks[sink_id]->channels) return -1;
        if (volume < 0.0) volume = 0.0;
        if (volume > MAX_VOLUME_FLOAT) volume = MAX_VOLUME_FLOAT;
        m_sinks[sink_id]->pVolume[channel] = volume;
        return 0;
}
/*
int CAudioSink::SetVolume(u_int32_t id, u_int32_t volume)
{
	// make checks
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id]) return -1;
	// Set volume. Legal value is between 0-255
	m_sinks[id]->volume = volume > 255 ? 255 : volume;
	return 0;
}
*/

int CAudioSink::SetFile(u_int32_t id, const char* file_name)
{
	// make checks
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id] || m_sinks[id]->state == RUNNING)
		return -1;
	// Close fd if open
	if (m_sinks[id]->sink_fd > -1) close(m_sinks[id]->sink_fd);
	m_sinks[id]->sink_fd = -1;
	// Free name if any
	if (m_sinks[id]->file_name) free(m_sinks[id]->file_name);
	// copy new name
	m_sinks[id]->file_name = strdup(file_name);
	trim_string(m_sinks[id]->file_name);			// No memory leak
	if (!m_sinks[id]->file_name) return 1;
	SetSinkBufferSize(id);
	if (m_verbose) fprintf(stderr, "audio sink %u will write to file <%s>\n",
		id, m_sinks[id]->file_name);
	return 0;
}
int CAudioSink::SetQueueMaxDelay(u_int32_t id, u_int32_t maxdelay)
{
	// make checks
fprintf(stderr, "SINK %u delay %u\n", id, maxdelay);
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id] || !m_sinks[id]->pAudioQueue)
		return -1;
	m_sinks[id]->pAudioQueue->max_bytes = maxdelay;
	return 0;
}


// Calculate samples lacking using start_time and sample_count.
int32_t CAudioSink::SamplesNeeded(u_int32_t id)
{
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id] || m_sinks[id]->state != RUNNING)
		return -1;
	struct timeval timenow, timediff;
	if (gettimeofday(&timenow,NULL)) return -1;
	timersub(&timenow, &m_sinks[id]->start_time, &timediff);
	u_int32_t samples = (typeof(samples))(timediff.tv_sec*m_sinks[id]->rate +
		m_sinks[id]->rate * (timediff.tv_usec/1000000.0));
	if (samples < m_sinks[id]->sample_count) {
		fprintf(stderr, "audio sink %u samples %u less than sample count %u\n",
			id, samples, m_sinks[id]->sample_count);
		return 0;
		// We may have wrapping - FIXME
	}
	if (m_verbose > 2) fprintf(stderr,
		"audio sink %u samples %u current sample count %u lacking %u\n", id,
		samples, m_sinks[id]->sample_count, samples - m_sinks[id]->sample_count);
	return samples - m_sinks[id]->sample_count;
}

int CAudioSink::SinkSource(u_int32_t sink_id, u_int32_t source_id, int source_type)
{
	// make checks
	if (sink_id >= m_max_sinks || !m_sinks || !m_sinks[sink_id] || !m_pVideoMixer || (source_type != 1 && source_type != 2))
		return -1;

	// Type is 1 - audio feed
	// Type is 2 - audio mixer
	// Check max id
	if (source_type == 1 && (!m_pVideoMixer->m_pAudioFeed ||
		source_id >= m_pVideoMixer->m_pAudioFeed->MaxFeeds())) return -1;
	else if (source_type == 2 && (!m_pVideoMixer->m_pAudioMixer ||
		source_id >= m_pVideoMixer->m_pAudioMixer->MaxMixers())) return -1;

	if (sink_id && (m_sinks[sink_id]->channels !=
		(source_type == 1 ? m_pVideoMixer->m_pAudioFeed->GetChannels(source_id) :
		m_pVideoMixer->m_pAudioMixer->GetChannels(source_id)))) {
		if (m_verbose) fprintf(stderr, "Audio sink %u channels %u is not "
					"equal to audio feed %u channels %u\n",
					sink_id, m_sinks[sink_id]->channels, source_id,
					source_type == 1 ?
					  m_pVideoMixer->m_pAudioFeed->GetChannels(source_id) :
					  m_pVideoMixer->m_pAudioMixer->GetChannels(source_id));
		return -1;
	}

	if (m_sinks[sink_id]->rate !=
		(source_type == 1 ? m_pVideoMixer->m_pAudioFeed->GetRate(source_id) :
		m_pVideoMixer->m_pAudioMixer->GetRate(source_id))) {
		if (m_verbose) fprintf(stderr, "Audio sink %u rate %u is not "
					"equal to audio feed %u rate %u\n",
					sink_id, m_sinks[sink_id]->rate, source_id,
					source_type == 1 ?
					  m_pVideoMixer->m_pAudioFeed->GetRate(source_id) :
					  m_pVideoMixer->m_pAudioMixer->GetRate(source_id));
		return -1;
	}


	// Check to see if sink already has an audio queue
	if (m_sinks[sink_id]->pAudioQueue) {

		audio_queue_t* ptmp = NULL;
		// Check if the state of the sink is RUNNING, STALLED or PENDING. If
		// it is, we need to add the queue
		if (m_sinks[sink_id]->state == RUNNING || m_sinks[sink_id]->state == STALLED ||
			m_sinks[sink_id]->state == PENDING) {
			if (m_sinks[sink_id]->source_type == 1)
				ptmp = m_pVideoMixer->m_pAudioFeed->
					AddQueue(source_id);
			else ptmp = m_pVideoMixer->m_pAudioMixer->
				AddQueue(source_id);
		}
		// Check to see if adding the queue was alright
		if (!ptmp) {
			if (m_verbose) fprintf(stderr,
				"Frame %u - audio sink %u unexpectedly could not add an audio queue\n",
				SYSTEM_FRAME_NO, sink_id);
			return 1;
		}
		if (m_verbose) fprintf(stderr,
			"Frame %u - audio sink %u dropping sourcing by audio %s %u\n",
			SYSTEM_FRAME_NO, sink_id,
			m_sinks[sink_id]->source_type == 1 ? "feed" : "mixer",
			m_sinks[sink_id]->source_id);

		// Remove the current queue to audio feed or mixer
		if (m_sinks[sink_id]->source_type == 1)
			m_pVideoMixer->m_pAudioFeed->RemoveQueue(m_sinks[sink_id]->
				source_id, m_sinks[sink_id]->pAudioQueue);
		else if (m_sinks[sink_id]->source_type == 2)
			m_pVideoMixer->m_pAudioMixer->RemoveQueue(m_sinks[sink_id]->
				source_id, m_sinks[sink_id]->pAudioQueue);
		if (m_sinks[sink_id]->state == RUNNING ||
			m_sinks[sink_id]->state == STALLED) {
			if (m_verbose) fprintf(stderr,
				"Frame %u - audio sink %u changed state from %s to PENDING\n",
				SYSTEM_FRAME_NO, sink_id, feed_state_string(m_sinks[sink_id]->state));
			m_sinks[sink_id]->state = PENDING;
			
		}
		m_sinks[sink_id]->pAudioQueue = ptmp;
	}

	m_sinks[sink_id]->source_id = source_id;
	m_sinks[sink_id]->source_type = source_type;

	return 0;
}

int CAudioSink::StartSink(u_int32_t id)
{
	// make checks
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id] || m_sinks[id]->state != READY || !m_pVideoMixer)
		return -1;

	audio_sink_t* p = m_sinks[id];


	// Now lets see what we should start
	if (p->file_name && *p->file_name) {
		// Close if open
		if (p->sink_fd > -1) close(p->sink_fd);
		// Open file
		//p->sink_fd = open(p->file_name, O_WRONLY | O_CREAT | O_TRUNC
		char* longname = NULL;
		p->sink_fd = m_pVideoMixer->OpenFile(p->file_name, O_WRONLY | O_CREAT | O_TRUNC
#ifndef WIN32
			, S_IRWXU,
#else
			| O_BINARY, _S_IREAD | _S_IWRITE,
#endif
			&longname);
		if (p->sink_fd < 0) return 1;
		else if (m_verbose) fprintf(stderr, "audio sink %u opened file <%sd> for writing\n",
			id, p->file_name);
		if (longname) {
			if (p->file_name) free(p->file_name),
			p->file_name = longname;
		}



	// Check if fd is already open
	} else {
		if (m_verbose) {
			// The sink fd was already opened. Assuming ctr isaudio command
			if (p->sink_fd > -1) fprintf(stderr,
				"Frame %u - audio sink %u sink fd was already opened\n",
				SYSTEM_FRAME_NO, id);

			// If sink_fd is -1, we assume we have a dummy sink
			else fprintf(stderr,
				"Frame %u - audio sink %u sink set as a dummy sink\n",
				SYSTEM_FRAME_NO, id);
		}
	}

/*
	if (p->sink_fd < 0) {
		fprintf(stderr, "Frame %u - audio sink %u sink opening fd failed\n",
			SYSTEM_FRAME_NO, id);
		return -1;
	}
*/

	// Set fd to non blocking
	if (p->sink_fd >= 0 && set_fd_nonblocking(p->sink_fd)) {
		fprintf(stderr, "audio sink %u failed to set file <%s> to non blocking\n",
			id, p->file_name);
		close(p->sink_fd);
		p->sink_fd = -1;
		return 1;
	}
	if (p->sink_fd >= 0 && m_verbose) fprintf(stderr, "Frame %u - audio sink %u fd %d set to nonblock\n",
		SYSTEM_FRAME_NO, id, p->sink_fd);

	// Adding an audio queue to audio feed (audio feed will start fill it
	// if something is feeding it.
	if (m_pVideoMixer && (p->source_type == 1 || p->source_type == 2)) {
		if (p->source_type == 1 && m_pVideoMixer->m_pAudioFeed)
			p->pAudioQueue = m_pVideoMixer->m_pAudioFeed->
				AddQueue(p->source_id);
		else if (m_pVideoMixer->m_pAudioMixer)
			p->pAudioQueue = m_pVideoMixer->m_pAudioMixer->AddQueue(p->source_id);
		else {
			fprintf(stderr, "Frame %u - Unexpected condition in StartSink "
				"for audio sink %u\n", SYSTEM_FRAME_NO, id);
			StopSink(id);
			return 1;
		}

		feed_state_enum state = p->state;
		p->state = PENDING;
		if (!(p->pAudioQueue)) {
			if (m_verbose) fprintf(stderr,
				"Failed to add source queue from audio %s %u "
				"to audio sink %u\n",
				p->source_type == 1 ? "feed" : "mixer",
				p->source_id, id);
			StopSink(id);
			return -1;
		}
		// Create a new out queue holding converted samples
		if (p->pOutQueue) RemoveQueue(id, p->pOutQueue);
		if (!(p->pOutQueue = AddQueue(id, false))) {
			fprintf(stderr, "Failed to add out queue to audio sink %u. Stopping sink.\n", id);
			StopSink(id);
			return 1;
		}
			
		if (m_verbose) {
			fprintf(stderr, "Frame %u - audio sink %u added audio queue to "
				"audio %s %u\n", SYSTEM_FRAME_NO, id,
				p->source_type == 1 ? "feed" : "mixer",
				p->source_id);
			if (p->state != state) fprintf(stderr,
				"Frame %u - audio sink %u started. Changing state from %s to %s\n",
				SYSTEM_FRAME_NO, id, feed_state_string(state),
				feed_state_string(p->state));
		}
	} else if (m_verbose)
			fprintf(stderr, "audio sink %u started but has no audio queue source\n", id);

	// Reset counter
	p->sample_count = 0;
	p->samples_rec	= 0;
	p->silence	= 0;
	p->dropped	= 0;
	p->clipped	= 0;

	p->start_time.tv_sec           = 0;
        p->start_time.tv_usec          = 0;
	p->update_time.tv_sec           = 0;
        p->update_time.tv_usec          = 0;
        p->samplespersecond             = 0.0;
        p->last_samples                 = 0;
        p->total_avg_sps                = 0.0;
        for (p->avg_sps_index = 0; p->avg_sps_index < MAX_AVERAGE_SAMPLES; p->avg_sps_index++)
                p->avg_sps_arr[p->avg_sps_index] = 0.0;
        p->avg_sps_index                = 0;

	return 0;
}

// Returns the number of channels for sink id
u_int32_t CAudioSink::GetChannels(u_int32_t id)
{
	// Make checks
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id]) return 0;
	return (m_sinks[id]->channels);
}

// Returns the sample rate for sink id
u_int32_t CAudioSink::GetRate(u_int32_t id)
{
	// Make checks
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id]) return 0;
	return (m_sinks[id]->rate);
}

// Returns the number bytes per sample for sink id
u_int32_t CAudioSink::GetBytesPerSample(u_int32_t id)
{
	// Make checks
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id]) return 0;
	return (m_sinks[id]->bytespersample);
}


// Get Audio Sink state
feed_state_enum CAudioSink::GetState(u_int32_t id) {
        if (id >= m_max_sinks || !m_sinks || !m_sinks[id]) return UNDEFINED;
        return m_sinks[id]->state;
}


int CAudioSink::StopSink(u_int32_t id)
{
	// make checks
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id] || !m_pVideoMixer ||
        (m_sinks[id]->source_type == 1 && !m_pVideoMixer->m_pAudioFeed) ||
        (m_sinks[id]->source_type == 2 && !m_pVideoMixer->m_pAudioMixer) ||
        (m_sinks[id]->state != RUNNING &&
         m_sinks[id]->state != PENDING &&
         m_sinks[id]->state != STALLED)) return -1;

	// Close fd if necessary. Should be true, but lets make a check anyway;
	if (m_sinks[id]->sink_fd > -1) close(m_sinks[id]->sink_fd);
	m_sinks[id]->sink_fd = -1;

	// Remove Source queue
	if (m_sinks[id]->pAudioQueue) {
		if (m_sinks[id]->source_type == 1)
			m_pVideoMixer->m_pAudioFeed->RemoveQueue(m_sinks[id]->source_id, m_sinks[id]->pAudioQueue);
		else if (m_sinks[id]->source_type == 2)
			m_pVideoMixer->m_pAudioMixer->RemoveQueue(m_sinks[id]->source_id, m_sinks[id]->pAudioQueue);
		else if (m_sinks[id]->source_type == 3)
			RemoveQueue(m_sinks[id]->source_id, m_sinks[id]->pAudioQueue);
		else fprintf(stderr, "audio sink %u can not recognize source type while removing queue\n", id);
		m_sinks[id]->pAudioQueue = NULL;
	}

	// Remove out queue
	if (m_sinks[id]->pOutQueue) {
		RemoveQueue(m_sinks[id]->source_id, m_sinks[id]->pOutQueue);
		m_sinks[id]->pOutQueue = NULL;
	}
	// We don't check on close.
	m_sinks[id]->start_time.tv_sec = m_sinks[id]->start_time.tv_usec = 0;
	m_sinks[id]->state = READY;
	if (m_verbose) fprintf(stderr, "audio sink %u changed state to READY\n", id);
	return 0;
}

int CAudioSink::SetMute(u_int32_t id, bool mute)
{
	// make checks
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id]) return -1;
	m_sinks[id]->mute = mute;
	return 0;
}
void CAudioSink::QueueDropSamples(audio_queue_t* pQueue, u_int32_t drop_samples)
{
	// Make checks
	if (!pQueue || !drop_samples) return;
	u_int32_t drop_bytes = drop_samples*sizeof(int32_t);
	while (drop_bytes > 0 && pQueue->sample_count > 0) {
		audio_buffer_t* buf = GetAudioBuffer(pQueue);
		if (!buf) break;
		if (drop_bytes >= buf->len) {
			drop_bytes -= buf->len;
			free(buf);
			continue;
		}
		buf->len -= drop_bytes;
		buf->data += drop_bytes;
		AddAudioBufferToHeadOfQueue(pQueue, buf);
		drop_bytes = 0;
	}
}

audio_queue_t* CAudioSink::AddQueue(u_int32_t id, bool add)
{
fprintf(stderr, "ADDING queue to sink %u\n", id);
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id]) return NULL;
	audio_queue_t* pQueue = (audio_queue_t*) calloc(sizeof(audio_queue_t),1);
	if (pQueue) {
		pQueue->buf_count	= 0;
		pQueue->bytes_count     = 0;
		pQueue->sample_count    = 0;
		pQueue->async_access    = false;
		//pQueue->queue_samples[]       = 0;    // this is zeroed by calloc
		pQueue->qp	      = 0;
		pQueue->queue_total     = 0;
		pQueue->queue_max       = (typeof(pQueue->queue_max))(DEFAULT_QUEUE_MAX_SECONDS*m_sinks[id]->rate*m_sinks[id]->channels);
		//pQueue->wait = 4;
		if (add) {
			pQueue->pNext = m_sinks[id]->pAudioQueues;
			m_sinks[id]->pAudioQueues = pQueue;
		}
if (m_verbose) fprintf(stderr, "audio sink %u adding queue\n", id);
	}
	return pQueue;
}

// Remove an audio queue from feed
bool CAudioSink::RemoveQueue(u_int32_t id, audio_queue_t* pQueue)
{
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id] ||
		!pQueue || !(m_sinks[id]->pAudioQueues || m_sinks[id]->pOutQueue)) return false;
	audio_queue_t* p = m_sinks[id]->pAudioQueues;
	bool found = false;

	// First we check if it is the output queue
	if (pQueue == m_sinks[id]->pOutQueue) {
		m_sinks[id]->pOutQueue = m_sinks[id]->pOutQueue->pNext;
		found = true;
	} else if (p == pQueue) {
		m_sinks[id]->pAudioQueues = pQueue->pNext;
		found = true;
		if (m_verbose) fprintf(stderr, "audio sink %u removing first audio queue\n", id);
	} else {
		while (p) {
			if (p->pNext == pQueue) {
				p->pNext = pQueue->pNext;
				found = true;
				if (m_verbose) fprintf(stderr, "audio sink %u removing "
					"audio queue\n", id);
				break;
			} else p = p->pNext;
		}
	}
	if (found) {
		if (m_verbose > 1) fprintf(stderr, "audio sink %u freeing %u buffers and %u bytes\n",
			id, pQueue->buf_count, pQueue->bytes_count);
		audio_buffer_t* pBuf = pQueue->pAudioHead;
		while (pBuf) {
			audio_buffer_t* next = pBuf->next;
			free(pBuf);
			pBuf = next;
		}
		free(pQueue);
	}
	if (!m_sinks[id]->pAudioQueues && !m_sinks[id]->pOutQueue) {
		if (m_sinks[id]->state == RUNNING || m_sinks[id]->state == STALLED) {
			if (m_verbose > 1) fprintf(stderr, "Frame %u - audio sink %u changing state from %s to %s\n",
				SYSTEM_FRAME_NO, id,
				feed_state_string(m_sinks[id]->state),
				feed_state_string(PENDING));
			m_sinks[id]->state = PENDING;
		}
	}
	return found;
}

void CAudioSink::AddAudioToSink(u_int32_t id, audio_buffer_t* buf)
{
//fprintf(stderr, "Add buffer to queue for sink %u len %u ", id, buf->len);
//fprintf(stderr, "buf <%lu> data %lu\n", (u_int64_t) buf, (u_int64_t) buf->data);
	if (!buf) return;
	if (id < m_max_sinks && m_sinks && m_sinks[id] &&
		!m_sinks[id]->mute && m_sinks[id]->pAudioQueues) {
			buf->seq_no = m_sinks[id]->buf_seq_no++;
			SDL_LockAudio();
			audio_queue_t* paq = m_sinks[id]->pAudioQueues;
			buf->next = NULL;
			while (paq) {
				// Check if the audio queues have more members
				if (paq->pNext) {
fprintf(stderr, "paq->pNext was not NULL\n");
exit(1);
					// we have more consumers and need to copy data
					audio_buffer_t* newbuf = GetEmptyBuffer(id, NULL);
					if (newbuf) {
						memcpy(newbuf, buf, sizeof(audio_buffer_t)+buf->len);
						if (paq->pAudioTail) {
							paq->pAudioTail->next = newbuf;
							paq->pAudioTail = newbuf;
						} else paq->pAudioTail = newbuf;
						if (!paq->pAudioHead) paq->pAudioHead = newbuf;
					}
				// This is the last member and we can use the buffer as is
				} else {
					if (paq->pAudioTail) {
						paq->pAudioTail->next = buf;
						paq->pAudioTail = buf;
					} else paq->pAudioTail = buf;
					if (!paq->pAudioHead) paq->pAudioHead = buf;
				}
				//if (paq->wait > 0) paq->wait--;
				paq = paq->pNext;
			}
		if (m_verbose)
			fprintf(stderr, " - Frame no %u - audio sink %u got buffer "
				"seq no %u with %u bytes\n",
				SYSTEM_FRAME_NO,
				id, buf->seq_no, buf->len);
		SDL_UnlockAudio();
	} else {
		free(buf);
	}
}

/*
// Deliver an audio buffer from a specific queue.
// Update bytes and buffer count for feed.
audio_buffer_t* CAudioSink::GetAudioBuffer(audio_queue_t* pQueue) {

	// First we check we have a queue and that it has buffer(s)
	if (!pQueue || !pQueue->pAudioHead) return NULL;
	audio_buffer_t* pBuf = pQueue->pAudioHead;
	if (pBuf) {
		pQueue->pAudioHead = pBuf->next;
		if (!pQueue->pAudioHead) {
			pQueue->pAudioTail = NULL;
		}
		pQueue->buf_count--;
		pQueue->bytes_count -= pBuf->len;
		pQueue->sample_count -= (pBuf->len/sizeof(int32_t));
	}
	return pBuf;
}

void fill_audio_sink(void *udata, Uint8 *stream, int len)
{	CAudioSink* p = (CAudioSink*) udata;
	if (!udata) return;
	p->MonitorSinkCallBack(1, stream, len);
}

void CAudioSink::MonitorSinkCallBack(u_int32_t id, Uint8 *stream, int len)
{
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id] || len < 1) return;
	fprintf(stderr, "Frame %u - Monitor call back len %d\n",
		SYSTEM_FRAME_NO, len);
	if (m_sinks[id]->monitor_queue) {
		audio_buffer_t* p = m_sinks[id]->monitor_queue->pAudioHead;
		//if (m_sinks[id]->monitor_queue->wait)
			//fprintf(stderr, " - wait %u\n", m_sinks[id]->monitor_queue->wait);
		int i=0;
		while(p) {
			fprintf(stderr, " - seq no %u len %u\n", p->seq_no, p->len);
			p = p->next;
			if (i++ > 16) break;
		}
	}
	audio_buffer_t* pBuf = NULL;
	while (len > 0) {
		if (!pBuf) pBuf = GetAudioBuffer(m_sinks[id]->monitor_queue);
		if (pBuf) {
			if (pBuf->len > (unsigned) len) {
				fprintf(stderr, "Frame %u - buf %u - Audio mix %d. Have %u buf "
					"<%lu> data <%lu>\n",
					SYSTEM_FRAME_NO,
					pBuf->seq_no, len, pBuf->len, (u_int64_t) pBuf,
					(u_int64_t) pBuf->data);
				SDL_MixAudio(stream, pBuf->data, len, SDL_MIX_MAXVOLUME);
if (m_monitor_fd > -1) write(m_monitor_fd, pBuf->data, len);
				pBuf->data = pBuf->data + len;
				pBuf->len -= len;
				len = 0;
				pBuf->next = m_sinks[id]->monitor_queue->pAudioHead;
				m_sinks[id]->monitor_queue->pAudioHead = pBuf;
				if (!m_sinks[id]->monitor_queue->pAudioTail)
					m_sinks[id]->monitor_queue->pAudioTail = pBuf;
				//if (m_sinks[id]->monitor_queue->wait > 0)
					//m_sinks[id]->monitor_queue->wait = 0;
			} else {
				fprintf(stderr, "Frame %u - buf %u - Audio mix %u but only have "
					"%d.\n",
					SYSTEM_FRAME_NO,
					pBuf->seq_no, len, pBuf->len);
				SDL_MixAudio(stream, pBuf->data, pBuf->len,
					SDL_MIX_MAXVOLUME);
if (m_monitor_fd > -1) write(m_monitor_fd, pBuf->data, pBuf->len);
				len -= pBuf->len;
				free(pBuf);
				pBuf = NULL;
			}
		} else {
fprintf(stderr, "Frame %u - We need to fill with silence %d\n",
SYSTEM_FRAME_NO, len);
			u_int8_t* buf = (u_int8_t*) calloc(len, 1);
			if (buf) {
				memset(buf, 0, len);
				SDL_MixAudio(stream, buf, len, SDL_MIX_MAXVOLUME);
				free(buf);
			}
			len = 0;
		}
	}
	//u_int8_t buf[2*8192];
	//memset(buf, 0, 2*8192);
	//SDL_MixAudio(stream, buf, len, SDL_MIX_MAXVOLUME);
	return;
	//if (!m_sinks[id]->pAudioDataHead)) {
		fprintf(stderr, "Monitor call back had no data to fill\n");
		return;
	//}
	while (len) {
		
	}
	//SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME)
}

//extern void fill_audio(void *udata, Uint8 *stream, int len);
void CAudioSink::MonitorSink(u_int32_t id)
{
	SDL_AudioSpec wanted;
	if (id >= m_max_sinks || !m_sinks || !m_sinks[id] ||
		!m_sinks[id]->rate || !m_sinks[id]->channels) {
		fprintf(stderr, "Can not monitor audio sink\n");
		return;
	}
	// Set the audio format
	wanted.freq = m_sinks[id]->rate;
	wanted.channels = m_sinks[id]->channels;    // 1 = mono, 2 = stereo 
	if (m_sinks[id]->is_signed) {
		if (m_sinks[id]->bytespersample == 1) wanted.format = AUDIO_S8;
		else wanted.format = AUDIO_S16LSB;
		// FIXME. What to do for 24 bit int or 32 float or 64 double
	} else {
		if (m_sinks[id]->bytespersample == 1) wanted.format = AUDIO_U8;
		else wanted.format = AUDIO_U16;
		// FIXME. What to do for 24 bit int or 32 float or 64 double
	}
	// Good low-latency value for callback
	wanted.samples = 2048;
	wanted.callback = fill_audio_sink;
	wanted.userdata = this;
	fprintf(stderr, "SDL Audio format rate %u channels %u format %u <%u>\n",
		wanted.freq, wanted.channels, wanted.format, AUDIO_S16);

	// Open the audio device, forcing the desired format
	if ( SDL_OpenAudio(&wanted, NULL) < 0 ) {
		fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
		return;
	}
	fprintf(stderr, "Monitor set to on\n");
	return;
}
*/

void CAudioSink::SinkVolumeAnimation() {
	// Check to see if animation is needed
	if (m_animation) {
		for (u_int32_t id = 0; id < m_max_sinks ; id++) {
			if (m_sinks[id] && m_sinks[id]->volume_delta_steps) {
				if (m_sinks[id]->pVolume) {
					float* p = m_sinks[id]->pVolume;
					for (u_int32_t i=0 ; i < m_sinks[id]->channels; i++, p++) {
						*p += m_sinks[id]->volume_delta;
						if (*p < 0.0) *p = 0;
						else if(*p > MAX_VOLUME_FLOAT) *p = MAX_VOLUME_FLOAT;
					}
				}
				m_sinks[id]->volume_delta_steps--;
			}
		}
		m_animation--;
	}
}

void CAudioSink::SinkAccounting(audio_sink_t* pSink, struct timeval *timenow) {
	struct timeval timedif;

	if (!pSink || !timenow) return;

	// Do some accounting on clipping and rms
	if (pSink->clipped) pSink->clipped--;
	if (pSink->rms) for (u_int32_t i=0 ; i < pSink->channels; i++)
		pSink->rms[i] = (u_int64_t)(pSink->rms[i]*0.95);

	// Find time diff, sample diff and calculate samplespersecond
	if (pSink->update_time.tv_sec > 0 || pSink->update_time.tv_usec > 0) {
		timersub(timenow, &pSink->update_time, &timedif);
		double time = timedif.tv_sec + ((double) timedif.tv_usec)/1000000.0;
		if (time != 0.0) {
			// Check for wrapping.
			if (pSink->samples_rec <  pSink->last_samples) {
				pSink->samplespersecond =
					(UINT_MAX - pSink->last_samples +
					pSink->samples_rec)/time;
			} else pSink->samplespersecond =
				(pSink->samples_rec - pSink->last_samples)/time;
		}
		pSink->total_avg_sps -= pSink->avg_sps_arr[pSink->avg_sps_index];
		pSink->total_avg_sps += pSink->samplespersecond;
		pSink->avg_sps_arr[pSink->avg_sps_index] = pSink->samplespersecond;
		pSink->avg_sps_index++;
		pSink->avg_sps_index %= MAX_AVERAGE_SAMPLES;
	}

	// Update values for next time
	pSink->last_samples = pSink->samples_rec;
	pSink->update_time.tv_sec = timenow->tv_sec;
	pSink->update_time.tv_usec = timenow->tv_usec;
}

void CAudioSink::SinkPendingCheckForData(audio_sink_t* pSink, struct timeval *timenow) {
	// Now we check if we have a sink pending start but we
	// will not start it until we have data in the audio queue
	if (pSink->state == PENDING && pSink->pAudioQueue &&
		pSink->pAudioQueue->bytes_count > 0) {
			if (pSink->pAudioQueue->sample_count <
				pSink->calcsamplespersecond/m_pVideoMixer->m_frame_rate) {
				if (m_verbose > 1)
					fprintf(stderr, "Frame %u - audio sink "
						"%u is pending. queue has %u samples. Sink needs %u samples\n",
						SYSTEM_FRAME_NO, 
						pSink->id, pSink->pAudioQueue->sample_count,
						(u_int32_t)(pSink->calcsamplespersecond/
						m_pVideoMixer->m_frame_rate));
				return;
			}

		// If start_time == 0 we set the start time, else the start_time has been set
		// previous (perhaps for another audio source and has already run
		if (pSink->start_time.tv_sec == 0 && pSink->start_time.tv_usec == 0)
			pSink->start_time = *timenow;
			//gettimeofday(&pSink->start_time, NULL);

		if (m_verbose)
			fprintf(stderr, "Frame %u - audio sink %u changed state from %s to %s\n"
				"Frame %u - audio sink %u audio queue has %u buffers and %u samples\n",
				SYSTEM_FRAME_NO, pSink->id, feed_state_string(pSink->state),
				feed_state_string(RUNNING),
				SYSTEM_FRAME_NO, pSink->id, pSink->pAudioQueue->buf_count,
				pSink->pAudioQueue->sample_count);

		// state is now running and we will write date next time around
		pSink->state = RUNNING;

		return;
	}

}
int CAudioSink::SinkAddSilence(audio_sink_t* pSink) {
	// Check if we have a pointer to a sink
	if (!pSink) return 0;

	// Lets check if we need to add silence
	if (pSink->write_silence_bytes) {
		u_int32_t size;
		int32_t n = 0;
		audio_buffer_t* newbuf = GetEmptyBuffer(pSink->id, &size);
		if (!newbuf) {
			fprintf(stderr, "Frame %u - audio sink %u could not get a buffer for silence.\n",
				SYSTEM_FRAME_NO, pSink->id);
			return 0;
		}
		memset(newbuf->data, 0, size);
		while (newbuf && pSink->write_silence_bytes) {
			int32_t n = pSink->write_silence_bytes > size ? size :
				pSink->write_silence_bytes;
fprintf(stderr, "Writing %d silence bytes\n", n);
			newbuf->len = n;

			// Write to fd if present or else ...
			if (pSink->sink_fd > -1)
				n = write(pSink->sink_fd, newbuf->data, newbuf->len);
			else n = newbuf->len;		// .... or else just dump the data
			if (n < 1) break;
			pSink->silence += (n/pSink->bytespersample);
			if ((unsigned) n <= pSink->write_silence_bytes) pSink->write_silence_bytes -= n;
			else pSink->write_silence_bytes = 0;
		}
		if (newbuf) free(newbuf);

		//if we failed to write, we skip to next sink
		if (n < 0 && (errno == EAGAIN || EWOULDBLOCK)) return 1;
	}
	return 0;
}

// Check if we need to drop bytes. If we have no sink or no buffer or drop
// all bytes, we return 1, otherwise we return 0
int CAudioSink::SinkDropBytes(audio_sink_t* pSink, audio_buffer_t* newbuf) {
	if (!pSink || !newbuf) return 1;
	// Now check if we should drop samples
	if (pSink->drop_bytes) {
		if (pSink->id && m_verbose > 1) fprintf(stderr,
			"Frame %u - audio sink %u dropped %u samples\n",
			SYSTEM_FRAME_NO, pSink->id,
			(newbuf->len <= pSink->drop_bytes ? newbuf->len :
			pSink->drop_bytes) / pSink->bytespersample);
		if (newbuf->len > pSink->drop_bytes) {
			newbuf->len -= pSink->drop_bytes;
			newbuf->data += pSink->drop_bytes;
			pSink->dropped += (pSink->drop_bytes/sizeof(int32_t));
			// We dropped all bytes we should, so zero counter
			pSink->drop_bytes = 0;
		} else {
			// All bytes will be dropped and buffer freed
			pSink->dropped += (newbuf->len/sizeof(int32_t));
			pSink->drop_bytes -= newbuf->len;
			free(newbuf);
			return 1;
		}
					
	}
	return 0;
}

// This function will copy all samples from its source queue to it possible queues
// and convert samples for output and add them to the out queue.
// Doing so, the volume/ mute will be set if needed and RMS calculated
void CAudioSink::SinkSendSampleOut(audio_sink_t* pSink) {
	// if we have no sink, no source queue or no data in queue, we skip
	if (!pSink || !pSink->pAudioQueue || !pSink->pAudioQueue->sample_count) return;

	// If we don't have a rate limit, we don't check how much time has passed, we don't
	// check how much we should write. We just write whatever we have in the queue
	if (!pSink->no_rate_limit) {
		fprintf(stderr, "Code for rate limiting audio sink out has not been written.\n");
		return;
	}

	// If the sink gets stopped while in loop, the queue will be removed
	// and GetAudioBuffer will return NULL
	audio_buffer_t* newbuf = NULL;
	while ((newbuf = GetAudioBuffer(pSink->pAudioQueue))) {

		if (m_verbose > 4) fprintf(stderr, "Frame %u - audio sink %u got "
			"buffer seq_no %u with %u bytes, sink mute %s buf mute %s\n",
			SYSTEM_FRAME_NO, pSink->id, newbuf->seq_no, newbuf->len,
			pSink->mute ? "true" : "false", newbuf->mute ? "true" : "false");

		// If sink is sink 0, we will drop all samples
		if (pSink->id == 0) pSink->drop_bytes = newbuf->len;

		// Now check if we should drop samples
		if (pSink->drop_bytes && SinkDropBytes(pSink, newbuf)) {
			// We know pSink and new_buf was non NULL, so apparently we dropped
			// all bytes. Buffer has been freed. So go get a new one.
			continue;
		}

		// Drop empty buffers
		if (!newbuf->len) {
			free (newbuf);
			continue;
		}

		// Now check for muting. If we should mute, the samples in the buffer
		// will be zero'ed.
		if (newbuf->mute || pSink->mute)
			memset(newbuf->data, 0, newbuf->len);

		// Then we set the volume
		if (m_verbose > 4) fprintf(stderr, "Frame %u - audio sink %u "
			"setting volume\n", SYSTEM_FRAME_NO, pSink->id);
		if (newbuf->channels == pSink->channels && pSink->pVolume)
			SetVolumeForAudioBuffer(newbuf, pSink->pVolume);
		else if (m_verbose > 2) fprintf(stderr,
			"Frame %u - audio sink %u volume error : %s\n",
			SYSTEM_FRAME_NO, pSink->id,
			newbuf->channels == pSink->channels ?
			  "missing volume" : "channel mismatch");

		// If setting the volume resulted in clipping, we signal for the next
		// 100 frames that clipping occured
		if (newbuf->clipped) pSink->clipped = 100;

		// Now we need to set RMS for source from buffer
		if (m_verbose > 4) fprintf(stderr, "Frame %u - audio sink %u "
			"setting rms\n", SYSTEM_FRAME_NO, pSink->id);
		if (!newbuf->mute) for (u_int32_t i=0 ; i < pSink->channels &&
			i < newbuf->channels; i++) {
			float vol = (typeof(vol))(pSink->pVolume ?
				pSink->pVolume[i] * pSink->pVolume[i] :
				  1.0);
			if (vol != 1.0) {
				if (pSink->rms && pSink->rms[i] < newbuf->rms[i]*vol)
					pSink->rms[i] = (u_int64_t)(newbuf->rms[i]*vol);
			}
			else if (pSink->rms && pSink->rms[i] < newbuf->rms[i])
				pSink->rms[i] = newbuf->rms[i];
		}

		if (pSink->pOutQueue) {
			u_int8_t* data = newbuf->data;
			u_int32_t size, samples, len = newbuf->len;

			while (len) {
				if (m_verbose > 4) fprintf(stderr, "Frame %u - audio sink "
					"%u buffer with %u bytes\n", SYSTEM_FRAME_NO, pSink->id, len);

				// we have data in the queue. Get a buffer to hold it for conversion for output.
				audio_buffer_t* samplebuf = GetEmptyBuffer(pSink->id, &size);
				if (!samplebuf) {
					// We could not get a buffer and must drop samples. However
					// some samples may already have been queued
					pSink->dropped += (len*sizeof(int32_t));
					newbuf->len -= len;
					if (m_verbose > 2) fprintf(stderr, "Frame %u - audio sink "
						"%u no sample buffer. Dropping %u samples.\n",
						SYSTEM_FRAME_NO, pSink->id, (unsigned int) (len/sizeof(int32_t)));
					break;
				}

				// Calculate how many samples we can handle in the buffer
				if (len/sizeof(int32_t) > size/pSink->bytespersample)
					samples = size/pSink->bytespersample;
				else samples = len/sizeof(int32_t);

				// Now we will convert the samples for output.
				if (m_verbose > 4) fprintf(stderr,
					"Frame %u - audio sink %u convert\n", SYSTEM_FRAME_NO, pSink->id);

				if (pSink->bytespersample == 2) {
					if (pSink->is_signed) {
						convert_int32_to_si16((u_int16_t*)samplebuf->data,
							(int32_t*)data, samples);
					} else {
						convert_int32_to_ui16((u_int16_t*)samplebuf->data,
							(int32_t*)data, samples);
					}
					samplebuf->len = samples*2;
					samplebuf->channels = pSink->channels;
					samplebuf->rate = pSink->rate;
				} else {
					fprintf(stderr, "audio sink %u unsupported audio format\n", pSink->id);
					return;
				}
//fprintf(stderr, "AddBufferToQueues samplebuf->len %u Queuelen %u samplelen %u\n", samplebuf->len, pSink->pOutQueue->buf_count, pSink->pOutQueue->sample_count);
				AddBufferToQueues(pSink->pOutQueue, samplebuf, 2, pSink->id, SYSTEM_FRAME_NO, 0);
//fprintf(stderr, " - Added AddBufferToQueues samplebuf->len %u Queuelen %u samplelen %u\n", samplebuf->len, pSink->pOutQueue->buf_count, pSink->pOutQueue->sample_count);
				len -= (samples*(sizeof(int32_t)));
				data += (samples*sizeof(int32_t));
//fprintf(stderr, "len %u\n", len);
			}
		}
//fprintf(stderr, "NEWBUF LEN %u\n", newbuf->len);

		// Now we check if someone is using the sink as source
		if (newbuf->len && pSink->pAudioQueues) {

fprintf(stderr, " Adding to audio queues\n");
			// This add newbuf to one or more audio queues. We can not
			// change newbuf after this, but is is okay to read data from it here.
			AddBufferToQueues(pSink->pAudioQueues, newbuf, 2, pSink->id, SYSTEM_FRAME_NO);
		} else {
//fprintf(stderr, " Freeing newbuf\n");
			if (newbuf) {
				free (newbuf);
			}
		}
		newbuf = NULL;
	}
	if (pSink->id && pSink->pOutQueue && pSink->pOutQueue->sample_count) {
		WriteSink(pSink->id);
	}
}

void CAudioSink::CheckReadWrite(fd_set *read_fds, fd_set *write_fds) {
	u_int32_t id, size, samples;
	if (!m_sinks || !write_fds) return;
	for (id = 0; id < m_max_sinks ; id++) {
		if (!m_sinks[id] || m_sinks[id]->sink_fd < 0 ||
		    !FD_ISSET(m_sinks[id]->sink_fd, write_fds) ||
		    !m_sinks[id]->pOutQueue) continue;

		// fd is set and we will get data from queue, convert and write
		audio_buffer_t* samplebuf = GetEmptyBuffer(id, &size);
		if (!samplebuf) {
			fprintf(stderr, "Frame %u - audio sink %u failed to "
				"get sample buffer for output.\n", SYSTEM_FRAME_NO, id);
			continue;
		}
		audio_buffer_t* newbuf = NULL;
		while ((newbuf = GetAudioBuffer(m_sinks[id]->pOutQueue))) {

			// First we need to find how many samples we can convert
			if (newbuf->len/sizeof(int32_t) > size/m_sinks[id]->bytespersample)
				samples = size/m_sinks[id]->bytespersample;
			else samples = newbuf->len/sizeof(int32_t);

			// Then we convert samples
			if (m_sinks[id]->bytespersample == 2) {
				if (m_sinks[id]->is_signed) {
					convert_int32_to_si16((u_int16_t*)samplebuf->data,
						(int32_t*)newbuf->data, samples);
				} else {
					convert_int32_to_ui16((u_int16_t*)samplebuf->data,
						(int32_t*)newbuf->data, samples);
				}
			} else {
				fprintf(stderr, "audio sink %u unsupported audio format\n", id);
				return;
			}

			// samplebuf->len is the number of bytes to write out
			samplebuf->len = samples*m_sinks[id]->bytespersample;
newbuf->len -= (samples*sizeof(int32_t));
			u_int32_t written = 0;
			while (written < samplebuf->len) {
				if (m_verbose > 4) fprintf(stderr, "Frame %u - audio "
					"sink %u write->fd %d buf->len %u written %u "
					"writing %u\n", SYSTEM_FRAME_NO, id,
					m_sinks[id]->sink_fd, samplebuf->len, written,
					samplebuf->len-written);

				// Write samplebuf->len-written bytes to output fd
				int n;
				// if sink_fd > -1, then write data else ...
				if (m_sinks[id]->sink_fd > -1) {
					n = write(m_sinks[id]->sink_fd, samplebuf->data +
						written, samplebuf->len-written);
fprintf(stderr, "Writing %d bytes\n", n);
				// .... or else just dump the data
				} else {
					n = samplebuf->len-written;
fprintf(stderr, "Dumping %d bytes\n", n);
					m_sinks[id]->dropped += (samplebuf->len-written);
				}

				// Check for EAGAIN and EWOULDBLOCK
				if ((n <= 0) && (errno == EAGAIN || errno == EWOULDBLOCK)) {
					n = samplebuf->len-written;
					m_sinks[id]->dropped += (samplebuf->len-written);
					written += n;
					//n = 0;
					if (m_verbose) fprintf(stderr, "Frame %u - audio sink %u "
						"failed writing. reason %s\n", SYSTEM_FRAME_NO, id,
						errno == EAGAIN ? "EAGAIN" : "EWOULDBLOCK");
// FIXME. We need to break the loop and skip to next sink. We also need to add the buffer back to the queue which is bad because it is converted.
					return;
				} else if (n <= 0) {
					perror("audio sink write failed");
				}

				// if n < 0 we failed and we will drop the buffer and stop the sink
				if (n < 0) {
					perror ("Could not write data for audio sink");
					fprintf(stderr, "audio sink %u failed writing buffer "
						"to fd %d.\n", id, m_sinks[id]->sink_fd);
					StopSink(id);	// StopSink will remove queue
					newbuf->len = 0;
					return;
				}
				written += n;
if (m_verbose > 4) fprintf(stderr, "Frame %u - audio sink %u wrote %d bytes written %u\n",
	SYSTEM_FRAME_NO, id, n, written);
			}
		}
	}
}

int CAudioSink::SetFds(int max_fd, fd_set* read_fds, fd_set* write_fds) {
	u_int32_t id;
	if (!m_sinks || !write_fds) return max_fd;
	for (id = 0; id < m_max_sinks ; id++) {
		if (!m_sinks[id] || !m_sinks[id]->pOutQueue ||
		    !m_sinks[id]->pOutQueue->sample_count || m_sinks[id]->sink_fd < 0 ||
		    (m_sinks[id]->state != RUNNING && m_sinks[id]->state != STALLED))
			continue;
		FD_SET(m_sinks[id]->sink_fd, write_fds);
		if (max_fd < m_sinks[id]->sink_fd) max_fd = m_sinks[id]->sink_fd;
	}
	return max_fd;
}

static struct timeval writetime = { 0, 0 };

int CAudioSink::WriteSink(u_int32_t id) {
	int32_t written = 0;
	audio_buffer_t* buf;
	if (!m_sinks || !m_sinks[id] || !m_sinks[id]->pOutQueue ||
	     m_sinks[id]->sink_fd < 0 ||
	    (m_sinks[id]->state != RUNNING && m_sinks[id]->state != STALLED)) return -1;
/*
if (id) {
  fprintf(stderr, "WriteSink %u\n", id);
  audio_queue_t* p = m_sinks[id]->pOutQueue;
  audio_buffer_t* b = m_sinks[id]->pOutQueue->pAudioHead;
  fprintf(stderr, " - outqueue has %u buf %u samples %u bytes\n", p->buf_count, p->sample_count, p->bytes_count);
  while (b) {
    fprintf(stderr, "   - len %u size %u\n", b->len, b->size);
    b = b->next;
  }
}
*/

int total = 0;

struct timeval timenow, deltatime;
gettimeofday(&timenow, NULL);
if ((writetime.tv_sec == 0 && writetime.tv_usec == 0)) writetime=timenow;
timersub(&timenow, &writetime, &deltatime);
//fprintf(stderr, "F%04u T%ld.%03ld AS %u\n", SYSTEM_FRAME_NO, deltatime.tv_sec, deltatime.tv_usec/1000, id);

	buf = GetAudioBuffer(m_sinks[id]->pOutQueue);
	while (buf) {
		// First we check for an empty buffer
		if (!buf->len) {
			free(buf);
			// Repeat loop with new buffer if any
			buf = GetAudioBuffer(m_sinks[id]->pOutQueue);
			continue;
		}
		// We have a buffer with data
		int32_t n = write(m_sinks[id]->sink_fd, buf->data, buf->len);

if (n>0) total += n;
/*
int error=0;
if (buf->len % (m_sinks[id]->channels<<1)) error=1;
fprintf(stderr, "%s - Written %d len %u samples %d = %.1lfms total written %d\n",
	error ? "Error. " : "",
	total,
	buf->len,
	total/m_sinks[id]->bytespersample,
	1000.0*total/(m_sinks[id]->rate*m_sinks[id]->bytespersample*m_sinks[id]->channels),
	total+m_sinks[id]->bytespersample*m_sinks[id]->samples_rec);
*/
if (n & (1 & m_sinks[id]->channels)) fprintf(stderr, "WARNING\n");
writetime = timenow;

		// We wrote data
		if (n > 0) {
if (m_verbose > 1 && ((unsigned) n) != buf->len) fprintf(stderr, "Frame %u - audio sink %u wrote less %d %u.\n",
                        SYSTEM_FRAME_NO, id, n, buf->len);
else if (m_verbose > 4) fprintf(stderr, "Frame %u - audio sink %u wrote %d == %u. errno %d\n",
                        SYSTEM_FRAME_NO, id, n, buf->len, errno);
			written += n;
			buf->data += n;
			buf->len -= n;

			// Loop with current buffer. If all data is written, the
			// buffer will be freed and a new one is fectched. Otherwise
			// we will try to write the rest.
			continue;
		}
		// n <= 0
		// Write returned 0 or less. Perhaps we would block?
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			// Writing failed for other reason that it would block
			perror("audio sink write failed.");
			StopSink(id);
		} else {
if (m_verbose) fprintf(stderr, "Frame %u - audio sink %u would block\n",
                        SYSTEM_FRAME_NO, id);
		}
		break;
	}
if (written > 0 && written % m_sinks[id]->bytespersample) fprintf(stderr, "ERROR. Audio sink %u wrote %d bytes. Bytes per sample is %u\n", id, written, m_sinks[id]->bytespersample);
	// We may have a buf we either need to free, if empty, or put back into queue
	// again if not empty.
	if (buf) {
		if (buf->len) AddAudioBufferToHeadOfQueue(m_sinks[id]->pOutQueue, buf);
		else free(buf);
	}
	m_sinks[id]->samples_rec += (written / m_sinks[id]->bytespersample);
	return written;
}

void CAudioSink::NewUpdate(struct timeval *timenow)
{
	u_int32_t id;

	// Do volume animation if needed
	if (m_animation) SinkVolumeAnimation();

	// Process all existing sinks
	for (id = 0; id < m_max_sinks ; id++) {

		// If sink does not exist, we skip to next
		if (!m_sinks[id]) continue;

		if (m_verbose > 4) fprintf(stderr, "Frame %u - audio sink %u update\n",
			SYSTEM_FRAME_NO, id);

		// Do accounting stuff
		SinkAccounting(m_sinks[id], timenow);
		if (m_verbose > 4) fprintf(stderr, "Frame %u - audio sink %u accounting done. State %s\n",
			SYSTEM_FRAME_NO, id, feed_state_string(m_sinks[id]->state));


		// Now we check if we have a sink pending start but we
		// will not start it until we have data in the audio queue
		if (m_sinks[id]->state == PENDING) {
			SinkPendingCheckForData(m_sinks[id], timenow);
			// If there is data enough, the state will change to RUNNING and we will
			// write data the next time this function is called.
			continue;
		}

		// We skip the sink, if the sink is not running or stalled
		if (m_sinks[id]->state != RUNNING &&
			m_sinks[id]->state != STALLED) continue;

		if (m_verbose > 4) fprintf(stderr, "Frame %u - audio sink %u checking if queue is present \n",
			SYSTEM_FRAME_NO, id);

		// Now we check we have an audio queue for the sink.
		if (!m_sinks[id]->pAudioQueue) {
			fprintf(stderr, "Frame %u - Ooops. Audio queue was gone for sink %u\n"
				" - audio sink %u will be stopped and change state to SETUP.\n",
				SYSTEM_FRAME_NO,
				id, id);
StopSink(id);
			m_sinks[id]->state = SETUP;
			continue;
		}

		// Lets check if we need to add silence
		if (m_sinks[id]->write_silence_bytes && SinkAddSilence(m_sinks[id])) continue;
		
		// If we don't have a rate limit, we don't check how much time has passed, we don't
		// check how much we should write. We just write whatever we have in the queue
		if (m_sinks[id]->no_rate_limit) {
//if (id == 1) fprintf(stderr, "Audio sink %u samples %u queues %s\n", id, m_sinks[id]->pAudioQueue->sample_count, m_sinks[id]->pAudioQueues ? "ptr" : "null");
			// if we have no data in queue, we skip
			if (!m_sinks[id]->pAudioQueue->sample_count) continue;
			// we have data in the queue

			SinkSendSampleOut(m_sinks[id]);
		} else {
			fprintf(stderr, "Code for rate limiting audio sink has not been written yet.\n");
			exit(1);
		}

	}
	if (m_verbose > 4) fprintf(stderr, "Frame %u - audio sink update done\n",
		SYSTEM_FRAME_NO);
}


void CAudioSink::Update()
{
	u_int32_t id;
	timeval timenow, timedif;

fprintf(stderr, "CAudioSink::Update() This function is deprecated\n");

	if (m_verbose > 4) fprintf(stderr, "Frame %u - audio sink update start\n",
		SYSTEM_FRAME_NO);
	if (!m_sinks) {
		if (m_verbose > 4) fprintf(stderr, "Frame %u - audio sink update done\n",
			SYSTEM_FRAME_NO);
		return;
	}

	// Check to see if animation is needed
	if (m_animation) {
		for (id = 0; id < m_max_sinks ; id++) {
			if (m_sinks[id] && m_sinks[id]->volume_delta_steps) {
				if (m_sinks[id]->pVolume) {
					float* p = m_sinks[id]->pVolume;
					for (u_int32_t i=0 ; i < m_sinks[id]->channels; i++, p++) {
						*p += m_sinks[id]->volume_delta;
						if (*p < 0.0) *p = 0;
						else if(*p > MAX_VOLUME_FLOAT) *p = MAX_VOLUME_FLOAT;
					}
				}
				m_sinks[id]->volume_delta_steps--;
			}
		}
		m_animation--;
	}


	gettimeofday(&timenow, NULL);

	// Process all existing sinks
	for (id = 0; id < m_max_sinks ; id++) {
		
		if (!m_sinks[id]) continue;
		if (m_verbose > 4) fprintf(stderr, "Frame %u - audio sink %u update\n",
			SYSTEM_FRAME_NO, id);

		// Do some accounting on clipping and rms
		if (m_sinks[id]->clipped) m_sinks[id]->clipped--;
		if (m_sinks[id]->rms) for (u_int32_t i=0 ; i < m_sinks[id]->channels; i++)
			m_sinks[id]->rms[i] *= (u_int64_t)(m_sinks[id]->rms[i]*0.95);

		// Find time diff, sample diff and calculate samplespersecond
		if (m_sinks[id]->update_time.tv_sec > 0 || m_sinks[id]->update_time.tv_usec > 0) {
                        timersub(&timenow, &m_sinks[id]->update_time, &timedif);
                        double time = timedif.tv_sec + ((double) timedif.tv_usec)/1000000.0;
                        if (time != 0.0) {
				// Check for wrapping.
				if (m_sinks[id]->samples_rec <  m_sinks[id]->last_samples) {
					m_sinks[id]->samplespersecond =
						(UINT_MAX - m_sinks[id]->last_samples +
						m_sinks[id]->samples_rec)/time;
				} else m_sinks[id]->samplespersecond =
					(m_sinks[id]->samples_rec - m_sinks[id]->last_samples)/time;
			}
                        m_sinks[id]->total_avg_sps -= m_sinks[id]->avg_sps_arr[m_sinks[id]->avg_sps_index];
                        m_sinks[id]->total_avg_sps += m_sinks[id]->samplespersecond;
                        m_sinks[id]->avg_sps_arr[m_sinks[id]->avg_sps_index] = m_sinks[id]->samplespersecond;
                        m_sinks[id]->avg_sps_index++;
                        m_sinks[id]->avg_sps_index %= MAX_AVERAGE_SAMPLES;
                }

                // Update values for next time
                m_sinks[id]->last_samples = m_sinks[id]->samples_rec;
                m_sinks[id]->update_time.tv_sec = timenow.tv_sec;
                m_sinks[id]->update_time.tv_usec = timenow.tv_usec;


		if (m_verbose > 4) fprintf(stderr, "Frame %u - audio sink %u accounting done\n",
			SYSTEM_FRAME_NO, id);

		// Now we check if we have a sink pending start but we
		// will not start it until we have data in the audio queue
		if (m_sinks[id]->state == PENDING && m_sinks[id]->pAudioQueue &&
			m_sinks[id]->pAudioQueue->bytes_count > 0) {
				if (m_sinks[id]->pAudioQueue->sample_count <
					m_sinks[id]->calcsamplespersecond/m_pVideoMixer->m_frame_rate) {
					if (m_verbose > 1)
						fprintf(stderr, "Frame %u - audio sink "
							"%u is pending. queue has %u samples. Sink needs %u samples\n",
							SYSTEM_FRAME_NO, 
							id, m_sinks[id]->pAudioQueue->sample_count,
							(u_int32_t)(m_sinks[id]->calcsamplespersecond/
							m_pVideoMixer->m_frame_rate));
					continue;
				}

			// If start_time == 0 we set the start time, else the start_time has been set
			// previous (perhaps for another audio source and has already run
			if (m_sinks[id]->start_time.tv_sec == 0 && m_sinks[id]->start_time.tv_usec == 0)
				gettimeofday(&m_sinks[id]->start_time, NULL);

			if (m_verbose)
				fprintf(stderr, "Frame %u - audio sink %u changed state from %s to %s\n"
					"Frame %u - audio sink %u audio queue has %u buffers and %u samples\n",
					SYSTEM_FRAME_NO, id, feed_state_string(m_sinks[id]->state),
					feed_state_string(RUNNING),
					SYSTEM_FRAME_NO, id, m_sinks[id]->pAudioQueue->buf_count,
					m_sinks[id]->pAudioQueue->sample_count);

			// state is now running and we will write date next time around
			m_sinks[id]->state = RUNNING;

			continue;
		}

		if (m_verbose > 4) fprintf(stderr, "Frame %u - audio sink %u checking if stalled \n",
			SYSTEM_FRAME_NO, id);

		// We skip the sink, if the sink is not running or stalled
		if (m_sinks[id]->state != RUNNING &&
			m_sinks[id]->state != STALLED) continue;

		if (m_verbose > 4) fprintf(stderr, "Frame %u - audio sink %u checking if queue is present \n",
			SYSTEM_FRAME_NO, id);

		// Now we check we have an audio queue for the sink.
		if (!m_sinks[id]->pAudioQueue) {
			fprintf(stderr, "Frame %u - Ooops. Audio queue was gone for sink %u\n"
				" - audio sink %u will be stopped and change state to SETUP.\n",
				SYSTEM_FRAME_NO,
				id, id);
			m_sinks[id]->state = SETUP;
			continue;
		}

		if (m_verbose > 4) fprintf(stderr, "Frame %u - audio sink %u checking if rate limited \n",
			SYSTEM_FRAME_NO, id);

		// Lets check if we need to add silence
		if (m_sinks[id]->write_silence_bytes) {
			u_int32_t size;
			int32_t n = 0;
			audio_buffer_t* newbuf = GetEmptyBuffer(id, &size);
			memset(newbuf->data, 0, size);
			while (newbuf && m_sinks[id]->write_silence_bytes) {
				int32_t n = m_sinks[id]->write_silence_bytes > size ? size :
					m_sinks[id]->write_silence_bytes;
				newbuf->len = n;

				// Write to fd if present or else ...
				if (m_sinks[id]->sink_fd > -1)
					n = write(m_sinks[id]->sink_fd, newbuf->data, newbuf->len);
				// .... or else just dump the data
				else n = newbuf->len;
				if (n < 1) break;
				m_sinks[id]->silence += (n/m_sinks[id]->bytespersample);
				if ((unsigned) n <= m_sinks[id]->write_silence_bytes) m_sinks[id]->write_silence_bytes -= n;
				else m_sinks[id]->write_silence_bytes = 0;
			}
			if (newbuf) free(newbuf);

			//if we failed to write, we skip to next sink
			if (n < 0 && (errno == EAGAIN || EWOULDBLOCK)) continue;
		}

		// If we don't have a rate limit, we don't check how much time has passed, we don't
		// check how much we should write. We just write whatever we have in the queue
		if (m_sinks[id]->no_rate_limit) {
			// if we have no data in queue, we skip
			if (!m_sinks[id]->pAudioQueue->sample_count) continue;
			// we have data in the queue
			u_int32_t size, samples;
			audio_buffer_t* newbuf = NULL;
			audio_buffer_t* samplebuf = GetEmptyBuffer(id, &size);

			// Check that we have a sample buffer for conversion
			if (!samplebuf) {
				fprintf(stderr, "Frame %u - audio sink %u failed to "
					"get sample buffer\n", SYSTEM_FRAME_NO, id);
				continue;
			}

			// If the sink gets stopped while in loop, the queue will be removed
			// and GetAudioBuffer will return NULL
			while ((newbuf = GetAudioBuffer(m_sinks[id]->pAudioQueue))) {

if (m_verbose > 4) fprintf(stderr, "Frame %u - audio sink %u got buffer seq_no %u with %u bytes, sink mute %s buf mute %s\n",
	SYSTEM_FRAME_NO, id, newbuf->seq_no, newbuf->len, m_sinks[id]->mute ? "true" : "false", newbuf->mute ? "true" : "false");
				// Now check if we should drop samples
				if (m_sinks[id]->drop_bytes) {
					if (m_verbose > 1) fprintf(stderr,
						"Frame %u - audio sink %u dropped %u samples\n", SYSTEM_FRAME_NO, id,
						(newbuf->len <= m_sinks[id]->drop_bytes ? newbuf->len :
						m_sinks[id]->drop_bytes) / m_sinks[id]->bytespersample);
					if (newbuf->len >= m_sinks[id]->drop_bytes) {
						newbuf->len -= m_sinks[id]->drop_bytes;
						newbuf->data += m_sinks[id]->drop_bytes;
						m_sinks[id]->dropped += (m_sinks[id]->drop_bytes/
							m_sinks[id]->bytespersample);
						m_sinks[id]->drop_bytes = 0;
					} else {
						m_sinks[id]->dropped += (newbuf->len/
							m_sinks[id]->bytespersample);
						m_sinks[id]->drop_bytes -= newbuf->len;
						free(newbuf);
						continue;
					}
						
				}
				if (newbuf->mute || m_sinks[id]->mute)
					memset(newbuf->data, 0, newbuf->len);
				while (newbuf->len) {

if (m_verbose > 4) fprintf(stderr, "Frame %u - audio sink %u buffer with %u bytes\n",
	SYSTEM_FRAME_NO, id, newbuf->len);
					if (newbuf->len/sizeof(int32_t) > size/m_sinks[id]->bytespersample)
						samples = size/m_sinks[id]->bytespersample;
					else
						samples = newbuf->len/sizeof(int32_t);

					// Then we set the volume
					if (m_verbose > 4) fprintf(stderr, "Frame %u - audio sink %u "
						"setting volume\n", SYSTEM_FRAME_NO, id);
					if (newbuf->channels == m_sinks[id]->channels && m_sinks[id]->pVolume)
						SetVolumeForAudioBuffer(newbuf, m_sinks[id]->pVolume);
					else if (m_verbose > 2) fprintf(stderr,
						"Frame %u - audio sink %u volume error : %s\n",
						SYSTEM_FRAME_NO, id,
						newbuf->channels == m_sinks[id]->channels ?
						  "missing volume" : "channel mismatch");

					if (newbuf->clipped) m_sinks[id]->clipped = 100;

					if (m_verbose > 4) fprintf(stderr, "Frame %u - audio sink %u "
						"setting rms\n", SYSTEM_FRAME_NO, id);

					// Now we need to set RMS for source from buffer
					if (!newbuf->mute) for (u_int32_t i=0 ; i < m_sinks[id]->channels &&
						i < newbuf->channels; i++) {
						float vol = (typeof(vol))(m_sinks[id]->pVolume ?
							m_sinks[id]->pVolume[i] * m_sinks[id]->pVolume[i] :
							  1.0);
						if (vol != 1.0) {
							if (m_sinks[id]->rms && m_sinks[id]->rms[i] < newbuf->rms[i]*vol)
								m_sinks[id]->rms[i] = (u_int64_t)(newbuf->rms[i]*vol);
						}
						else if (m_sinks[id]->rms && m_sinks[id]->rms[i] < newbuf->rms[i])
							m_sinks[id]->rms[i] = newbuf->rms[i];
					}

					if (m_verbose > 4) fprintf(stderr,
						"Frame %u - audio sink %u convert\n", SYSTEM_FRAME_NO, id);

					if (m_sinks[id]->bytespersample == 2) {
						if (m_sinks[id]->is_signed) {
							convert_int32_to_si16((u_int16_t*)samplebuf->data,
								(int32_t*)newbuf->data, samples);
						} else {
							convert_int32_to_ui16((u_int16_t*)samplebuf->data,
								(int32_t*)newbuf->data, samples);
						}
					} else {
						fprintf(stderr, "audio sink %u unsupported audio format\n", id);
						break;
					}

					samplebuf->len = samples*m_sinks[id]->bytespersample;
					newbuf->len -= (samples*sizeof(int32_t));
					u_int32_t written = 0;
					while (written < samplebuf->len) {
						if (m_verbose > 4) fprintf(stderr, "Frame %u - audio "
							"sink %u write->fd %d buf->len %u written %u "
							"writing %u\n", SYSTEM_FRAME_NO, id,
							m_sinks[id]->sink_fd, samplebuf->len, written,
							samplebuf->len-written);

						// Write samplebuf->len-written bytes to output fd
						int n;
						// if sink_fd > -1, then write data else ...
						if (m_sinks[id]->sink_fd > -1)
							n = write(m_sinks[id]->sink_fd, samplebuf->data +
								written, samplebuf->len-written);
						// .... or else just dump the data
						else {
							n = samplebuf->len-written;
							m_sinks[id]->dropped += (samplebuf->len-written);
						}
						
						// Check for EAGAIN and EWOULDBLOCK
						if ((n <= 0) && (errno == EAGAIN || errno == EWOULDBLOCK)) {
							n = samplebuf->len-written;
							m_sinks[id]->dropped += (samplebuf->len-written);
							written += n;
							//n = 0;
							if (m_verbose) fprintf(stderr, "Frame %u - audio sink %u "
								"failed writing. reason %s\n", SYSTEM_FRAME_NO, id,
								errno == EAGAIN ? "EAGAIN" : "EWOULDBLOCK");
// FIXME. We need to break the loop and skip to next sink. We also need to add the buffer back to the queue which is bad because it is converted.
							continue;
						} else if (n <= 0) {
							perror("audio sink write failed");
						}

						// if n < 0 we failed and we will drop the buffer and stop the sink
						if (n < 0) {
							perror ("Could not write data for audio sink");
							fprintf(stderr, "audio sink %u failed writing buffer "
								"to fd %d.\n", id, m_sinks[id]->sink_fd);
							StopSink(id);	// StopSink will remove queue
							newbuf->len = 0;
							break;
						}
						written += n;
if (m_verbose > 4) fprintf(stderr, "Frame %u - audio sink %u wrote %d bytes written %u\n",
	SYSTEM_FRAME_NO, id, n, written);
					}
					// May wrap, but that is okay
					m_sinks[id]->samples_rec += (written/m_sinks[id]->bytespersample);
				}
				if (newbuf) free(newbuf);
			}
			if (samplebuf) free(samplebuf);
			continue;
		}
fprintf(stderr, "WE SHOULD NOT BE HERE\n");
/*
		// We have rate limit and must see if we need to calculate how much we should write,
		// insert silence if needed and drop data in queue if it becomes too long 

		// The sink is running or stalled. Lets calculate how many
		// bytes we need to write
		int32_t needed_samples = SamplesNeeded(id)*m_sinks[id]->channels;

		// Lets check if we should write any samples at all
		if (needed_samples < 1) {
			if (m_verbose)
				fprintf(stderr, "Frame %u - audio sink %u does not need any samples. "
				"Skipping\n", id, SYSTEM_FRAME_NO);
			continue;
		}
if (m_sinks[id]->drop_bytes) fprintf(stderr, "DROPPING -7 sink %u %u\n", id, m_sinks[id]->drop_bytes);

		// Now lets see how many samples we in the queue.
		u_int32_t queue_samples = m_sinks[id]->pAudioQueue->sample_count;

		int32_t average_samples = QueueUpdateAndAverage(m_sinks[id]->pAudioQueue);
		if (average_samples > m_sinks[id]->pAudioQueue->queue_max) {
			if (m_verbose) fprintf(stderr, "Frame %u - "
				"audio sink %u av. samp. %d (%d ms) exceeds max. queue = %u (%u ms) max = %d (%u ms)\n",
				SYSTEM_FRAME_NO, id,
				average_samples, 1000*average_samples/(m_sinks[id]->rate*m_sinks[id]->channels),
				queue_samples, 1000*queue_samples/(m_sinks[id]->rate*m_sinks[id]->channels),
				m_sinks[id]->pAudioQueue->queue_max,
				1000*m_sinks[id]->pAudioQueue->queue_max/(m_sinks[id]->rate*m_sinks[id]->channels));
			if (m_verbose) fprintf(stderr, "Frame %u - audio mixer %u queue bytes %u queue buffers %u\n",
				SYSTEM_FRAME_NO, id,
				m_sinks[id]->pAudioQueue->bytes_count, m_sinks[id]->pAudioQueue->buf_count);

			// Now we will check if we should drop samples
			if ((int32_t) queue_samples > m_sinks[id]->pAudioQueue->queue_max) {
				u_int32_t drop_samples = queue_samples - m_sinks[id]->pAudioQueue->queue_max;
				if (m_sinks[id]->channels > 1) {
					drop_samples /= m_sinks[id]->channels;
					drop_samples *= m_sinks[id]->channels;
				}
				if (m_verbose)  fprintf(stderr, "Frame %u - audio %s %u dropping samples %u (%u ms)\n",
					SYSTEM_FRAME_NO,
					m_sinks[id]->source_type == 1 ? "feed" :
					  m_sinks[id]->source_type == 2 ? "mixer" :
					    m_sinks[id]->source_type == 3 ? "sink" : "unknown",
					id, drop_samples,
					1000*drop_samples/(m_sinks[id]->channels*m_sinks[id]->rate));

				if (m_sinks[id]->source_type == 1)
					m_pVideoMixer->m_pAudioFeed->QueueDropSamples(m_sinks[id]->pAudioQueue,
						drop_samples);
				else if (m_sinks[id]->source_type == 2)
					m_pVideoMixer->m_pAudioMixer->QueueDropSamples(m_sinks[id]->pAudioQueue,
						drop_samples);
				else if (m_sinks[id]->source_type == 3)
					QueueDropSamples(m_sinks[id]->pAudioQueue, drop_samples);
				else fprintf(stderr, "Frame %u - audio sink %u source is unknown\n",
					SYSTEM_FRAME_NO, id);
				queue_samples = m_sinks[id]->pAudioQueue->sample_count;
			}
				
	
		}
if (m_sinks[id]->drop_bytes) fprintf(stderr, "DROPPING -6 sink %u %u\n", id, m_sinks[id]->drop_bytes);

*/
/*
		// Lets calculate the average surplus after subtracting needed samples
		// queue_samples[] = [count1, count2 ... countn, total]. First n counts in round robin
		int32_t* total = &m_sinks[id]->queue_samples[sizeof(m_sinks[id]->queue_samples)/
			sizeof(m_sinks[id]->queue_samples[0])-1];
		*total -= m_sinks[id]->queue_samples[m_sinks[id]->nqb];
		m_sinks[id]->queue_samples[m_sinks[id]->nqb] = queue_samples - needed_samples;
		*total += m_sinks[id]->queue_samples[m_sinks[id]->nqb];
		m_sinks[id]->nqb++;
		m_sinks[id]->nqb %= ((sizeof(m_sinks[id]->queue_samples)/sizeof(m_sinks[id]->queue_samples[0]))-1);
		if (m_verbose > 2 && !m_sinks[id]->nqb)
			fprintf(stderr, "audio sink %u average(%2u) %d queue samples %u missing %u\n",
				id, m_sinks[id]->nqb,
				*total/(int32_t)(sizeof(m_sinks[id]->queue_samples)/
					sizeof(m_sinks[id]->queue_samples[0])-1),
				queue_samples, needed_samples);

		if (m_verbose > 3) 
			fprintf(stderr, "Frame %u - audio sink %u has %u samples in queue "
				"and miss %u samples%s\n",
				SYSTEM_FRAME_NO,
				id, queue_samples, needed_samples,
				(u_int32_t)needed_samples > queue_samples ? " Adding silence needed" : "");

		// total / (120 / 4 - 1) = total / 39
		if (*total/(int32_t)(sizeof(m_sinks[id]->queue_samples)/
			sizeof(m_sinks[id]->queue_samples[0])-1) >
			(2*sizeof(int32_t)*m_sinks[id]->buffer_size/m_sinks[id]->bytespersample) &&
			m_sinks[id]->pAudioQueue->sample_count > m_sinks[id]->buffer_size) {
			if (m_verbose)
				fprintf(stderr, "Frame %u - audio sink %u has %u buffers and %u samples in queue. "
					"Dropping 1 buffer with %u samples\n", 
					SYSTEM_FRAME_NO, id,
					m_sinks[id]->pAudioQueue->buf_count,
					m_sinks[id]->pAudioQueue->sample_count,
					(u_int32_t)(m_sinks[id]->pAudioQueue->pAudioHead->len/sizeof(int32_t)));
			audio_buffer_t* buf = GetAudioBuffer(m_sinks[id]->pAudioQueue);
u_int32_t no_samples = buf->len/(m_sinks[id]->channels*m_sinks[id]->bytespersample);
	if (no_samples*m_sinks[id]->channels*m_sinks[id]->bytespersample != buf->len)
		fprintf(stderr, "Audio sink drops MISMATCH for buffer size %u\n",buf->len);
			if (buf) {
				free(buf);
			}
			queue_samples = m_sinks[id]->pAudioQueue->sample_count;
		}



if (m_sinks[id]->drop_bytes) fprintf(stderr, "DROPPING -5 sink %u %u\n", id, m_sinks[id]->drop_bytes);

		// Lets check if we have enough bytes to write
		while ((u_int32_t)needed_samples > queue_samples) {
			// First we calculate the samples we need (and add one extra)
			u_int32_t samples = needed_samples - queue_samples;

			// Then we get a buffer
			u_int32_t size = 0;
			audio_buffer_t* newbuf = GetEmptyBuffer(id, &size);

			// If we dont get a buffer we can't do much about it
			if (!newbuf) break;

			// The we figure out how much silence to fill in
			size = size / sizeof(int32_t) > samples ? samples*sizeof(int32_t) : size;

			// Then we fill it with silence and add it to head of queue
			// The sequence number will be zero.
			memset(newbuf->data, 0, size);
			newbuf->len = size;
			newbuf->seq_no = 0;
			AddAudioBufferToHeadOfQueue(m_sinks[id]->pAudioQueue, newbuf);
			queue_samples = m_sinks[id]->pAudioQueue->sample_count;
			if (m_verbose > 1) {
				fprintf(stderr, "Frame %u - audio sink %u added %u samples of silence\n",
					SYSTEM_FRAME_NO, id, (u_int32_t)(newbuf->len/sizeof(int32_t)));
			}
		}

if (m_sinks[id]->drop_bytes) fprintf(stderr, "DROPPING -4 sink %u %u\n", id, m_sinks[id]->drop_bytes);
		// Now if the sink is a file write
		if (m_sinks[id]->sink_fd >= 0) {

			// We need to convert internal format int32_t to si16 or ui16
			// and we need a buffer for that.
			u_int32_t size;
if (m_sinks[id]->drop_bytes) fprintf(stderr, "DROPPING -3 sink %u %u\n", id, m_sinks[id]->drop_bytes);
			audio_buffer_t* samplebuffer = GetEmptyBuffer(id, &size);
			while (needed_samples > 0) {
				if (m_verbose > 2) fprintf(stderr, "Frame %u - audio sink %u "
					"needed samples %u queue samples %u\n",
					SYSTEM_FRAME_NO, id, needed_samples, queue_samples);
				audio_buffer_t* p = m_sinks[id]->pAudioQueue->pAudioHead;
				unsigned count = 0;
				if (m_verbose > 3) fprintf(stderr,
					"Frame %u - audio sink %u counting\n", SYSTEM_FRAME_NO, id);
				while (p) {
					count++;
					p = p->next;
				}
				if (m_verbose > 3) fprintf(stderr,
					"Frame %u - Audio queue said %u counted %u\n",
					SYSTEM_FRAME_NO, m_sinks[id]->pAudioQueue->buf_count, count);
				if (count != m_sinks[id]->pAudioQueue->buf_count) {
					fprintf(stderr, "Frame %u - audio sink %u queue "
						"count mismatch. Counted %u queue says %u\n",
						SYSTEM_FRAME_NO, id,
						count, m_sinks[id]->pAudioQueue->buf_count);
				}
				audio_buffer_t* newbuf = GetAudioBuffer(m_sinks[id]->pAudioQueue);
u_int32_t no_samples = newbuf->len/(m_sinks[id]->channels*m_sinks[id]->bytespersample);
	if (no_samples*m_sinks[id]->channels*m_sinks[id]->bytespersample != newbuf->len)
		fprintf(stderr, "Audio sink drops MISMATCH for buffer size %u\n",newbuf->len);
if (m_sinks[id]->drop_bytes) fprintf(stderr, "DROPPING -2 sink %u %u\n", id, m_sinks[id]->drop_bytes);
				if (!newbuf) {
					fprintf(stderr, "Frame %u - audio sink %u failed to "
						"get audio buffer\n"
						" - audio queue has %u samples and %u buffers\n",
						SYSTEM_FRAME_NO, id,
						m_sinks[id]->pAudioQueue->sample_count,
						m_sinks[id]->pAudioQueue->buf_count);
					break;
				}
				u_int32_t samples = (newbuf->len/sizeof(int32_t)) >
					(size/m_sinks[id]->bytespersample) ?
					(size/m_sinks[id]->bytespersample) :
					(newbuf->len/sizeof(int32_t));
				if (samples > (u_int32_t)needed_samples) samples = needed_samples;
				if (m_sinks[id]->bytespersample == 2) {
					if (m_sinks[id]->is_signed) {
						convert_int32_to_si16((u_int16_t*)samplebuffer->data,
							(int32_t*)newbuf->data, samples);
					} else {
						convert_int32_to_ui16((u_int16_t*)samplebuffer->data,
							(int32_t*)newbuf->data, samples);
					}
				} else {
					fprintf(stderr, "audio sink %u unsupported audio format\n", id);
					break;
				}
				samplebuffer->len = samples*m_sinks[id]->bytespersample;

				if (m_verbose > 2) fprintf(stderr, "Frame %u - audio sink %u will "
					"write %u samples = %u bytes\n"
					" - the queue has %u bytes and %u buffers\n",
					SYSTEM_FRAME_NO, id, samples, samplebuffer->len,
					m_sinks[id]->pAudioQueue->bytes_count,
					m_sinks[id]->pAudioQueue->buf_count);

if (m_sinks[id]->drop_bytes) fprintf(stderr, "DROPPING -1 sink %u %u\n", id, m_sinks[id]->drop_bytes);
				// This adds silence samples to output without affecting
				// SamplesNeeded()
				if (m_sinks[id]->write_silence_bytes) {
					u_int8_t* buf = (u_int8_t *)calloc(m_sinks[id]->write_silence_bytes,1);
					if (buf) {
						int n=write(m_sinks[id]->sink_fd, buf, m_sinks[id]->write_silence_bytes);
						if (n == (signed) m_sinks[id]->write_silence_bytes)
							fprintf(stderr, "wrote %u bytes of silence\n",
								m_sinks[id]->write_silence_bytes);
						else
							fprintf(stderr, "wrote %u bytes of silence\n",
								m_sinks[id]->write_silence_bytes);
						m_sinks[id]->write_silence_bytes = 0;
						free(buf);
					}

				}

				// Check and see if we should drop some output bytes
				int dropped_bytes = 0;
				if (m_sinks[id]->drop_bytes) {
if (m_sinks[id]->drop_bytes) fprintf(stderr, "DROPPING sink %u %u\n", id, m_sinks[id]->drop_bytes);
					if (m_sinks[id]->drop_bytes < samplebuffer->len) {
						samplebuffer->data += m_sinks[id]->drop_bytes;
						samplebuffer->len -= m_sinks[id]->drop_bytes;
						dropped_bytes = m_sinks[id]->drop_bytes;
						m_sinks[id]->drop_bytes = 0;
					} else {
						samplebuffer->data += samplebuffer->len;
						m_sinks[id]->drop_bytes -= samplebuffer->len;
						dropped_bytes = samplebuffer->len;
						samplebuffer->len = 0;
					}
					m_sinks[id]->dropped = dropped_bytes/m_sinks[id]->bytespersample;
					if (m_verbose) fprintf(stderr, "audio mixer %u dropping %u samples per channel on output\n",
						id, dropped_bytes/(m_sinks[id]->channels*m_sinks[id]->bytespersample));
				}
				int n = 0;
				if (samplebuffer->len) {
					n = write(m_sinks[id]->sink_fd, samplebuffer->data, samplebuffer->len);
					if ((n <= 0) && (errno == EAGAIN || errno == EWOULDBLOCK)) n = 0;
					// if n < 0 we failed and we will drop the buffer and stop the sink
					if (n < 0) {
						perror ("Could not write data for audio sink");
						fprintf(stderr, "audio sink %u failed writing buffer to fd %d.\n",
							id, m_sinks[id]->sink_fd);
						if (newbuf) free(newbuf); newbuf = NULL;
						if (samplebuffer) free(samplebuffer); samplebuffer = NULL;
						
						StopSink(id);
						break;
					}
				}
				n += dropped_bytes;
				newbuf->len -= ((sizeof(int32_t)*n)/m_sinks[id]->bytespersample);

				m_sinks[id]->sample_count += (n/(m_sinks[id]->bytespersample*m_sinks[id]->channels));

				if (newbuf->len > 0) {
					newbuf->data += ((sizeof(int32_t)*n)/m_sinks[id]->bytespersample);
					AddAudioBufferToHeadOfQueue(m_sinks[id]->pAudioQueue,newbuf);
					if (m_verbose > 2) fprintf(stderr, "Frame %u - audio sink %u added back buf seq "
#if __WORDSIZE == 32
                        			"no %u to %u\n", SYSTEM_FRAME_NO,
						id, newbuf->seq_no, (u_int32_t) newbuf);
#else
#if __WORDSIZE == 64
                        			"no %u to "FUI64"\n", SYSTEM_FRAME_NO,
						id, newbuf->seq_no, (u_int64_t) newbuf);
#else
#error __WORDSIZE not defined to 32 or 64
#endif
#endif

				} else free(newbuf);
				newbuf = NULL;
				if (n == 0) break;
				needed_samples -= (n/m_sinks[id]->bytespersample);
			}
			if (samplebuffer) free(samplebuffer); samplebuffer=NULL;
		} else {
			fprintf(stderr, "audio sink was not a file write sink\n");
		}
*/
	}
	if (m_verbose > 4) fprintf(stderr, "Frame %u - audio sink update done\n",
		SYSTEM_FRAME_NO);
}
