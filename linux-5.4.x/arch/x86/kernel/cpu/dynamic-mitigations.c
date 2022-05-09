#include <asm/cpufeatures.h>
#include <asm/dynamic-mitigations.h>
#include <asm/pgtable.h>
#include <linux/cpuset.h>
#include <linux/dynamic-mitigations.h>
#include <linux/gfp.h>
#include <linux/sched/coredump.h>	/* MMF_HAS_SUSPECTED_MASK */


#undef pr_fmt
#define pr_fmt(fmt)     "** Dyanmic Mitigations **: " fmt

DEFINE_PER_CPU(u32, pcpu_dynamic_mitigations) = 0;
EXPORT_SYMBOL(pcpu_dynamic_mitigations);

/* System configutration for dynamic mitigations */
static u32 gbl_dynamic_mitigations = 0;

void inline enable_mitigations_on_system(unsigned mitigation)
{
	gbl_dynamic_mitigations |= _BITUL(mitigation);
}
EXPORT_SYMBOL(enable_mitigations_on_system);

void inline disable_mitigations_on_system(unsigned mitigation)
{
	gbl_dynamic_mitigations &= ~_BITUL(mitigation);
}
EXPORT_SYMBOL(disable_mitigations_on_system);

bool inline has_mitigations(struct task_struct *tsk)
{
	return tsk && tsk->mm && (tsk->mm->flags & MMF_DM_ACTIVE_MASK);
}
EXPORT_SYMBOL(has_mitigations);

bool inline has_pending_mitigations(struct task_struct *tsk)
{
	return tsk && tsk->mm && !(tsk->mm->flags & MMF_DM_ACTIVE_MASK) &&
		(tsk->mm->flags & MMF_DM_PENDING_MASK);
}
EXPORT_SYMBOL(has_pending_mitigations);

void request_mitigations_on_task(struct task_struct *tsk, bool delayed)
{
	if (has_mitigations(tsk)) {
		pr_warn("Request - PID %u has mitigations enabled\n",
			tsk->pid);
		return;
	}
	if(has_pending_mitigations(tsk)) {
		pr_warn("Request - PID %u has pending mitigations\n",
			tsk->pid);
		return;
	}

	if (!tsk || !tsk->mm)
		return;
	
	tsk->mm->flags |= MMF_DM_PENDING_MASK;

	if (delayed) {
		pr_warn("Request - PID %u set delayed request (f: %lx)\n",
			tsk->pid, tsk->mm->flags);
		set_tsk_need_resched(tsk);
	}
	else {
		pr_warn("Request - PID %u processing request\n",
			tsk->pid);
		enable_mitigations_on_task(tsk);
	}
}
EXPORT_SYMBOL(request_mitigations_on_task);

/* Mark the task as suspected and activate the mitigations if any */
void enable_mitigations_on_task(struct task_struct *tsk)
{	
	unsigned cpu;
	
	BUG_ON(!tsk);

	cpu = get_cpu();

	if (!tsk->mm)
		goto exit;

	if (tsk->mm->flags & MMF_DM_ACTIVE_MASK) {
		pr_warn("[%u] %s: Dynamic mitigations already enabled\n",
		tsk->pid, tsk->comm);
		goto exit;
	}

	if (!(tsk->mm->flags & MMF_DM_PENDING_MASK)) {
		pr_warn("[%u] %s: Dynamic mitigations not pending\n",
		tsk->pid, tsk->comm);
		goto exit;
	}

	init_te_mitigations_on_task(tsk);
	exile_task_from_physical_cpu(tsk, cpu);
		
	/* Finalize initialization */
	tsk->mm->flags |= MMF_DM_ACTIVE_MASK;
	tsk->mm->flags &= ~MMF_DM_PENDING_MASK;	

	pr_warn("[%u] %s: Dynamic mitigations enabled\n",
		tsk->pid, tsk->comm);

exit:
	put_cpu();
}
EXPORT_SYMBOL(enable_mitigations_on_task);

/* Mark the task as suspected and activate the mitigations if any */
void disable_mitigations_on_task(struct task_struct *tsk)
{	
	BUG_ON(!tsk);

	if (!tsk->mm)
		return;

	if (!has_mitigations(tsk)) {
		pr_warn("[%u] %s: Dynamic mitigations is not enabled\n",
		tsk->pid, tsk->comm);
		return;
	}

	tsk->mm->flags &= ~MMF_DM_ACTIVE_MASK;
	pr_warn("[%u] %s: Dynamic mitigations disabled\n",
		tsk->pid, tsk->comm);
}
EXPORT_SYMBOL(disable_mitigations_on_task);

