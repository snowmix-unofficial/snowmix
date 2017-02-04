/*
 * (c) 2015 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */
//#include <stdlib.h>
//#include <stdint.h>
#include <sys/socket.h>
#ifdef HAVE_LINUX
#include <linux/if_alg.h>
#endif
#include <sys/param.h>
//#include <strings.h>

#include <unistd.h>
#include <stdio.h>
#ifdef HAVE_MALLOC
#include <malloc.h>
#else
#include <stdlib.h>
#endif
#include <stdlib.h>

//#include <stdarg.h>
//#include <stdlib.h>
//#include <errno.h>
//#include <sys/stat.h>
#include <sys/time.h>
//#include <sys/types.h>
//#include <sys/socket.h>
//#include <sys/un.h>
//#include <arpa/inet.h>
//#include <netinet/in.h>
//#include <fcntl.h>
#include <string.h>
#include <ctype.h>
//#include <sys/select.h>

//#include <stdint.h>
//#include <sys/select.h>
//#include <sys/time.h>
//#include <stdbool.h>
//#include <glib.h>
//#ifdef WITH_GUI
//#include <gtk/gtk.h>
//#endif

#include "controller.h"
#include "tsanalyzer.h"


#define PCR2nsec(base,ext) ((9000*(300LLU*base + ext))/243)

static u_int32_t crc32_mpeg2_tab[256] = {
  0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b,
  0x1a864db2, 0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
  0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd, 0x4c11db70, 0x48d0c6c7,
  0x4593e01e, 0x4152fda9, 0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
  0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3,
  0x709f7b7a, 0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
  0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58, 0xbaea46ef,
  0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
  0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49, 0xc7361b4c, 0xc3f706fb,
  0xceb42022, 0xca753d95, 0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
  0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 0x34867077, 0x30476dc0,
  0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
  0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 0x018aeb13, 0x054bf6a4,
  0x0808d07d, 0x0cc9cdca, 0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
  0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08,
  0x571d7dd1, 0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
  0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b, 0xbb60adfc,
  0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
  0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a, 0xe0b41de7, 0xe4750050,
  0xe9362689, 0xedf73b3e, 0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
  0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,
  0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
  0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb, 0x4f040d56, 0x4bc510e1,
  0x46863638, 0x42472b8f, 0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
  0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5,
  0x3f9b762c, 0x3b5a6b9b, 0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
  0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e, 0xf5ee4bb9,
  0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
  0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f, 0xc423cd6a, 0xc0e2d0dd,
  0xcda1f604, 0xc960ebb3, 0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
  0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71,
  0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
  0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640, 0x4e8ee645, 0x4a4ffbf2,
  0x470cdd2b, 0x43cdc09c, 0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
  0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e,
  0x18197087, 0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
  0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d, 0x2056cd3a,
  0x2d15ebe3, 0x29d4f654, 0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
  0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c, 0xe3a1cbc1, 0xe760d676,
  0xea23f0af, 0xeee2ed18, 0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
  0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662,
  0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
  0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
};

CAnalyzers::CAnalyzers(CRelayControl *pRelayControl, u_int32_t max_id, u_int32_t verbose) {
	m_verbose		= verbose;
	m_max_analyzers		= max_id;
	m_pRelayControl	= pRelayControl;
	m_count_analyzers	= 0;
	m_analyzers		= (CAnalyzer**) calloc(sizeof(CAnalyzer*), max_id);
	if (!m_analyzers) fprintf(stderr, "Failed to allocate memory for array of place holders for analyzers for class CAnalyzers\n");
}
CAnalyzers::~CAnalyzers() {
	if (m_analyzers) {
		for (unsigned i = 0; i < m_max_analyzers; i++)
			if (m_analyzers[i]) {
				delete m_analyzers[i];
				m_analyzers[i] = NULL;
			}
		free(m_analyzers);
	}
}
bool CAnalyzers::AnalyzerExist(u_int32_t id) {
	return (m_analyzers && id < m_max_analyzers && m_analyzers[id] ?
		true : false);
}

int CAnalyzers::AddAnalyzer(u_int32_t analyzer_id, CAnalyzer* pAnalyzer)
{
	if (analyzer_id >= m_max_analyzers || !pAnalyzer) return -1;
	return m_analyzers[analyzer_id]->AddAnalyzer(pAnalyzer);
}
int CAnalyzers::RemoveAnalyzer(u_int32_t analyzer_id, CAnalyzer* pAnalyzer)
{
	if (analyzer_id >= m_max_analyzers || !pAnalyzer) return -1;
	return m_analyzers[analyzer_id]->RemoveAnalyzer(pAnalyzer);
}


// Returns : 0=OK, 1=System error, -1=Syntax error
int CAnalyzers::ParseCommand(CController* pCtr, const char* str) {

	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) return -1;

	// analyzer add [(ts|rtp|pes) <analyzer id> [<analyzer name>]]]
	if (!strncmp(str, "add ", 4))
		return analyzer_add(pCtr, str+4);
	else if (!strncmp(str, "source ", 7))
		return set_analyzer_source(pCtr, str+7);
	else if (!strncmp(str, "start ", 6))
		return set_analyzer_startstop(pCtr, str+6, true);
	else if (!strncmp(str, "stop ", 5))
		return set_analyzer_startstop(pCtr, str+5, false);
	else if (!strncmp(str, "stat", 4)) {
		if (*(str+4) == ' ') str += 4;
		else if (!strncmp(str, "status ", 7)) str += 7;
		else return -1;
		return set_analyzer_status(pCtr, str);
	}
	else if (!strncmp(str, "help ", 5))
		return list_help(pCtr, str+5);
	else return -1;
}

int CAnalyzers::list_help(CController* pCtr, const char* str) {
	if (!pCtr) return 1;
	pCtr->WriteMsg("STAT: commands analyzer\n"
		"STAT: analyzer add [(ts|rtp|pes) <analyzer id> [<analyzer name>]]\n"
		"STAT: analyzer source <analyzer id> (receiver | sender | analyzer) <source id>\n"
		"STAT: analyzer start <analyzer id>\n"
		"STAT: analyzer stop <analyzer id>\n"
		"STAT: analyzer help     // This list\n");
	return 0;
}

int CAnalyzers::set_analyzer_status(CController* pCtr, const char* str) {
	u_int32_t analyzer_id;
	if (!pCtr || !str) return -1;
	while(isspace(*str)) str++;
	if (sscanf(str,"%u", &analyzer_id) != 1) return -1;
	while(isdigit(*str)) str++;
	while(isspace(*str)) str++;
	return AnalyzerStatus(analyzer_id, pCtr, str);
}


int CAnalyzers::analyzer_add(CController* pCtr, const char* str) {
	if (!str) return -1;
	while(isspace(*str)) str++;

	// See if we should list analyzers
	if (!(*str)) {
		if (!m_analyzers) return -1;
		for (unsigned i = 0; i < m_max_analyzers ; i++)
			if (m_analyzers[i]) {
				const char* name = m_analyzers[i]->GetName();
				pCtr->WriteMsg("STAT: Analyzer %u type %s name <%s>\n", i,
					AnalyzerType2String(m_analyzers[i]->GetType()), 
					name ? name : "");
			}
		pCtr->WriteMsg("STAT:\n");
		return 0;
	}

	// Now get the type
	analyzer_type_t type = ANALYZER_UNDEFINED;
	u_int32_t analyzer_id;
	int n;
	if (!strncmp(str, "ts ", 3)) type = ANALYZER_TS;
	else if (!strncmp(str, "rtp ", 4)) type = ANALYZER_RTP;
	else if (!strncmp(str, "pes ", 4)) type = ANALYZER_PES;
	else return -1;
	while (*str && !isspace(*str)) str++;
	while(isspace(*str)) str++;
	if (sscanf(str, "%u", &analyzer_id) != 1) return -1;
	while(isdigit(*str)) str++;
	while(isspace(*str)) str++;
	if (!(*str)) n = DeleteAnalyzer(analyzer_id);
	else n = AddAnalyzer(analyzer_id, type, str);
	if (!n && m_verbose && pCtr)
		pCtr->WriteMsg("MSG: Analyzer %u %s.\n", analyzer_id, *str ? "added" : "deleted");
	return n;
}

