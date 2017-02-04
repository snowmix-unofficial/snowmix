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
#include <string.h>
#ifdef HAVE_MALLOC
#include <malloc.h>
#else
#include <stdlib.h>
#endif
//#include <stdlib.h>
//#include <sys/types.h>
//#include <math.h>
//#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "audio_util.h"
#include "audio_mixer.h"
#include "snowmix.h"

u_int32_t set_mix_samples(audio_buffer_t* mix_buf, audio_buffer_t* src_buf, u_int32_t mix_samples, source_map_t* pSourceMap)
{
	if (!mix_buf || !src_buf || !mix_samples || src_buf->len < sizeof(int32_t)) return 0;

	// src_samples is the number of samples in the src_buf.
	u_int32_t src_samples = src_buf->len/sizeof(int32_t);

	if (pSourceMap) {
		mix_samples /= mix_buf->channels;
		src_samples /= src_buf->channels;
		if (mix_samples > src_samples) mix_samples = src_samples;
		return (mix_samples * mix_buf->channels);
	}

	// if mix and src channels are equal, we mix the least number of
	// samples of mix_samples and src_samples
	if (src_buf->channels == mix_buf->channels) {
		if (mix_samples > src_samples) mix_samples = src_samples;

	// else if src channels eq 1 and mix channels > 1, src_samples will
	// reused in each mix channel and we mix the least number of 
	// samples of mix_samples and src_samples
	} else if (src_buf->channels == 1 && mix_buf->channels > 1) {
//fprintf(stderr, "SRC channel is 1 and mix channel is %u. mix_samples %u src_samples %u\n", mix_buf->channels, mix_samples, src_samples );
		if (mix_samples > mix_buf->channels*src_samples) {
			mix_samples = mix_buf->channels*src_samples;
		}

	// else if mix channels eq 1 and src channels > 1, each src channel
	// will be added to the mix channel and we will mix the least number of 
        // samples of mix_samples and src_samples/src_channels
	} else if (mix_buf->channels == 1 && src_buf->channels > 1) {
		if (mix_samples > src_samples/src_buf->channels)
			mix_samples = src_samples/mix_buf->channels;

	// else both src channels and mix channels are greater than 1 and not equal
	// and we have no clear strategy for mixing lets say 2 into 3 or 3 into 2.
	// This could be a fixme, but should probably better be dealt with with
	// source maps
	} else {
		fprintf(stderr, "channels mismatch and srcbuf and mixbuf channels both not 1\n");
		return 0;
	}
	// we can now safely mix mix_samples of samples as src_buf will
	// hold enogh data to satisfy this
	return mix_samples;
}

// mixbuf must point to an audio buffer large enough to hold at least mix_samples of samples
// srcbuf
u_int32_t MixSamples(audio_buffer_t* mix_buf, audio_buffer_t* src_buf,
	u_int32_t offset_samples, u_int32_t mix_samples, source_map_t* pSourceMap)
{
	if (!mix_buf || !src_buf || !mix_samples || src_buf->len < sizeof(int32_t)) return 0;

	u_int32_t samples_mixed = 0;

	if (pSourceMap) {
		mix_samples /= mix_buf->channels;
		while (pSourceMap) {
			int32_t* src = ((int32_t*) src_buf->data) + pSourceMap->source_index;
			int32_t* mix = ((int32_t*) mix_buf->data) + offset_samples + pSourceMap->mix_index;
			samples_mixed = 0;
			while (samples_mixed++ < mix_samples) {
				*mix += *src;
				mix += mix_buf->channels;
				src += src_buf->channels;
			}
	
			pSourceMap = pSourceMap->next;
			if (!pSourceMap) break;
		}
		u_int32_t offset = src_buf->channels*mix_samples*sizeof(int32_t);
if (offset > src_buf->len) fprintf(stderr, "OFFSET %u LEN %u\n", offset, src_buf->len);
		src_buf->len -= offset;
		src_buf->data += offset;
		mix_samples *= mix_buf->channels;
		return mix_samples;
	}


	int32_t* mix = ((int32_t*) mix_buf->data) + offset_samples;
	int32_t* src = (int32_t*) src_buf->data;

	if (mix_buf->channels == src_buf->channels) {
		while (samples_mixed < mix_samples) mix[samples_mixed++] += *src++;
		src_buf->len -= (samples_mixed*sizeof(int32_t));
		src_buf->data += (samples_mixed*sizeof(int32_t));
	} else if (src_buf->channels == 1 && mix_buf->channels > 1) {
		while (samples_mixed < mix_samples) {
			for (u_int32_t channel=0 ; channel < mix_buf->channels && samples_mixed < mix_samples; channel++) {
				mix[samples_mixed++] += *src;
			}
			src++;
		}
		src_buf->len -= (samples_mixed*sizeof(int32_t)/mix_buf->channels);
		src_buf->data += (samples_mixed*sizeof(int32_t)/mix_buf->channels);
	} else if (mix_buf->channels == 1 && src_buf->channels > 1) {
		while (samples_mixed < mix_samples) {
			for (u_int32_t channel=0 ; channel < src_buf->channels; channel++)
				mix[samples_mixed] += *src++;
			samples_mixed++;
		}
		src_buf->len -= (samples_mixed*sizeof(int32_t)*src_buf->channels);
		src_buf->data += (samples_mixed*sizeof(int32_t)*src_buf->channels);
	}
	return samples_mixed;
}

