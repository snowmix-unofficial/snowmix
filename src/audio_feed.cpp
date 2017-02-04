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
#include "audio_feed.h"

CAudioFeed::CAudioFeed(CVideoMixer* pVideoMixer, u_int32_t max_feeds)
{
	m_max_feeds = 0;
	m_feed_count = 0;
	m_feeds = (audio_feed_t**) calloc(
		sizeof(audio_feed_t*), max_feeds);
	if (m_feeds) {
		m_max_feeds = max_feeds;
	}
	m_pVideoMixer = pVideoMixer;
	m_verbose = 0;
	m_animation = 0;
}

CAudioFeed::~CAudioFeed()
{
	if (m_feeds) {
		for (unsigned int id=0; id<m_max_feeds; id++) DeleteFeed(id);
		free(m_feeds);
		m_feeds = NULL;
	}
	m_max_feeds = 0;
}

// audio feed ....
int CAudioFeed::ParseCommand(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) return -1;

	//if (!m_pVideoMixer) return 0;
	// audio feed status
	if (!strncasecmp (str, "status ", 7)) {
		return set_feed_status(ctr, str+7);
	}
	// audio feed add silence [<feed id> <silence in ms>]]
	else if (!strncasecmp (str, "add silence ", 12)) {
		return set_feed_add_silence(ctr, str+12);
	}
	// audio feed drop <sink id> <ms>]
	else if (!strncasecmp (str, "drop ", 5)) {
		return set_drop(ctr, str+5);
	}
	// audio feed info
	if (!strncasecmp (str, "info ", 5)) {
		return list_info(ctr, str+5);
	}
	// audio feed add [<feed id> [<feed name>]]
	if (!strncasecmp (str, "add ", 4)) {
		return set_feed_add(ctr, str+4);
	}
	// audio feed channels [<feed id> <channels>]
	else if (!strncasecmp (str, "channels ", 9)) {
		return set_feed_channels(ctr, str+9);
	}
	// audio feed rate [<feed id> <rate>]
	else if (!strncasecmp (str, "rate ", 5)) {
		return set_feed_rate(ctr, str+5);
	}
	// audio feed format [<feed id> (8 | 16 | 32 | 64) (signed | unsigned | float) ]
	else if (!strncasecmp (str, "format ", 7)) {
		return set_feed_format(ctr, str+7);
	}
	// audio feed ctr isaudio <feed id>
	else if (!strncasecmp (str, "ctr isaudio ", 12)) {
		return set_ctr_isaudio(ctr, str+12);
	}
	// audio feed volume [<feed id> <volume> [<volume> ....]] // volume = 0..MAX_VOLUME
	else if (!strncasecmp (str, "volume ", 7)) {
		return set_volume(ctr, str+7);
	}
	// audio feed move volume [<feed id> <delta volume> <steps>]
	else if (!strncasecmp (str, "move volume ", 12)) {
		return set_move_volume(ctr, str+12);
	}
	// audio feed mute [(on | off) <feed id>]
	else if (!strncasecmp (str, "mute ", 5)) {
		return set_mute(ctr, str+5);
	}
	// audio feed queue
	else if (!strncasecmp (str, "queue ", 6)) {
		return list_queue(ctr, str+6);
	}
	// audio feed delay [ <delay in ms>]
	else if (!strncasecmp (str, "delay ", 6)) {
		return set_delay(ctr, str+6);
	}
	// audio feed verbose [<verbose level>]
	else if (!strncasecmp (str, "verbose ", 8)) {
		return set_verbose(ctr, str+8);
	}
	// audio feed help
	else if (!strncasecmp (str, "help ", 5)) {
		return list_help(ctr, str+5);
	} else return 1;
	return 0;
}