int CAnalyzers::set_analyzer_startstop(CController* pCtr, const char* str, bool start) {
	if (!str) return -1;
	while(isspace(*str)) str++;

	// See if we should list analyzers
	if (!(*str)) {
		if (!m_analyzers) return -1;
		for (unsigned i = 0; i < m_max_analyzers ; i++)
			if (m_analyzers[i]) {
				pCtr->WriteMsg("STAT: Analyzer %u %s\n", i, m_analyzers[i]->Started() ? "started" : "stopped");
			}
		pCtr->WriteMsg("STAT:\n");
		return 0;
	}

	u_int32_t analyzer_id;
	int n;
	if (sscanf(str, "%u", &analyzer_id) != 1) return -1;
	n = StartStopAnalyzer(analyzer_id, start);
	if (!n && m_verbose && pCtr)
		pCtr->WriteMsg("MSG: Analyzer %u %s.\n", analyzer_id, start ? "started" : "stopped");
	return n;
}


// analyzer source <analyzer id> (receiver | sender | analyzer) <source id>
int CAnalyzers::set_analyzer_source(CController* pCtr, const char* str) {
	if (!str) return -1;
	while(isspace(*str)) str++;

	// See if we should list analyzers
	if (!(*str)) {
		if (!m_analyzers) return -1;
		for (unsigned i = 0; i < m_max_analyzers ; i++)
			if (m_analyzers[i]) {
				source_type_t type = m_analyzers[i]->GetSource();
				pCtr->WriteMsg("STAT: Analyzer %u source %s %u\n", i,
					SourceType2String(type), m_analyzers[i]->GetSourceID());
			}
		pCtr->WriteMsg("STAT:\n");
		return 0;
	}

	// Now get the type
	source_type_t type = SOURCE_UNDEFINED;
	u_int32_t analyzer_id, source_id;
	int n;
	if (sscanf(str, "%u", &analyzer_id) != 1) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	if (!strncmp(str, "rec", 3)) type = SOURCE_RECEIVER;
	else if (!strncmp(str, "anal", 4)) type = SOURCE_ANALYZER;
	else if (!strncmp(str, "send", 4)) type = SOURCE_SENDER;
	else return -1;
	while (*str && !isspace(*str)) str++;
	while(isspace(*str)) str++;
	if (sscanf(str, "%u", &source_id) != 1) return -1;
	n = SetAnalyzerSource(analyzer_id, type, source_id);
	if (!n && m_verbose && pCtr)
		pCtr->WriteMsg("MSG: Analyzer %u source %s %u\n", analyzer_id,
			SourceType2String(type), source_id);
	return n;
}

int CAnalyzers::AddAnalyzer(u_int32_t analyzer_id, analyzer_type_t type, const char* name) {
	if (!m_analyzers) return 1;
	if (analyzer_id >= m_max_analyzers || m_analyzers[analyzer_id] || type == ANALYZER_UNDEFINED || !name || !(*name)) return -1;
	if (type == ANALYZER_TS)
		m_analyzers[analyzer_id] = (CAnalyzerTS*) new CAnalyzerTS(name, analyzer_id, m_verbose);
	else if (type == ANALYZER_RTP)
		m_analyzers[analyzer_id] = (CAnalyzerRTP*) new CAnalyzerRTP(name, analyzer_id, m_verbose);
	else return -1;
	if (!m_analyzers[analyzer_id]) return 1;
	if (m_analyzers[analyzer_id]->GetName() == NULL || m_analyzers[analyzer_id]->GetID() != analyzer_id) {
		delete m_analyzers[analyzer_id];
		m_analyzers[analyzer_id] = NULL;
		return 1;
	}
	return 0;
}

int CAnalyzers::StartStopAnalyzer(u_int32_t analyzer_id, bool start) {
	if (!m_analyzers) return 1;
	if (analyzer_id >= m_max_analyzers || !m_analyzers[analyzer_id]) return -1;
	return (start ? m_analyzers[analyzer_id]->Start(m_pRelayControl) : m_analyzers[analyzer_id]->Stop());
}

int CAnalyzers::SetAnalyzerSource(u_int32_t analyzer_id, source_type_t type, u_int32_t source_id) {
	if (!m_analyzers) return 1;
	if (analyzer_id >= m_max_analyzers || !m_analyzers[analyzer_id]) return -1;
	if (m_analyzers[analyzer_id]->Started()) return -1;
	return m_analyzers[analyzer_id]->SetSource(m_pRelayControl, type, source_id);
}

int CAnalyzers::DeleteAnalyzer(u_int32_t analyzer_id) {
	if (!m_analyzers || analyzer_id >= m_max_analyzers || !m_analyzers[analyzer_id]) return -1;
	delete m_analyzers[analyzer_id];
	m_analyzers[analyzer_id] = NULL;
	if (m_count_analyzers) m_count_analyzers--;
	return 0;
}

int CAnalyzers::AnalyzerStatus(u_int32_t analyzer_id, CController* pCtr, const char* str) {
	if (!m_analyzers || !pCtr) return 1;
	if (analyzer_id >= m_max_analyzers || !m_analyzers[analyzer_id]) return -1;
	return m_analyzers[analyzer_id]->Status(pCtr, str);
}


CAnalyzer::CAnalyzer(const char* name, u_int32_t id, u_int32_t verbose) {
	fprintf(stderr, "CAnalyzer Constructor verbose %u\n", verbose);
	m_id = id;
	if (name) {
		m_name	= strdup(name);
		if (m_name) trim_string(m_name);
	} else {
		fprintf(stderr, "Failed to get memory for name for analyzer %u in constructor\n", id);
		m_name = NULL;
	}
	m_verbose		= verbose;
	m_started		= false;
	m_source_type		= SOURCE_UNDEFINED;
	m_source_id		= 0;
	m_analyzer_type		= ANALYZER_UNDEFINED;
	m_pRelayControl 	= NULL;
	m_bytes			= 0;
	m_start_time		= 0;
	m_first_byte_time	= 0;
	m_run_time		= 0;
	m_pAnalyzerList		= NULL;
}

CAnalyzer::~CAnalyzer() {
	Stop();
	if (m_name) free(m_name);
	fprintf(stderr, "CAnalyzer destroy\n");
}

int CAnalyzer::AddAnalyzer(CAnalyzer* pAnalyzer) {
	analyzer_list_t* pAnalyzerList;
	if (!pAnalyzer || pAnalyzer == this) return -1;
	if (!(pAnalyzerList = (analyzer_list_t*) calloc(sizeof(analyzer_list_t),1))) return -1;
	pAnalyzerList->pAnalyzer = pAnalyzer;
	pAnalyzerList->next = m_pAnalyzerList;
	m_pAnalyzerList = pAnalyzerList;
	return 0;
}

int CAnalyzer::RemoveAnalyzer(CAnalyzer* pAnalyzer) {
	analyzer_list_t* pAnalyzerList;
	if (!pAnalyzer || !m_pAnalyzerList || pAnalyzer == this) return -1;
	pAnalyzerList = m_pAnalyzerList;
	if (pAnalyzerList->pAnalyzer == pAnalyzer) {
		m_pAnalyzerList = pAnalyzerList->next;
		free(pAnalyzerList);
		return 0;
	}
	while (pAnalyzerList->next) {
		if (pAnalyzerList->next->pAnalyzer == pAnalyzer) {
			pAnalyzerList = pAnalyzerList->next;
			pAnalyzerList->next = pAnalyzerList->next->next;
			free (pAnalyzerList);
			return 0;
		}
		pAnalyzerList = pAnalyzerList->next;
	}
	return -1;

}


