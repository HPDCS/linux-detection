
#ifdef FAST_IRQ_ENABLED
#include <asm/fast_irq.h>
#endif
#include <asm/msr.h>

#include <linux/percpu-defs.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/stringhash.h>

#include <asm/tsc.h>

#include "dependencies.h"
#include "device/proc.h"

#include "recode.h"
#include "pmu/pmi.h"
#include "recode_config.h"
#include "recode_collector.h"

/* TODO - create a module */
#ifdef TMA_MODULE_ON
#include "logic/recode_tma.h"
#endif
#ifdef SECURITY_MODULE_ON
#include "logic/recode_security.h"
#endif

bool dummy_on_state_change(enum recode_state state)
{
	return false;
}

void dummy_on_pmi(unsigned cpu, struct pmus_metadata *pmus_metadata)
{
	// Empty call
}

static void dummy_on_ctx(struct task_struct *prev, bool prev_on, bool curr_on)
{
	// Empty call
}

static void dummy_on_hw_events_change(struct hw_events *events)
{
	// Empty call
}

struct recode_callbacks recode_callbacks = {
	.on_hw_events_change = dummy_on_hw_events_change,
	.on_pmi = dummy_on_pmi,
	.on_ctx = dummy_on_ctx,
	.on_state_change = dummy_on_state_change,
};

enum recode_state __read_mostly recode_state = OFF;

DEFINE_PER_CPU(struct data_logger *, pcpu_data_logger);

void pmc_generate_snapshot(struct pmcs_snapshot *old_pmcs, bool pmc_off);

int recode_data_init(void)
{
	unsigned cpu;

	for_each_online_cpu (cpu) {
		per_cpu(pcpu_data_collector, cpu) = init_collector(cpu);
		if (!per_cpu(pcpu_data_collector, cpu))
			goto mem_err;
	}

	return 0;
mem_err:
	pr_info("failed to allocate percpu pcpu_pmc_buffer\n");

	while (--cpu)
		fini_collector(cpu);

	return -1;
}

void recode_data_fini(void)
{
	unsigned cpu;

	for_each_online_cpu (cpu) {
		fini_collector(cpu);
	}
}

int recode_pmc_init(void)
{
	int err = 0;
	/* Setup fast IRQ */

	if (recode_pmi_vector == NMI) {
		err = pmi_nmi_setup();
	} else {
		err = pmi_irq_setup();
	}

	if (err) {
		pr_err("Cannot initialize PMI vector\n");
		goto err;
	}

	/* READ MACHINE CONFIGURATION */
	get_machine_configuration();

	// if (setup_pmc_on_system(pmc_events_management))
	if (init_pmu_on_system())
		goto no_cfgs;

	disable_pmc_on_system();

#ifdef TMA_MODULE_ON
	recode_tma_init();
#endif
#ifdef SECURITY_MODULE_ON
	recode_security_init();
#endif
	on_each_cpu(setup_hw_events_on_cpu, gbl_hw_evts_groups[0], 1);

	return err;
no_cfgs:
	if (recode_pmi_vector == NMI) {
		pmi_nmi_cleanup();
	} else {
		pmi_irq_cleanup();
	}
err:
	return -1;
}

void recode_pmc_fini(void)
{
	/* Disable Recode */
	recode_state = OFF;

	disable_pmc_on_system();

	/* Wait for all PMIs to be completed */
	while (atomic_read(&active_pmis))
		;

	cleanup_pmc_on_system();

#ifdef TMA_MODULE_ON
	recode_tma_fini();
#endif
#ifdef SECURITY_MODULE_ON
	recode_security_init();
#endif

	if (recode_pmi_vector == NMI) {
		pmi_nmi_cleanup();
	} else {
		pmi_irq_cleanup();
	}

	pr_info("PMI uninstalled\n");
}

/* Must be implemented */
static void recode_reset_data(void)
{
	// unsigned cpu;
	// unsigned pmc;

	// for_each_online_cpu (cpu) {

	/* TODO - Check when need to reset pcpu_pmcs_snapshot */
	// for (pmc = 0; pmc <= gbl_nr_pmc_fixed; ++pmc)
	// 	per_cpu(pcpu_pmcs_snapshot, cpu) = 0;

	// if (gbl_fixed_pmc_pmi <= gbl_nr_pmc_fixed)
	// 	per_cpu(pcpu_pmcs_snapshot.fixed[gbl_fixed_pmc_pmi], cpu) =
	// 		PMC_TRIM(reset_period);

	// for (pmc = 0; pmc < gbl_nr_pmc_general; ++pmc)
	// 	per_cpu(pcpu_pmcs_snapshot.general[pmc], cpu) = 0;
	// }
}

