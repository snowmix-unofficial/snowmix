/*
 * (c) 2015 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */

#ifndef __TSANALYZER_H__
#define __TSANALYZER_H__

#include <sys/types.h>
//#include <stdint.h>
//#include <netinet/in.h>

//#define HAVE_LINUX

#ifdef HAVE_LINUX
#include <linux/if_alg.h>
#endif


#include "relay.h"

#define TS_PACKET_SIZE 188

class CController;
class CRelayControl;

//#define		0x0000
//#define		0x0001
#define	TS_IS_PSI	0x0002
#define	TS_IS_PMT	0x0004
//#define		0x0008
#define TS_IS_PROGRAMME	0x0010
#define TS_IS_PES	0x0020
#define TS_IS_ES	0x0040
//			0x0080
#define TS_IS_AUDIO	0x0100
#define TS_IS_VIDEO	0x0200
#define TS_IS_TEXT	0x0400
	
struct pid_list_t {
	u_int16_t		pid;		// PID
	struct pid_list_t*	next;		// Next in list
};

struct pmt_list_t {
	u_int16_t		program_number;		// This is the number of the program
	u_int16_t		pmt_pid;		// This is the PMT pid the program is described in
	//pid_list_t		pid_list;		// This is the list of pids associated with the program
	struct pmt_list_t*	next;			// This is the pointer to the next program in a sorted list of programs
};

struct pat_info_t {
	u_int8_t		current_version;
	u_int8_t		old_current_version;
	u_int8_t		next_version;
	
	u_int8_t		sec_n;			// This is the last section number in a PAT table we saw.
	u_int8_t		last_num;		// This is the last section number we should see
	pmt_list_t*		pmt_list[32];	// There can be 32 versions

	u_int32_t		n_version_shifts;	// This is a counter couting number of version shifts
};
	

struct pid_info_t {
	u_int16_t		pid;			// PID number - 0-8191
	u_int32_t		type_flag;		// TS_IS_PSI etc.

	// TS Header. Counters of occurences
	//u_int32_t		n_teis;			// Transport Indicator Error Counter
	u_int32_t		n_payload_start;	// Payload Unit Start Indicator Counter
	u_int32_t		n_priority;		// Transport Priority Counter
	u_int32_t		n_scramble;		// Scrambling Control Counter
	u_int32_t		n_adaptation;		// Adaptation Counter
	u_int64_t		last_time;		// Time of last packet analyzed

	// Adaptation values
	u_int64_t		last_pcr;		// Last PCR received in an adaptation field
	u_int64_t		last_opcr;		// Last OPCR received in an adaptation field

	// Adaptation. Counters of occurences
	u_int32_t		n_adap_len_error;	// Adaptation Length error counter
	u_int32_t		n_payload_len_error;	// Payload Length error counter
	u_int32_t		n_discontinuity;	// Adaptation Discontinuity Counter
	u_int32_t		n_random;		// Adaptation Random Counter
	u_int32_t		n_aespriority;		// Adaptation ES Priority Counter
	u_int32_t		n_payload;		// Payload counter
	u_int32_t		n_pcr;			// PCR counter - not the PCR
	u_int32_t		n_opcr;			// OPCR counter - not the OPCR
	u_int32_t		n_pcr_ext_error;	// Number of PCR ext > 299 error
	u_int32_t		n_opcr_ext_error;	// Number of OPCR ext > 299 error
	u_int32_t		n_private_data;		// Private data counter
	u_int32_t		n_adaptation_ext;	// Adaptation Extention counter

	struct psi_basic_info_t	*psi;			// Pointer to basic PSI data;

	struct pid_list_t	*pmt_list;		// List of PMTs detected
	struct pid_info_t	*next;
};

struct table_syntax_data_t {
	u_int16_t		table_id;
	u_int8_t		version;
	u_int8_t		sec_num;
	u_int8_t		last_num;
	table_syntax_data_t	*next;
	table_syntax_data_t	*nextid;
	u_int32_t		len;
	u_int8_t*		pData;
};

struct psi_basic_info_t {
	u_int16_t	tid;		// Table ID
	bool		ssi;		// Section Syntax Indicator
	u_int16_t	sec_len;	// Section Length in bytes

	// Table header
	u_int16_t	tsid;		// Table Syntax Section ID
	u_int8_t	version;	// Syntax Version. Incremented when data change. 5 bits
	bool		current_next;	// If true, then data is for now, otherwise for the future
	u_int8_t	section_number;	// Series of sections of tables. First section start with 0
	u_int8_t	last_section_number;	// Last section number in a series of sections. First is 0. There may only be one.

