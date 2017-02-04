/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */

#ifndef __AUDIO_UTIL_H__
#define __AUDIO_UTIL_H__

//#include <stdlib.h>
#include <sys/types.h>
#include <SDL/SDL.h>
//#include <pango/pangocairo.h>
//#include <sys/time.h>
//#include <video_image.h>

#include "snowmix.h"

// Number of samples for calculating average samples
#define QUEUE_SAMPLES 60

// Number of average samples of samples per second
#define MAX_AVERAGE_SAMPLES (24*10)

// Max average seconds allowed in queue
#define DEFAULT_QUEUE_MAX_SECONDS 2.125

#define MAX_VOLUME 255
#define DEFAULT_VOLUME ((MAX_VOLUME>>1)+1)
#define MAX_VOLUME_FLOAT 4.0
#define DEFAULT_VOLUME_FLOAT 1.0
//#define system_frame_no m_pVideoMixer->m_pController->m_frame_seq_no
#define SYSTEM_FRAME_NO (m_pVideoMixer ? m_pVideoMixer->SystemFrameNo() : 0)

struct audio_buffer_t {
	u_int8_t*		data;		// pointer to audio data
	u_int32_t		len;		// number of data bytes
	u_int32_t		size;		// number of data bytes the buffer can hold
	u_int32_t		seq_no;		// Seq number of buffer
	u_int32_t		channels;	// Channels
	u_int32_t		rate;		// Sample rate
	u_int64_t*		rms;		// Array with sum of samples squared
	bool			clipped;
	bool			mute;
	struct audio_buffer_t*	next;		// link to next buffer in list
};

struct audio_queue_t {
	audio_buffer_t*		pAudioHead;
	audio_buffer_t*		pAudioTail;
	feed_state_enum		state;
	bool			async_access;	// SDL_Audio uses callbacks and we need to lock audio
	bool			invert;		// Invert samples when added
	u_int32_t		rms_threshold;	// Threshold level for rms for adding to queue
	u_int32_t		buf_count;	// Number of buffers in queue
	u_int32_t		bytes_count;	// Number of bytes in queue
	u_int32_t		sample_count;	// Number of samples in queue
	u_int32_t		max_bytes;	// Buffer will be dropped if exceeded
	u_int32_t		min_bytes;	// A mixer source will add up, if min is not reached when used
	u_int32_t		dropped_bytes;	// Total number of bytes dropped
	u_int32_t		dropped_buffers;	// Total number of buffers dropped
	int32_t			queue_samples[QUEUE_SAMPLES];	// queue samples
	int32_t			queue_total;	// total queue samples
	int32_t			queue_max;	// max number of queue samples
	u_int32_t		qp;		// pointer to queue_samples
	audio_queue_t*		pNext;		// Link to next queue (list of queues)
};

u_int32_t		set_mix_samples(audio_buffer_t* mix_buf, audio_buffer_t* src_buf,
				u_int32_t mix_samples, struct source_map_t* pSourceMap = NULL);
u_int32_t		MixSamples(audio_buffer_t* mix_buf, audio_buffer_t* src_buf,
				u_int32_t offset_samples, u_int32_t mix_samples, struct source_map_t* pSourceMap = NULL);
audio_buffer_t*		GetAudioBuffer(audio_queue_t* pQueue);
void			AddAudioBufferToHeadOfQueue(audio_queue_t* pQueue, audio_buffer_t* pBuf);
void			SetVolumeForAudioBuffer(audio_buffer_t* buf, u_int32_t volume);
void			SetVolumeForAudioBuffer(audio_buffer_t* buf, float* volume);
int32_t			QueueAverage(audio_queue_t* pQueue);
int32_t			QueueUpdateAndAverage(audio_queue_t* pQueue);

void			convert_si16_to_int32(int32_t* dst, u_int16_t* src, u_int32_t n);
void			convert_ui16_to_int32(int32_t* dst, u_int16_t* src, u_int32_t n);
	
void			convert_int32_to_si16(u_int16_t* src, int32_t* dst, u_int32_t n);
void			convert_int32_to_ui16(u_int16_t* src, int32_t* dst, u_int32_t n);
void			MakeRMS(audio_buffer_t* buf);
audio_buffer_t*		CopyAudioBuffer(audio_buffer_t* buf);
void			AddBufferToQueues(audio_queue_t* pQueue, audio_buffer_t* buf,
				u_int32_t type, u_int32_t id, u_int32_t systemframeno,
				u_int32_t verbose = 0);

#endif	// AUDIO_UTIL_H
