 #include "proc.h"

/* 
 * Proc and Fops related to Recode state
 */

static ssize_t state_write(struct file *file,
		const char __user *buffer, size_t count, loff_t *ppos)
{
	int err;
	unsigned state;

	err = kstrtouint_from_user(buffer, count, 0, &state);
	if (err) {
		pr_info("Pid buffer err\n");
		goto err;
	}

	recode_set_state(state);

	return count;
err:
	return -1;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,6,0)
struct file_operations state_proc_fops = {
    .write = state_write,
#else
struct proc_ops state_proc_fops = {
    .proc_write = state_write,
#endif
};


int register_proc_state(void)
{
        struct proc_dir_entry *dir;

	dir = proc_create(GET_PATH("state"), 0222, NULL,
		    &state_proc_fops);

        return !dir;
}