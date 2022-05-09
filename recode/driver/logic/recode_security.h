#ifndef _RECODE_SECURITY_H
#define _RECODE_SECURITY_H

#include "recode_collector.h"
#include "pmu/pmu.h"
#include "recode.h"

enum tuning_type {
	FR = 0,
	XL = 1
};

extern enum tuning_type tuning_type;

struct sc_thresholds {
	unsigned nr;
	unsigned cnt;
	s64 ths[];
};

extern struct sc_thresholds sc_ths_fr;
extern struct sc_thresholds sc_ths_xl;

extern bool signal_detected;
extern unsigned ts_window;
extern unsigned ts_alpha;
extern unsigned ts_beta;

extern atomic_t detected_theads;

/* L2_miss / L1_miss */
#define DM0(p, sn)                                                             \
	((pmcs_general(sn->pmcs)[1] * p) / (pmcs_general(sn->pmcs)[0] + 1))
/* LLC_miss / L1_miss */
#define DM1(p, sn)                                                             \
	((pmcs_general(sn->pmcs)[2] * p) / (pmcs_general(sn->pmcs)[0] + 1))
/* L2_write_back / L2_lines_in */
#define DM2(p, sn)                                                             \
	((pmcs_general(sn->pmcs)[4] * p) / (pmcs_general(sn->pmcs)[5] + 1))
	// ((pmcs_general(sn->pmcs)[0] * p) / (pmcs_general(sn->pmcs)[0] + 1))
/* TLB_l2_miss / L1_miss */
#define DM3(p, sn)                                                             \
	((pmcs_general(sn->pmcs)[3] * p) / (pmcs_general(sn->pmcs)[0] + 1))

#define LESS_THAN_TS(ts, v, p) ((ts - p) < v)
#define MORE_THAN_TS(ts, v, p) (v < (ts + p))

struct detect_stats {
	/* Execution time */
	u64 utime;
	u64 stime;
	/* Process data*/
	pid_t pid;
	pid_t tgid;
	char comm[32];
	/* Detection data */
	unsigned s1;
	unsigned p4;
	unsigned score;
	unsigned ticks;
};

int recode_security_init(void);

void recode_security_fini(void);

/* on_pmi callback */
void on_pmi(unsigned cpu, struct pmus_metadata *pmus_metadata);

void on_ctx(struct task_struct *prev, bool prev_on, bool curr_on);

bool on_state_change(enum recode_state state);

struct data_collector_sample *
get_sample_and_compute_tma(struct pmcs_collection *collection, u64 mask,
			   u8 cpu);

void pmc_evaluate_activity(unsigned cpu, struct task_struct *tsk,
			   struct pmcs_collection *pmcs);

#endif /* _RECODE_SECURITY_H */
