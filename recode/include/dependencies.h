#ifndef _DEPENDENCIES_H
#define _DEPENDENCIES_H

#include <linux/types.h>
#include <linux/sched.h>

extern int idt_patcher_install_entry(unsigned long handler, unsigned vector,
				     unsigned dpl);

typedef void ctx_func(struct task_struct *prev, bool prev_on, bool curr_on);

extern void switch_hook_set_state_enable(bool state);

extern void switch_hook_set_system_wide_mode(bool mode);

extern void set_hook_callback(ctx_func *hook);

extern int tracker_add(struct task_struct *tsk);

extern int tracker_del(struct task_struct *tsk);

extern bool query_tracker(struct task_struct *tsk);

extern unsigned get_tracked_pids(void);

extern void set_exit_callback(smp_call_func_t callback);

#endif /* _DEPENDENCIES_H */