int CAnalyzer::Start(CRelayControl* pRelayControl) {
	if (m_started || m_source_type == SOURCE_UNDEFINED || !pRelayControl) return -1;
	if (pRelayControl->AddAnalyzer2Source(m_source_type, m_source_id, this, true)) {
		// We failed to add analyzer
		if (m_verbose) fprintf(stderr, "Failed to add analyzer %u to %s %u\n", m_id,
			SourceType2String(m_source_type), m_source_id);
		return -1;
	}
	m_pRelayControl = pRelayControl;
	m_started = true;
	{
		struct timeval timenow;
		gettimeofday(&timenow,NULL);
		m_start_time = Timeval2U64(timenow);
	}
	return 0;
}

int CAnalyzer::Stop() {
	if (!m_started || m_source_type == SOURCE_UNDEFINED || !m_pRelayControl) return -1;
	if (m_pRelayControl->AddAnalyzer2Source(m_source_type, m_source_id, this, false)) {
		// We failed to remove analyzer
		if (m_verbose) fprintf(stderr, "Failed to add analyzer %u to %s %u\n", m_id,
			SourceType2String(m_source_type), m_source_id);
		return -1;
	}
	m_pRelayControl = NULL;
	m_started = false;
	{
		struct timeval timenow;
		gettimeofday(&timenow,NULL);
		m_run_time += (Timeval2U64(timenow) - m_start_time);
	}
	return 0;
}

int CAnalyzer::SetSource(CRelayControl* pRelayControl, source_type_t source_type, u_int32_t source_id) {
	if (m_started || !pRelayControl) return -1;
	if (!pRelayControl->SourceExist(source_type, source_id)) return -1;
	m_source_type = source_type;
	m_source_id = source_id;
	return 0;
}

int CAnalyzer::NewPackets(u_int8_t *packet, u_int32_t packet_len, u_int64_t timenow) {

	int n = 0;
	if (!m_started) return 0;
	if (!packet || !packet_len) return 1;
	m_bytes += packet_len;
	if (!timenow) {
		struct timeval timenowval;
		gettimeofday(&timenowval,NULL);
		timenow = Timeval2U64(timenowval);
	}
	if (!m_first_byte_time) m_first_byte_time = timenow;
	m_current_time = timenow;
	if (m_analyzer_type == ANALYZER_TS) {
		n = ((CAnalyzerTS*)this)->NewPackets(packet, packet_len);
	} else if (m_analyzer_type == ANALYZER_RTP) {
		n = ((CAnalyzerRTP*)this)->NewPackets(packet, packet_len);
	}
	m_last_time = timenow;
	return n;
}

int CAnalyzer::Status(CController* pCtr, const char* str) {
	struct timeval timenow;
	int n = 0;
	if (!pCtr || !str) return 1;
	gettimeofday(&timenow,NULL);
	m_current_time = Timeval2U64(timenow);
	pCtr->WriteMsg("STAT: Analyzer %u type %s bytes %u time %.3lf\n", m_id,
		AnalyzerType2String(m_analyzer_type), m_bytes, m_current_time/1000000.0);
	if (m_analyzer_type == ANALYZER_TS)
		n = ((CAnalyzerTS*)this)->Status(pCtr, str);
	else if (m_analyzer_type == ANALYZER_RTP)
		n = ((CAnalyzerRTP*)this)->Status(pCtr, str);
	pCtr->WriteMsg("STAT:\n");
	return n;
}

CAnalyzerTS::CAnalyzerTS(const char* name, u_int32_t id, u_int32_t verbose) : CAnalyzer(name, id, verbose) {
	fprintf(stderr, "CAnalyzerTS Constructor verbose %u\n", verbose);
	//m_verbose		= verbose;
	m_drop_no_sync		= true;
	m_drop_teis		= true;
	m_teis			= 0;
	m_psi_error		= 0;
	m_ts_packets		= 0;
	m_invalid_sync		= 0;
	m_programs		= 0;
	m_count_pids		= 0;
	memset(&m_pids[0], 0, sizeof(m_pids));
	memset(&m_pid_count[0], 0, sizeof(m_pid_count));
	m_p_pid_list		= NULL;

	// PAT info. Setting current version to 255. Valid values are 0-31.
	memset(&m_pat, 0, sizeof(m_pat));
	m_pat.current_version	= 255;		// As long as version is 255 then we have not seen a valid version
	m_pat.old_current_version	= 255;		// As long as version is 255 then we have not seen a valid version
	m_pat.next_version	= 255;		// As long as version is 255 then we have not seen a valid version

	m_analyzer_type		= ANALYZER_TS;

	// A valid TS stream must contain a PAT table
	if (AddNewPid(0)) fprintf(stderr, "Failed to add new pid 0\n");
	else m_pids[0]->type_flag = TS_IS_PSI;

/*
	if (CRC32C_Init())
		fprintf(stderr, "Failed to initalize CRC32C Socket for analyzer %u\n", m_id);
	else if (m_verbose) fprintf(stderr, "CAnalyzerTS CRC32C Initialized. socket %d accept %d\n", m_fd_alg_socket, m_fd_alg_accept);
//fprintf(stderr, "Socket %d %016lX\n", m_fd_alg_socket, (u_int64_t)&m_fd_alg_socket);
//fprintf(stderr, "Accept %d %016lX\n", m_fd_alg_accept, (u_int64_t)&m_fd_alg_accept);

	//u_int8_t	data[1024*1024];
	u_int8_t	data[9];
	u_int32_t	res1, res;
	for (int i = 0; i < sizeof(data) ; i++) data[i] = i;
	struct timeval start_time, timenow, time_diff;

#define SOL_ALG 279
	res1 = htonl(calc_crc32(data,sizeof(data)));

	// 77558401
	for (u_int32_t key = 0x77558401; ; key-- ) {
		if (setsockopt(m_fd_alg_socket, SOL_ALG, ALG_SET_KEY, &key, sizeof(key))) {
			perror("Socket error");
		}
		CRC32C(data,sizeof(data), &res);
		if (res == res1) { fprintf(stderr, "key ident %08X\n", key); exit(0); }
		if ((key & 0x0001ffff) == 0x00010000) fprintf(stderr, "Key %08X %08X %08X\n", key, res, res1);
		if (!key) exit(1);
	}
	exit(0);

	{
		gettimeofday(&start_time, NULL);
		for (int i = 0; i < 1000 ; i++) {
			res = htonl(calc_crc32(data,sizeof(data)));
			//fprintf(stderr, "CRC %08X\n", res);
		}
		gettimeofday(&timenow, NULL);
		timersub(&timenow, &start_time, &time_diff);
		fprintf(stderr, "Timediff %lu.%06lu secs. res = %08X\n", time_diff.tv_sec, time_diff.tv_usec, res);
	}

	{
		gettimeofday(&start_time, NULL);
		for (int i = 0; i < 1000 ; i++)
			CRC32C(data,sizeof(data), &res);
		gettimeofday(&timenow, NULL);
		timersub(&timenow, &start_time, &time_diff);
		fprintf(stderr, "Timediff %lu.%06lu secs. res = %08X\n", time_diff.tv_sec, time_diff.tv_usec, res);
	}


int key = 0xffffffff;
	if (setsockopt(m_fd_alg_socket, SOL_ALG, ALG_SET_KEY, &key, sizeof(key))) {
		perror("Socket error");
	}

	{
		gettimeofday(&start_time, NULL);
		for (int i = 0; i < 1000 ; i++)
			CRC32C(data,sizeof(data), &res);
		gettimeofday(&timenow, NULL);
		timersub(&timenow, &start_time, &time_diff);
		fprintf(stderr, "Timediff %lu.%06lu secs. res = %08X\n", time_diff.tv_sec, time_diff.tv_usec, res);
	}



	exit(0);
	
*/
}

CAnalyzerTS::~CAnalyzerTS() {
	fprintf(stderr, "CAnalyzerTS destroy\n");
	while (m_p_pid_list) {
		pid_info_t* p = m_p_pid_list->next;
		if (m_p_pid_list->psi) free(m_p_pid_list->psi);
		free(m_p_pid_list);
		m_p_pid_list = p;
	}
#ifdef HAVE_LINUX
	CRC32C_Close(0);
#endif
}