// Average samples in queue
int32_t QueueAverage(audio_queue_t* pQueue)
{
	if (!pQueue) return 0;
	return sizeof(int32_t)*pQueue->queue_total/sizeof(pQueue->queue_samples);
}

// Update sample count index and return average
int32_t QueueUpdateAndAverage(audio_queue_t* pQueue)
{
	if (!pQueue) return 0;
	pQueue->queue_total -= pQueue->queue_samples[pQueue->qp];
	pQueue->queue_samples[pQueue->qp] = pQueue->sample_count;
	pQueue->queue_total += pQueue->sample_count;
	pQueue->qp = (pQueue->qp + 1) % (sizeof(pQueue->queue_samples)/sizeof(int32_t));
	return sizeof(int32_t)*pQueue->queue_total/sizeof(pQueue->queue_samples);
}

void MakeRMS(audio_buffer_t* buf) {
	if (!buf || !buf->rms || !buf->channels) return;
	u_int32_t samples_per_channel = buf->len / (sizeof(int32_t) * buf->channels);

	// Check for smaples_per_channels is not 0
	if (!samples_per_channel) return;
	for (u_int32_t i=0 ; i < buf->channels; i++) {
		buf->rms[i] = 0;
		int32_t* sample = ((int32_t*)buf->data) + i;
		for (u_int32_t j=0; j < samples_per_channel ; j++) {
			buf->rms[i] += ((*sample)*(*sample));
			sample += buf->channels;
		}
		buf->rms[i] /= samples_per_channel;
	}
}

