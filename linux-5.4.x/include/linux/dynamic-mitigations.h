#ifndef _DYNAMIC_PATCHES_H
#define _DYNAMIC_PATCHES_H

#include <linux/sched.h>

extern void enable_mitigations_on_system(unsigned mitigation);

extern void disable_mitigations_on_system(unsigned mitigation);

extern void request_mitigations_on_task(struct task_struct *tsk, bool delayed);

extern void enable_mitigations_on_task(struct task_struct *tsk);

extern void disable_mitigations_on_task(struct task_struct *tsk);

extern bool has_mitigations(struct task_struct *tsk);

extern bool has_pending_mitigations(struct task_struct *tsk);

/* Dynamic PTI mitigation */
extern void enable_pti_on_mm(struct mm_struct *mm);

extern void disable_pti_on_mm(struct mm_struct *mm);

/* Enable Transient Execution mitigations (PTI, SBB, etc.) */
extern void init_te_mitigations_on_task(struct task_struct *tsk);

extern void fini_te_mitigations_on_task(struct task_struct *tsk);

/* Side-channels mitigations */
extern void LLC_flush(struct task_struct *tsk);

extern void exile_task_from_physical_cpu(struct task_struct *tsk, unsigned no_cpu);

extern void mitigations_switch(struct task_struct *prev, struct task_struct *curr);

/* This flags are related to task->monitor_state */
#define DM_SCM_CPU_EXILED 	0
#define DM_ACTIVATING		1
#define DM_ACTIVE		2

#endif /* _DYNAMIC_PATCHES_H */