	// Event and Error counters
	u_int32_t	tid_error;		// tid error. tid == 255 is forbidden
	u_int32_t	len_error;		// Length error
	u_int32_t	no_tables;		// If we have no table or tables in PSI, we increment this
	u_int32_t	table_id_error;		// Number of table ID errors.
	u_int32_t	sec_len_errors;		// Section Length must not exceed 1021
	u_int32_t	last_section_missed;	// If we see section 0 and the last we saw was not last_section_number.
	u_int32_t	version_changed;	// Whenever the version changes, this gets incremented
	u_int32_t	version_hop;		// version changed more than 1 version number
	u_int64_t	version_change_time;	// Time the version changed

	table_syntax_data_t*	pTable;
};


enum analyzer_type_t {
	ANALYZER_UNDEFINED,
	ANALYZER_RTP,
	ANALYZER_TS,
	ANALYZER_PES
};

#define AnalyzerType2String(a)	( a == ANALYZER_RTP ? "rtp" :					\
				    a == ANALYZER_TS ? "ts" :					\
				      a == ANALYZER_PES ? "pes" :				\
				        a == ANALYZER_UNDEFINED ? "undefined" : "unknown")

class CAnalyzer;

struct analyzer_list_t {
	CAnalyzer*		pAnalyzer;
	struct analyzer_list_t*	next;
};


class CAnalyzer {
  public:
	CAnalyzer(const char* name, u_int32_t analyzer_id, u_int32_t verbose = 0);
	~CAnalyzer();
	int			SetSource(CRelayControl* pRelayControl, source_type_t source_type, u_int32_t source_id);
	int			NewPackets(u_int8_t *packet, u_int32_t packet_len, u_int64_t timenow = 0LLU);
	int			AddAnalyzer(CAnalyzer* pAnalyzer);
	int			RemoveAnalyzer(CAnalyzer* pAnalyzer);


	int			Start(CRelayControl* pCRelayControl);
	int			Stop();
	int			Status(CController* pCtr, const char* str);
	u_int32_t		GetID() { return m_id; };
	const char*		GetName() { return m_name; };
	analyzer_type_t		GetType() { return m_analyzer_type; };
	source_type_t		GetSource() { return m_source_type; };
	u_int32_t		GetSourceID() { return m_source_id; };
	u_int32_t		GetBytes() { return m_bytes; };
	u_int32_t		Started() { return m_started; };
protected:


	u_int32_t		m_id;			// Analyzer ID
	char*			m_name;			// Analyzer name
	u_int32_t		m_verbose;		// Verbosity of analyzer
	bool			m_started;		// True if analyzer is started
	analyzer_type_t		m_analyzer_type;	// Analyzer type
	source_type_t		m_source_type;		// Analyzer source type (receiver, sender, analyzer)
	u_int32_t		m_source_id;		// ID of analyzer source
	u_int32_t		m_bytes;		// Bytes analyzed
	CRelayControl*		m_pRelayControl;	// Link back to CRelayControl

	analyzer_list_t*	m_pAnalyzerList;	// List of analyzers sourced by this analyzer

	// Time variables
	u_int64_t		m_start_time;		// Time in usec the analyzer was started
	u_int64_t		m_first_byte_time;	// Time in usec the analyzer received its first bytes
	u_int64_t		m_run_time;		// Total time in usec the analyzer has been running. Gets updated every time the analyzer is stopped
	u_int64_t		m_current_time;		// Gets updated every time NewPackets is called (with corret timenow value) and other times
	u_int64_t		m_last_time;		// Time of last packet analyzed
  private:
};


class CAnalyzerRTP : public CAnalyzer {
  public:
	CAnalyzerRTP(const char* name, u_int32_t analyzer_id, u_int32_t verbose = 0);
	~CAnalyzerRTP();
	int		NewPackets(u_int8_t *packet, u_int32_t packet_len);
	int		Status(CController* pCtr, const char* str);

  private:
	bool		m_verbose;
	u_int32_t	m_rtp_packets;
	u_int32_t	m_invalid_size;
	int32_t		m_version;
	
	u_int32_t	m_last_payload_type;
	u_int32_t	m_last_seq_no;
	u_int32_t	m_last_timestamp;
	u_int32_t	m_last_SSRC;
	u_int32_t	m_last_CSRC;

