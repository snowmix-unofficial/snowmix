/*
 * (c) 2013 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */

#ifndef __VIDEO_OUTPUT_H__
#define __VIDEO_OUTPUT_H__

#include <sys/types.h>

#ifdef WIN32
#include <winsock.h>
#else
#include <sys/select.h>
#endif

#include "SDL/SDL.h"
#include "SDL/SDL_thread.h"

#include "snowmix.h"
#include "snowmix_util.h"
#include "video_mixer.h"

struct output_memory_list_type {
	struct output_memory_list_type	*prev;
	struct output_memory_list_type	*next;
	struct output_memory_list_type	*next_withframe;
	struct output_memory_list_type	*prev_withframe;

	unsigned long			offset;	// Offset inside the memory_base.
	int				use;	// Use counter.
};


class CVideoOutput {
  public:
	CVideoOutput(CVideoMixer* pVideoMixer);
	~CVideoOutput();

	int	ParseCommand(struct controller_type* ctr, const char* str);

	char*	Query(const char* query, bool tcllist = true);

	int	InitOutput(struct controller_type *ctr, char *path);
	//int	UnlinkOutput();

	// Populate rfds and wrfd for the output subsystem.
	int	SetFds(int maxfd, fd_set *rfds, fd_set *wfds);


	struct output_memory_list_type* OutputGetFreeBlock();
	void	CheckRead (fd_set *rfds);
	int	OutputBufferReady4Output(struct output_memory_list_type* buf);

	void		InUseCount(u_int32_t* buf_count, u_int32_t* inuse,
				bool locked = false);
	u_int8_t*	FrameAddress(struct output_memory_list_type* buf);

	int		OutputMasterSocket() { return m_output_master_socket; };
	int		OutputClientSocket() { return m_output_client_socket; };
	char*		OutputMasterName() { return m_pOutput_master_name; };
	char*		OutputMemoryHandleName() { return m_output_memory_handle_name; };
	bool		OutputSocketSet() { return m_system_socket_set; }
	u_int32_t	OutputDelay() { return m_buffer_delay; }
	u_int32_t	OutputBuffers() { return m_buffer_allocate; }
	u_int32_t	OutputMode() { return m_output_mode; }
	u_int32_t	OutputFreeze() { return m_output_freeze; }
	u_int32_t	Verbose() { return m_verbose; }
	u_int32_t	FramesRepeated(bool clear = false);
	int		ReadBuffersAhead();

	int		MainLoopSimple();
	int		MainLoopTimed();
	int		MainLoopTimed2();
	int		MainLoop();

	int		SendOutBuffer(struct output_memory_list_type* buf);

private:
	void		OutputAckBuffer(int area_id, unsigned long offset);
	void		OutputReset();
	int		SetMode(u_int32_t mode);
	int		SetDelay(u_int32_t delay);
	int		SetBuffers(u_int32_t buffers);
	int		SetFreeze(u_int32_t buffers);
	int		ListStatus(struct controller_type* ctr, const char*);
	int		ListInfo(struct controller_type* ctr, const char*);

	SDL_mutex*		m_pOutputMutex;
	SDL_sem*		m_pOutputSem;
	CVideoMixer*		m_pVideoMixer;
	u_int32_t		m_verbose;
	bool			m_system_socket_set;

	// Output modes (0=direct (default), 1=Simple Thread, 2=Timed Thread)
	u_int32_t		m_output_mode;
	//
	int			m_output_master_socket;
	int			m_output_client_socket;

	// Size of the shared memory block for output
	int			m_output_memory_size;

	// File handle for the shared memory block.
	int			m_output_memory_handle;

	// Name of the shared memory handle.
	char			m_output_memory_handle_name[64];

	// Name of the output control socket
	char*			m_pOutput_master_name;

	// List of individual buffer blocks.
	struct output_memory_list_type*  m_pOutput_memory_list;

	// List with overlayed frames
	struct output_memory_list_type*  m_pFrames_list;
	struct output_memory_list_type*  m_pFrames_tail_list;
	u_int32_t			m_buffer_count; // minimum count to start output
							// only in threaded mode.
	u_int32_t			m_buffer_delay;	// minimum required buffer in queue
	u_int32_t			m_buffer_allocate;	// Allocated buffers for shared mem.
	u_int32_t			m_output_freeze;	// Freeze output
	u_int32_t			m_output_repeated;	// Last output was repeated
	bool				m_got_no_free_buffers;	// Flag is set when no buffers where available
								// Should be cleared when it is okay to deliver buffers again

	u_int32_t			m_con_seq_no;		// Connection sequence number
	u_int32_t			m_buffers_sent;		// Buffers signaled to output control socket
	u_int32_t			m_buffers_ack;		// Buffers acknowledged over control socket
	u_int32_t			m_timed_loops;		// Number of looped in timed loop
	struct timeval*			m_pFrame_period;	// Current frame period for Timed loop
	struct timeval			m_last_time;		// Last time we sent a frame out

	u_int32_t			m_copied_frames;	// Number of frames copied out


	// Pointer to the shared mamory block as mapped in RAM.
	uint8_t*        m_pOutput_memory_base;

	// Loop thread
	SDL_Thread*		m_pLoopThread;

	CAverage*		m_pAverage_loop;

public:
};
	
#endif	// __VIDEO_OUTPUT_H__