#define AUDIOTYPE "audio feed"
#define AUDIOCLASS "CAudioFeed"
// Query type 0=info 1=status 2=extended
// format	: format 
// ids		: list of IDs
// maxid	: max number of ID
// nextavail	: Next available ID
// id or ids	: listing each ID
char* CAudioFeed::Query(const char* query, bool tcllist)
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
       	else if (!(strncasecmp("syntax", query, 6))) {
		return strdup("audio (feed | sink) (info | status | extended | syntax) (format | ids | maxid | nextavail | <id_list>)");
	} else return NULL;

	while (isspace(*query)) query++;
	maxid = MaxFeeds();
	if (!strncasecmp(query, "format", 6)) {
		char* str = NULL;
		if (type == 0) {
			if (tcllist) str = strdup("id rate channels signed bytespersample inidelay");
			else str = strdup(AUDIOTYPE" <id> <rate> <channels> <format> <inidelay>");
		}
		else if (type == 1) {
			if (tcllist) str = strdup("id state starttime mute volume");
			else str = strdup(AUDIOTYPE" <id> <state> <starttime> <mute> <volume>");
		}
		else if (type == 2) {
			if (tcllist) str = strdup("id seq_no received silence dropped clipped delay rms");
			else str = strdup(AUDIOTYPE" <id> <seq_no> <received> <silence> <dropped> <clipped> <delay> <rms>");
		}
		if (!str) fprintf(stderr, AUDIOCLASS"::Query failed to allocate memory\n");
		return str;
	}
	else if (!strncasecmp(query, "maxid", 5)) {
		int len = ((typeof(len))log10((double)maxid)) + 3;	// in case of rounding error
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
		int len = ((typeof(len))log10((double)maxid)) + 3;	// in case of rounding error
		char* str = (char*) malloc(len);
		if (!str) {
			fprintf(stderr, AUDIOCLASS"::Query failed to allocate memory\n");
			return NULL;
		}
		*str = '\0';
		for (u_int32_t i = 0; i < maxid ; i++) {
			if (m_feeds[i]) {
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
				if (m_feeds[i]) {
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
			if (!(m_feeds[from])) { from++ ; continue; }
			if (type == 0) {
				// id rate channels signed bytespersample delay");
				// 1234567890123456789012345678901234567890123456789012345678901234567890
				// audio feed 1234567890 1234567890 1234567890 unsigned 16 01234567890
				len += 68;
			}
			else if (type == 1) {
				// id state starttime mute volume
				// audio feed 01234567890 DISCONNECTED 1234567890.123 unmuted 0.12345,0.12345 .... 
				// 12345678901234567890123456789012345678901234567890123456789
				len += (60 + m_feeds[from]->channels * 9);
			}
			else if (type == 2) {
				// id seq_no received silence dropped clipped delay rms
				//          1         2         3         4         5         7         8
				// 12345678901234567890123456789012345678901234567890123456789012345678901234567
				// audio feed 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890,1234567890 1234567890,1234567890....
				len += (88 + m_feeds[from]->channels * 22);
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
		if (!nextstr) return NULL;
		while (from <= to) {
			if (!m_feeds[from]) {
				from++;
				continue;
			}
			audio_feed_t* p = m_feeds[from];
			if (type == 0) {
				// id rate channels signed bytespersample delay");
				sprintf(pstr, (tcllist ? "{%u %u %u %s %u %u} " : AUDIOTYPE" %u %u %u %s %u %u\n"), 
					from, p->rate, p->channels,
					p->is_signed ? "signed" : "unsigned",
					p->bytespersample, p->delay);
			}
			else if (type == 1) {
				// id state starttime mute volume......
#ifdef HAVE_MACOSX                  // suseconds_t is long int on OS X 10.9
				sprintf(pstr, "%s%u %s %ld.%03d %smuted %s", tcllist ? "{" : AUDIOTYPE,
#else                               // suseconds_t is unsigned int on Linux
				sprintf(pstr, "%s%u %s %ld.%06lu %smuted %s", tcllist ? "{" : AUDIOTYPE,
#endif
					from, feed_state_string(p->state), p->start_time.tv_sec, p->start_time.tv_usec/1000,
					p->mute ? "" : "un", tcllist ? "{" : "");
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
				sprintf(pstr, "%s%u %u %u %u %u %u %s%s",
					tcllist ? "{" : AUDIOTYPE,
					from, p->buf_seq_no, p->samples_rec, p->silence, p->dropped, p->clipped,
					p->pAudioQueues ? "" : " 0 ",
					tcllist ? "{" : "");
				while (*pstr) pstr++;

				audio_queue_t* pQueue = p->pAudioQueues;
				while (pQueue) {
					sprintf(pstr, "%u%s", 1000*pQueue->sample_count/p->calcsamplespersecond,
						pQueue->pNext ?  (tcllist ? " " : ",") : (tcllist ? "} {" : " "));
					pQueue = pQueue->pNext;
					while (*pstr) pstr++;
				}

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
			while (*pstr) pstr++;
			from++;
		}
		while (isspace(*nextstr)) nextstr++;
		str = nextstr;
	}
	return retstr;
}

// help list for audio feed
int CAudioFeed::list_help(struct controller_type* ctr, const char* str)
{
	if (m_pVideoMixer && m_pVideoMixer->m_pController)
		m_pVideoMixer->m_pController->controller_write_msg (ctr, MESSAGE"Commands:\n"
		MESSAGE"audio feed add [<feed id> [<feed name>]]  "
			"// empty <feed name> deletes entry\n"
		MESSAGE"audio feed add silence <feed id> <silence in ms>\n"
		MESSAGE"audio feed channels [<feed id> <channels>]\n"
		MESSAGE"audio feed ctr isaudio <feed id>\n"
		MESSAGE"audio feed delay [<delay in ms>]\n"
		MESSAGE"audio feed drop <feed id> <drop in ms>\n"
		MESSAGE"audio feed format [<feed id> (8 | 16 | 24 | 32 | 64) "
			"(signed | unsigned | float)]\n"
		MESSAGE"audio feed info\n"
		MESSAGE"audio feed move volume [<sink id> <delta volume> <delta steps>]\n"
		MESSAGE"audio feed mute [(on | off) <feed id>]\n"
		MESSAGE"audio feed rate [<feed id> <rate>]\n"
		MESSAGE"audio feed status\n"
		MESSAGE"audio feed verbose [<verbose level>]\n"
		MESSAGE"audio feed volume [<feed id> (- | <volume>) [(- | <volume> ...)]] // volume = 0..MAX_VOLUME\n"
		MESSAGE"audio feed help\n"
		MESSAGE"\n"
		);
	return 0;
}

// audio feed info
int CAudioFeed::list_info(struct controller_type* ctr, const char* str)
{
	if (!ctr || !m_pVideoMixer || !m_pVideoMixer->m_pController || !m_feeds) return 1;
	m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS" audio feed info\n"
		STATUS" audio feeds       : %u\n"
		STATUS" max audio feeds   : %u\n"
		STATUS" verbose level     : %u\n",
		m_feed_count,
		m_max_feeds,
		m_verbose
		);
	if (m_feed_count) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS" audio feed id : state, rate, channels, bytespersample, "
			"signess, volume, mute, buffersize, delay, queues\n");
		for (u_int32_t id=0; id < m_max_feeds; id++) {
			if (!m_feeds[id]) continue;
			u_int32_t queue_count = 0;
			audio_queue_t* pQueue = m_feeds[id]->pAudioQueues;
			while (pQueue) {
				queue_count++;
				pQueue = pQueue->pNext;
			}
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				STATUS" - audio feed %u : %s, %u, %u, %u, %ssigned, %s", id,
				feed_state_string(m_feeds[id]->state),
				m_feeds[id]->rate,
				m_feeds[id]->channels,
				m_feeds[id]->bytespersample,
				m_feeds[id]->is_signed ? "" : "un",
				m_feeds[id]->pVolume ? "" : "0,");
			if (m_feeds[id]->pVolume) {
				float* p = m_feeds[id]->pVolume;
				for (u_int32_t i=0 ; i < m_feeds[id]->channels; i++, p++)
					m_pVideoMixer->m_pController->controller_write_msg (ctr,
						"%.3f,", *p);
			}
			m_pVideoMixer->m_pController->controller_write_msg (ctr, " %smuted, %u, %u, %u\n",
				m_feeds[id]->mute ? "" : "un",
				m_feeds[id]->buffer_size,
				m_feeds[id]->delay,
				queue_count);
		}
	}
	m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
	return 0;
}

// audio sink verbose [<level>] // set control channel to write out
int CAudioFeed::set_verbose(struct controller_type* ctr, const char* str)
{
	int n;
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (m_verbose) m_verbose = 0;
		else m_verbose = 1;
		if (m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				STATUS"audio feed verbose %s\n", m_verbose ? "on" : "off");
		return 0;
	}
	while (isspace(*str)) str++;
	n = sscanf(str, "%u", &m_verbose);
	if (n != 1) return -1;
	if (m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"audio feed verbose level set to %u for CAudioFeed\n",
			m_verbose);
	return 0;
}

// audio feed ctr isaudio <feed id> // Create/delete/list an audio feed
int CAudioFeed::set_ctr_isaudio(struct controller_type* ctr, const char* str)
{
	u_int32_t id;
	int n;
	if (!str || !ctr) return -1;
	if (!m_feeds) return 1;
	while (isspace(*str)) str++;
	n = sscanf(str, "%u", &id);
	if (n != 1 || id < 1) return -1;
	if (!m_feeds[id]) {
		ctr->close_controller = true;
		if (m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"controller requested to change to audio feed %u, "
				"but the feed is not defined. Closing\n",id);
		return 0;
	}
	audio_feed_t* p= m_feeds[id];
	if (m_verbose) fprintf(stderr, "Frame %u - controller input changed to "
		"feed for audio feed %u. String = <%s>\n", SYSTEM_FRAME_NO, id, str);
	ctr->is_audio = true;
	ctr->feed_id = id;
	gettimeofday(&(p->start_time), NULL);
	SetFeedBufferSize(id);
	if (m_verbose) fprintf(stderr, "Frame %u - audio feed %u changing state from %s to %s\n",
		SYSTEM_FRAME_NO, id,
		feed_state_string(p->state),
		feed_state_string(PENDING));
	p->state = PENDING;
	audio_queue_t* pQueue = p->pAudioQueues;
	while (pQueue) {
		if (m_verbose) fprintf(stderr, "Frame %u - audio feed %u queue state set to PENDING\n",
			SYSTEM_FRAME_NO, id);
		pQueue->state = PENDING;
		pQueue = pQueue->pNext;
	}
	p->start_time.tv_sec		= 0;
	p->start_time.tv_usec		= 0;
	p->buf_seq_no			= 0;
	p->samples_rec			= 0;
	p->silence			= 0;
	p->dropped			= 0;
	p->clipped			= 0;
	u_int64_t* prms = p->rms;
	for (u_int32_t i=0; prms && i < p->channels; prms++, i++) *prms = 0;
	p->update_time.tv_sec		= 0;
	p->update_time.tv_usec		= 0;
	p->samplespersecond		= 0.0;
	p->last_samples			= 0;
	p->total_avg_sps		= 0.0;
	for (p->avg_sps_index = 0; p->avg_sps_index < MAX_AVERAGE_SAMPLES; p->avg_sps_index++)
		p->avg_sps_arr[p->avg_sps_index] = 0.0;
	p->avg_sps_index		= 0;
	return 0;
}

