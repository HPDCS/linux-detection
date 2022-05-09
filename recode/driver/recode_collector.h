#ifndef _RECODE_COLLECTOR_H
#define _RECODE_COLLECTOR_H

#include "pmu/pmu.h"
#include "recode_config.h"

struct data_collector_sample {
	pid_t id;
	unsigned tracked;
	unsigned k_thread;
	pmc_ctr system_tsc;

	pmc_ctr tsc_cycles;
	pmc_ctr core_cycles;
	pmc_ctr core_cycles_tsc_ref;
	// unsigned ctx_evts;
	struct pmcs_collection pmcs;
};

struct data_collector {
	unsigned cpu;
	unsigned rd_i;
	unsigned rd_p;
	unsigned ov_i;
	unsigned wr_i;
	unsigned wr_p;
	size_t size;
	u8 raw_memory[];
};

DECLARE_PER_CPU(struct data_collector *, pcpu_data_collector);

extern atomic_t on_dc_samples_flushing;

struct data_collector *init_collector(unsigned cpu);

void fini_collector(unsigned cpu);

struct data_collector_sample *get_write_dc_sample(struct data_collector *dc,
						  unsigned hw_events_cnt);

void put_write_dc_sample(struct data_collector *dc);

struct data_collector_sample *get_read_dc_sample(struct data_collector *dc);

void put_read_dc_sample(struct data_collector *dc);

bool check_read_dc_sample(struct data_collector *dc);

#endif /* _RECODE_COLLECTOR_H */