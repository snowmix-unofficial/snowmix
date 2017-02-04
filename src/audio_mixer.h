/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */

#ifndef __AUDIO_MIXER_H__
#define __AUDIO_MIXER_H__

//#include <stdlib.h>
#include <sys/types.h>
#include <SDL/SDL.h>
//#include <pango/pangocairo.h>
#ifdef WIN32
#include "windows_util.h"
#else
#include <sys/time.h>
#endif

//#include <video_image.h>

#include "snowmix.h"
#include "audio_util.h"

#define MAX_AUDIO_MIXERS 10

class CAudioFeed;

struct source_map_t {
	u_int32_t	source_index;
	u_int32_t	mix_index;
	struct source_map_t* next;
};

struct audio_source_t {
	int 		id;
	int		source_type;			// 0=none, 1=audio feed, 2=audio mixer
	u_int32_t	source_id;			// id of source
	u_int32_t	channels;			// Channels
	u_int32_t	rate;				// Rate of source
	//u_int32_t	volume;				// volume of source input (not at the source)
	float*		pVolume;			// Pointer to array of Volume, one per channel
	u_int32_t	mute;				// should the source input be muted?

	// Volume animation
	float			volume_delta;
	u_int32_t		volume_delta_steps;

	bool		got_samples;
	bool		invert;				// Invert samples
	u_int32_t	pause;				// Pause mixing this source
	u_int32_t	rms_threshold;			// RMS Threshold for adding samples to source queue
	u_int32_t	drop_bytes;
	u_int32_t	max_delay_in_bytes;
	u_int32_t	min_delay_in_bytes;
	u_int32_t	samples_rec;			// Samples received.
	u_int32_t	silence;			// Samples silence added.
	u_int32_t	dropped;			// Samples dropped.
	u_int32_t	clipped;			// Samples clipped.
	u_int64_t*	rms;				// array of RMS values for source
	audio_queue_t*	pAudioQueue;			// Pointer to queue at source
	source_map_t*	pSourceMap;			// List of source2mix index
	struct audio_source_t*	next;			// pointer to next source in list for mixer
};

struct audio_mixer_t {
	char*			name;
	int32_t			id;

	u_int32_t		channels;
	u_int32_t		rate;
	u_int32_t		bytespersample;
	u_int32_t		bytespersecond;
	u_int32_t		calcsamplespersecond;	// rate*channels
	u_int32_t		drop_bytes;
	bool			is_signed;
	bool			mute;
	u_int32_t		buffer_size;		// Size of buffer for audio data
							// to allocate
	//u_int32_t		volume;			// Used for SDL. 255 is max
	float*			pVolume;		// Pointer to array of Volume, one per channel

	// Volume animation
	float			volume_delta;
	u_int32_t		volume_delta_steps;

	audio_source_t*		pAudioSources;		// Pointer to list of source;
	u_int32_t		no_sources;		// Number of sources;
	audio_queue_t*		pAudioQueues;		// list of queues for consumers of mixer
	u_int64_t		mixer_sample_count;

	// Counters, statistics and status
	enum feed_state_enum	state;			// State of the feed.
	struct timeval		start_time;		// Time feed changed to RUNNING
	u_int32_t		buf_seq_no;		// Number of buffers received
	u_int32_t		samples;		// Number of samples received
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



class CAudioMixer {
  public:
	CAudioMixer(CVideoMixer* pVideoMixer, u_int32_t max_mixers = MAX_AUDIO_MIXERS);
	~CAudioMixer();
	u_int32_t		MaxMixers() { return m_max_mixers; }
	int 			ParseCommand(struct controller_type* ctr, const char* ci);
	char*			Query(const char* query, bool tcllist = true);
	audio_buffer_t*		GetEmptyBuffer(u_int32_t mixer_id, u_int32_t* bytes);
	audio_queue_t*		AddQueue(u_int32_t mixer_id);
	void                    QueueDropSamples(audio_queue_t* pQueue, u_int32_t drop_samples);
	bool			RemoveQueue(u_int32_t mixer_id, audio_queue_t* pQueue);
	u_int32_t		GetChannels(u_int32_t id);
	u_int32_t		GetRate(u_int32_t id);
	u_int32_t		GetBytesPerSample(u_int32_t id);
	//audio_buffer_t*		GetAudioBuffer(audio_queue_t* pQueue);
	//void			AddAudioBufferToHeadOfQueue(audio_queue_t* pQueue, audio_buffer_t* pBuf);
	//void			SetVolume_SI16(audio_buffer_t* buf, u_int32_t volume);
	//void			SetVolume_US16(audio_buffer_t* buf, u_int32_t volume);

	//void			MonitorMixerCallBack(u_int32_t mixer_id, Uint8 *stream, int len);

	//void MonitorMixer(u_int32_t mixer_id);
	feed_state_enum		GetState(u_int32_t id);