void CAudioFeed::StopFeed(u_int32_t id)
{
	if (id >= m_max_feeds || !m_feeds[id]) return;
	audio_feed_t* p= m_feeds[id];
	p->state = SETUP;
	if (p->left_over_buf) {
		free(p->left_over_buf);
		p->left_over_buf = NULL;
	}
	SetFeedBufferSize(id);
	audio_queue_t* pQueue = p->pAudioQueues;
	while (pQueue) {
		if (m_verbose) fprintf(stderr, "Frame %u - audio feed %u queue state set to %s\n",
			SYSTEM_FRAME_NO, id, feed_state_string(READY));
		pQueue->state = READY;
		pQueue = pQueue->pNext;
	}
}

// audio feed status
int CAudioFeed::set_feed_status(struct controller_type* ctr, const char* str)
{
	//u_int32_t id;
	//int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_feeds) return 1;
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS"feed_id : state samples samplespersecond avg_samplespersecond silence dropped clipped delay rms");
		for (unsigned id=0 ; id < m_max_feeds; id++) if (m_feeds[id]) {
			audio_queue_t* pQueue = m_feeds[id]->pAudioQueues;
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				"\n"STATUS"audio feed %u : %s %u %.0lf %.0lf %u %u %u %s",
				id, feed_state_string(m_feeds[id]->state),
				m_feeds[id]->samples_rec,
				m_feeds[id]->samplespersecond,
				m_feeds[id]->total_avg_sps/MAX_AVERAGE_SAMPLES,
				m_feeds[id]->silence,
				m_feeds[id]->dropped,
				m_feeds[id]->clipped,
				pQueue ? "" : "0 ");
			while (pQueue) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr, "%u%s",
					1000*pQueue->sample_count/m_feeds[id]->calcsamplespersecond,
					pQueue->pNext ? "," : " ");
				pQueue = pQueue->pNext;
			}
			// RMS * 100 / 32767 = percentage
			for (u_int32_t i=0; i < m_feeds[id]->channels; i++) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%.1lf%s", (double) (sqrt((double)(m_feeds[id]->rms[i]))/327.67),
					i+1 < m_feeds[id]->channels ? "," : "");
			}

		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"\n"STATUS"\n");
		return 0;

	}
	return -1;
}

// audio feed mute (on | off) <feed_id> 
int CAudioFeed::set_mute(struct controller_type* ctr, const char* str)
{
	u_int32_t id;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_feeds) return 1;
		for (id=0 ; id < m_max_feeds; id++) {
			if (!m_feeds[id]) continue;
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				STATUS"audio feed %2d volume %s", id,
				m_feeds[id]->pVolume ? "" : "0");
			if (m_feeds[id]->pVolume) {
				float* p = m_feeds[id]->pVolume;
				for (u_int32_t i=0 ; i < m_feeds[id]->channels; i++, p++) {
					m_pVideoMixer->m_pController->controller_write_msg (ctr,
						"%.3f%s", *p, i+1 < m_feeds[id]->channels ? "," : "");
				}
			}
			m_pVideoMixer->m_pController->controller_write_msg (ctr, "%s\n",
				m_feeds[id]->mute ? " muted" : "");
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS"\n");
		return 0;

	}

	// Get the feed id.
	if ((n = sscanf(str, "on %u", &id)) == 1) {
		n = SetMute(id, true);
		if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio feed %u mute\n", id);
		return n;
	}
	else if ((n = sscanf(str, "off %u", &id)) == 1) {
		n = SetMute(id, false);
		if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio feed %u unmute\n", id);
		return n;
	}
	else return -1;
}

// audio feed move volume [<feed_id> <delta volume> <steps>]
int CAudioFeed::set_move_volume(struct controller_type* ctr, const char* str)
{
	u_int32_t id, steps;
	float volume_delta;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_feeds) return 1;
		for (id=0 ; id < m_max_feeds; id++) {
			if (!m_feeds[id]) continue;
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				STATUS"audio feed %2d volume %s", id,
				m_feeds[id]->pVolume ? "" : "0");
			if (m_feeds[id]->pVolume) {
				float* p = m_feeds[id]->pVolume;
				for (u_int32_t i=0 ; i < m_feeds[id]->channels; i++, p++) {
					m_pVideoMixer->m_pController->controller_write_msg (ctr,
						"%.3f%s", *p, i+1 < m_feeds[id]->channels ? "," : "");
				}
			}
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				"%smuted delta %.4f steps %u\n",
				m_feeds[id]->mute ? " " : " un",
				m_feeds[id]->volume_delta, m_feeds[id]->volume_delta_steps);
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS"\n");
		return 0;

	}

	// Get the feed id.
	if ((n = sscanf(str, "%u %f %u", &id, &volume_delta, &steps)) == 3) {
		n = SetMoveVolume(id, volume_delta, steps);
		if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio feed %u delta volume %.4f steps %u\n",
				id, volume_delta, steps);
		return n;
	}
	else return -1;
}

// audio feed volume <feed_id> <volume>
int CAudioFeed::set_volume(struct controller_type* ctr, const char* str)
{
	u_int32_t id, channel;
	float volume;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_feeds) return 1;
		for (id=0 ; id < m_max_feeds; id++) {
			if (!m_feeds[id]) continue;
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				STATUS"audio feed %2d volume %s", id,
				m_feeds[id]->pVolume ? "" : "0");
			if (m_feeds[id]->pVolume) {
				float* p = m_feeds[id]->pVolume;
				for (u_int32_t i=0 ; i < m_feeds[id]->channels; i++, p++) {
					m_pVideoMixer->m_pController->controller_write_msg (ctr,
						"%.3f%s", *p, i+1 < m_feeds[id]->channels ? "," : "");
				}
			}
			m_pVideoMixer->m_pController->controller_write_msg (ctr, "%s\n",
				m_feeds[id]->mute ? " muted" : "");
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS"\n");
		return 0;

	}

	// Get the feed id.
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
				MESSAGE"audio feed %u volume %s\n", id, volume_list);
		return 0;
	}
	else return -1;
}

