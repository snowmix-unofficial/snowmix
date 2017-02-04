/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */

#ifndef __AUDIO_SINK_H__
#define __AUDIO_SINK_H__

//#include <stdlib.h>
#include <sys/types.h>
#include <SDL/SDL.h>
//#include <pango/pangocairo.h>
//#include <sys/time.h>
//#include <video_image.h>
#ifdef WIN32
#include "windows_util.h"
#else
#include <sys/select.h>
//#include <sys/time.h>
#endif

#include "snowmix.h"
//#include <controller.h>
//#include <video_mixer.h>

#define MAX_AUDIO_SINKS	10

#include "audio_util.h"

struct audio_sink_t {
	char*			name;
	int32_t			id;

	int			write_fd;		//
	enum feed_state_enum	previous_state;		// Previous state of the feed.
	u_int32_t		channels;
	u_int32_t		rate;
	u_int32_t		bytespersample;
	u_int32_t		bytespersecond;		// rate*channels*bytespersample
	u_int32_t		calcsamplespersecond;	// rate*channels
	bool			is_signed;
	bool			mute;
	u_int32_t		buffer_size;		// Size of buffer allocated for audio data
							// to allocate
	//u_int32_t		volume;			// Volume 0..255
	float*			pVolume;		// Pointer to array of Volume, one per channel
	u_int32_t		delay;			// Delay in ms at start (adding silence)

        // Animation settings
        u_int32_t               volume_delta_steps;     // Number of times to add delta volume
        float                   volume_delta;


	bool			no_rate_limit;		// do not rate limit output

	audio_queue_t*		monitor_queue;
	char*			file_name;		// File name to write to.
	int			sink_fd;		// fd to write to.
	u_int32_t		sample_count;		// Samples written until now

	u_int32_t		source_type;		// 0=none, 1=audio feed, 2=audio mixer
	u_int32_t		source_id;		// Id of source
	audio_queue_t*		pAudioQueue;		// This is the sinks input queue.
							// Allocated/freed by the sink
	audio_queue_t*		pOutQueue;		// This is the queue for writing out
							// converted samples
	audio_queue_t*		pAudioQueues;		// This is the list of queues sourced
							// by the sink. Allocation/freeing
							// happens in other elements.

	int32_t			queue_samples[60];	// Store the missing sample count and
							// calculate an average
	u_int32_t		nqb;
	u_int32_t		write_silence_bytes;	// add no of silence samples to output
	u_int32_t		drop_bytes;		// add no of silence samples to output

	// Counters, statistics and status
	enum feed_state_enum	state;			// State of the sink.
	struct timeval		start_time;		// Time feed changed to RUNNING
	u_int32_t		buf_seq_no;		// Number of buffers received
	u_int32_t		samples_rec;		// Number of samples received
	u_int32_t		silence;		// Number of silence samples added
	u_int32_t		dropped;		// Number samples dropped
	u_int32_t		clipped;		// If samples in buffer are clipped
	u_int64_t*		rms;			// Array with RMS value, one per channel

	// For samples per second
	struct timeval		update_time;
	double			samplespersecond;
	u_int32_t		last_samples;
	double			total_avg_sps;
	double			avg_sps_arr[MAX_AVERAGE_SAMPLES];
	u_int32_t		avg_sps_index;
};



class CAudioSink {
  public:
	CAudioSink(CVideoMixer* pVideoMixer, u_int32_t max_sinks = MAX_AUDIO_SINKS);
	~CAudioSink();
	u_int32_t		MaxSinks() { return m_max_sinks; }
	int 			ParseCommand(struct controller_type* ctr, const char* ci);
	char*			Query(const char* query, bool tcllist = true);
	audio_buffer_t*		GetEmptyBuffer(u_int32_t sink_id, u_int32_t* bytes);
	void			AddAudioToSink(u_int32_t sink_id, audio_buffer_t* buf);
	audio_queue_t*		AddQueue(u_int32_t sink_id, bool add = true);
	bool			RemoveQueue(u_int32_t sink_id, audio_queue_t* pQueue);
	u_int32_t		GetChannels(u_int32_t id);
	u_int32_t		GetRate(u_int32_t id);
	u_int32_t		GetBytesPerSample(u_int32_t id);
	void			QueueDropSamples(audio_queue_t* pQueue, u_int32_t drop_samples);
	//audio_buffer_t*		GetAudioBuffer(audio_queue_t* pQueue);
/*
	feed_state_enum*	PreviousSinkState(int id);
	feed_state_enum*	SinkState(int id);
*/

//	void			MonitorSinkCallBack(u_int32_t sink_id, Uint8 *stream, int len);

