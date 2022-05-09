#ifndef _PMU_H
#define _PMU_H

#include "pmu_structs.h"

#include <linux/percpu.h>

extern u64 __read_mostly gbl_reset_period;
extern unsigned __read_mostly gbl_fixed_pmc_pmi;

#define MAX_PARALLEL_HW_EVENTS	32

#define MSR_CORE_PERF_GENERAL_CTR0 MSR_IA32_PERFCTR0
#define MSR_CORE_PERFEVTSEL_ADDRESS0 MSR_P6_EVNTSEL0

#define RDPMC_FIXED_CTR0 BIT(30)
#define RDPMC_GENERAL_CTR0 (0)

#define PERF_GLOBAL_CTRL_FIXED0_SHIFT 32
#define PERF_GLOBAL_CTRL_FIXED0_MASK BIT_ULL(PERF_GLOBAL_CTRL_FIXED0_SHIFT)

#define PMC_TRIM(n) ((n) & (BIT_ULL(48) - 1))
#define PMC_CTR_MAX PMC_TRIM(~0)

#ifdef CONFIG_RUNNING_ON_VM
#define READ_FIXED_PMC(n) n
#define READ_GENERAL_PMC(n) n

#define WRITE_FIXED_PMC(n, v)
#define WRITE_GENERAL_PMC(n, v)
#else
#define READ_FIXED_PMC(n) native_read_pmc(RDPMC_FIXED_CTR0 + n)
#define READ_GENERAL_PMC(n) native_read_pmc(RDPMC_GENERAL_CTR0 + n)

#define WRITE_FIXED_PMC(n, v) wrmsrl(MSR_CORE_PERF_FIXED_CTR0 + n, PMC_TRIM(v))
#define WRITE_GENERAL_PMC(n, v) wrmsrl(MSR_IA32_PMC0 + n, PMC_TRIM(v))
#endif

#define SETUP_GENERAL_PMC(n, v) wrmsrl(MSR_CORE_PERFEVTSEL_ADDRESS0 + n, v)
#define QUERY_GENERAL_PMC(n) native_read_msr(MSR_CORE_PERFEVTSEL_ADDRESS0 + n)

DECLARE_PER_CPU(struct pmus_metadata, pcpu_pmus_metadata);

extern unsigned __read_mostly gbl_nr_pmc_fixed;
extern unsigned __read_mostly gbl_nr_pmc_general;

#define MAX_GBL_HW_EVTS 8
extern unsigned __read_mostly gbl_nr_hw_evts_groups;
extern struct hw_events * __read_mostly gbl_hw_evts_groups[MAX_GBL_HW_EVTS];

#define FIXED_TO_BITS_MASK                                                     \
	((BIT_ULL(gbl_nr_pmc_fixed) - 1) << PERF_GLOBAL_CTRL_FIXED0_SHIFT)

#define for_each_pmc(pmc, max) for ((pmc) = 0; (pmc) < (max); ++(pmc))
#define for_each_fixed_pmc(pmc) for_each_pmc (pmc, gbl_nr_pmc_fixed)
#define for_each_general_pmc(pmc) for_each_pmc (pmc, gbl_nr_pmc_general)

#define for_each_active_pmc(ctrl, pmc, off, max)                               \
	for ((pmc) = 0; (pmc) < (max); ++(pmc))                                \
		if (ctrl & BIT_ULL(pmc + off))

#define for_each_active_general_pmc(ctrl, pmc)                                 \
	for_each_active_pmc (ctrl, pmc, 0, gbl_nr_pmc_general)

#define for_each_active_fixed_pmc(ctrl, pmc)                                   \
	for_each_active_pmc (ctrl, pmc, PERF_GLOBAL_CTRL_FIXED0_SHIFT,         \
			     gbl_nr_pmc_fixed)

#define pmcs_fixed(pmcs) (pmcs)
#define pmcs_general(pmcs) (pmcs + gbl_nr_pmc_fixed)
#define nr_pmcs_general(cnt) (cnt - gbl_nr_pmc_fixed)

#define perf_evt_sel_to_code(x) (x & (BIT(16) - 1))

void enable_pmc_on_this_cpu(bool force);
void disable_pmc_on_this_cpu(bool force);

void enable_pmc_on_system(void);
void disable_pmc_on_system(void);

void get_machine_configuration(void);

void setup_hw_events_on_cpu(void *hw_events);

int setup_hw_events_on_system(pmc_evt_code *hw_events_codes, unsigned cnt);

void fast_setup_general_pmc_on_cpu(unsigned cpu, struct pmc_evt_sel *pmc_cfgs,
				   unsigned off, unsigned cnt);

int setup_pmc_on_system(pmc_evt_code *pmc_cfgs);

int init_pmu_on_system(void);
void cleanup_pmc_on_system(void);

/* Debug functions */
void debug_pmu_state(void);

u64 compute_hw_events_mask(pmc_evt_code *hw_events_codes, unsigned cnt);

void update_reset_period_on_system(u64 reset_period);

bool pmc_generate_collection(unsigned cpu);

#endif /* _PMU_H */