// audio feed add [<feed id> [<feed name>]] // Create/delete/list an audio feed
int CAudioFeed::set_feed_add(struct controller_type* ctr, const char* str)
{
	u_int32_t id;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_feeds) return 1;
		for (id = 0; id < m_max_feeds ; id++)
			if (m_feeds[id])
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS"audio feed %u <%s>\n", id,
					m_feeds[id]->name ? m_feeds[id]->name : "");
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS"\n");
		return 0;

	}
	// Get the feed id.
	if ((n = sscanf(str, "%u", &id)) != 1) return -1;
	while (isdigit(*str)) str++; while (isspace(*str)) str++;

	//n = -1;
	// Now if eos, only id was given and we delete the feed
	if (!(*str)) {
		if ((n = DeleteFeed(id))) return n;
		if (m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio feed %u deleted\n");
	} else {

		// Now we know there is more after the id and we can create the feed
		if ((n = CreateFeed(id))) return n;
		m_feed_count++;
		if ((n = SetFeedName(id, str))) {
			DeleteFeed(id);
			return n;
		}
		if (m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio feed %u <%s> added\n", id, m_feeds[id]->name);
	}
	return n;
}

// audio feed channels [<feed id> <channels>]] // Set number of channels for an audio feed
int CAudioFeed::set_feed_channels(struct controller_type* ctr, const char* str)
{
	u_int32_t id, channels;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	// Check to see if we should list channels
	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_feeds) return 1;
		for (id = 0; id < m_max_feeds ; id++)
			if (m_feeds[id])
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS"audio feed %u channels %u\n", id,
					m_feeds[id]->channels);
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		return 0;
	}
	// Get the feed id.
	if ((n = sscanf(str, "%u %u", &id, &channels)) != 2) return -1;
	if (!(n = SetFeedChannels(id, channels))) 
		if (m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio feed %u channels %u\n", id, channels);
	return n;
}

// audio feed rate [<feed id> <rate in Hz>]] // Set sample rate for an audio feed
int CAudioFeed::set_feed_rate(struct controller_type* ctr, const char* str)
{
	u_int32_t id, rate;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	// Check to see if we should list rate
	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_feeds) return 1;
		for (id = 0; id < m_max_feeds ; id++)
			if (m_feeds[id])
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS"audio feed %u rate %u Hz\n", id,
					m_feeds[id]->rate);
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		return 0;
	}
	// Get the feed id and rate.
	if ((n = sscanf(str, "%u %u", &id, &rate)) != 2) return -1;
	if (!(n = SetFeedRate(id, rate))) 
		if (m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio feed %u sample rate %u Hz\n", id, rate);
	return n;
}

// audio feed format [<feed id> (8 | 16 | 32 | 64) (signed | unsigned | float) ]
// Set format for audio feed
int CAudioFeed::set_feed_format(struct controller_type* ctr, const char* str)
{
	u_int32_t id, bits;
	int n;
	bool is_signed = false;

	if (!str) return -1;
	while (isspace(*str)) str++;

	// Check to see if we should list rate
	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController || !m_feeds) return 1;
		for (id = 0; id < m_max_feeds ; id++)
			if (m_feeds[id])
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS"audio feed %u is %s and has %u bytes per "
					"sample\n", id,
					m_feeds[id]->is_signed ? "signed" : "unsigned",
					m_feeds[id]->bytespersample);
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		return 0;
	}
	// Get the feed id.
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
	if (!(n = SetFeedFormat(id, bits >> 3, is_signed)))
		if (m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio feed %u sample is %u bytes per "
				"sample and %s\n", id, bits >> 3, is_signed ?
				"signed" : "unsigned");
	return n;
}

// audio feed queue
int CAudioFeed::list_queue(struct controller_type* ctr, const char* str)
{
	if (!m_feeds || !m_pVideoMixer || !m_pVideoMixer->m_pController) return -1;
	for (u_int32_t id = 0; id < m_max_feeds ; id++) {
		if (m_feeds[id]) {
			if (!m_feeds[id]->pAudioQueues) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS"audio feed %u has no queues\n", id);
				continue;
			}
			audio_queue_t* pQueue = m_feeds[id]->pAudioQueues;
			while (pQueue) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS"audio feed %u samples %u (%u ms) average %u (%u ms) max %u (%u)\n", id,
					pQueue->sample_count, 
					1000*pQueue->sample_count/(m_feeds[id]->channels*m_feeds[id]->rate),
					pQueue->queue_total/QUEUE_SAMPLES,
					(1000*pQueue->queue_total/QUEUE_SAMPLES)/(m_feeds[id]->channels*m_feeds[id]->rate),
					pQueue->queue_max,
					(1000*pQueue->queue_max)/(m_feeds[id]->channels*m_feeds[id]->rate)
					);
				pQueue = pQueue->pNext;
			
			}
		}
	}
	m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
	return 0;
}

// audio feed delay [<id> <delay in ms>]
int CAudioFeed::set_delay(struct controller_type* ctr, const char* str)
{
	if (!m_feeds || !str || !m_pVideoMixer || !m_pVideoMixer->m_pController) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		for (u_int32_t id = 0; id < m_max_feeds ; id++) {
			if (m_feeds[id]) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS"audio feed %u delay %u ms\n",
					id, m_feeds[id]->delay);
			}
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS"\n");
		return 0;
	}
	u_int32_t id, delay;
	if ((sscanf(str, "%u %u", &id, &delay) == 2)) {
		int n = SetFeedDelay(id, delay);
		if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: audio feed %u delay %u ms\n", id, delay);
		return n;
	}
	return -1;
}


// audio feed drop <feed id> <ms>
int CAudioFeed::set_drop(struct controller_type* ctr, const char* str)
{
	u_int32_t id, ms;
	int n;

	if (!str) return -1;
	while (isspace(*str)) str++;

	// Get the feed id.
	if ((n = sscanf(str, "%u %u", &id, &ms)) == 2) {
		if (m_feeds && m_feeds[id] && (m_feeds[id]->state == RUNNING ||
			m_feeds[id]->state == STALLED)) {
			if (m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					MESSAGE"Frame %u - audio feed %u dropping %u ms per channel\n",
					SYSTEM_FRAME_NO, id, ms);
			m_feeds[id]->drop_bytes = ms * m_feeds[id]->bytespersample *
				m_feeds[id]->channels * m_feeds[id]->rate/1000;
			return 0;
		}
	}
	return -1;
}

// audio feed silence <id> <silence in ms>
int CAudioFeed::set_feed_add_silence(struct controller_type* ctr, const char* str)
{
	if (!m_feeds || !str) return -1;
	while (isspace(*str)) str++;
	u_int32_t id, silence;
	int32_t n = -1;
	if ((sscanf(str, "%u %u", &id, &silence) == 2)) {
		if ((n = SetFeedSilence(id, silence))) return n;
		if (m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"audio feed %u added %u ms of silence\n",
				id, silence);
	}
	return n;
}

