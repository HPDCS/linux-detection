#include <asm/apic.h>
#include <asm/perf_event.h>
#include <linux/sched.h>
#include <linux/slab.h>

#include "recode.h"
#include "recode_config.h"
#include "pmu/pmu.h"
#include "pmu/pmi.h"
#include "pmu/hw_events.h"

DEFINE_PER_CPU(struct pmus_metadata, pcpu_pmus_metadata) = { 0 };

unsigned __read_mostly gbl_nr_pmc_fixed = 0;
unsigned __read_mostly gbl_nr_pmc_general = 0;

unsigned __read_mostly gbl_nr_hw_evts_groups = 0;
struct hw_events *__read_mostly gbl_hw_evts_groups[MAX_GBL_HW_EVTS] = { 0 };

void get_machine_configuration(void)
{
	union cpuid10_edx edx;
	union cpuid10_eax eax;
	union cpuid10_ebx ebx;
	unsigned int unused;
	unsigned version;

	cpuid(10, &eax.full, &ebx.full, &unused, &edx.full);

	if (eax.split.mask_length < 7)
		return;

	version = eax.split.version_id;
	gbl_nr_pmc_general = eax.split.num_counters;
	gbl_nr_pmc_fixed = edx.split.num_counters_fixed;

	pr_info("PMU Version: %u\n", version);
	pr_info(" - NR Counters: %u\n", eax.split.num_counters);
	pr_info(" - Counter's Bits: %u\n", eax.split.bit_width);
	pr_info(" - Counter's Mask: %llx\n", (1ULL << eax.split.bit_width) - 1);
	pr_info(" - NR PEBS' events: %x\n",
		min_t(unsigned, 8, eax.split.num_counters));
}

void fast_setup_general_pmc_on_cpu(unsigned cpu, struct pmc_evt_sel *pmc_cfgs,
				   unsigned off, unsigned cnt)
{
	u64 ctrl;
	unsigned pmc;

	/* Avoid to write wrong MSRs */
	if (cnt > gbl_nr_pmc_general)
		cnt = gbl_nr_pmc_general;

	ctrl = FIXED_TO_BITS_MASK | (BIT_ULL(cnt) - 1);

	/* Uneeded PMCs are disabled in ctrl */
	for_each_active_general_pmc (ctrl, pmc) {
		// pr_debug("pmc_num %u, pmc_cfgs: %llx\n", pmc, pmc_cfgs[pmc + off].perf_evt_sel);
		SETUP_GENERAL_PMC(pmc, pmc_cfgs[pmc + off].perf_evt_sel);
		WRITE_GENERAL_PMC(pmc, 0ULL);
	}

	per_cpu(pcpu_pmus_metadata.perf_global_ctrl, cpu) = ctrl;
}

static void __enable_pmc_on_cpu(void *dummy)
{
	if (recode_state == OFF) {
		pr_warn("Cannot enable pmc on cpu %u while Recode is OFF\n",
			smp_processor_id());
		return;
	}
	this_cpu_write(pcpu_pmus_metadata.pmcs_active, true);
#ifndef CONFIG_RUNNING_ON_VM
	wrmsrl(MSR_CORE_PERF_GLOBAL_CTRL,
	       this_cpu_read(pcpu_pmus_metadata.perf_global_ctrl));
	pr_debug("enabled pmcs on cpu %u - gbl_ctrl: %llx\n",
		 smp_processor_id(),
		 this_cpu_read(pcpu_pmus_metadata.perf_global_ctrl));
#endif
}

static void __disable_pmc_on_cpu(void *dummy)
{
	this_cpu_write(pcpu_pmus_metadata.pmcs_active, false);
#ifndef CONFIG_RUNNING_ON_VM
	wrmsrl(MSR_CORE_PERF_GLOBAL_CTRL, 0ULL);
	pr_debug("disabled pmcs on cpu %u\n", smp_processor_id());
#endif
}

void inline __attribute__((always_inline)) enable_pmc_on_this_cpu(bool force)
{
	if (force || !this_cpu_read(pcpu_pmus_metadata.pmcs_active))
		__enable_pmc_on_cpu(NULL);
}

void inline __attribute__((always_inline)) disable_pmc_on_this_cpu(bool force)
{
	if (force || this_cpu_read(pcpu_pmus_metadata.pmcs_active))
		__disable_pmc_on_cpu(NULL);
}

// static void __restore_and_enable_pmc_on_this_cpu(bool *state)
// {
// 	if (*state)
// 		enable_pmc_on_this_cpu(true);
// }

// static void __save_and_disable_pmc_on_this_cpu(bool *state)
// {
// 	*state = this_cpu_read(pcpu_pmus_metadata.pmcs_active);
// 	disable_pmc_on_this_cpu(true);
// }

void inline __attribute__((always_inline)) enable_pmc_on_system(void)
{
	on_each_cpu(__enable_pmc_on_cpu, NULL, 1);
}

void inline __attribute__((always_inline)) disable_pmc_on_system(void)
{
	on_each_cpu(__disable_pmc_on_cpu, NULL, 1);
}