#ifdef HAVE_LINUX
int CAnalyzerTS::CRC32C_Close(int res) {
	if (m_fd_alg_socket > -1) {
		close(m_fd_alg_socket);
		m_fd_alg_socket = -1;
	}
	if (m_fd_alg_accept > -1) {
		close(m_fd_alg_accept);
		m_fd_alg_accept = -1;
	}
	return res;
}

int CAnalyzerTS::CRC32C_Init() {
	m_fd_alg_socket = -1;
	m_fd_alg_accept = -1;

	struct sockaddr_alg sa;
	sa.salg_family	= AF_ALG;
	
	memcpy(&sa.salg_type[0], "hash", 5);
	strcpy((char*)&sa.salg_type[0], "hash");
	//memcpy(&sa.salg_type[0], "cipher", 5);
	//memset(&sa.salg_type[0], 0x00, 4);
	//sa.salg_type[4] = 0;
	//sa.salg_mask = 0xffffffff;
	//sa.salg_mask = 0x00000000;
	//sa.salg_feat = 0xffffffff;
	//sa.salg_feat = 0x00000000;
	memcpy(&sa.salg_name[0], "crc32", 6);

	if ((m_fd_alg_socket = socket(AF_ALG, SOCK_SEQPACKET, 0)) == -1 )
		CRC32C_Close(-1);

	if( bind(m_fd_alg_socket, (struct sockaddr *) &sa, sizeof(sa)) != 0 )
		CRC32C_Close(-1);

	if( (m_fd_alg_accept = accept(m_fd_alg_socket, NULL, 0)) == -1 )
		CRC32C_Close(-1);
	u_int32_t key = 0x00000000;
#define SOL_ALG 279
	if (setsockopt(m_fd_alg_socket, SOL_ALG, ALG_SET_KEY, &key, sizeof(key))) {
		perror("SetSocket");
	}

	return 0;
}

int CAnalyzerTS::CRC32C(u_int8_t* data, ssize_t len, u_int32_t* res) {
	if (!data || !len || !res || m_fd_alg_accept < 0) return -1;
	int n;
//fprintf(stderr, "CRC32C - len %ld fd %d\n", len, m_fd_alg_accept);

	if ((n = send(m_fd_alg_accept, data, len, MSG_MORE)) != len) {
//fprintf(stderr, " - send CRC32C - len %d\n", n);
		perror ("Send to AF_ALG Failed");
		return -1;
	}
//fprintf(stderr, " - send CRC32C - len %d\n", n);

	//u_int32_t crc32c = 0x00000000;
	u_int32_t crc32c = 0x00ffffff;
	if((n = read(m_fd_alg_accept, &crc32c, 4)) != 4) {
		return -1;
	}
//fprintf(stderr, " - read CRC32C - len %d crc %08X\n", n, crc32c);
	*res = htonl(crc32c);
	return 0;
}
#endif


// Adds a new pid to pid list. Returns:
//  -1 : Invalid Pid or Failed to allocate memory
//   0 : Succeeded
//   1 : Existed already
int CAnalyzerTS::AddNewPid(u_int16_t pid) {
	if (pid >= 8192) {
		fprintf(stderr, "Pid larger than 8192 %hu\n", pid);
		return -1;
	}
	if (m_pids[pid]) return 1;
	pid_info_t *pNewPid = (pid_info_t*) calloc(sizeof(pid_info_t),1);
	if (!pNewPid) return -1;
	pNewPid->pid = pid;
	m_pids[pid] = pNewPid;

	// Update count of different pids created
	m_count_pids++;

	if (!m_p_pid_list || pid < m_p_pid_list->pid) {
		pNewPid->next = m_p_pid_list;
		m_p_pid_list = pNewPid;
	} else {
		pid_info_t* p = m_p_pid_list;
		while (p) {
			if (!p->next || pid < p->next->pid) {
				pNewPid->next = p->next;
				p->next = pNewPid;
				break;
			}
			p = p->next;
		}
	}
	if (m_verbose) fprintf(stderr, "CAnalyzerTS added new pid %hu\n", pid);
	return 0;
}

int CAnalyzerTS::DropNullPackets(u_int8_t *packet, u_int32_t packet_len) {
	if (!packet || !packet_len) return 0;
	if (packet_len % 188) return packet_len;
	if (packet_len >= 32*188) return packet_len;	// we can only handle up to 31 packets

	int len = packet_len;
	u_int32_t nulls = 0;
	u_int8_t* p = packet;

	while (len) {
		if (p[0] != 0x47) return packet_len;
		u_int16_t pid = ((p[1] & 0x1F) << 8 ) | p[2];
		if (pid == 8191) {
			nulls = (nulls << 1) | 0x01;
		}
		p += 188;
		len -= 188;
	}
	if (!nulls) return packet_len;

	// 7 packets. nulls could be 101.1111
	// Check for only nulls
	if ((nulls & 0xffffffff) == nulls) return 0;
	int n = packet_len / 188;
	u_int8_t* p2 = p = packet;
	len = packet_len;
	bool dontcopy = true;
	int newlen = 0;
	while (len) {
		n--;
		len -= 188;
		// Should we skip
		if ((nulls >> n) & 0x1) {
			// p2 is copy2 address and is unchanged,
			p += 188;
			dontcopy = false;
			continue;
		}
		// We have a non null packet
		newlen += 188;
		if (!dontcopy) memcpy(p2, p, 188);
		p2 += 188;
		p += 188;
	}
	return newlen;
}