// Create an audio feed
int CAudioFeed::CreateFeed(u_int32_t id)
{
	if (id >= m_max_feeds || !m_feeds) return -1;
	audio_feed_t* p = m_feeds[id];
	if (!p) p = (audio_feed_t*) calloc(sizeof(audio_feed_t),1);
	if (!p) return -1;
	m_feeds[id]		= p;
	p->name			= NULL;
	p->id			= id;
	p->state		= SETUP;
	//p->previous_state	= SETUP;
	p->channels		= 0;
	p->rate			= 0;
	p->is_signed		= true;
	p->bytespersample	= 0;
	p->calcsamplespersecond	= 0;
	p->mute			= true;
	p->start_time.tv_sec	= 0;
	p->start_time.tv_usec	= 0;
	p->buffer_size		= 0;
	p->pAudioQueues		= NULL;
	//p->monitor_queue	= NULL;
	//p->volume		= DEFAULT_VOLUME;
	p->pVolume		= NULL;
	p->delay		= 0;
	p->drop_bytes		= 0;
	p->buf_seq_no		= 0;
	p->samples_rec		= 0;
	p->silence		= 0;
	p->dropped		= 0;
	p->clipped		= 0;
	p->rms			= NULL;
	
	p->update_time.tv_sec	= 0;
	p->update_time.tv_usec	= 0;
	p->samplespersecond	= 0.0;
	p->last_samples		= 0;
	p->total_avg_sps	= 0.0;
	for (p->avg_sps_index = 0; p->avg_sps_index < MAX_AVERAGE_SAMPLES; p->avg_sps_index++)
		p->avg_sps_arr[p->avg_sps_index] = 0.0;
	p->avg_sps_index	= 0;
	//if (m_verbose) fprintf(stderr, "audio feed %u added\n", id);

	return 0;
}

// Delete an audio feed
int CAudioFeed::DeleteFeed(u_int32_t id)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return -1;
	m_feed_count--;
	if (m_feeds[id]->name) free(m_feeds[id]->name);
	if (m_feeds[id]->pVolume) free(m_feeds[id]->pVolume);
	if (m_feeds[id]->rms) free(m_feeds[id]->rms);
	if (m_feeds[id]->left_over_buf) {
		free(m_feeds[id]->left_over_buf);
		m_feeds[id]->left_over_buf = NULL;
	}
	free(m_feeds[id]);
	m_feeds[id] = NULL;
	if (m_verbose) fprintf(stderr, "Deleted audio feed %u\n",id);
	return 0;
}

// Set feed name
int CAudioFeed::SetFeedName(u_int32_t id, const char* feed_name)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id] || !feed_name)
		return -1;
	m_feeds[id]->name = strdup(feed_name);
	trim_string(m_feeds[id]->name);			// No memory leak
	return m_feeds[id]->name ? 0 : -1;
}

// Set feed channels
int CAudioFeed::SetFeedChannels(u_int32_t id, u_int32_t channels)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id] || !channels)
		return -1;
	m_feeds[id]->channels = channels;
	if (m_feeds[id]->rms) free(m_feeds[id]->rms);
	if (m_feeds[id]->pVolume) free(m_feeds[id]->pVolume);
	m_feeds[id]->pVolume = (float*) calloc(channels,sizeof(float));
	m_feeds[id]->rms = (u_int64_t*) calloc(channels,sizeof(u_int64_t));	// No memory leak
	if (!m_feeds[id]->pVolume || !m_feeds[id]->rms) {
		if (m_feeds[id]->rms) free(m_feeds[id]->rms);
		if (m_feeds[id]->pVolume) free(m_feeds[id]->pVolume);
		m_feeds[id]->rms = NULL;
		m_feeds[id]->pVolume = NULL;
		return 1;
	}
	for (u_int32_t i=0 ; i < channels ; i++) m_feeds[id]->pVolume[i] = DEFAULT_VOLUME_FLOAT;
	SetFeedBufferSize(id);
	return 0;
}

// Set feed rate
int CAudioFeed::SetFeedRate(u_int32_t id, u_int32_t rate)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id])
		return -1;
	m_feeds[id]->rate = rate;
	SetFeedBufferSize(id);
	return 0;
}
// Set feed delay
int CAudioFeed::SetFeedDelay(u_int32_t id, u_int32_t delay)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id])
		return -1;
	m_feeds[id]->delay = delay;
	return 0;
}

// Set feed Silence
int CAudioFeed::SetFeedSilence(u_int32_t id, u_int32_t silence)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id] || !m_feeds[id]->pAudioQueues)
		return -1;
	audio_queue_t* pQueue = m_feeds[id]->pAudioQueues;
	bool added = false;
	while (pQueue) {
		if (!AddSilence(pQueue, id, silence)) added = true;
		pQueue = pQueue->pNext;
	}
	if (added) m_feeds[id]->silence += (silence*m_feeds[id]->rate*m_feeds[id]->channels/1000);
	return (!added);
}
// Set feed format
int CAudioFeed::SetFeedFormat(u_int32_t id, u_int32_t bytes, bool is_signed)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id])
		return -1;
	m_feeds[id]->bytespersample = bytes;
	m_feeds[id]->is_signed = is_signed;
	SetFeedBufferSize(id);
	return 0;
}

// Calculate feed buffer size
int CAudioFeed::SetFeedBufferSize(u_int32_t id)
{
	// Make checks
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id] || !m_pVideoMixer ||
		!m_feeds[id]->channels || !m_feeds[id]->bytespersample ||
		!m_feeds[id]->rate || m_pVideoMixer->m_frame_rate == 0) {
		if (m_feeds && m_feeds[id]) m_feeds[id]->buffer_size = 0;
		return -1;
	}

	m_feeds[id]->calcsamplespersecond = m_feeds[id]->channels*m_feeds[id]->rate;

	// Calculate buffersize 
	int32_t size = (typeof(size))(m_feeds[id]->channels*m_feeds[id]->bytespersample*
		m_feeds[id]->rate/ m_pVideoMixer->m_frame_rate);

	// make size a power of 2
	int32_t n = 0;
	int32_t i = size;
	for (n=0 ; i ; n++) i >>=  1;
	m_feeds[id]->buffer_size = (1 << n);
	if (m_verbose) fprintf(stderr,
		"audio feed %u buffer size need %u but set to %u bytes\n",
		id, size, m_feeds[id]->buffer_size);
	if (m_feeds[id]->buffer_size) {
		if (m_verbose && m_feeds[id]->state != READY)
			fprintf(stderr, "audio feed %u changed state from %s to %s\n",
				id, feed_state_string(m_feeds[id]->state), 
				feed_state_string(READY)); 
		m_feeds[id]->state = READY;
	}
	return m_feeds[id]->buffer_size;
}

// Returns the number of channels for feed id
u_int32_t CAudioFeed::GetChannels(u_int32_t id)
{
	// Make checks
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return 0;
	return (m_feeds[id]->channels);
}

