#include <asm/tsc.h>
#include <linux/vmalloc.h>

#include "proc.h"
#include "recode_config.h"
#include "recode_collector.h"

extern unsigned int tsc_khz;

/* 
 * Proc and Fops related to CPU device
 */

static void *cpu_logger_seq_start(struct seq_file *m, loff_t *pos)
{
	// unsigned pmc;
	unsigned *cpu = (unsigned *) PDE_DATA(file_inode(m->file));
	struct data_collector *dc = per_cpu(pcpu_data_collector, *cpu);
	struct hw_events *hw_events;

	if (!dc)
		goto no_data;
	
	if (!check_read_dc_sample(dc))
		goto no_data;

	hw_events = per_cpu(pcpu_pmus_metadata.hw_events, *cpu);
	if (!hw_events)
		goto no_data;

	return pos;

no_data:
	pr_warn("Cannot read data\n");
	return NULL;
}

static void *cpu_logger_seq_next(struct seq_file *m, void *v, loff_t *pos)
{
	unsigned *cpu = (unsigned *) PDE_DATA(file_inode(m->file));
	struct data_collector *dc = per_cpu(pcpu_data_collector, *cpu);
	
	(*pos)++;

	if (!check_read_dc_sample(dc))
		goto err;

	return pos;
err:
	return NULL;
}

static int cpu_logger_seq_show(struct seq_file *m, void *v)
{
	u64 time;
	unsigned pmc;
	struct data_collector_sample *dc_sample;
	
	unsigned *cpu = (unsigned *) PDE_DATA(file_inode(m->file));
	struct data_collector *dc = per_cpu(pcpu_data_collector, *cpu);

	if (!v || !dc)
		goto err;
		
	dc_sample = get_read_dc_sample(dc);

	if (!dc_sample)
		goto err;

	// pmcs = &sample->pmcs;
	// time = pmcs->tsc / tsc_khz;
	time = 0;

	// seq_printf(m, " %u |", dc_sample->id);
	// seq_printf(m, " %u ", dc_sample->tracked);
	// seq_printf(m, "- %u |", dc_sample->k_thread);
	// seq_printf(m, " %llu |", dc_sample->system_tsc);
	
	// seq_printf(m, " %llu ", dc_sample->tsc_cycles);
	// seq_printf(m, " %llu ", dc_sample->core_cycles);
	// seq_printf(m, "- %llu |", dc_sample->core_cycles_tsc_ref);

	// // seq_printf(m, "- %u |", dc_sample->ctx_evts);

	// seq_printf(m, " [%llx] - ", dc_sample->pmcs.mask);

	seq_printf(m, "%u,", dc_sample->id);
	seq_printf(m, "%u,", dc_sample->tracked);
	seq_printf(m, "%u,", dc_sample->k_thread);
	seq_printf(m, "%llu,", dc_sample->system_tsc);
	
	seq_printf(m, "%llu,", dc_sample->tsc_cycles);
	seq_printf(m, "%llu,", dc_sample->core_cycles);
	seq_printf(m, "%llu,", dc_sample->core_cycles_tsc_ref);

	// seq_printf(m, "- %u |", dc_sample->ctx_evts);

	seq_printf(m, "%llx", dc_sample->pmcs.mask);

	for_each_pmc(pmc, dc_sample->pmcs.cnt)
		seq_printf(m, ",%llu", dc_sample->pmcs.pmcs[pmc]);

	put_read_dc_sample(dc);

	seq_printf(m, "\n");
	
	return 0;
err:
	return -1;
}

static void cpu_logger_seq_stop(struct seq_file *m, void *v)
{
	/* Nothing to free */
}

static struct seq_operations cpu_logger_seq_ops = {
	.start = cpu_logger_seq_start,
	.next = cpu_logger_seq_next,
	.stop = cpu_logger_seq_stop,
	.show = cpu_logger_seq_show
};

static int cpu_logger_open(struct inode *inode, struct file *filp)
{
	return seq_open(filp, &cpu_logger_seq_ops);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,6,0)
struct file_operations cpu_logger_proc_fops = {
	.open = cpu_logger_open,
	.llseek = seq_lseek,
	.read = seq_read,
	.release = seq_release,
#else
struct proc_ops cpu_logger_proc_fops = {
	.proc_open = cpu_logger_open,
	.proc_lseek = seq_lseek,
	.proc_read = seq_read,
	.proc_release = seq_release,
#endif
};

int register_proc_cpus(void)
{
	unsigned cpu;
	unsigned *cpus;
	char name[17];
	struct proc_dir_entry *dir;
	struct proc_dir_entry *tmp_dir;

	dir = proc_mkdir(GET_PATH("cpus"), NULL);

	cpus = vmalloc(sizeof(unsigned) * nr_cpu_ids);
	if (!cpus) {
		pr_warn("Cannot allocate memory\n");
		return -ENOMEM;
	}

	for_each_online_cpu (cpu) {
		cpus[cpu] = cpu;
		sprintf(name, "cpu%u", cpu);

		/* memory leak when releasing */
		tmp_dir =
			proc_create_data(name, 0444, dir, &cpu_logger_proc_fops,
					 &cpus[cpu]);

		// TODO Add cleanup code
		if (!tmp_dir)
			return -1;
	}

	return 0;
}
