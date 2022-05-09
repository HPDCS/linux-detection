#include "proc.h"

#include "logic/recode_security.h"
#include <linux/dynamic-mitigations.h>


static ssize_t suspect_write(struct file *file,
		const char __user *buffer, size_t count, loff_t *ppos)
{
	int ret;
	struct task_struct *ts;
	pid_t pidp;

	if (kstrtoint_from_user(buffer, count, 0, &pidp)) {
		ret = -EFAULT;
		goto skip;
	}

        /* Retrieve pid task_struct */
	ts = get_pid_task(find_get_pid(pidp), PIDTYPE_PID);
	if (!ts) {
		pr_info("Cannot find task_struct for pid %u\n", pidp);
		ret = -EINVAL;
		goto skip;
	}

        request_mitigations_on_task(ts, false);

	put_task_struct(ts);

	ret = count;

skip:
	return ret;
}


static int parameters_seq_show(struct seq_file *m, void *v)
{
	unsigned *param = (unsigned *) PDE_DATA(file_inode(m->file));
	seq_printf(m, "%u\n", *param);

	return 0;
}

static int parameters_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, parameters_seq_show, NULL);
}

static ssize_t parameters_write(struct file *file,
		const char __user *buffer, size_t count, loff_t *ppos)
{
	int err;
	unsigned value;
	unsigned *param = (unsigned *) PDE_DATA(file_inode(file));


	err = kstrtouint_from_user(buffer, count, 0, &value);
	if (err) {
		pr_err("Cannot read from user buffer\n");
		goto err;
	}

	*param = value;

	return count;
err:
	return -1;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,6,0)
struct file_operations parameters_proc_fops = {
	.open = parameters_open,
	.read = seq_read,
	.write = parameters_write,
	.release = single_release
#else
struct proc_ops parameters_proc_fops = {
	.proc_open = parameters_open,
	.proc_read = seq_read,
	.proc_write = parameters_write,
	.proc_release = single_release
#endif
};


static int tuning_seq_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%u\n", tuning_type);
	return 0;
}

static int tuning_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, tuning_seq_show, NULL);
}

static ssize_t tuning_write(struct file *file,
		const char __user *buffer, size_t count, loff_t *ppos)
{
	int err;
	enum tuning_type state;

	err = kstrtouint_from_user(buffer, count, 0, &state);
	if (err) {
		pr_err("Cannot read from user buffer\n");
		goto err;
	}

	switch (state) {
	case FR:
		tuning_type = FR;
		pr_warn("Tuning type set to FR (%u)\n", tuning_type);
		break;
	case XL:
		tuning_type = XL;
		pr_warn("Tuning type set to XL (%u)\n", tuning_type);
		break;
	default:
		pr_warn("Cannot set tuning state, invalid value: %u\n", state);
	}

	return count;
err:
	return -1;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,6,0)
struct file_operations tuning_proc_fops = {
	.open = tuning_open,
	.read = seq_read,
	.write = tuning_write,
	.release = single_release
#else
struct proc_ops tuning_proc_fops = {
	.proc_open = tuning_open,
	.proc_read = seq_read,
	.proc_write = tuning_write,
	.proc_release = single_release
#endif
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,6,0)
struct file_operations suspect_proc_fops = {
    .write = suspect_write,
#else
struct proc_ops suspect_proc_fops = {
    .proc_write = suspect_write,
#endif
};


int register_proc_security(void)
{
	struct proc_dir_entry *dir;
	struct proc_dir_entry *tmp_dir;

	dir = proc_mkdir(GET_PATH("security"), NULL);

	tmp_dir = proc_create_data("tuning", 0666, dir, &tuning_proc_fops, NULL);

	tmp_dir = proc_create_data("alpha", 0666, dir, &parameters_proc_fops, &ts_alpha);
	tmp_dir = proc_create_data("beta", 0666, dir, &parameters_proc_fops, &ts_beta);
	tmp_dir = proc_create_data("gamma", 0666, dir, &parameters_proc_fops, &ts_window);
	tmp_dir = proc_create_data("signal", 0666, dir, &parameters_proc_fops, &signal_detected);

	tmp_dir = proc_create_data("suspect", 0222, dir, &suspect_proc_fops, NULL);

	if (!tmp_dir)
		return -1;

	return 0;
}