// Returns the sample rate for feed id
u_int32_t CAudioFeed::GetRate(u_int32_t id)
{
	// Make checks
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return 0;
	return (m_feeds[id]->rate);
}

// Returns the number bytes per sample for feed id
u_int32_t CAudioFeed::GetBytesPerSample(u_int32_t id)
{
	// Make checks
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return 0;
	return (m_feeds[id]->bytespersample);
}

int CAudioFeed::AddSilence(audio_queue_t* pQueue, u_int32_t id, u_int32_t ms)
{
	if (!pQueue || !ms || id >= m_max_feeds || !m_feeds || !m_feeds[id] ||
		!m_feeds[id]->channels || !m_feeds[id]->rate) return -1;
	u_int32_t added = 0;
	u_int32_t samples = ms*m_feeds[id]->channels*m_feeds[id]->rate/1000;
	u_int32_t size;
	while (added < samples) {
		audio_buffer_t* buf = GetEmptyBuffer(id, &size);
		if (!buf) {
			fprintf(stderr, "Failed to get buffer for adding silence\n");
			return 1;
		}
		memset(buf->data, 0, size);
		buf->mute = true;
		if (size >= (samples-added)*sizeof(int32_t)) {
			buf->len = (samples-added)*sizeof(int32_t);
			added += (samples-added);
		} else {
			buf->len = size;
			added += (size/sizeof(int32_t));
		}
		//m_feeds[id]->silence += (buf->len/(m_feeds[id]->channels*sizeof(int32_t)));
		if (m_verbose) fprintf(stderr, "audio feed %u added %u samples\n",
			id, (u_int32_t)(buf->len/sizeof(int32_t)));
		AddAudioBufferToHeadOfQueue(pQueue, buf);
	}
	return 0;
}


// Allocate an empty audio buffer. If bytespersample > 0 we need to data size
audio_buffer_t* CAudioFeed::GetEmptyBuffer(u_int32_t id, u_int32_t* size, u_int32_t bytespersample)
{
	// Make checks
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id] || !m_feeds[id]->buffer_size)
		return NULL;

	// Set data size
	u_int32_t datasize = bytespersample ?
		bytespersample*m_feeds[id]->buffer_size/m_feeds[id]->bytespersample :
		m_feeds[id]->buffer_size;

	// Allocate audio buffer and intialize it
	audio_buffer_t* p = (audio_buffer_t*)
		malloc(sizeof(audio_buffer_t) + datasize +
			m_feeds[id]->channels*sizeof(u_int64_t));
	if (p) {
		//if (size) *size = m_feeds[id]->buffer_size;
		if (size) *size = datasize;
		p->data		= ((u_int8_t*)p)+sizeof(audio_buffer_t);
		p->size		= datasize;
		p->len		= 0;
		p->next		= NULL;
		p->mute		= false;
		p->clipped	= false;
		p->channels	= m_feeds[id]->channels;
		p->rate		= m_feeds[id]->rate;
		p->rms		= (u_int64_t*)(p->data+datasize);
		for (u_int32_t i=0 ; i < m_feeds[id]->channels ; i++)
			p->rms[i] = 0;
	}
	return p;
}

// Set volume level for feed
int CAudioFeed::SetMoveVolume(u_int32_t feed_id, float volume_delta, u_int32_t steps)
{
	if (feed_id >= m_max_feeds || !m_feeds || !m_feeds[feed_id]) return -1;
	m_feeds[feed_id]->volume_delta = volume_delta;
	m_feeds[feed_id]->volume_delta_steps = steps;
	if (m_animation < steps) m_animation = steps;
	return 0;
}
// Set volume level for feed
int CAudioFeed::SetVolume(u_int32_t feed_id, u_int32_t channel, float volume)
{
	if (feed_id >= m_max_feeds || !m_feeds || !m_feeds[feed_id] ||
		channel >= m_feeds[feed_id]->channels) return -1;
	if (volume < 0.0) volume = 0.0;
	if (volume > MAX_VOLUME_FLOAT) volume = MAX_VOLUME_FLOAT;
	m_feeds[feed_id]->pVolume[channel] = volume;
	return 0;
}

/*
// Set volume level for feed
int CAudioFeed::SetVolumeold(u_int32_t feed_id, u_int32_t volume)
{
	if (feed_id >= m_max_feeds || !m_feeds || !m_feeds[feed_id]) return -1;
	m_feeds[feed_id]->volume = volume > MAX_VOLUME ? MAX_VOLUME : volume;
	return 0;
}
*/

// Set feed mute/unmute
int CAudioFeed::SetMute(u_int32_t feed_id, bool mute)
{
	if (feed_id >= m_max_feeds || !m_feeds || !m_feeds[feed_id]) return -1;
	m_feeds[feed_id]->mute = mute;
	return 0;
}

// Remove an audio queue from feed
bool CAudioFeed::RemoveQueue(u_int32_t feed_id, audio_queue_t* pQueue)
{
	if (feed_id >= m_max_feeds || !m_feeds || !m_feeds[feed_id] ||
		!pQueue || !m_feeds[feed_id]->pAudioQueues) return false;
	audio_queue_t* p = m_feeds[feed_id]->pAudioQueues;
	bool found = false;
	if (p == pQueue) {
		m_feeds[feed_id]->pAudioQueues = pQueue->pNext;
		found = true;
		if (m_verbose) fprintf(stderr, "audio feed %u removing first audio queue\n", feed_id);
	} else {
		while (p) {
			if (p->pNext == pQueue) {
				p->pNext = pQueue->pNext;
				found = true;
				if (m_verbose) fprintf(stderr, "audio feed %u removing "
					"audio queue\n", feed_id);
				break;
			} else p = p->pNext;
		}
	}
	if (found) {
		if (m_verbose > 1) fprintf(stderr, "audio feed %u freeing %u buffers and %u bytes\n",
			feed_id, pQueue->buf_count, pQueue->bytes_count);
		audio_buffer_t* pBuf = pQueue->pAudioHead;
		while (pBuf) {
			audio_buffer_t* next = pBuf->next;
			free(pBuf);
			pBuf = next;
		}
		free(pQueue);
	}
	if (!m_feeds[feed_id]->pAudioQueues) {
		if (m_feeds[feed_id]->state == RUNNING || m_feeds[feed_id]->state == STALLED) {
			if (m_verbose > 1) fprintf(stderr, "Frame %u - audio feed %u changing state from %s to %s\n",
				SYSTEM_FRAME_NO, feed_id,
				feed_state_string(m_feeds[feed_id]->state),
				feed_state_string(PENDING));
			m_feeds[feed_id]->state = PENDING;
		}
	}
	return found;
}

// Drop a specific number of samples in a specific queue
void CAudioFeed::QueueDropSamples(audio_queue_t* pQueue, u_int32_t drop_samples)
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