static void set_ssb_mitigation(bool enable)
{
	u64 msr;

	if (!boot_cpu_has(X86_FEATURE_MSR_SPEC_CTRL))
		return;

	rdmsrl(MSR_IA32_SPEC_CTRL, msr);

	if (enable) {
		msr |= SPEC_CTRL_SSBD;
	} else {
		msr &= ~SPEC_CTRL_SSBD;
	}
	wrmsrl(MSR_IA32_SPEC_CTRL, msr);
}

static void set_ibrs_mitigation(bool enable)
{
	u64 msr;

	if (!boot_cpu_has(X86_FEATURE_MSR_SPEC_CTRL) ||
	    !boot_cpu_has(X86_FEATURE_IBRS_ENHANCED))
		return;

	rdmsrl(MSR_IA32_SPEC_CTRL, msr);

	if (enable) {
		msr |= SPEC_CTRL_IBRS;
	} else {
		msr &= ~SPEC_CTRL_IBRS;
	}
	wrmsrl(MSR_IA32_SPEC_CTRL, msr);
}

static void set_bug_patches(bool enable)
{
	unsigned flags;

	/* Store Bypass mitigation */
	set_ssb_mitigation(enable);

	set_ibrs_mitigation(enable);

	flags = DM_RETPOLINE_SHIFT | DM_RSB_CTXSW_SHIFT |
	    DM_USE_IBPB_SHIFT | DM_COND_STIBP_SHIFT | DM_COND_IBPB_SHIFT |
	    DM_ALWAYS_IBPB_SHIFT | DM_MDS_CLEAR_SHIFT | DM_USE_IBRS_FW |
	    DM_ENABLED;

	/* Enable dynamic mitigations */
	if (enable)
		this_cpu_or(pcpu_dynamic_mitigations, flags);
	else
		this_cpu_and(pcpu_dynamic_mitigations, (~flags));

	if (gbl_dynamic_mitigations & DM_G_VERBOSE_SHIFT)
		pr_info ("PID %u: TE Mitigations %s\n", current->pid, 
			enable ? "ENABLED" : "DISABLED");
}

void enable_pti_on_mm(struct mm_struct *mm)
{
        unsigned restored = 0;

        /* Kernel View PGD */
	pgd_t *k_pgdp = mm->pgd;

	if (mm->flags & MMF_PTI_ENABLED_MASK) {
		pr_warn("Attempting to enable PTI on already patched mm: %lx\n",
		mm->flags);
		return;
	}

	if (mm->flags & MMF_PTI_DISABLING_MASK) {
		pr_warn("Attempting to enable PTI while disabling PTI: %lx\n",
		mm->flags);
		return;
	}

        /* Set the flag to be checked at kernel mode exit  */
        WRITE_ONCE(mm->flags, READ_ONCE(mm->flags) | MMF_PTI_ENABLED_MASK);
	
        /* 
         * Until now, all threads with this mm worked on Kernel View.
         * Force the reschedule on all other cores to ensure that threads with
         * this mm will load the User View during user mode execution. 
         */
        kick_all_cpus_sync();

        /* 
         * We may block the reschedule of interested threads or even the entire
         * execution to wait for the Kernel View is fixed. However, we consider
         * this time slot as a grace period. Of course, an attack may still be
         * perpetrated in this small time window. 
         */

	/* Scan for all present kernel pgd entries and set the NX bit */
	while (pgdp_maps_userspace(k_pgdp)) {
	       	

		if ((k_pgdp->pgd & (_PAGE_USER | _PAGE_PRESENT)) ==
		    (_PAGE_USER | _PAGE_PRESENT) &&
		    (__supported_pte_mask & _PAGE_NX)) {
		    	k_pgdp->pgd |= _PAGE_NX;
                        restored++;
                    }

		/* k_pgdp can be seen as an array of pgd_t entries */
		++k_pgdp;
	}

        pr_info("Restored %u PGPD entries\n", restored);
}
EXPORT_SYMBOL(enable_pti_on_mm);