// Returns:
// 0 : Packets analyzed
int CAnalyzerTS::NewPackets(u_int8_t *packets, u_int32_t data_len)
{

	int errors = 0, n;
	u_int16_t tei;

	// Check that we have a pointer to one or more packets
	if (!packets || !data_len) {
		fprintf(stderr, "TS packet error. Packet %s len %u\n",
			packets ? "pointer" : "null", data_len);
		return -1;
	}

	// Data len must be at least 188 bytes
	if (data_len < TS_PACKET_SIZE) {
		fprintf(stderr, "Invalid size %u\n", data_len);
		m_invalid_size++;
		return 1;
	}
	u_int32_t len = data_len;
	//if (pTimeNow) memcpy(&(m_last_time), pTimeNow, sizeof(struct timeval));

	// Loop through all packets in data
	for (u_int8_t* p = packets; len >= 188 ; len -= 188) {

		// Each TS packet starts with a 4 byte header

		// Check for sync in first byte.
		if (p[0] != 0x47) {
			if (m_verbose > 1) fprintf(stderr, "No sync in TS packet.\n");
			m_invalid_sync++;
			errors++;
			if (m_drop_no_sync) continue;
		}

		// Update Packet counter
		// Total packets = m_ts_packets + (m_drop_no_sync ? m_invalid_sync : 0)
		m_ts_packets++;

		// Check for Transport Error Indicator. If set, we can't trust content of package
		if ((tei = (p[1] & 0x80) >> 7)) {
			// fprintf(stderr, "TEI for packet %u\n", m_ts_packets);
			m_teis++;
			if (m_drop_teis) continue;
		}

		// Get Pid, update pid count and add pid to list if it does not exist
		// PID can only be between 0-8192 due to masking
		u_int16_t pusi = (p[1] & 0x40) >> 6;			// Payload Unit Start Indicator. This packet has the start of a payload
		u_int16_t priority = (p[1] & 0x20) >> 5;		// Current packet has higher priority than other with same PID
		u_int16_t pid = ((p[1] & 0x1F) << 8 ) | p[2];		// This the PID of the packet. 0-8191
		u_int16_t scrambling = (p[3] & 0xc0) >> 6;		// Scrambling. 00=not scrambled
		u_int16_t adaptation = (p[3] & 0x20) >> 5;		// Does the packet have an adaptation field
		u_int16_t payload = (p[3] & 0x10) >> 4;			// Does the packet have a payload
		//u_int16_t cc = p[3] & 0xf;				// Contenuity counter.


		// Possible payload length before scanning for adaptation field is 188 - TSHeader
		u_int16_t payload_len = payload ? 184 : 0;
		u_int16_t payload_offset = payload ? 4 : 0;
		//fprintf(stderr, "Pack %u pid %hu adap %hu pay %hu cc %hu\n", m_ts_packets, pid, adaptation, payload, cc);

		// Update the count of the pid and add the pid if this was the first occurence
		if (!m_pid_count[pid]++) {
			if (!m_pids[pid]) {
				if ((n = AddNewPid(pid))) {
					fprintf(stderr, "Failed to add new pid %hu : %s.\n",
						pid, n > 0 ? "Exists already" : "Failed to allocate memory");
					errors++;
					continue;
				}
			}
		}

		// We now save the current time so we later know whe nwe last saw this PID.
		m_pids[pid]->last_time = m_current_time;

		// Check for NULL packets. They have Pid 8191 and will not contain valid data
		if (pid == 8191) continue;		// If PID == 8191 then we skip to next packet

		// We update counters.
		if (pusi) m_pids[pid]->n_payload_start++;
		if (priority) m_pids[pid]->n_priority++;
		if (scrambling) m_pids[pid]->n_scramble++;
		if (adaptation) m_pids[pid]->n_adaptation++;

		// Adaptation field
		u_int8_t adaptation_len = 0;			// Length in bytes of the adaptation field
		u_int16_t discontinuity;			// Discontinuity Indicator
		u_int16_t random;				// Random Access Indicator
		u_int16_t es_priority;				// Elementary Stream Priority Indicator
		u_int16_t pcr_present;				// PCR flag
		u_int16_t opcr_present;				// OPCR flag
		u_int16_t splicing;				// Splicing Point flag
		u_int16_t private_data;				// Transport Private Data flag
		u_int16_t adaptation_ext;			// Adaptation Extention flag
		//u_int16_t splice;				// Splice Countdown
		u_int64_t pcr_base;				// PCR base, if present
		u_int64_t pcr_ext;				// PCR ext, if present

		// If we have an adaptation field, we need to scan it
		if (adaptation) {
			// Check for minimum adaptation length
			if ((adaptation_len = p[4]) < 1) {		// Adaptation field length. Number of bytes follwing the 5th byte.
				if (m_verbose > 1) fprintf(stderr, "PID %hu Adaptation Length Error. Aadaptation length %hhu for packet %u\n",
					pid, adaptation_len, m_ts_packets);
				m_pids[pid]->n_adap_len_error++;
				errors++;
				continue;
			}

			// Read flags
			discontinuity = (p[5] & 0x80) >> 7;	// Discontinuity Indicator
			random = (p[5] & 0x40) >> 6;		// Random Access Indicator
			es_priority = (p[5] & 0x20) >> 5;	// Elementary Stream Priority Indicator
			pcr_present = (p[5] & 0x10) >> 4;	// PCR flag
			opcr_present = (p[5] & 0x8) >> 3;	// OPCR flag
			splicing = (p[5] & 0x4) >> 2;		// Splicing Point flag
			private_data = (p[5] & 0x2) >> 1;	// Transport Private Data flag
			adaptation_ext = p[5] & 0x1;		// Adaptation Extention flag
//fprintf(stderr, " alen %hhu discon %hu rnd %hu esprio %hu pcr %hu opcr %hu splice %hu priv %hu aext %hu\n", adaptation_len, discontinuity, random, es_priority, pcr_present, opcr_present, splicing, private_data, adaptation_ext);

			// Update adaptation field counters
			if (es_priority) m_pids[pid]->n_aespriority++;
			if (discontinuity) m_pids[pid]->n_discontinuity++;
			if (random) m_pids[pid]->n_random++;
			if (private_data) m_pids[pid]->n_private_data++;
			if (adaptation_ext) m_pids[pid]->n_adaptation_ext++;

			// We don't know what to do if there is private data
			unsigned int bytes_needed = 1 + (pcr_present ? 6 : 0) + (opcr_present ? 6 : 0) + (splicing ? 1 : 0);
			if (adaptation_len < bytes_needed) {
				if (m_verbose) fprintf(stderr, "PID %hu Adaptation Length Error. Length %hhu for packet %u\n",
					pid, adaptation_len, m_ts_packets);
				m_pids[pid]->n_adap_len_error++;
				errors++;
				continue;
			}

			// Program Clock Reference
			if (pcr_present) {
				m_pids[pid]->n_pcr++;
				pcr_base = (p[6]<<25) | (p[7]<<17) | (p[8]<<9) | (p[9]<1) | (p[10]>>7);
				pcr_ext = ((p[10] & 0x1)<<8) | p[11];
				if (pcr_ext < 300)
					m_pids[pid]->last_pcr = PCR2nsec(pcr_base, pcr_ext);
				else m_pids[pid]->n_pcr_ext_error++;
			}
			// Original Clock Reference
			if (opcr_present) {
				m_pids[pid]->n_opcr++;
				u_int8_t* op = p + (pcr_present ? 12 : 6);
				pcr_base = (op[6]<<25) | (op[7]<<17) | (op[8]<<9) | (op[9]<1) | (op[10]>>7);
				pcr_ext = ((op[10] & 0x1)<<8) | op[11];
				if (pcr_ext < 300)
					m_pids[pid]->last_opcr = PCR2nsec(pcr_base, pcr_ext);
				else m_pids[pid]->n_opcr_ext_error++;
			}
			// If opcr_present 33+6+9 bits
			//if (splicing) splice = p[6 + (pcr_present ? 6 : 0) + (opcr_present ? 6 :0)];

			// We must check for valid adaptation length. It can at most be 188 - TSHeader - adaplen_byte = 183
			if (adaptation_len > 183) {
				if (m_verbose) fprintf(stderr, "PID %hu Adaptation Length Error. Length %hhu for packet %u\n",
					pid, adaptation_len, m_ts_packets);
				m_pids[pid]->n_adap_len_error++;
				errors++;
				continue;
			}

			// If we have a payload, then update payload offset and payload length.
			// Payload length is whatever is left of the 188 bytes
			if (payload) {
				payload_offset += (1+adaptation_len);
				payload_len -= (1+adaptation_len);
			}
		}

		// Now we check payload size.
		if (payload && !payload_len) {
			m_pids[pid]->n_payload_len_error++;
			if (m_verbose > 1) fprintf(stderr, "Payload len error offset %hu paylen %hu alen %hu\n",
				payload_offset, payload_len, adaptation_len);
			errors++;
			continue;
		}
		if (payload_len > 0) m_pids[pid]->n_payload++;
		if ((m_pids[pid]->type_flag & TS_IS_PSI) && payload) {
			if (m_verbose > 1) fprintf(stderr, "Pid %hu alen %hu payoff %hu aylen %hu : ",
				pid, adaptation_len, payload_offset, payload_len);
			if ((n = AnalyzePSI(p+payload_offset, payload_len, pid)) > 0)
				m_psi_error++;
		}
	}

	// Check that we analyzed all bytes
	if (len > 0) {
		fprintf(stderr, "Invalid packet size. %u bytes left of %u\n", len, data_len);
		m_invalid_size++;
		return -1;
	}

	if (errors && m_verbose > 1) fprintf(stderr, "TS errors %d\n", errors);
	return errors;
}

