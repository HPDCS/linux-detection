#ifndef _RECODE_CONFIG_H
#define _RECODE_CONFIG_H

#include "pmu/pmu.h"

/* TODO Refactor */
extern unsigned params_cpl_os;
extern unsigned params_cpl_usr;

#define BUFFER_MEMORY_SIZE (1024 * 1024 * 256)

enum recode_pmi_vector {
	NMI,
#ifdef FAST_IRQ_ENABLED
	IRQ
#endif
};

extern enum recode_pmi_vector recode_pmi_vector;

#define DECLARE_PMC_EVT_CODES(n, s) extern pmc_evt_code n[s]

DECLARE_PMC_EVT_CODES(pmc_events_management, 8);

DECLARE_PMC_EVT_CODES(pmc_events_tma_l0, 8);
DECLARE_PMC_EVT_CODES(pmc_events_tma_l1, 8);
DECLARE_PMC_EVT_CODES(pmc_events_tma_l2, 8);

#endif /* _RECODE_CONFIG_H */