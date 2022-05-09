#include "proc.h"

struct proc_dir_entry *root_pd_dir;

void init_proc(void)
{
	root_pd_dir = proc_mkdir(PROC_TOP, NULL);

	register_proc_cpus();
	register_proc_frequency();
	register_proc_sample_info();
	register_proc_processes();
	register_proc_state();

#ifdef SECURITY_MODULE_ON
	register_proc_mitigations();
	register_proc_thresholds();
	register_proc_security();
	register_proc_statistics();
#endif
}

void fini_proc(void)
{
	proc_remove(root_pd_dir);
}
