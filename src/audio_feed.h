/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */

#ifndef __AUDIO_FEED_H__
#define __AUDIO_FEED_H__

//#include <stdlib.h>
#include <sys/types.h>
#include <SDL/SDL.h>
//#include <pango/pangocairo.h>
//#include <sys/time.h>
//#include <video_image.h>

#include "snowmix.h"
#include "audio_util.h"

#define MAX_AUDIO_FEEDS 20

struct audio_feed_t {
	char*			name;			// Name for feed
	int32_t			id;			// id for feed (m_feeds[id])

	// Channel config
	u_int32_t		channels;		// Number of channels
	u_int32_t		rate;			// Sample rate
	u_int32_t		bytespersample;		// Bytes per sample
	u_int32_t		calcsamplespersecond;	// Bytes per sample
	bool			is_signed;

	// Sound settings
	//u_int32_t		volume;			// Volume for feed
	float*			pVolume;		// Pointer to array of Volume, one per channel
	bool			mute;			// Muting channel

	// Animation settings
	u_int32_t		volume_delta_steps;	// Number of times to add delta volume
	float			volume_delta;
	

	// Additional
	u_int32_t		buffer_size;		// Size of buffer for audio data
							// to allocate
	u_int32_t		delay;			// Delay in ms to be added when first
	u_int32_t		drop_bytes;		// Number of bytes to drop from input
							// samples arrive

	audio_buffer_t*		left_over_buf;		// Pointer to left over data due to alignment issue

	audio_queue_t*		pAudioQueues;

	// Counters, statistics and status
	enum feed_state_enum	state;			// State of the feed.
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



class CAudioFeed {
  public:
	CAudioFeed(CVideoMixer* pVideoMixer, u_int32_t max_feeds = MAX_AUDIO_FEEDS);
	~CAudioFeed();
	u_int32_t		MaxFeeds() { return m_max_feeds; }
	int 			ParseCommand(struct controller_type* ctr, const char* ci);
	char*			Query(const char* query, bool tcllist = true);
	audio_buffer_t*		GetEmptyBuffer(u_int32_t feed_id, u_int32_t* bytes, u_int32_t bytespersample = 0);
	int			AddSilence(audio_queue_t* pQueue, u_int32_t id, u_int32_t ms);
	void			AddAudioToFeed(u_int32_t feed_id, audio_buffer_t* buf);
	audio_queue_t*		AddQueue(u_int32_t feed_id);
	void			QueueDropSamples(audio_queue_t* pQueue, u_int32_t drop_samples);
	bool			RemoveQueue(u_int32_t feed_id, audio_queue_t* pQueue);
	void			SetState(u_int32_t id, feed_state_enum state);
	feed_state_enum		GetState(u_int32_t id);
	void			StopFeed(u_int32_t id);
	u_int32_t		GetChannels(u_int32_t id);
	u_int32_t		GetRate(u_int32_t id);
	u_int32_t		GetBytesPerSample(u_int32_t id);
	//audio_buffer_t*		GetAudioBuffer(audio_queue_t* pQueue);
	//void			AddAudioBufferToHeadOfQueue(audio_queue_t* pQueue, audio_buffer_t* pBuf);
	//void			SetVolume_SI16(audio_buffer_t* buf, u_int32_t volume);
	//void			SetVolume_US16(audio_buffer_t* buf, u_int32_t volume);

	//void			MonitorFeedCallBack(u_int32_t feed_id, Uint8 *stream, int len);

	//void MonitorFeed(u_int32_t feed_id);
	void			Update();



private:
	int 		list_help(struct controller_type* ctr, const char* str);
	int 		list_info(struct controller_type* ctr, const char* str);
	int 		list_queue(struct controller_type* ctr, const char* str);
	int 		set_verbose(struct controller_type* ctr, const char* ci);
	//int 		set_monitor(struct controller_type* ctr, const char* ci);
	int 		set_feed_status(struct controller_type* ctr, const char* ci);
	int 		set_mute(struct controller_type* ctr, const char* ci);
	int 		set_delay(struct controller_type* ctr, const char* ci);
	int 		set_silence(struct controller_type* ctr, const char* ci);
	int 		set_volume(struct controller_type* ctr, const char* ci);
	int 		set_move_volume(struct controller_type* ctr, const char* ci);
	int 		set_ctr_isaudio(struct controller_type* ctr, const char* ci);
	int 		set_feed_add(struct controller_type* ctr, const char* ci);
	int 		set_feed_add_silence(struct controller_type* ctr, const char* ci);
	int 		set_drop(struct controller_type* ctr, const char* ci);
	int 		set_feed_channels(struct controller_type* ctr, const char* ci);
	int 		set_feed_rate(struct controller_type* ctr, const char* ci);
	int 		set_feed_format(struct controller_type* ctr,
				const char* ci);
	int		CreateFeed(u_int32_t feed_id);
	int		DeleteFeed(u_int32_t feed_id);
	int		SetFeedName(u_int32_t feed_id, const char* name);
	int		SetFeedChannels(u_int32_t feed_id, u_int32_t channels);
	int		SetFeedRate(u_int32_t feed_id, u_int32_t rate);
	int		SetFeedDelay(u_int32_t feed_id, u_int32_t delay);
	int		SetFeedSilence(u_int32_t feed_id, u_int32_t delay);
	int		SetFeedFormat(u_int32_t feed_id, u_int32_t bytes, bool is_signed);
	int		SetFeedBufferSize(u_int32_t feed_id);
	int		SetMute(u_int32_t feed_id, bool mute);
	int		SetVolume(u_int32_t feed_id, u_int32_t channel, float volume);
	int		SetMoveVolume(u_int32_t feed_id, float volume, u_int32_t steps);
	int		SetVolumeold(u_int32_t feed_id, u_int32_t volume);


	u_int32_t		m_max_feeds;
	audio_feed_t**		m_feeds;
	//struct feed_type*	m_pFeed_list;
	CVideoMixer*		m_pVideoMixer;
	u_int32_t		m_verbose;
	u_int32_t		m_feed_count;
	//int			m_monitor_fd;
	u_int32_t		m_animation;		// Mark for animation needed.
};
	
#endif	// AUDIO_FEED_H