// Add a queue to an audio feed.
audio_queue_t* CAudioFeed::AddQueue(u_int32_t feed_id)
{
	// Make checks
	if (feed_id >= m_max_feeds || !m_feeds || !m_feeds[feed_id]) return NULL;
	audio_queue_t* pQueue = (audio_queue_t*) calloc(sizeof(audio_queue_t),1);
	if (pQueue) {
						// SETUP	SETUP
						// READY	READY
						// PENDING	PENDING
						// RUNNING	PENDING
						// STALLED	PENDING
						// DISCONNECTED	SETUP
		pQueue->state		= m_feeds[feed_id]->state == RUNNING ||
			m_feeds[feed_id]->state == STALLED ? PENDING :
			  m_feeds[feed_id]->state == DISCONNECTED ? SETUP : m_feeds[feed_id]->state;
		pQueue->buf_count	= 0;
		pQueue->bytes_count	= 0;
		pQueue->sample_count	= 0;
		pQueue->async_access	= false;
		//pQueue->queue_samples[]	= 0;	// this is zeroed by calloc
		pQueue->qp		= 0;
		pQueue->queue_total	= 0;
		pQueue->queue_max = (typeof(pQueue->queue_max))(DEFAULT_QUEUE_MAX_SECONDS*m_feeds[feed_id]->rate*
			m_feeds[feed_id]->channels);
		pQueue->pNext = m_feeds[feed_id]->pAudioQueues;
		m_feeds[feed_id]->pAudioQueues = pQueue;
		if (m_verbose) fprintf(stderr, "Frame %u - audio feed %u was added an audio queue\n",
			SYSTEM_FRAME_NO, feed_id);
	}
	return pQueue;
}

// Set audio feed state
void CAudioFeed::SetState(u_int32_t id, feed_state_enum state) {
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id] || m_feeds[id]->state == state) return;
	if (m_verbose) fprintf(stderr, "Frame %u - audio feed %u changed state from %s to %s\n",
		SYSTEM_FRAME_NO, id, feed_state_string(m_feeds[id]->state), feed_state_string(state));
	m_feeds[id]->state = state;
}

// Get Audio feed state
feed_state_enum CAudioFeed::GetState(u_int32_t id) {
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return UNDEFINED;
	return m_feeds[id]->state;
}

// AddAudioToFeed()
// Adding a buffer with audio samples to an audio feed
// The buffer is dropped if the feed doesn't exist or if the feed
// has no audio queue(s). The buffer is added to end of the list.
// The volume for the feed is applied to the sample.
// The format of the audio buffer is the one set for the feed.
// The format of the buffer inserted in the queue is int32_t per sample
//
void CAudioFeed::AddAudioToFeed(u_int32_t id, audio_buffer_t* addbuf)
{
	// Do some initial cheks
	if (!addbuf || !addbuf->len || id >= m_max_feeds || !m_feeds || !m_feeds[id] || 
		!m_feeds[id]->pAudioQueues) {
//if (id == 4) fprintf(stderr, "F%04d AF%d Skipping %u samples %.1lfms len %u\n", SYSTEM_FRAME_NO, id, addbuf->len>>1, 1000.0*addbuf->len/(2.0*addbuf->channels*addbuf->rate), addbuf->len);
		if (addbuf) free(addbuf);
		return;
	}
	// Check if any samples are left to add
	if (!addbuf->len) {
		free(addbuf);
		return;
	}

	// Check to see if we have left over bytes
	if (m_feeds[id]->left_over_buf) {
		fprintf(stderr, "Misaligned audio data for feed %u reinserting %u bytes\n", id, m_feeds[id]->left_over_buf->len);
		audio_buffer_t* newbuf = m_feeds[id]->left_over_buf;
		m_feeds[id]->left_over_buf = NULL;
		u_int32_t copybytes = MIN(newbuf->size-newbuf->len,addbuf->len);
		fprintf(stderr, " - Misalignment adding %u of %u bytes. Remaining bytes are %u\n", copybytes, addbuf->len, addbuf->len- copybytes);
		memcpy(newbuf->data+newbuf->len,addbuf->data,copybytes);
		newbuf->len += copybytes;
		AddAudioToFeed(id,newbuf);
		if (addbuf->len == copybytes) {
			free(addbuf);
			return;
		}
		addbuf->len -= copybytes;
		fprintf(stderr, " - Remains from misaligned audio data for feed %u = %u bytes\n", id, addbuf->len);
		memmove(addbuf->data, addbuf->data + copybytes, addbuf->len);
		AddAudioToFeed(id,addbuf);
		return;
	}

	// Check to see if we should drop some samples
	if (m_feeds[id]->drop_bytes) {
if (id == 4) fprintf(stderr, "F%04d AF%d dropping %u samples %.1lfms len %u\n", SYSTEM_FRAME_NO, id,
	m_feeds[id]->drop_bytes>>1, 1000.0*m_feeds[id]->drop_bytes/(2.0*addbuf->channels*addbuf->rate), m_feeds[id]->drop_bytes);
		u_int32_t drop_bytes = addbuf->len >= m_feeds[id]->drop_bytes ?
			m_feeds[id]->drop_bytes : addbuf->len;
		u_int32_t samples = drop_bytes/(m_feeds[id]->channels*m_feeds[id]->bytespersample);
		addbuf->len -= drop_bytes;
		addbuf->data += drop_bytes;
		m_feeds[id]->drop_bytes -= drop_bytes;
		m_feeds[id]->dropped += samples;
		if (m_verbose) fprintf(stderr,
			"Frame %u - audio feed %u dropping %u samples per channel\n",
			SYSTEM_FRAME_NO, id, samples);
	}

	// If state is not RUNNING, set it to RUNNING
	if (m_feeds[id]->state != RUNNING) {

		// Check if we need to add intial delay
		if ((m_feeds[id]->state == READY || m_feeds[id]->state == PENDING) &&
			m_feeds[id]->delay) {
			audio_queue_t* pQueue = m_feeds[id]->pAudioQueues;
			if (pQueue) m_feeds[id]->silence += (m_feeds[id]->delay*
				m_feeds[id]->rate*m_feeds[id]->channels/1000);
			while (pQueue) {
				AddSilence(pQueue, id, m_feeds[id]->delay);
				pQueue = pQueue->pNext;
			}
			if (m_verbose) fprintf(stderr,
				"Frame %u - audio feed %u adding %u ms delay\n",
				SYSTEM_FRAME_NO, id, m_feeds[id]->delay);
		}
		if (m_verbose) fprintf(stderr,
			"Frame %u - audio feed %u changed state from %s to %s\n",
			SYSTEM_FRAME_NO, id, feed_state_string(m_feeds[id]->state),
			feed_state_string(RUNNING));
		m_feeds[id]->state = RUNNING;
	}

//if (id == 4) fprintf(stderr, "F%04d AF%d Adding %u samples %.1lfms len %u\n", SYSTEM_FRAME_NO, id, addbuf->len>>1, 1000.0*addbuf->len/(2.0*addbuf->channels*addbuf->rate), addbuf->len);

	// Now we need to allocate a buffer and convert the format
	u_int32_t size = 0;
	//u_int32_t samples = addbuf->len/m_feeds[id]->bytespersample;
	audio_buffer_t* buf = GetEmptyBuffer(id, &size, sizeof(int32_t));

	// Check buffer size
	if (addbuf->len/m_feeds[id]->bytespersample > size/sizeof(int32_t)) {
		fprintf(stderr, "audio feed %u allocated buffer is too "
			"small to hold data. Skipping it",id);
		free(addbuf);
		free(buf);
		return;
	}

	// Find number of samples
	u_int32_t no_samples = addbuf->len/(m_feeds[id]->channels*
		m_feeds[id]->bytespersample);

	// Possible wrapping, but that is alright.
	m_feeds[id]->samples_rec += no_samples;

	// Convert buffer and drop both if we dont support the format
	if (m_feeds[id]->bytespersample == 2) {
		if (m_feeds[id]->is_signed) {
			convert_si16_to_int32((int32_t*)buf->data,
				(u_int16_t*)addbuf->data, no_samples*m_feeds[id]->channels);
		} else {
			convert_ui16_to_int32((int32_t*)buf->data,
				(u_int16_t*)addbuf->data, no_samples*m_feeds[id]->channels);
		}
		buf->len = no_samples*m_feeds[id]->channels*sizeof(int32_t);

		// Check mismatch in length. We might not have a complete sample set
		u_int32_t left_over_bytes = addbuf->len - no_samples*m_feeds[id]->channels*m_feeds[id]->bytespersample;
		if (left_over_bytes) {
			fprintf(stderr, "AddAudioToFeed MISMATCH for buffer seq no %u size %u. Saving bytes left %u\n",
				m_feeds[id]->buf_seq_no, addbuf->len, left_over_bytes);
			memcpy(addbuf->data, addbuf->data + (addbuf->len - left_over_bytes), left_over_bytes);
			addbuf->len = left_over_bytes;
			m_feeds[id]->left_over_buf = addbuf;	// The buffer is saved using left_over_buf
			addbuf = NULL;				// Prevent the buffer from being freed
		}
		
	} else {
		buf->len = 0;
		fprintf(stderr, "audio feed %u audio format not supported\n", id);
		m_feeds[id]->dropped += no_samples;
	}

if (m_feeds[id]->left_over_buf) fprintf(stderr, "Processing for feed %u bytes %u while left_overs bytes\n", id, buf->len); 

	if (addbuf) free(addbuf);
	if (!buf->len) {
		if (buf) free(buf);
		return;
	}

	// First we assign a seq no to the buffer.
	buf->seq_no = m_feeds[id]->buf_seq_no++;

	// Then we set the volume
	if (buf->channels == m_feeds[id]->channels && m_feeds[id]->pVolume)
		SetVolumeForAudioBuffer(buf, m_feeds[id]->pVolume);
	else if (m_verbose > 2) fprintf(stderr, "Frame %u - audio feed %u volume error : %s\n",
		SYSTEM_FRAME_NO, id,
		buf->channels == m_feeds[id]->channels ? "missing volume" : "channel mismatch");

	// Check for clipping
	if (buf->clipped) m_feeds[id]->clipped = 100;
	else if (m_feeds[id]->clipped) m_feeds[id]->clipped--;

	// Now we calculate the RMS sum for the buffer
	MakeRMS(buf);
	if (m_feeds[id]->rms) for (u_int32_t i=0 ; i < m_feeds[id]->channels ; i++) {
//fprintf(stderr, "RMS %u\n", (u_int32_t) sqrt(m_feeds[id]->rms[i]));
		if (buf->rms[i] > m_feeds[id]->rms[i]) m_feeds[id]->rms[i] = buf->rms[i];
	}

	// Then we check if we should mute it. Mute is not the same as vol = 0
	// though both mute as well as volume==0 should give silence
	if (m_feeds[id]->mute) buf->mute = true;

	// Now we will add the buffer to the tail of the queue(s),
	// If there is more than one queue, we will need to provide
	// each queue with a copy of the buffer.

	audio_queue_t* paq = m_feeds[id]->pAudioQueues;
	buf->next = NULL;
	if (m_verbose > 3)
		fprintf(stderr, "Frame %u - audio feed %u got buffer seq no %u with %u bytes buf count %u\n",
			SYSTEM_FRAME_NO,
			id, buf->seq_no, buf->len, paq->buf_count);
		// we go to next queue, if any
	AddBufferToQueues(m_feeds[id]->pAudioQueues, buf, 1, id, SYSTEM_FRAME_NO, m_verbose);
}

