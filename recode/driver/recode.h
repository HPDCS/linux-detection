#ifndef _RECODE_CORE_H
#define _RECODE_CORE_H

#include <asm/bug.h>
#include <asm/msr.h>
#include <linux/sched.h>
#include <linux/string.h>

#include "pmu/pmu_structs.h"

#if __has_include(<asm/fast_irq.h>)
#define FAST_IRQ_ENABLED 1
#endif
#if __has_include(<asm/fast_irq.h>) && defined(SECURITY_MODULE) 
#define SECURITY_MODULE_ON 1
#elif defined(TMA_MODULE) 
#define TMA_MODULE_ON 1
#endif


#define MODNAME	"ReCode"

#undef pr_fmt
#define pr_fmt(fmt) MODNAME ": " fmt

enum recode_state {
	OFF = 0,
	TUNING = 1,
	PROFILE = 2,
	SYSTEM = 3,
	IDLE = 4, // Useless
};

extern enum recode_state __read_mostly recode_state;

struct recode_callbacks {
	void (*on_hw_events_change)(struct hw_events *events);
	void (*on_pmi)(unsigned cpu, struct pmus_metadata *pmus_metadata);
	void (*on_ctx)(struct task_struct *prev, bool prev_on,
				       bool curr_on);
	bool (*on_state_change)(enum recode_state);
};

extern struct recode_callbacks __read_mostly recode_callbacks;

/* Recode module */
extern int recode_data_init(void);
extern void recode_data_fini(void);

extern int recode_pmc_init(void);
extern void recode_pmc_fini(void);

extern void recode_set_state(unsigned state);

extern int register_ctx_hook(void);
extern void unregister_ctx_hook(void);

extern int attach_process(struct task_struct *tsk);
extern void detach_process(struct task_struct *tsk);

/* Recode PMI */
extern void pmi_function(unsigned cpu);

void setup_hw_events_from_proc(pmc_evt_code *hw_events_codes, unsigned cnt);

#endif /* _RECODE_CORE_H */