	void			Update();



private:
	void		AddQueueToSource(audio_source_t* pSource, bool add_queue);
	bool		ConditionalStart(u_int32_t id, timeval* pTimenow, bool hardstart = false);
	void		AddAudioToMixer(u_int32_t mixer_id, audio_buffer_t* buf);
	int 		list_help(struct controller_type* ctr, const char* str);
	int 		list_info(struct controller_type* ctr, const char* str);
	int 		set_mixer_status(struct controller_type* ctr, const char* ci);
	int 		set_mixer_verbose(struct controller_type* ctr, const char* ci);
	int		set_add_silence(struct controller_type* ctr, const char* str);
	int		set_drop(struct controller_type* ctr, const char* str);
	int 		set_mixer_queue(struct controller_type* ctr, const char* ci);
	//int 		set_monitor(struct controller_type* ctr, const char* ci);
	int 		set_mixer_mute(struct controller_type* ctr, const char* ci);
	int 		set_mixer_start(struct controller_type* ctr, const char* ci);
	int 		set_mixer_stop(struct controller_type* ctr, const char* ci);
	int 		set_mixer_volume(struct controller_type* ctr, const char* ci);
	int 		set_mixer_move_volume(struct controller_type* ctr, const char* ci);
	//int 		set_ctr_isaudio(struct controller_type* ctr, const char* ci);
	int 		set_mixer_add(struct controller_type* ctr, const char* ci);
	int 		set_mixer_channels(struct controller_type* ctr, const char* ci);
	int 		set_mixer_rate(struct controller_type* ctr, const char* ci);
	//int 		set_mixer_format(struct controller_type* ctr,
	//			const char* ci);
	int		set_mixer_source(struct controller_type* ctr, const char* str);
	int		CreateMixer(u_int32_t mixer_id);
	int		DeleteMixer(u_int32_t mixer_id);
	int		MixerSourceDelete(u_int32_t mixer_id, u_int32_t source_no);
	int		DeleteSource(audio_source_t* pSource);
	int		SetMixerName(u_int32_t mixer_id, const char* name);
	int		SetMixerChannels(u_int32_t mixer_id, u_int32_t channels);
	int		SetMixerRate(u_int32_t mixer_id, u_int32_t rate);
	//int		SetMixerFormat(u_int32_t mixer_id, u_int32_t bytes, bool is_signed);
	int		SetMixerBufferSize(u_int32_t mixer_id);
	int		SetMixerSilence(u_int32_t id, u_int32_t silence);
	int		SetMute(u_int32_t mixer_id, bool mute);
	//int		SetVolume(u_int32_t mixer_id, u_int32_t volume);
	int		SetVolume(u_int32_t mixer_id, u_int32_t channel, float volume);
	int		SetMoveVolume(u_int32_t mixer_id, float volume_delta, u_int32_t delta_steps);
	int		MixerSource(u_int32_t mixer_id, u_int32_t source_id, int source_type);
	int		MixerSourceVolume(u_int32_t mixer_id, u_int32_t source_no,
				u_int32_t channel, float volume);
	int		MixerSourceMoveVolume(u_int32_t mixer_id, u_int32_t source_no,
				float volume_delta, u_int32_t delta_steps);
	int		MixerSourceDrop(u_int32_t mixer_id, u_int32_t source_no, u_int32_t drop);
	int		MixerSourcePause(u_int32_t mixer_id, u_int32_t source_no, u_int32_t pause);
	int		MixerSourceThreshold(u_int32_t mixer_id, u_int32_t source_no, u_int32_t pause);
	int		MixerSourceDelay(u_int32_t mixer_id, u_int32_t source_no, u_int32_t delay, bool max);
	int		MixerSourceSilence(u_int32_t mixer_id, u_int32_t source_no, u_int32_t silence, bool verbose = true);
	int		MixerSourceMute(u_int32_t mixer_id, u_int32_t source_no, bool mute);
	int		SetSourceMap(u_int32_t mixer_id, u_int32_t source_no, const char* str);
	int		SetSourceInvert(u_int32_t mixer_id, u_int32_t source_no);
	int		StartMixer(u_int32_t mixer_id, bool soft);
	int		StopMixer(u_int32_t mixer_id);
	int32_t		SamplesNeeded(u_int32_t id);
	int		AddSilence(audio_queue_t* pQueue, u_int32_t id, u_int32_t ms, bool verbose = true);


	u_int32_t		m_max_mixers;
	audio_mixer_t**		m_mixers;
	CVideoMixer*		m_pVideoMixer;
	CAudioFeed*		m_pAudioFeed;
	u_int32_t		m_verbose;
	u_int32_t		m_mixer_count;
	u_int32_t		m_animation;
};
	
#endif	// AUDIO_MIXER_H
