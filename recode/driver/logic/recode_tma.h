#ifndef _RECODE_TMA_H
#define _RECODE_TMA_H

#include "recode_collector.h"
#include "pmu/pmu.h"

int recode_tma_init(void);

void recode_tma_fini(void);

/* on_pmi callback */
void update_events_index_on_this_cpu(struct hw_events *events);

void on_pmi_callback(unsigned cpu, struct pmus_metadata *pmus_metadata);

struct data_collector_sample *
get_sample_and_compute_tma(struct pmcs_collection *collection, u64 mask,
			   u8 cpu);

#endif /* _RECODE_TMA_H */