	//void MonitorSink(u_int32_t sink_id);
	void			Update();

	void		NewUpdate(struct timeval *timenow);
	int			StopSink(u_int32_t sink_id);
	feed_state_enum		GetState(u_int32_t id);
	int			SetFds(int max_fd, fd_set* read_fds, fd_set* write_fds);
	void			CheckReadWrite(fd_set *read_fds, fd_set *write_fds);
	int			WriteSink(u_int32_t id);



private:
	int 		list_help(struct controller_type* ctr, const char* str);
	int 		list_info(struct controller_type* ctr, const char* str);
	int 		set_sink_status(struct controller_type* ctr, const char* ci);
	int 		set_add_silence(struct controller_type* ctr, const char* ci);
	int 		set_drop(struct controller_type* ctr, const char* ci);
	int 		set_verbose(struct controller_type* ctr, const char* ci);
	int 		set_queue(struct controller_type* ctr, const char* ci);
	int 		set_queue_maxdelay(struct controller_type* ctr, const char* ci);
	//int 		set_monitor(struct controller_type* ctr, const char* ci);
	int 		set_sink_start(struct controller_type* ctr, const char* ci);
	int 		set_sink_stop(struct controller_type* ctr, const char* ci);
	int 		set_sink_source(struct controller_type* ctr, const char* ci);
	int 		set_file(struct controller_type* ctr, const char* ci);
	int 		set_mute(struct controller_type* ctr, const char* ci);
	int 		set_volume(struct controller_type* ctr, const char* ci);
	int 		set_move_volume(struct controller_type* ctr, const char* ci);
	int 		set_ctr_isaudio(struct controller_type* ctr, const char* ci);
	int 		set_sink_add(struct controller_type* ctr, const char* ci);
	int 		set_sink_channels(struct controller_type* ctr, const char* ci);
	int 		set_sink_rate(struct controller_type* ctr, const char* ci);
	int 		set_sink_format(struct controller_type* ctr,
				const char* ci);
	int		CreateSink(u_int32_t sink_id);
	int		DeleteSink(u_int32_t sink_id);
	int		SetSinkName(u_int32_t sink_id, const char* name);
	int		SetSinkChannels(u_int32_t sink_id, u_int32_t channels);
	int		SetSinkRate(u_int32_t sink_id, u_int32_t rate);
	int		SetSinkFormat(u_int32_t sink_id, u_int32_t bytes, bool is_signed);
	int		SetSinkBufferSize(u_int32_t sink_id);
	int		SetMute(u_int32_t sink_id, bool mute);
	int		SetVolume(u_int32_t sink_id, u_int32_t channel, float volume);
	int		SetMoveVolume(u_int32_t sink_id, float volume_delta, u_int32_t delta_steps);
	int		SetFile(u_int32_t sink_id, const char* file_name);
	int		StartSink(u_int32_t sink_id);
	int32_t		SamplesNeeded(u_int32_t id);
	int		SinkSource(u_int32_t sink_id, u_int32_t source_id, int source_type);
	int		SetQueueMaxDelay(u_int32_t sink_id, u_int32_t maxdelay);

	void		SinkAccounting(audio_sink_t* pSink, struct timeval *timenow);
	void		SinkVolumeAnimation();
	void		SinkPendingCheckForData(audio_sink_t* pSink, struct timeval *timenow);
	int		SinkAddSilence(audio_sink_t* pSink);
	void		SinkSendSampleOut(audio_sink_t* pSink);
	int		SinkDropBytes(audio_sink_t* pSink, audio_buffer_t* newbuf);


	u_int32_t		m_max_sinks;
	audio_sink_t**		m_sinks;
	CVideoMixer*		m_pVideoMixer;
	int			m_verbose;
	int			m_monitor_fd;
	u_int32_t		m_sink_count;
	u_int32_t		m_animation;
};
	
#endif	// AUDIO_SINK_H