int CAnalyzerTS::Status(CController* pCtr, const char* str) {
	if (!pCtr || !str) return 1;
	//struct timeval timenow, since_last;
	while (isspace(*str)) str++;
	pCtr->WriteMsg("STAT: TS Analyzer %u packets %u no sync %u tei %u wrong size %u psie %u progs %u pids %u\n",
		m_id, m_ts_packets, m_invalid_sync, m_teis, m_invalid_size, m_psi_error, m_programs, m_count_pids);
	while (*str) {
		if (!strncmp(str, "pat ", 4)) {
			if (m_pat.current_version < 32 ) {
				char *str, list[10*2+90*3+900*4+7192*5+1];
				list[0] = ' ';
				list[1] = 0x0;
				str = &list[1];
				pmt_list_t* p = m_pat.pmt_list[m_pat.current_version];
				while (p) {
					sprintf(str, "%hu%s", p->program_number, p->next ? " " : "");
					while (*str) str++;
					p = p->next;
				}
				pCtr->WriteMsg("STAT: PAT packets %u version %hhu next_version %hhu versions %u "
					"progs %u list :%s\n",
					m_pid_count[0], m_pat.current_version, m_pat.next_version, m_pat.n_version_shifts, m_programs, list);
			}
		}
		else if (!strncmp(str, "pid ", 4)) {
			for (pid_info_t* p = m_p_pid_list ; p ; p = p->next) {
				pCtr->WriteMsg("STAT: Pid %4hu count %6u ps %5u prio %u scr %u adap %4u:%-2u "
					"discon %u rnd %u aprio %u pay %6u priv %u aext %u pcr %u:%u opcr %u:%u last %.4lf s\n",
					p->pid,
					m_pid_count[p->pid],
					//p->n_teis,
					p->n_payload_start,
					p->n_priority,
					p->n_scramble,
					p->n_adaptation,p->n_adap_len_error,
					p->n_discontinuity,
					p->n_random,
					p->n_aespriority,
					p->n_payload,
					p->n_private_data,
					p->n_adaptation_ext,
					p->n_pcr, p->n_pcr_ext_error,
					p->n_opcr, p->n_opcr_ext_error,
					(m_current_time - p->last_time) / 1000000.0D
					);
			}
		}
		while (*str && !isspace(*str)) str++;
		while (isspace(*str)) str++;
	}
	//pCtr->WriteMsg("STAT:\n");
	return 0;
}

// Returns : 0=OK, 1=System error, -1=Syntax error
int CAnalyzerTS::ParseCommand(CController* pCtr, const char* str) {

	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) return -1;
//	if (!strncmp(str, "stat ", 5) || !strncmp(str, "status ", 7))
//		return list_status(pCtr, str+ (str[4] == ' ' ? 5 : 7));
	return 0;
}

/*
  PID : Type
   0  : PAT - Program Association Table. List program number and the PID for the PMT of each program.
   1  : CAT - Conditional Access.
  2?3 : TSDT - Transport Stream Description Table contains descriptors relating to the overall transport stream

  TID
*/
/* PSI Payload
	 0		: fb=Number of filler bytes
	 1 to fb	: filler bytes
	 1+fb		: tid=Table ID
	 2+fb		: ssi, private, res, part of sec_len
	 3+fb		: sec_len : rest of it. Sec_len must not exceed 1021
   Syntax Section. N Tables. n=0 to n=N-1. Table start ts=4+fb
	4+fb+n*8		: Table ID Extension MSB
	5+fb+n*8	: Table ID Extension LSB
	6+fb+n*8	: Reserved, Version, current-next-indicator cni
	7+fb+n*8	: Section number. First table starts with 0
	8+fb+n*8	: Last Section number
*/
int CAnalyzerTS::AnalyzePSI(u_int8_t* p, u_int16_t len, u_int16_t pid) {

	// We make some saftety checks
	if (!p || pid > 8191 || !m_pids[pid] || len < 1) return -1;

	// First we check if we have len for basic PSI header and advance p due to the filler byte counter byte
	u_int8_t fillbytes = p++[0];
	if (len < 4+fillbytes) {
		if (m_verbose) fprintf(stderr, "PSI packet pid %hu has payload length %hu < 4 (not includin fill bytes %hhu) for ts packet no. %u\n",
			pid, len, fillbytes, m_ts_packets);
		if (m_pids[pid] && m_pids[pid]->psi) m_pids[pid]->psi->len_error++;
		return 1;
	}
	len -= (fillbytes+1);
	p += fillbytes;

	// We have a basic PSI header and we can create a place holder for PSI header info within the PID if necessary
	if (!m_pids[pid]->psi) {
		if (!(m_pids[pid]->psi = (psi_basic_info_t *) (calloc(sizeof(psi_basic_info_t), 1)))) {
			if (m_verbose)
				fprintf(stderr, "Packet pid %hu has no internal psi place holder for ts packet no. %u\n",
					pid, m_ts_packets);
			return 1;
		}
		m_pids[pid]->psi->version = 32;		// version is between 0-31. 32 means version not set yet
	}

	// Time to look at the basic PSI header
	m_pids[pid]->psi->tid		=   p[0];		// Table ID : 1111.1111
	m_pids[pid]->psi->ssi		=  (p[1] & 0x80) >> 7;	// Section Syntax Indicator : 1xxx.xxxx			set to 1
	u_int16_t nulbit		=  (p[1] & 0x40) >> 6;	// x1xx.xxxx			set to 1
	u_int16_t reserved1		=  (p[1] & 0x30) >> 4;	// xx11.xxxx			set to 11
	u_int16_t unused_sec_len	=   p[1] & 0x0C;	// xxxx.11xx			set to 00
	u_int16_t sec_len = m_pids[pid]->psi->sec_len	= ((p[1] & 0x03) << 8 ) | p[2];	// xxxx.xx11 1111.1111

	// Lets check CRC32. CRC32 is on the entire length minus pointer field, fillers, header (3 bytes) and trailing CRC (4 bytes)
	// Len is already rspecting filler bytes and pointer field. and does not include CRC32

//fprintf(stderr, " >>>>> CRC32 ", pid);
	if (pid != 8192 && m_pids[pid]->psi->ssi && len > 7) {
		//u_int8_t* crc = p+len-4;
		u_int8_t* crc = p+3+sec_len-4;
		u_int32_t crc32 = htonl(CalcCrc32Mpeg2(p, 3+sec_len-4));
		if (crc32 != *((u_int32_t *) (crc)))
			fprintf(stderr, ">>>>>>>>>>>>>>>>>>>>> mismatch %08X != %08X\n", crc32, *((u_int32_t *) (crc)));
//		else
//			fprintf(stderr, "match packet %u crc %08X\n", m_ts_packets, crc32);
		
/*
//for (int m=0; m < 4 ; m++) {
		if (CRC32C(p+3, crc-p-3, &crc32)) {
			fprintf(stderr, "CRC32 calculating error PID %hu. Len %hu packet %u\n", pid, len-3, m_ts_packets);
		} else {
			if (crc32 != *((u_int32_t *) (crc)))
				fprintf(stderr, "CRC32 mismatch %08X != %08X\n", crc32, *((u_int32_t *) (crc)));
			else
				fprintf(stderr, "CRC32 match packet %u\n", m_ts_packets);
		}
*/
//}
	} else {
		if (pid == 0 && m_pids[pid]->psi->ssi) fprintf(stderr, "PSI CRC32 check not possible with len %hu\n", len);
	}


	// We update len to reflect how many bytes we can possible have left
	len -= 3;

	// Check for forbidden tid. 0xff is used for filling and is forbidden.
	if (p[0] == 0xff) {
		m_pids[pid]->psi->tid_error++;
		if (m_verbose)
			fprintf(stderr, "Packet pid %hu PSI has forbidden table id %hhu packet no %u\n",
					pid, p[0], m_ts_packets);
		return 1;
	}

	// Check that the PAT has tid == 0;
	if (pid == 0 && p[0] != 0) {
		if (m_verbose) fprintf(stderr, "PID 0 (PAT) has table ID != 0 for packet %u\n", m_ts_packets);
		m_pids[pid]->psi->table_id_error++;
	}

//if (m_verbose && pid == 0) fprintf(stderr, "PID 0 table %hhu ssi %u sec_len %hu len %hu packet %u\n", m_pids[pid]->psi->tid, m_pids[pid]->psi->ssi ? 1 : 0, sec_len, len, m_ts_packets);

	// See if there is a table section after sec_len. If not, there is no more data we should analyze
	if (!m_pids[pid]->psi->ssi) {
		if (m_verbose) fprintf(stderr, "PID %hu packet %u has no table\n", pid, m_ts_packets);
		m_pids[pid]->psi->no_tables++;
		return 0;
	}

	// We have a Syntax Section header. Check length
	if (len < 5) {
		if (m_verbose) fprintf(stderr, "PID %hu packet %u has ssi but length %hu is too short\n", pid, m_ts_packets, len);
		m_pids[pid]->psi->len_error++;
		return 1;
	}


	// header = 3 bytes
	// syntax = 5 bytes
	// Check that len >= header + syntax
	// Faulty sec_len gets corrected in stream.
	if (sec_len > 1121 || sec_len  > len) {
		fprintf(stderr, "PSI sec_len error. Packet pid %hu has sec_len %hu > len %hu. fills %hhu"
			" for ts packet %u\n",
			pid, sec_len, len, fillbytes, m_ts_packets);
		m_pids[pid]->psi->sec_len_errors++;
		return 1;
	}

	//u_int16_t tsid;			// 3+4 : 1111.1111 1111.1111
	u_int16_t reserved2;			// 5   : 11xx.xxxx
	u_int8_t version;			// 5   : xx11.111x
	//u_int16_t current_next;		// 5   : xxxx.xxx1
	//u_int16_t section_number;		// 6   : 1111.1111
	//u_int16_t last_section_number;	// 7   : 1111.1111

	/* sec_len = data available for table syntax
	   header = 5 bytes
	   CRC = 4 bytes at end.
	   PAT requires 4 bytes per entry in table
	   13 - 9 = 4
	   17 - 9 = 8
	   N = (sec_len - 9) >> 2 */


	if (0) fprintf(stderr, " - PAT id %hu ssi %hhu nulb %hu res1 %hu useclen %hu slen %hu(%hu bytes)%s\n",
		m_pids[pid]->psi->tid,
		(u_int8_t) m_pids[pid]->psi->ssi,
		nulbit, reserved1, unused_sec_len,
		(sec_len-9)>>2,
		sec_len,
		m_pids[pid]->psi->ssi ? " " : "\n");



	// Check for section number error. If new_section_number == 0 and last(section_number) != last_section_number)
	if (p[6] == 0 && m_pids[pid]->psi->section_number != m_pids[pid]->psi->last_section_number) {
		if (m_verbose > 0)
			fprintf(stderr, "PSI pid %hu did not reach last section %hhu before starting a new.",
				pid, m_pids[pid]->psi->last_section_number);
		m_pids[pid]->psi->last_section_missed++;
	}

	m_pids[pid]->psi->tsid			=  (p[3] << 8) | p[4];		// 1111.1111 1111.1111
	reserved2				=  (p[5] & 0xC0) >> 6;		// 11xx.xxxx
	version					=  (p[5] & 0x3E) >> 1;		// xx11.111x
	m_pids[pid]->psi->current_next		=  (p[5] & 0x01);		// xxxx.xxx1
	m_pids[pid]->psi->section_number	=   p[6];			// 1111.1111
	m_pids[pid]->psi->last_section_number	=   p[7];			// 1111.1111

	// Check for version change
	if (m_pids[pid]->psi->current_next && version != m_pids[pid]->psi->version) {
		if (m_pids[pid]->psi->version < 32) {
			if ((version+1)%32 != m_pids[pid]->psi->version) {
				if (m_verbose > 1)
					fprintf(stderr, "PSI pid %hu had version hop from %hhu to %hhu.", pid, m_pids[pid]->psi->version, version);
				m_pids[pid]->psi->version_hop++;
			}
			m_pids[pid]->psi->version_changed++;
		} // else this is the first version
		m_pids[pid]->psi->version = version;
		m_pids[pid]->psi->version_change_time = m_current_time;
	} else if (!m_pids[pid]->psi->current_next) {
		fprintf(stderr, "Missing code for handling PSI with current/next set to 0\n");
	}



	if (0) fprintf(stderr, " - tsid %hu res2 %hu version %hhu curnext %hhu secn %hhu lastsec %hhu\n",
		m_pids[pid]->psi->tsid,
		reserved2,
		m_pids[pid]->psi->version,
		(u_int8_t) m_pids[pid]->psi->current_next,
		m_pids[pid]->psi->section_number,
		m_pids[pid]->psi->last_section_number);

	if (pid == 0) {
		return AnalyzePAT(&p[8], sec_len - 9);
	} else if (m_pids[pid]->type_flag & TS_IS_PMT) {
		return AnalyzePMT(&p[8], sec_len - 9, pid);
	} else fprintf(stderr, "No Analyzer for PSI PID %hu\n", pid);


/*
	for (int i=0 ; i<8 ; i++)
	  for (int j=0 ; j<16 ; j++)
		fprintf(stderr, "%02hhx%s", p[8+i*16+j], j == 15 ? "\n" : " ");
*/
	return 0;
}