void debug_pmu_state(void)
{
	u64 msr;
	unsigned pmc;
	unsigned cpu = get_cpu();

	pr_debug("Init PMU debug on core %u\n", cpu);

	rdmsrl(MSR_CORE_PERF_GLOBAL_STATUS, msr);
	pr_debug("MSR_CORE_PERF_GLOBAL_STATUS: %llx\n", msr);

	rdmsrl(MSR_CORE_PERF_FIXED_CTR_CTRL, msr);
	pr_debug("MSR_CORE_PERF_FIXED_CTR_CTRL: %llx\n", msr);

	rdmsrl(MSR_CORE_PERF_GLOBAL_CTRL, msr);
	pr_debug("MSR_CORE_PERF_GLOBAL_CTRL: %llx\n", msr);

	pr_debug("GP_sel 0: %llx\n", QUERY_GENERAL_PMC(0));
	pr_debug("GP_sel 1: %llx\n", QUERY_GENERAL_PMC(1));
	pr_debug("GP_sel 2: %llx\n", QUERY_GENERAL_PMC(2));
	pr_debug("GP_sel 3: %llx\n", QUERY_GENERAL_PMC(3));

	for_each_fixed_pmc (pmc) {
		pr_debug("FX_ctrl %u: %llx\n", pmc, READ_FIXED_PMC(pmc));
	}

	for_each_general_pmc (pmc) {
		pr_debug("GP_ctrl %u: %llx\n", pmc, READ_GENERAL_PMC(pmc));
	}

	pr_debug("Fini PMU debug on core %u\n", cpu);

	put_cpu();
}

static void __init_pmu_on_cpu(void *hw_pmcs)
{
#ifndef CONFIG_RUNNING_ON_VM
	u64 msr;
	unsigned pmc;

	/* Refresh APIC entry */
	if (recode_pmi_vector == NMI)
		apic_write(APIC_LVTPC, LVT_NMI);
	else
		apic_write(APIC_LVTPC, RECODE_PMI);

	/* Clear a possible stale state */
	rdmsrl(MSR_CORE_PERF_GLOBAL_STATUS, msr);
	wrmsrl(MSR_CORE_PERF_GLOBAL_OVF_CTRL, msr);

	/* Enable FREEZE_ON_PMI */
	wrmsrl(MSR_IA32_DEBUGCTLMSR, BIT(12));

	for_each_fixed_pmc (pmc) {
		if (pmc == gbl_fixed_pmc_pmi) {
			WRITE_FIXED_PMC(pmc, PMC_TRIM(~gbl_reset_period));
		} else {
			WRITE_FIXED_PMC(pmc, 0ULL);
		}
	}

	/* Setup FIXED PMCs */
	wrmsrl(MSR_CORE_PERF_FIXED_CTR_CTRL,
	       this_cpu_read(pcpu_pmus_metadata.fixed_ctrl));

	/* Assign the memory for the fixed PMCs snapshot */
	this_cpu_write(pcpu_pmus_metadata.hw_pmcs,
		       hw_pmcs + (smp_processor_id() *
				  (gbl_nr_pmc_fixed + gbl_nr_pmc_general)));

	/* Assign here the memory for the per-cpu pmc-collection */
	this_cpu_write(
		pcpu_pmus_metadata.pmcs_collection,
		kzalloc(sizeof(struct pmcs_collection) +
				(sizeof(pmc_ctr) * MAX_PARALLEL_HW_EVENTS),
			GFP_KERNEL));

#else
	pr_warn("CONFIG_RUNNING_ON_VM is enabled. PMCs are ignored\n");
#endif
}

int init_pmu_on_system(void)
{
	unsigned cpu, pmc;
	u64 gbl_fixed_ctrl = 0;
	pmc_ctr *hw_pmcs;
	/* Compute fixed_ctrl */

	pr_debug("num_possible_cpus: %u\n", num_possible_cpus());

	/* TODO Free this memory */
	hw_pmcs = kzalloc(sizeof(pmc_ctr) * num_possible_cpus() *
				  (gbl_nr_pmc_fixed + gbl_nr_pmc_general),
			  GFP_KERNEL);
	if (!hw_pmcs) {
		pr_warn("Cannot allocate memory in init_pmu_on_system\n");
		return -ENOMEM;
	}

	for_each_fixed_pmc (pmc) {
		/**
		 * bits: 3   2   1   0
		 * 	PMI, 0, USR, OS
		 */
		if (pmc == gbl_fixed_pmc_pmi) {
			/* Set PMI */
			gbl_fixed_ctrl |= (BIT(3) << (pmc * 4));
		}
		if (params_cpl_usr)
			gbl_fixed_ctrl |= (BIT(1) << (pmc * 4));
		if (params_cpl_os)
			gbl_fixed_ctrl |= (BIT(0) << (pmc * 4));
	}

	for_each_online_cpu (cpu) {
		per_cpu(pcpu_pmus_metadata.fixed_ctrl, cpu) = gbl_fixed_ctrl;
	}

	/* Metadata doesn't require initialization at the moment */
	on_each_cpu(__init_pmu_on_cpu, hw_pmcs, 1);

	pr_info("PMI set on fixed pmc %u\n", gbl_fixed_pmc_pmi);
	pr_warn("PMUs initialized on all cores\n");
	return 0;
}

void cleanup_pmc_on_system(void)
{
	/* TODO - To be implemented */
	on_each_cpu(__disable_pmc_on_cpu, NULL, 1);

	/* TODO - Check if we need a delay */
	/* TODO - pcpu_metadata keeps a NULL ref */
	while (gbl_nr_hw_evts_groups) {
		kfree(gbl_hw_evts_groups[gbl_nr_hw_evts_groups--]);
	}
}

void update_reset_period_on_system(u64 reset_period)
{
	unsigned cpu;
	struct hw_events *hw_events;

	disable_pmc_on_system();

	gbl_reset_period = reset_period;

	for_each_online_cpu (cpu) {
		hw_events = per_cpu(pcpu_pmus_metadata.hw_events, cpu);
		if (hw_events)
			smp_call_function_single(cpu, setup_hw_events_on_cpu,
						 hw_events, 1);
	}

	pr_info("Updated gbl_reset_period to %llx\n", reset_period);

	// TODO create a global state
	if (recode_state != OFF)
		enable_pmc_on_system();
}
