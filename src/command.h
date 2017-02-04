/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */

#ifndef __COMMAND_H__
#define __COMMAND_H__

//#include <stdlib.h>
#include <sys/types.h>
#ifdef WIN32
#include "windows_util.h"
#include <inttypes.h>
#else
#include <sys/time.h>
#endif
//#include <SDL/SDL.h>
//#include <pango/pangocairo.h>
//#include <sys/time.h>


struct command_t {
	char*			command;
	int32_t			frame_counter;
	struct command_t*	pElse;
	struct command_t*	pEndif;
	struct command_t*	next;
};

struct timed_command_t {
	char*			command;
	u_int32_t		len;
	struct timeval		time;
	struct timed_command_t*	next;
};

struct command_head_t {
	char*			name;
	u_int32_t		len;
	bool			deleted;
	u_int32_t		command_counter;
	command_t*		next_command;
	command_t*		head;
	command_t*		tail;
	command_head_t*		left;
	command_head_t*		right;
};

//class CController;
#include "controller.h"
#include "video_mixer.h"

class CCommand {
  public:
			CCommand(CVideoMixer* pVideoMixer);
			~CCommand();

			// Command entry to manipulate class CCommand
	int		ParseCommand(CController* pController, struct controller_type* ctr,
				const char* str);

			// Add command lines to command by name
			// Command will be created if it doesn't exist
			// If pCommandHead is not NULL, this will be used
	bool		AddCommandByName(const char* name, const char* command,
				command_head_t* pCommandHead = NULL);

			// True if command exists
	bool		CommandExists(const char* name);

			// True if found. Sets lines and pointer counter if found.
	bool		CommandLinesAndPointer(const char* name, u_int32_t* lines = NULL, u_int32_t* pointer = NULL);

			// Finalize command (at command end) to initialize
			// jumps (for goto and if-else-endif)
	int		CommandCheck(const char* name);

			// Delete a command by name
	void		DeleteCommandByName(const char* name);

			// Get command line by name and line number
	char*		GetCommandLine(const char* name, u_int32_t line);

			// Get a list of all stored command names
	char*		GetCommandNames();

			// Get next command line and remember status
	char*		GetNextCommandLine(const char* name);

			// Reset pointer to first command (for GetNextCommandLine)
	void		RestartCommandPointer(const char* name);

			// Run list of timed commands if time has elapsed
	void		RunTimedCommands(struct timeval time_now);

			// Set command pointer beyond last command
	void		SetCommandPointerAfterEnd(const char* name);

			// Return all of command in one string with newlines inbetween lines.
	char*		GetWholeCommand(const char* name);

			// Returns the number of commands created
	u_int32_t	CommandCount() { return m_command_count; }

			// Return the collective length of command names
	u_int32_t	CommandNamesLength(command_head_t* pHead);

	char*		Query(const char* query, bool tcllist = true);


 private:

	int		set_command_addatline(struct controller_type* ctr,
				CController* pController, const char* str);
	int		set_command_at(struct controller_type* ctr,
				CController* pController, const char* str);
	int		set_command_afterend(struct controller_type* ctr,
				CController* pController, const char* str);
	int		set_command_create(struct controller_type* ctr,
				CController* pController, const char* str);
	int		set_command_delete(struct controller_type* ctr,
				CController* pController, const char* str);
	int		set_command_deleteline(struct controller_type* ctr,
				CController* pController, const char* str);
	int		set_command_help(struct controller_type* ctr,
				CController* pController, const char* str);
	int		set_command_list(struct controller_type* ctr,
				CController* pController, const char* str);
	int		set_command_pop(struct controller_type* ctr,
				CController* pController, const char* str);
	int		set_command_push(struct controller_type* ctr,
				CController* pController, const char* str);
	int		set_command_pointer_atline(struct controller_type* ctr,
				CController* pController, const char* str);
	int		set_command_restart(struct controller_type* ctr,
				CController* pController, const char* str);

	void		AddCommandToList(command_head_t* pCommand_head, const char* command);
	void		AddCommandToListAtLine(command_head_t* pHead, u_int32_t line, const char* command);
	void		AddTimedCommand(const char* command, struct timeval* time, bool relative);
	void		DeleteCommandHead(command_head_t* pCommand_head, command_head_t* left,
				command_head_t* right);
	void		DeleteLineCommandByName(const char* name, u_int32_t line);
	command_head_t*	FindCommand(const char* name);
	command_head_t*	NewCommandHead(const char* name);
	command_t*	NewCommandLine(const char* command, u_int32_t* len);
	int		ParseIfCommand(const char* condition);
	int		Parse4IfElseEndif(command_t* pCommand, command_t* pIf);
	void		PopCommandByName(const char* name, u_int32_t lines);
	int		CommandPointerAtLine(const char* name, u_int32_t line);
	void		WriteCommandPointer(struct controller_type* ctr,
				command_head_t* pHead, bool recursive = true);
	int		SetPointer4Command(const char* name, u_int32_t line);

	// Private variables

	// head of binary tree holding commands created
	command_head_t*		m_command_head;

	// Number of commands inserted in the tree.
	u_int32_t		m_command_count;

	// Pointer to the last command used (for cahing)
	command_head_t*		m_last_command;

	// List of timed commands to be executed in the future
	timed_command_t*	m_pTimedCommands;

	// Pointer to class VideoMixer using this class
	CVideoMixer*		m_pVideoMixer;

	// Class should be verbose
	bool			m_verbose;	
};
	
#endif	// COMMAND_H