// Deliver an audio buffer from a specific queue.
// Update bytes and buffer count for feed.
audio_buffer_t* GetAudioBuffer(audio_queue_t* pQueue) {

	// First we check we have a queue and that it has buffer(s)
	if (!pQueue || !pQueue->pAudioHead) {
		//fprintf(stderr, "Audio queue had no buffer\n");
		return NULL;
	}
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

// Add an audio buffer to head of list. Samples are expected to be internal format int32_t
void AddAudioBufferToHeadOfQueue(audio_queue_t* pQueue, audio_buffer_t* pBuf)
{
	if (!pQueue || !pBuf) return;

	// Lock for audio if we have async access
	if (pQueue->async_access) SDL_LockAudio();


	  // Add head to buffer and buffer to head and update tail if necessary
	  pBuf->next = pQueue->pAudioHead;
	  pQueue->pAudioHead = pBuf;
	  if (!pQueue->pAudioTail) pQueue->pAudioTail = pBuf;
	  pQueue->buf_count++;
	  pQueue->bytes_count += pBuf->len;
	  pQueue->sample_count += (pBuf->len/sizeof(int32_t));
// fprintf(stderr, "AddAudioBufferToHeadOfQueue sample count %u bytes count %u\n", pQueue->sample_count, pQueue->bytes_count);
	  if (pQueue->sample_count * sizeof(int32_t) != pQueue->bytes_count)
		fprintf(stderr, "audio queue add to head sample/bytes mismatch. Samples %u bytes %u\n",
			pQueue->sample_count, pQueue->bytes_count);

	if (pQueue->async_access) SDL_UnlockAudio();
}

// Convert 2 complement signed 16 bit int to int32_t
void convert_si16_to_int32(int32_t* dst, u_int16_t* src, u_int32_t n) {
	if (!dst || !src || !n) return;
	int16_t* s = (int16_t*) src;
	while (n--) {
		*dst++ = (int32_t) *s++;
		if (*(dst-1) > 32767 || *(dst-1) < -32768)
			fprintf(stderr, "RANGE ERROR %hd was converted to %d\n", *(s-1), *(dst-1));
/*
		if (*src && 0x8000) {
			*dst = 0x3fff & ((~(*src++))+1);
			*dst = -(*dst);
			dst++;
		} else {
			*dst++ = (int32_t) *src++;
		}
*/
	}
}

// Convert 2 complement signed 16 bit int to int32_t
void convert_ui16_to_int32(int32_t* dst, u_int16_t* src, u_int32_t n) {
	if (!dst || !src || !n) return;
	while (n--) {
		if (*src < 32768) {
			*dst = (int32_t) *src++;
			*dst++ -= 32767;
		} else {
			*dst = (int32_t) *src++;
			*dst++ -= 32768;
		}
	}
}

// Convert int32_t to 2 complement signed 16 bit int
void convert_int32_to_si16(u_int16_t* dst, int32_t* src, u_int32_t n) {
	if (!dst || !src || !n) return;
	int16_t* d = (int16_t*) dst;
	while (n--) {
		*d++ = (int16_t) *src++;
/*
		if (*src < 0) {
			u_int16_t tmp = -(*src++);
			tmp = (~tmp)+1;
			// *dst++ = (~((u_int16_t)(-(*src++))))+1;
			*dst++ = tmp;
		} else {
			*dst++ = (u_int16_t) ((*src++) & 0x3fff);
		}
*/
	}
}

// Convert 2 complement signed 16 bit int to int32_t
void convert_int32_to_ui16(u_int16_t* dst, int32_t* src, u_int32_t n) {
	if (!dst || !src || !n) return;
	while (n--) {
		if (*src < 0) {
			*dst++ = (u_int16_t)(32767 + (*src++)) & 0xffff;
		} else {
			*dst++ = (u_int16_t)(32768 + (*src++)) & 0xffff;
		}
	}
}

// Set volume for an audio buffer. The format is int32_t per sample
void SetVolumeForAudioBuffer(audio_buffer_t* buf, u_int32_t volume) {
	if (!buf || !buf->data || !buf->len || volume == DEFAULT_VOLUME) return;
	u_int32_t n = buf->len/sizeof(int32_t);
	int32_t* sample = (int32_t*) buf->data;
	if (volume > MAX_VOLUME) volume = MAX_VOLUME;
	int32_t vol = volume;
	if (volume <= DEFAULT_VOLUME) {
		while (n--) {
			*sample = ((*sample) * vol)/DEFAULT_VOLUME;
			//*sample = (*sample)*32/DEFAULT_VOLUME;
			sample++;
		}
	} else {
		while (n--) {
			*sample = ((*sample) * vol)/DEFAULT_VOLUME;
			if (*sample > 32767) {
				*sample = 32767;
				buf->clipped = true;
			} else if (*sample < -32768) {
				*sample = -32768;
				buf->clipped = true;
			}
			sample++;
		}
	}
}

// Set volume for an audio buffer. The format is int32_t per sample
void SetVolumeForAudioBuffer(audio_buffer_t* buf, float* volume) {
	if (!buf || !buf->data || !buf->len || !volume) return;
	//u_int32_t n = buf->len/sizeof(int32_t);
	//int32_t* sample = (int32_t*) buf->data;
	if (*volume > MAX_VOLUME_FLOAT) *volume = MAX_VOLUME_FLOAT;
	//int32_t vol = volume;

	u_int32_t samples_per_channel = buf->len / (sizeof(int32_t) * buf->channels);
	for (u_int32_t i=0 ; i < buf->channels; i++, volume++) {
		int32_t* sample = ((int32_t*)buf->data) + i;
		if (*volume == DEFAULT_VOLUME_FLOAT) continue;
		if (*volume <= DEFAULT_VOLUME_FLOAT) {
			for (u_int32_t j=0; j < samples_per_channel ; j++) {
				*sample = (int32_t)((*sample) * (*volume));
				sample += buf->channels;
			}
		} else {	// Volume larger than 1. Need to check for clipping
int32_t max_val = 0;
			
			for (u_int32_t j=0; j < samples_per_channel ; j++) {

				*sample = (int32_t)((*sample) * (*volume));
				//*sample = (int32_t)((*sample + *sample) * (vol));
				//*sample = (int32_t)((*sample)*1.4);

				if (*sample > 32767) {
					*sample = 32767;
					buf->clipped = true;
				} else if (*sample < -32767) {
					*sample = -32767;
					buf->clipped = true;
				}
				if (*sample > max_val) max_val = *sample; else if (-(*sample) > max_val) max_val = -(*sample);

				sample += buf->channels;
			}
if (buf->clipped) fprintf(stderr, "SetVolumeForAudioBuffer vol=%.3f ch=%u samples=%04u spc=%04u max=%05d clip=%s\n", *volume,buf->channels, buf->len>>2, samples_per_channel, max_val, buf->clipped ? "clip" : "");
		}
	}
}

// Copy audio buffer
audio_buffer_t* CopyAudioBuffer(audio_buffer_t* buf)
{
        // Make checks
        if (!buf || !buf->len) return NULL;

        // Allocate audio buffer and intialize it
        u_int32_t datasize = sizeof(audio_buffer_t) + buf->len + buf->channels*sizeof(u_int64_t);
        audio_buffer_t* newbuf = (audio_buffer_t*) malloc(datasize);
        if (newbuf) {
                memcpy(newbuf, buf, datasize);
                newbuf->data = ((u_int8_t*)newbuf)+sizeof(audio_buffer_t);
                //newbuf->len = buf->len;
                newbuf->next = NULL;
                //newbuf->mute = buf->mute;
                //newbuf->clipped       = buf->clipped;
                //newbuf->channels      = buf->channels;
                //newbuf->rate          = buf->rate;
                newbuf->rms             = (u_int64_t*)(newbuf->data+buf->len);

                // The new buffer may not have the same allocated size so RMS needs to
                // be copied manually
                for (u_int32_t i=0 ; i < buf->channels ; i++)
                        newbuf->rms[i] = buf->rms[i];
        }
        return newbuf;
}


void AddBufferToQueues(audio_queue_t* pQueue, audio_buffer_t* buf, u_int32_t type, u_int32_t id, u_int32_t systemframeno, u_int32_t verbose)
{
	if (!pQueue || !buf || !buf->rate || !buf->channels) return;

	u_int32_t max_rms = 0;          // For the max_rms value in case we
					// need to drop it due to threshold
	if (buf->rms) {
			if (verbose > 4) fprintf(stderr, "Frame %u - audio %s %u "
				"channels %u rms calculating\n",
				systemframeno,
				type == 0 ? "feed" : type == 1 ? "mixer" :
				  type == 2 ? "sink" : "unknown",
                        	id, buf->channels);
		for (u_int32_t i=0 ; i < buf->channels ; i++) {
			if (buf->rms[i] > max_rms) max_rms = (u_int32_t) buf->rms[i];
		}
		if (verbose > 4) fprintf(stderr, "Frame %u - audio %s %u rms done\n",
			systemframeno,
			type == 0 ? "feed" : type == 1 ? "mixer" : type == 2 ? "sink" : "unknown",
			id);
	}

	// Now for each queue as there may be more than one
	while (pQueue) {
		audio_buffer_t* newbuf = buf;
		if (verbose > 4) fprintf(stderr,
#if __WORDSIZE == 32
			"Frame %u - audio %s %u buf %u newbuf %u pQueue->next %s\n",
			systemframeno,
			type == 0 ? "feed" : type == 1 ? "mixer" : type == 2 ? "sink" : "unknown",
			id, (u_int32_t)buf, (u_int32_t)newbuf,
			pQueue->pNext ? "ptr" : "null");
#else
#if __WORDSIZE == 64
			"Frame %u - audio %s %u buf "FUI64" newbuf "FUI64" pQueue->next %s\n",
			systemframeno,
			type == 0 ? "feed" : type == 1 ? "mixer" : type == 2 ? "sink" : "unknown",
			id, (u_int64_t)buf, (u_int64_t)newbuf,
			pQueue->pNext ? "ptr" : "null");
#else
#error __WORDSIZE is not 32 or 64
#endif
#endif

		// Check if the audio queues have more members
		// We have to give each queue its own copy of buf
		if (pQueue->pNext) {
			if (verbose > 4) fprintf(stderr,
#if __WORDSIZE == 32
				"Frame %u - audio %s %u 0A newbuf %u\n",
				systemframeno,
				type == 0 ? "feed" : type == 1 ? "mixer" : type == 2 ? "sink" : "unknown",
				id, (u_int32_t) newbuf);
#else
#if __WORDSIZE == 64
				"Frame %u - audio %s %u 0A newbuf "FUI64"\n",
				systemframeno,
				type == 0 ? "feed" : type == 1 ? "mixer" : type == 2 ? "sink" : "unknown",
				id, (u_int64_t) newbuf);
#else
#error __WORDSIZE is not 32 or 64
#endif
#endif

			bool drop = false;
			// Check if we should drop the whole buffer
			if (pQueue->max_bytes && pQueue->max_bytes <= pQueue->bytes_count) {
				if (verbose > 2) fprintf(stderr, "Frame %u - audio %s %u maxdelay reached. "
					"Dropping %u bytes for copy queue\n", systemframeno,
					type == 0 ? "feed" : type == 1 ? "mixer" : type == 2 ? "sink" :
					  "unknown", id, buf->len);
				drop = true;
			} else if (max_rms < pQueue->rms_threshold) {
				if (verbose > 2) fprintf(stderr, "Frame %u - audio %s %u rms_threshold. "
					"Max RMS %u Threshold %u. Dropping %u bytes for copy queue\n", systemframeno,
					type == 0 ? "feed" : type == 1 ? "mixer" : type == 2 ? "sink" :
					  "unknown", id, max_rms, pQueue->rms_threshold, buf->len);
				drop = true;
			} else {
				// we have more consumers and need to copy data.
				// We may drop part or whole of it later
				if (!(newbuf = CopyAudioBuffer(buf))) {
					if (verbose) fprintf(stderr, "Frame %u - audio %s %u "
						"could not copy buffer. Dropping %u bytes for queue\n",
						systemframeno,
						type == 0 ? "feed" : type == 1 ? "mixer" : type == 2 ? "sink" :
						  "unknown", id, buf->len);
					drop=true;
				}
			}

			// If either we reached maxdelay or we failed to get a copy of the buffer
			// then we drop and continue to next
			if (drop) {
				pQueue->dropped_buffers++;
				pQueue->dropped_bytes += buf->len;
				pQueue = pQueue->pNext;
				continue;
			}
			
			if (verbose > 4) fprintf(stderr,
				"Frame %u - audio %s %u copied "
#if __WORDSIZE == 32
				"buf seq no %u. Had %u bytes, has %u bytes seq no %u newbuf %u\n",
				systemframeno,
				type == 0 ? "feed" : type == 1 ? "mixer" : type == 2 ? "sink" : "unknown",
				id, buf->seq_no, buf->len, newbuf->len,
				newbuf->seq_no, (u_int32_t) newbuf);
#else
#if __WORDSIZE == 64
				"buf seq no %u. Had %u bytes, has %u bytes seq no %u newbuf "FUI64"\n",
				systemframeno,
				type == 0 ? "feed" : type == 1 ? "mixer" : type == 2 ? "sink" : "unknown",
				id, buf->seq_no, buf->len, newbuf->len,
				newbuf->seq_no, (u_int64_t) newbuf);
#else
#error __WORDSIZE is not 32 or 64
#endif
#endif

		}
		// Now check if have reached or will reach max_bytes
		if ((pQueue->max_bytes && pQueue->max_bytes < newbuf->len + pQueue->bytes_count) || (max_rms < pQueue->rms_threshold)) {
			u_int32_t drop_bytes = pQueue->bytes_count + newbuf->len - pQueue->max_bytes;
			if (drop_bytes > newbuf->len) drop_bytes = newbuf->len;
			newbuf->len -= drop_bytes;
			newbuf->data += drop_bytes;
			pQueue->dropped_bytes += drop_bytes;
			if (verbose > 2) {
				if (pQueue->max_bytes && pQueue->max_bytes < newbuf->len + pQueue->bytes_count)
					fprintf(stderr, "Frame %u - audio %s %u maxdelay reached. "
						"Dropping %u bytes for queue\n", systemframeno,
						type == 0 ? "feed" : type == 1 ? "mixer" :
				  		type == 2 ? "sink" : "unknown", id, buf->len);
				else fprintf(stderr, "Frame %u - audio %s %u rms_threshold. "
					"Max RMS %u Threshold %u. Dropping %u bytes for copy queue\n", systemframeno,
					type == 0 ? "feed" : type == 1 ? "mixer" : type == 2 ? "sink" :
					  "unknown", id, max_rms, pQueue->rms_threshold, buf->len);
			}
			pQueue->state = RUNNING;
			if (!newbuf->len) {
				pQueue->dropped_buffers++;
				pQueue = pQueue->pNext;
				free(newbuf);
				if (!pQueue) break;
				continue;
			}
		}

		if (verbose > 4) fprintf(stderr,
			"Frame %u - audio %s %u adding buffer "
#if __WORDSIZE == 32
			"seq no %u len %u next %s pQueue->tail %s newbuf addr %u\n",
			systemframeno,
			type == 0 ? "feed" : type == 1 ? "mixer" : type == 2 ? "sink" : "unknown",
			id, newbuf->seq_no, newbuf->len, newbuf->next ? "ptr" : "null",
			pQueue->pAudioTail ? "ptr" : "null", (u_int32_t) newbuf );