	// Counters
	u_int32_t	m_n_wrong_version;	// Version should be 2
	u_int32_t	m_n_out_of_seq;		// We increment this by one whenever we have the current is not previous + 1
	u_int32_t	m_n_large_seq_loss;	// We increment this by one whenever the detected seq_no loss is larger than 128
	u_int32_t	m_n_payload_change;	// We count the number of times the payload changes
	u_int32_t	m_n_ssrc_change;
	u_int32_t	m_n_padding_bytes;
	u_int32_t	m_n_seq_loss;
	
};

class CAnalyzerTS : public CAnalyzer {
  public:
	CAnalyzerTS(const char* name, u_int32_t analyzer_id, u_int32_t verbose = 0);
	~CAnalyzerTS();
	int			NewPackets(u_int8_t *packet, u_int32_t packet_len);
	int			ParseCommand(CController* pCtr, const char* str);
	int			Status(CController* pCtr, const char* str);
	int			DropNullPackets(u_int8_t *packet, u_int32_t packet_len);

  private:
	int			AddNewPid(u_int16_t pid);
	int			AnalyzePSI(u_int8_t* p, u_int16_t payload_len, u_int16_t pid);
	int			AnalyzePAT(u_int8_t* pData, u_int16_t len);
//	int			AnalyzePAT(table_syntax_data_t* pTable);
	int			AnalyzePMT(u_int8_t* pData, u_int16_t len, u_int16_t pid);
	int			AddProgram(u_int16_t program, u_int16_t pmt_pid, u_int8_t version);
	u_int32_t		CalcCrc32Mpeg2(u_int8_t * data, unsigned int datalen);

//#ifdef HAVE_LINUX
	int			CRC32C_Init();
	int			CRC32C_Close(int res);
	int			CRC32C(u_int8_t* data, ssize_t len, u_int32_t* res);
//#endif

//	u_int32_t		m_verbose;
	bool			m_drop_no_sync;		// If true then no sync packets will not be analyzed.
	bool			m_drop_teis;		// If true then packets with tei set will not be analyzed.
	u_int32_t		m_ts_packets;		// Number of valid ts packets
	u_int32_t		m_invalid_sync;		// Number of packets with invalid sync
	u_int32_t		m_invalid_size;		// Number of invalid size packets
	u_int32_t		m_psi_error;		// Number of errors parsing PSI
	u_int32_t		m_teis;			// Number of Transport Error Indicators
	u_int32_t		m_count_pids;		// Number of different Pids seen in stream
	u_int32_t		m_pid_count[8192];	// Array of counters counting occurence of each pid
	struct pid_info_t*	m_pids[8192];		// Array of pointers to each pid
	struct pid_info_t	*m_p_pid_list;
	struct pat_info_t	m_pat;

	u_int32_t		m_programs;		// Number of programs;

#ifdef HAVE_LINUX
	// CRC32C
	int			m_fd_alg_socket;	// fd to algorithm socket in Linux
	int			m_fd_alg_accept;	// fd to connected algorithm socket in Linux
	struct sockaddr_alg	m_sa;			// sock_addr structure for algorithm
#endif



};

class CAnalyzers {
  public:
	CAnalyzers(CRelayControl* pRelayControl, u_int32_t max_id = 10, u_int32_t verbose = 0);
	~CAnalyzers();
	int			ParseCommand(CController* pCtr, const char* str);
	bool			AnalyzerExist(u_int32_t id);
	int			AddAnalyzer(u_int32_t analyzer_id, CAnalyzer* pAnalyzer);
	int			RemoveAnalyzer(u_int32_t analyzer_id, CAnalyzer* pAnalyzer);

  private:
	int			analyzer_add(CController* pCtr, const char* str);
	int			list_help(CController* pCtr, const char* str);
	int			set_analyzer_source(CController* pCtr, const char* str);
	int			set_analyzer_startstop(CController* pCtr, const char* str, bool start);
	int			set_analyzer_status(CController* pCtr, const char* str);

	int			AddAnalyzer(u_int32_t analyzer_id, analyzer_type_t type, const char* name);
	int			DeleteAnalyzer(u_int32_t analyzer_id);
	int			StartStopAnalyzer(u_int32_t analyzer_id, bool start);
	int			SetAnalyzerSource(u_int32_t analyzer_id, source_type_t type, u_int32_t source_id);
	int			AnalyzerStatus(u_int32_t analyzer_id, CController* pCtr, const char* str);

	u_int32_t		m_verbose;
	u_int32_t		m_max_analyzers;
	u_int32_t		m_count_analyzers;
	CAnalyzer**		m_analyzers;			// Array of analyzers;
	CRelayControl*		m_pRelayControl;
};

#endif	// __TSANALYZER_H__