void disable_pti_on_mm(struct mm_struct *mm)
{
        unsigned set = 0;
         
        /* Kernel View PGD */
	pgd_t *k_pgdp = mm->pgd;

        /* 
         * We don't need any sync mechanism because every scheduled thread will
         * keep running with the already selected Page Table View. Once the
         * Kernel View is ready, we can clear the flag in the mm and allow using
         * the Kernel View during user mode execution. However, it may happens 
         * that while restoring the Kernel View, some interested thread may
         * allocate a new PGP entry with NX bit set. This may cause a fault at
         * runtime after the clearing the flag bit. We use an extra flag to
         * avoid the NX bit setting during this phase.
         */

        mm->flags |= MMF_PTI_DISABLING_MASK;

	/* Scan for all present kernel pgd entries and clear the NX bit */
	while (pgdp_maps_userspace(k_pgdp)) {

		if ((k_pgdp->pgd & (_PAGE_USER | _PAGE_PRESENT)) ==
		    (_PAGE_USER | _PAGE_PRESENT) &&
		    (__supported_pte_mask & _PAGE_NX)) {
		    	k_pgdp->pgd &= ~_PAGE_NX;
                        set++;
                    }
		++k_pgdp;
	}
       
        /* Clear the flag to be checked at kernel mode exit */
        mm->flags &= ~(MMF_PTI_ENABLED_MASK & MMF_PTI_DISABLING_MASK);
	smp_mb();

        /* End of PTI OFF transition */
        pr_info("Set %u PGPD entries\n", set);
}
EXPORT_SYMBOL(disable_pti_on_mm);


void mitigations_switch(struct task_struct *prev, struct task_struct *curr)
{
	if ((this_cpu_read_stable(pcpu_dynamic_mitigations) & DM_ENABLED) &&
	    (!(gbl_dynamic_mitigations & DM_G_TE_MITIGATE_SHIFT) ||
	    (!curr->mm || !(curr->mm->flags & MMF_PTI_ENABLED_MASK))))
	 	set_bug_patches(false);
	else if (curr->mm && (curr->mm->flags & MMF_PTI_ENABLED_MASK) &&
	    !(this_cpu_read_stable(pcpu_dynamic_mitigations) & DM_ENABLED))
		set_bug_patches(true);
}
EXPORT_SYMBOL(mitigations_switch);

void init_te_mitigations_on_task(struct task_struct *tsk)
{
	if (!(gbl_dynamic_mitigations & DM_G_TE_MITIGATE_SHIFT))
		return;
	
	if (!tsk->mm)
		pr_warn("Cannot enable PTI on Kernel Thread %u\n", tsk->pid);
	
	enable_pti_on_mm(tsk->mm);
}
EXPORT_SYMBOL(init_te_mitigations_on_task);

void fini_te_mitigations_on_task(struct task_struct *tsk)
{
	if (gbl_dynamic_mitigations & DM_G_TE_MITIGATE_SHIFT)
		return;
		
	if (!tsk->mm)
		disable_pti_on_mm(tsk->mm);
}
EXPORT_SYMBOL(fini_te_mitigations_on_task);

void inline LLC_flush(struct task_struct *tsk)
{
	if (has_mitigations(tsk) && 
	    gbl_dynamic_mitigations & DM_G_LLC_FLUSH_SHIFT) {
		if (gbl_dynamic_mitigations & DM_G_VERBOSE_SHIFT)
			pr_info ("PID %u: flushing LLC\n", tsk->pid);
		asm volatile ("wbinvd");
	    }
}
EXPORT_SYMBOL(LLC_flush);

void exile_task_from_physical_cpu(struct task_struct *tsk, unsigned no_cpu)
{
	unsigned int cpu;
	cpumask_var_t cpu_mask;

	if (!(gbl_dynamic_mitigations & DM_G_CPU_EXILE_SHIFT) ||
	    (tsk->monitor_state & DM_SCM_CPU_EXILED))
		return;

	if (!alloc_cpumask_var(&cpu_mask, GFP_KERNEL))
		return;

	pr_info ("PID %u: try to migrate wasy %u CPU\n", tsk->pid, no_cpu);

        // Retrieve available cpus for tsk
	cpuset_cpus_allowed(tsk, cpu_mask);
	
	// Disable current cores and siblings
        for_each_cpu(cpu, topology_sibling_cpumask(no_cpu)) {
		if (cpu == no_cpu)
			pr_info("CPU %u is DISABLED for PID %u\n", cpu, tsk->pid);
		
		pr_info("CPU %u (SIBLING of CPU %u) is DISABLED for PID %u\n",
			no_cpu, cpu, tsk->pid);
		cpumask_clear_cpu(cpu, cpu_mask);
	}

	if(sched_setaffinity(tsk->pid, cpu_mask)) {
		pr_warn("[PID %u : %s] Cannot set affinity\n",
			tsk->pid, tsk->comm);
		goto err_sched;
	}

        tsk->monitor_state |= DM_SCM_CPU_EXILED;
	pr_warn("[PID %u : %s] scheduled away\n", tsk->pid, tsk->comm);
err_sched:
	free_cpumask_var(cpu_mask);
}
EXPORT_SYMBOL(exile_task_from_physical_cpu);