pid_list_t* AddPidToList(pid_list_t* pid_list, u_int16_t pid) {
	pid_list_t* p = (pid_list_t*) calloc(sizeof(pid_list_t), 1);
	if (p) p->pid = pid;
	else return pid_list;
	if (!pid_list) return p;
	if (pid < pid_list->pid) {
		p->next = pid_list;
		return p;
	}
	if (pid == pid_list->pid) {
		free (p);
		return pid_list;
	}
	if (!pid_list->next) {
		pid_list->next = p;
		return pid_list;
	}
	pid_list_t* head = pid_list;
	while (pid_list) {
		if (!pid_list->next || pid_list->next->pid > pid) {
			p->next = pid_list->next;
			pid_list->next = p;
			return head;
		}
		if (pid_list->next->pid == pid) {
			free (p);
			return head;
		}
		pid_list = pid_list->next;
	}
	return head;
}

/*
  m_pids[pid]->psi->tsid		=  (p[3] << 8) | p[4];		// 1111.1111 1111.1111
  reserved2				=  (p[5] & 0xC0) >> 6;		// 11xx.xxxx
  version				=  (p[5] & 0x3E) >> 1;		// xx11.111x
  m_pids[pid]->psi->current_next	=  (p[5] & 0x01);		// xxxx.xxx1
  m_pids[pid]->psi->section_number	=   p[6];			// 1111.1111
  m_pids[pid]->psi->last_section_number	=   p[7];			// 1111.1111
*/
// pData points to table , len is table length in bytes not including CRC. Rest is in the psi for the pid. Pid is 0
int CAnalyzerTS::AnalyzePAT(u_int8_t* pData, u_int16_t len) {
	int ntables = len >> 2;				// There will be 4 bytes for CRC32 after len
	if (!pData || len < 4) return -1;

	// Check if this is for current or next
	if (m_pids[0]->psi->current_next) {
		if (m_pat.current_version != m_pids[0]->psi->version) {
			if (m_verbose) fprintf(stderr, "New current version in the PAT. Old version %hhu new version %hhu. Packet %u\n"
				" - This section %hhu last section %hhu. Table entries %d\n",
				m_pat.current_version, m_pids[0]->psi->version, m_ts_packets,
				m_pids[0]->psi->section_number, m_pids[0]->psi->last_section_number, ntables);

			// We should now delete old PMT list.
			if (m_pat.current_version < 32 && m_pat.pmt_list[m_pat.current_version]) {
				//clear PMT flag on all pids in list.
				//and then delete list
			}

			m_pat.current_version = m_pids[0]->psi->version;
			for (int i = 0 ; i < ntables ; i++) {
				u_int16_t program = pData[i*4]<<8 | pData[1+i*4];
				u_int16_t pmt_pid = (pData[2+i*4] & 0x1F) <<8 | pData[3+i*4];
				if (!m_pids[pmt_pid]) {
					
					if (AddNewPid(pmt_pid)) {
						if (m_verbose) fprintf(stderr, "Failed to add PMT pid %hu in packet %u\n", pmt_pid, m_ts_packets);
					}
				}
				m_pids[pmt_pid]->type_flag = TS_IS_PMT | TS_IS_PSI;
				if (m_verbose) fprintf(stderr, " - %d. Added pid %hu as PMT for program %hu\n", i, pmt_pid, program);
				if (AddProgram(program, pmt_pid, m_pids[0]->psi->version)) {
					fprintf(stderr, "Failed to add program %hu pmt pid %u for packet %u\n", program, pmt_pid, m_ts_packets);
					continue;
				}
			}
		}
	} else {
	}
	return 0;
}

