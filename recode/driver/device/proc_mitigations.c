#include "proc.h"

#include <linux/dynamic-mitigations.h>

static int mitigations_seq_show(struct seq_file *m, void *v)
{
	seq_printf(m, "\n");

	return 0;
}

static int mitigations_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, mitigations_seq_show, NULL);
}

static ssize_t mitigations_write(struct file *file,
		const char __user *buffer, size_t count, loff_t *ppos)
{
	int err;
	unsigned long *maskp;

	maskp = vmalloc(sizeof(unsigned long));
	if (!maskp) goto err;

	err = kstrtoul_from_user(buffer, count, 0, maskp);
	if (err) {
		pr_info("Mask buffer err\n");
		goto err;
	}

	if (*maskp & DM_G_LLC_FLUSH_SHIFT)
		enable_mitigations_on_system(DM_G_LLC_FLUSH);
	else
		disable_mitigations_on_system(DM_G_LLC_FLUSH);


	if (*maskp & DM_G_TE_MITIGATE_SHIFT)
		enable_mitigations_on_system(DM_G_TE_MITIGATE);
	else
		disable_mitigations_on_system(DM_G_TE_MITIGATE);


	if (*maskp & DM_G_CPU_EXILE_SHIFT)
		enable_mitigations_on_system(DM_G_CPU_EXILE);
	else
		disable_mitigations_on_system(DM_G_CPU_EXILE);


	if (*maskp & DM_G_VERBOSE_SHIFT)
		enable_mitigations_on_system(DM_G_VERBOSE);
	else
		disable_mitigations_on_system(DM_G_VERBOSE);


	pr_info("Mitigations configured: %lx\n", *maskp);
	return count;

err:
	return -1;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,6,0)
struct file_operations mitigations_proc_fops = {
	.open = mitigations_open,
	.read = seq_read,
	.write = mitigations_write,
	.release = single_release
#else
struct proc_ops mitigations_proc_fops = {
	.proc_open = mitigations_open,
	.proc_read = seq_read,
	.proc_write = mitigations_write,
	.proc_release = single_release
#endif
};

int register_proc_mitigations(void)
{
	struct proc_dir_entry *dir;

	dir = proc_create(GET_PATH("mitigations"), 0666, NULL,
			  &mitigations_proc_fops);

	return !dir;
}