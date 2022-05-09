#include "proc.h"
#include "logic/recode_security.h"

static int thresholds_show(struct seq_file *m, void *v)
{
	unsigned i;

	seq_printf(m, "FR:\t");
	for (i = 0; i < sc_ths_fr.nr; i++) {
		seq_printf(m, "%llu\t", sc_ths_fr.ths[i]);
	}
	seq_printf(m, "\n");
	
	seq_printf(m, "XL:\t");
	for (i = 0; i < sc_ths_xl.nr; i++) {
		seq_printf(m, "%llu\t", sc_ths_xl.ths[i]);
	}
	seq_printf(m, "\n");

	return 0;
}

static int thresholds_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, thresholds_show, NULL);
}

struct file_operations thresholds_proc_fops = {
	.open = thresholds_open,
	.read = seq_read,
	.release = single_release,
};

int register_proc_thresholds(void)
{
	struct proc_dir_entry *dir;

	dir = proc_create(GET_PATH("thresholds"), 0444, NULL,
			  &thresholds_proc_fops);

	return !dir;
}