int CAnalyzerTS::AnalyzePMT(u_int8_t* pData, u_int16_t len, u_int16_t pid) {
	if (m_verbose > 1) fprintf(stderr, "PMT PID %hu : len %hu\n", pid,len);
	return 0;
}

int CAnalyzerTS::AddProgram(u_int16_t program, u_int16_t pmt_pid, u_int8_t version) {
//fprintf(stderr, "Adding program %hu pid %hu version %hhu\n", program, pmt_pid, version);
	if (version >31) return -1;
	pmt_list_t* new_program, *p = m_pat.pmt_list[version];
	while (p) {
		if (p->program_number > program) break;
		if (p->program_number == program) {
			if (p->pmt_pid != pmt_pid) {
				fprintf(stderr, "Error. Program %hu changed PMT pid from %hu to %hu in packet %u\n",
					program, p->pmt_pid, pmt_pid, m_ts_packets);
				p->pmt_pid = pmt_pid;
				return 1;
			}
			return 0;
		}
		p = p->next;
	}
	
	// Program was not found. We allocate a list entry
	if (!(new_program = (pmt_list_t*) calloc(sizeof(pmt_list_t),1))) {
		fprintf(stderr, "Failed to allocate memory for pmt list.\n");
		return -1;
	}
	m_programs++;
	new_program->program_number = program;
	new_program->pmt_pid = pmt_pid;
	if (!m_pat.pmt_list[version] || m_pat.pmt_list[version]->program_number > program) {
		new_program->next = m_pat.pmt_list[version];
		m_pat.pmt_list[version] = new_program;
		return 0;
	}
	p = m_pat.pmt_list[version];
	while (p) {
		if (!p->next || p->next->program_number > program) {
			new_program->next = p->next;
			p->next = new_program;

			p = m_pat.pmt_list[version];
			fprintf(stderr, "Programs : ");
			while (p)  {
				fprintf(stderr, "%hu ", p->program_number);
				p = p->next;
			}
			fprintf(stderr, "\n");

			return 0;
		}
		p = p->next;
	}
	return 1;
}
u_int32_t CAnalyzerTS::CalcCrc32Mpeg2 (u_int8_t * data, unsigned int datalen) {
  u_int32_t crc_sum = 0xffffffff;
  for (unsigned int i = 0; i < datalen; i++) {
    crc_sum = (crc_sum << 8) ^ crc32_mpeg2_tab[((crc_sum >> 24) ^ *data++) & 0xff];
  }
  return crc_sum;
}


CAnalyzerRTP::CAnalyzerRTP(const char* name, u_int32_t analyzer_id, u_int32_t verbose) : CAnalyzer(name, analyzer_id, verbose) {
	//m_verbose		= 0;
	m_analyzer_type		= ANALYZER_RTP;
	m_rtp_packets		= 0;
	m_invalid_size		= 0;
	m_version		= -1;	// -1 == no packets received
	m_last_payload_type	= 0;
	m_last_seq_no		= 0;
	m_last_timestamp	= 0;
	m_last_SSRC		= 0;
	m_last_CSRC		= 0;

	// Counters
	m_n_wrong_version	= 0;
	m_n_out_of_seq		= 0;
	m_n_large_seq_loss	= 0;
	m_n_payload_change	= 0;
	m_n_ssrc_change		= 0;
	m_n_padding_bytes	= 0;
	m_n_seq_loss		= 0;
	
}

CAnalyzerRTP::~CAnalyzerRTP() {
}

int CAnalyzerRTP::NewPackets(u_int8_t *packet, u_int32_t packet_len) {
	u_int32_t len;
	u_int8_t* p = packet;
	u_int8_t padding_bytes = 0;
	u_int8_t payload_offset = 12;

	if (!packet) return -1;

	if (packet_len < 12) {
		if (m_verbose > 1) fprintf(stderr, "Invalid RTP len %u for packet %u\n", packet_len, m_rtp_packets);
		m_invalid_size++;
		return -2;
	}
	m_rtp_packets++;

	int version = (0xc0 & (*p)) >> 6;
	bool padding = (0x20 & (*p)) >> 5;
	if (padding) padding_bytes = packet[packet_len-1];
	bool extension = (0x10 & (*p)) >> 4;
	u_int8_t csrc_count = (0x0f & (*p));

	bool marker = (0x80 & p[1]) >> 7;
	u_int8_t payload_type = (0x7f & p[1]);

	// packet advanced to next byte. Seq number, timestamp etc are network number with MSB first
	u_int16_t seq_no = (p[2] << 8) | p[3];
	u_int32_t timestamp = ntohl(*((u_int32_t*) (p+4)));
	u_int32_t SSRC = ntohl(*((u_int32_t*) (p+8)));

	u_int32_t* p_CSRC;
	if (csrc_count) {
		payload_offset += (csrc_count<<2);
		u_int32_t* p_CSRC = (u_int32_t*) (p+12);
	}

fprintf(stderr, "RTP packet %u version %d seq %hu pltype %hhu time %u SSRC %u len csrc %hhu %u\n", m_rtp_packets, version, seq_no, payload_type, timestamp, SSRC, csrc_count, packet_len);
	if (extension) {
		u_int16_t profile_identifier = (p[payload_offset] << 8) | p[payload_offset+1];
		u_int16_t profile_length = (p[payload_offset+2] << 8) | p[payload_offset+3];
		payload_offset += (4 + (profile_length << 2));
	}

	if (marker) {
	}

	// Detection of a new stream by looking at SSRC will fail on average in 1 out of 2^32 cases
	if (m_version < 0 || SSRC != m_last_SSRC) {
		// This is the first valid packet
		if (m_version > -1) m_n_ssrc_change++;
		m_version		= version;
		m_last_payload_type	= payload_type;
		m_last_seq_no		= seq_no;
		m_last_timestamp	= timestamp;
		m_last_SSRC		= SSRC;
		if (csrc_count && p_CSRC) m_last_CSRC = ntohl(*p_CSRC);

		// We reset a couple of counters
		m_n_wrong_version	= 0;
		m_n_payload_change	= 0;
		m_n_out_of_seq		= 0;
		m_n_large_seq_loss	= 0;
		m_n_padding_bytes	= padding_bytes;
	} else {
		// Check version
		if (version != m_version) {
			fprintf(stderr, "RTP version shift from %d to %d\n",
				m_version, version);
			m_version = version;
			m_n_wrong_version++;
			return -1;
		}

		// Update total padding byte counter
		m_n_padding_bytes += padding_bytes;

		// Check for payload type change
		if (m_last_payload_type != payload_type) {
			fprintf(stderr, "RTP payload shift from %d to %d\n",
				m_last_payload_type, payload_type);
			m_last_payload_type = payload_type;
			m_n_payload_change++;
		}

		// Check for loss of packets
		u_int16_t next_seq_no = m_last_seq_no + 1;	// We wrap when we reach 2^16-1
		if (next_seq_no != seq_no) {

			// We count number of out of sequence events
			m_n_out_of_seq++;

			u_int32_t seq_diff = (seq_no > next_seq_no ? seq_no - next_seq_no : next_seq_no - seq_no);
			if (seq_diff > 128) {
				// Either we have a new stream
			}
			if (seq_no > next_seq_no) {

				// seq_no is bigger than expected. Either a new stream or we lost packets
				if (seq_no - next_seq_no > 128) {
					m_n_large_seq_loss++;
				} else {
					m_n_seq_loss += next_seq_no;
				}
			} else {
				// seq_no < next_seq_no
			}
		}

		// Save seq_no for next time
		m_last_seq_no = seq_no;
	}
	m_last_timestamp = timestamp;
	m_last_SSRC = SSRC;
	m_last_CSRC = *p_CSRC;

	return 0;
}

int CAnalyzerRTP::Status(CController* pCtr, const char* str) {
	if (!pCtr) return 1;
	while (isspace(*str)) str++;
	pCtr->WriteMsg("STAT: RTP Analyzer %u packets %u wrong size %u\n",
		m_id, m_rtp_packets, m_invalid_size);
	while (*str) {
	}
	return 0;
}