#else
#if __WORDSIZE == 64
			"seq no %u len %u next %s pQueue->tail %s newbuf addr "FUI64"\n",
			systemframeno,
			type == 0 ? "feed" : type == 1 ? "mixer" : type == 2 ? "sink" : "unknown",
			id, newbuf->seq_no, newbuf->len, newbuf->next ? "ptr" : "null",
			pQueue->pAudioTail ? "ptr" : "null", (u_int64_t) newbuf );
#else
#error __WORDSIZE is not 32 or 64
#endif
#endif

		// Check to see if we have to invert samples
		if (pQueue->invert) {
			if (verbose > 0) fprintf(stderr, "Frame %u - audio %s %u inverting buffer "
				"seq no %u len %u\n", systemframeno,
				type == 0 ? "feed" : type == 1 ? "mixer" : type == 2 ?
				  "sink" : "unknown", id, newbuf->seq_no, newbuf->len);
			int32_t* p = (int32_t*) newbuf->data;
			u_int32_t n = newbuf->len / sizeof(int32_t);
			while (n--) { *p = -(*p); p++; }
		}
	
		// Now lock for SDL_Audio callback to queue and insert newbuf
		if (pQueue->async_access) SDL_LockAudio();
		  if (pQueue->pAudioTail) {
			pQueue->pAudioTail->next = newbuf;
			pQueue->pAudioTail = newbuf;
		  } else pQueue->pAudioTail = newbuf;
		  if (!pQueue->pAudioHead) pQueue->pAudioHead = newbuf;
		  pQueue->buf_count++;
		  pQueue->bytes_count += buf->len;
	  	  pQueue->sample_count += (buf->len/sizeof(int32_t));
		  pQueue->state = RUNNING;
		if (pQueue->async_access) SDL_UnlockAudio();
// fprintf(stderr, "AddBuffersToQueue %s id %u sample count %u bytes count %u\n", type == 0 ? "feed" : type == 1 ? "mixer" : type == 2 ? "sink" : "unknown", id, pQueue->sample_count, pQueue->bytes_count);
	  	if (pQueue->sample_count * sizeof(int32_t) != pQueue->bytes_count)
		  	fprintf(stderr, "Frame %u - audio %s %u queue sample/bytes mismatch. "
				"Samples %u bytes %u\n", systemframeno,
				type == 0 ? "feed" : type == 1 ? "mixer" : type == 2 ? "sink" : "unknown",
				id, pQueue->sample_count, pQueue->bytes_count);
		pQueue = pQueue->pNext;
	}
}
