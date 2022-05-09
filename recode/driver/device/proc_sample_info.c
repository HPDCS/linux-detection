/* 
 * Proc and Fops related to PMC events
 */

#include "proc.h"
#include "../pmu/pmu.h"
#include "../pmu/hw_events.h"
#include "../recode_config.h"

#define DATA_HEADER "# PID,TRACKED,KTHREAD,CTX_EVTS,TIME,TSC,INST,CYCLES,TSC_CYCLES"

static int sampling_sample_info_show(struct seq_file *m, void *v)
{
	unsigned k, i;

	seq_printf(m, "%s\n", DATA_HEADER);

	for (k = 0; k < gbl_nr_hw_evts_groups; ++k) {
		seq_printf(m, "%llx\t\t", gbl_hw_evts_groups[k]->mask);
			/* Check bit - Avoid comma*/
			for (i = 0; i < 64; i++) {
				/* Check bit */
				if (gbl_hw_evts_groups[k]->mask & BIT(i)) {
					seq_printf(m, ",%x", gbl_raw_events[i].raw);
				}
			}
			seq_printf(m, "\n");
	}

	return 0;
}

static int sample_info_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, sampling_sample_info_show, NULL);
}

#define MAX_USER_HW_EVENTS 32

static ssize_t sample_info_write(struct file *filp, const char __user *buffer_user,
			    size_t count, loff_t *ppos)
{
	int err, i = 0;
	char *p, *buffer;
	pmc_evt_code codes[MAX_USER_HW_EVENTS];

	buffer = (char *)kzalloc(sizeof(char) * count, GFP_KERNEL);
	err = copy_from_user((void *)buffer, (void *)buffer_user,
			     sizeof(char) * count);

	if (err)
		return err;

	/* Parse hw_events from user input */
	while ((p = strsep(&buffer, ",")) != NULL && i < MAX_USER_HW_EVENTS)
		sscanf(p, "%x", &codes[i++].raw);

	setup_hw_events_from_proc(codes, i);

	// recode_stop_and_reset();
	return count;
}
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,6,0)
struct file_operations sample_info_proc_fops = {
	.open = sample_info_open,
	.read = seq_read,
	.write = sample_info_write,
	.release = single_release,
#else
struct proc_ops sample_info_proc_fops = {
	.proc_open = sample_info_open,
	.proc_read = seq_read,
	.proc_write = sample_info_write,
	.proc_release = single_release,
#endif
};

int register_proc_sample_info(void)
{
	struct proc_dir_entry *dir;

	dir = proc_create(GET_PATH("sample_info"), 0666, NULL, &sample_info_proc_fops);

	return !dir;
}