void CAudioFeed::Update()
{
	u_int32_t id;
	if (m_verbose > 4) fprintf(stderr, "Frame %u - audio feed update start\n",
		SYSTEM_FRAME_NO);
	if (!m_feeds) return;

	// Check to see if animation is needed
	if (m_animation) {
		for (id = 0; id < m_max_feeds ; id++) {
			if (m_feeds[id] && m_feeds[id]->volume_delta_steps) {
				if (m_feeds[id]->pVolume) {
					float* p = m_feeds[id]->pVolume;
					for (u_int32_t i=0 ; i < m_feeds[id]->channels; i++, p++) {
						*p += m_feeds[id]->volume_delta;
						if (*p < 0.0) *p = 0;
						else if(*p > MAX_VOLUME_FLOAT) *p = MAX_VOLUME_FLOAT;
					}
				}
				m_feeds[id]->volume_delta_steps--;
			}
		}
		m_animation--;
	}

	timeval timenow, timedif;
	gettimeofday(&timenow, NULL);
	for (id = 0; id < m_max_feeds ; id++) {
		audio_feed_t* p = m_feeds[id];
		if (!p) continue;

		// Lower RMS towards zero for each frame
		// RMS may be adjusted upwards for each buffer added.
		// Here we lower it towards zero
		if (p->rms) for (u_int32_t i=0 ; i < p->channels ; i++) {
			p->rms[i] = (int64_t)(p->rms[i]*0.95);
		}

		// Find time diff, sample diff and calculate samplespersecond
		if (p->update_time.tv_sec > 0 || p->update_time.tv_usec > 0) {
			timersub(&timenow, &p->update_time, &timedif);
			double time = timedif.tv_sec + ((double) timedif.tv_usec)/1000000.0;
			if (time != 0.0) {
				// Check for wrapping
				if (p->samples_rec < p->last_samples) {
				  p->samplespersecond = (UINT_MAX - p->last_samples + p->samples_rec)/time;
				} else p->samplespersecond = (p->samples_rec - p->last_samples)/time;
			}
			p->total_avg_sps -= p->avg_sps_arr[p->avg_sps_index];
			p->total_avg_sps += p->samplespersecond;
			p->avg_sps_arr[p->avg_sps_index] = p->samplespersecond;
			p->avg_sps_index++;
			p->avg_sps_index %= MAX_AVERAGE_SAMPLES;
		}

		// Update values for next time 
		// samples_rec may have wrapped, but that is alright
		p->last_samples = p->samples_rec;
		p->update_time.tv_sec = timenow.tv_sec;
		p->update_time.tv_usec = timenow.tv_usec;

		// Do something
	}
	if (m_verbose > 4) fprintf(stderr, "Frame %u - audio feed update done\n",
		SYSTEM_FRAME_NO);
}
