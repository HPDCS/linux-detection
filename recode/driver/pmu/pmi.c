#include <linux/percpu-defs.h>
#include <linux/smp.h>
#include <asm/apic.h>
#include <asm/nmi.h>

#include "recode.h"
#include "pmu/pmi.h"
#include "pmu/pmu.h"
#include "pmu/multiplexer.h"
#include "recode_config.h"

/* TODO - place inside metadata */
static DEFINE_PER_CPU(u32, pcpu_lvt_bkp);
static struct nmiaction handler_na;

atomic_t active_pmis = ATOMIC_INIT(0);

/*
 * Performance Monitor Interrupt handler
 */
static int pmi_handler(unsigned int cmd, struct pt_regs *regs)
{
	u64 global;
	unsigned handled = 0;
	// unsigned cpu = get_cpu();
	unsigned cpu = smp_processor_id();

	/* Read the PMCs state */
	rdmsrl(MSR_CORE_PERF_GLOBAL_STATUS, global);

	/* Nothing to do here */
	if (!global) {
		pr_info("[%u] Got PMI on vector %u - FIXED: %llx\n", cpu,
			gbl_fixed_pmc_pmi,
			(u64)READ_FIXED_PMC(gbl_fixed_pmc_pmi));
		goto end;
	}

	/* This IRQ is not originated from PMC overflow */
	if (!(global & (PERF_GLOBAL_CTRL_FIXED0_MASK << gbl_fixed_pmc_pmi)) &&
	    !(global && PERF_COND_CHGD_IGNORE_MASK)) {
		pr_info("Something triggered PMI - GLOBAL: %llx\n", global);
		goto no_pmi;
	}

	/* 
	 * The current implementation of this function does not
	 * provide a sliding window for a discrete samples collection.
	 * If a PMI arises, it means that there is a pmc multiplexing
	 * request. 	 
	 */

	if (pmc_access_on_pmi(cpu))
		pmi_function(cpu);

	handled++;

	WRITE_FIXED_PMC(gbl_fixed_pmc_pmi,
			this_cpu_read(pcpu_pmus_metadata.pmi_reset_value));

no_pmi:
	if (recode_pmi_vector == NMI) {
		apic_write(APIC_LVTPC, LVT_NMI);
	} else {
		apic_write(APIC_LVTPC, RECODE_PMI);
		apic_eoi();
	}

	wrmsrl(MSR_CORE_PERF_GLOBAL_OVF_CTRL, global);
end:
	// put_cpu();

	return handled;
}

static void pmi_lvt_setup_on_cpu(void *dummy)
{
	/* Backup old LVT entry */
	*this_cpu_ptr(&pcpu_lvt_bkp) = apic_read(APIC_LVTPC);
	apic_write(APIC_LVTPC, LVT_NMI);
}

static void pmi_lvt_cleanup_on_cpu(void *dummy)
{
	/* Restore old LVT entry */
	apic_write(APIC_LVTPC, *this_cpu_ptr(&pcpu_lvt_bkp));
}

/* Setup the PMI's NMI handler */
int pmi_nmi_setup(void)
{
	int err = 0;

	handler_na.handler = pmi_handler;
	handler_na.name = NMI_NAME;
	handler_na.flags = NMI_FLAG_FIRST;

	pr_info("Setting NMI as PMI vector\n");

	err = __register_nmi_handler(NMI_LOCAL, &handler_na);
	if (err)
		goto out;

	on_each_cpu(pmi_lvt_setup_on_cpu, NULL, 1);
out:
	return err;
}

void pmi_nmi_cleanup(void)
{
	on_each_cpu(pmi_lvt_cleanup_on_cpu, NULL, 1);
	unregister_nmi_handler(NMI_LOCAL, NMI_NAME);
}

#ifdef FAST_IRQ_ENABLED
static int fast_irq_pmi_handler(void)
{
	return pmi_handler(0, NULL);
}
#endif

int pmi_irq_setup(void)
{
#ifdef CONFIG_RUNNING_ON_VM
	return 0;
#else
#ifndef FAST_IRQ_ENABLED
	pr_info("PMI on IRQ not available on this kernel. Proceed with NMI\n");
	return pmi_nmi_setup();
#else
	int irq = 0;

	pr_info("Mapping PMI on IRQ #%u\n", RECODE_PMI);

	/* Setup fast IRQ */
	irq = request_fast_irq(RECODE_PMI, fast_irq_pmi_handler);

	if (irq != RECODE_PMI)
		return -1;

	return 0;
#endif
#endif
}

void pmi_irq_cleanup(void)
{
#ifndef FAST_IRQ_ENABLED
	pmi_nmi_cleanup();
}
#else
	int unused = 0;
	unused = free_fast_irq(RECODE_PMI);
}
#endif