static void on_context_switch_callback(struct task_struct *prev, bool prev_on,
				       bool curr_on)
{
	unsigned cpu = get_cpu();
	// struct data_logger_sample sample;

	switch (recode_state) {
	case OFF:
		disable_pmc_on_this_cpu(false);
		goto end;
	case IDLE:
		enable_pmc_on_this_cpu(false);
		break;
	case SYSTEM:
		enable_pmc_on_this_cpu(false);
		prev_on = true;
		break;
	case TUNING:
		fallthrough;
	case PROFILE:
		/* Toggle PMI */
		if (!curr_on)
			disable_pmc_on_this_cpu(false);
		else
			enable_pmc_on_this_cpu(false);
		break;
	default:
		pr_warn("Invalid state on cpu %u... disabling PMCs\n", cpu);
		disable_pmc_on_this_cpu(false);
		goto end;
	}

	if (unlikely(!prev)) {
		goto end;
	}

	recode_callbacks.on_ctx(prev, prev_on, curr_on);

	// per_cpu(pcpu_pmus_metadata.has_ctx_switch, cpu) = true;

	/* TODO - Think aobut enabling or not the sampling in CTX_SWITCH */

	// if (!prev_on) {
	// 	pmc_generate_snapshot(NULL, false);
	// } else {
	// 	pmc_generate_snapshot(&sample.pmcs, true);
	// 	// TODO - Note here pmcs are active, so we are recording
	// 	//        part of the sample collection work
	// 	sample.id = prev->pid;
	// 	sample.tracked = true;
	// 	sample.k_thread = !prev->mm;
	// 	write_log_sample(per_cpu(pcpu_data_logger, cpu), &sample);
	// }

end:
	put_cpu();
}

void pmi_function(unsigned cpu)
{
	if (recode_state == OFF) {
		pr_warn("Recode is OFF - This PMI shouldn't fire, ignoring\n");
		return;
	}

	if (recode_state == IDLE) {
		return;
	}

	atomic_inc(&active_pmis);

	recode_callbacks.on_pmi(cpu, per_cpu_ptr(&pcpu_pmus_metadata, cpu));

	atomic_dec(&active_pmis);
}

static void manage_pmu_state(void *dummy)
{
	/* This is not precise */
	this_cpu_write(pcpu_pmus_metadata.last_tsc, rdtsc_ordered());

	on_context_switch_callback(NULL, false, query_tracker(current));
}

void recode_set_state(unsigned state)
{
	enum recode_state old_state = recode_state;
	recode_state = state;

	if (old_state == recode_state)
		return;

	if (recode_callbacks.on_state_change(state))
		goto skip;

	switch (state) {
	case OFF:
		pr_info("Recode state: OFF\n");
		debug_pmu_state();
		disable_pmc_on_system();
		// TODO - Restore
		// flush_written_samples_on_system();
		return;
	case IDLE:
		pr_info("Recode ready for IDLE\n");
		break;
	case PROFILE:
		pr_info("Recode ready for PROFILE\n");
		break;
	case SYSTEM:
		pr_info("Recode ready for SYSTEM\n");
		break;
	case TUNING:
		pr_info("Recode ready for TUNING\n");
		break;
	default:
		pr_warn("Recode invalid state\n");
		recode_state = old_state;
		return;
	}

skip:
	recode_reset_data();
	on_each_cpu(manage_pmu_state, NULL, 0);

	/* Use the cached value */
	/* TODO - check pmc setup methods */
	// setup_pmc_on_system(NULL);
	// TODO - Make this call clear
}

/* Register thread for profiling activity  */
int attach_process(struct task_struct *tsk)
{
	pr_info("Attaching process: [%u:%u]\n", tsk->tgid, tsk->pid);
	tracker_add(tsk);
	return 0;
}

/* Remove registered thread from profiling activity */
void detach_process(struct task_struct *tsk)
{
	pr_info("Detaching process: [%u:%u]\n", tsk->tgid, tsk->pid);
	tracker_del(tsk);
}

int register_ctx_hook(void)
{
	int err = 0;

	set_hook_callback(&on_context_switch_callback);
	switch_hook_set_state_enable(true);

	pr_info("Registered context_switch_callback\n");

	return err;
}

void unregister_ctx_hook(void)
{
	set_hook_callback(NULL);
}

void setup_hw_events_from_proc(pmc_evt_code *hw_events_codes, unsigned cnt)
{
	if (!hw_events_codes || !cnt) {
		pr_warn("Invalid hw evenyts codes from user\n");
		return;
	}

	setup_hw_events_on_system(hw_events_codes, cnt);
}
