#include <linux/slab.h>

#include "recode.h"
#include "recode_config.h"
#include "pmu/pmu.h"
#include "pmu/pmi.h"
#include "pmu/hw_events.h"

/**
 * This macro creates an instance of struct hw_event for each hw_event present
 * inside HW_EVENTS. That struct is used to dynamically compute the mask of the
 * hw_events_set and other utility functions.
 */
#define X_HW_EVENTS(name, code)                                                \
	const struct hw_event hw_evt_##name = { .idx = CTR_HW_EVENTS,          \
						.evt = { code } };
HW_EVENTS
#undef X_HW_EVENTS

/* Define an array containing all the hw_events present inside HW_EVENTS. */
#define X_HW_EVENTS(name, code) { HW_EVT_COD(name) },
const pmc_evt_code gbl_raw_events[NR_HW_EVENTS + 1] = { HW_EVENTS };
#undef X_HW_EVENTS

static void flush_and_clean_hw_events(void)
{
}

/* Required preemption disabled */
void setup_hw_events_on_cpu(void *hw_events)
{
	// bool state;
	unsigned hw_cnt, pmcs_cnt, pmc;
	pmc_ctr reset_period;
	struct pmcs_collection *pmcs_collection;

	// TODO restore
	// __save_and_disable_pmc_on_this_cpu(&state);

	flush_and_clean_hw_events();

	if (!hw_events) {
		pr_warn("Cannot setup hw_events on cpu %u: hw_events is NULL\n",
			smp_processor_id());
		goto end;
	}

	hw_cnt = ((struct hw_events *)hw_events)->cnt;
	pmcs_cnt = hw_cnt + gbl_nr_pmc_fixed;

	pmcs_collection = this_cpu_read(pcpu_pmus_metadata.pmcs_collection);

	// 	/* Free old values */
	// 	if (pmcs_collection && pmcs_collection->cnt >= pmcs_cnt)
	// 		goto skip_alloc;

	// 	if (pmcs_collection)
	// 		kfree(pmcs_collection);

	// 	pmcs_collection = kzalloc(sizeof(struct pmcs_collection) +
	// 					  (sizeof(pmc_ctr) * pmcs_cnt),
	// 				  GFP_KERNEL);

	// 	if (!pmcs_collection) {
	// 		pr_warn("Cannot allocate memory for pmcs_collection on cpu %u\n",
	// 			smp_processor_id());
	// 		goto end;
	// 	}

	// 	pmcs_collection->complete = false;
	// 	pmcs_collection->cnt = pmcs_cnt;
	// 	pmcs_collection->mask = ((struct hw_events *)hw_events)->mask;

	// 	/* Update the new pmcs_collection value */
	// 	this_cpu_write(pcpu_pmus_metadata.pmcs_collection, pmcs_collection);

	// skip_alloc:

	pmcs_collection->complete = false;
	pmcs_collection->cnt = pmcs_cnt;
	pmcs_collection->mask = ((struct hw_events *)hw_events)->mask;

	/* Update hw_events */
	this_cpu_write(pcpu_pmus_metadata.pmi_partial_cnt, 0);
	this_cpu_write(pcpu_pmus_metadata.hw_events_index, 0);
	this_cpu_write(pcpu_pmus_metadata.hw_events, hw_events);
	this_cpu_write(pcpu_pmus_metadata.multiplexing,
		       hw_cnt > gbl_nr_pmc_general);

	/* Update pmc index array */
	recode_callbacks.on_hw_events_change(hw_events);

	if (this_cpu_read(pcpu_pmus_metadata.multiplexing)) {
		reset_period =
			gbl_reset_period / ((hw_cnt / gbl_nr_pmc_general) + 1);
	} else {
		reset_period = gbl_reset_period;
	}

	reset_period = PMC_TRIM(~reset_period);

	this_cpu_write(pcpu_pmus_metadata.pmi_reset_value, reset_period);
	pr_debug("[%u] Reset period set: %llx - Multiplexing time: %u\n",
		 smp_processor_id(), reset_period,
		 (hw_cnt / gbl_nr_pmc_general) + 1);

	fast_setup_general_pmc_on_cpu(smp_processor_id(),
				      ((struct hw_events *)hw_events)->cfgs, 0,
				      hw_cnt);

	for_each_fixed_pmc (pmc)
		WRITE_FIXED_PMC(pmc, 0);

	WRITE_FIXED_PMC(gbl_fixed_pmc_pmi,
			this_cpu_read(pcpu_pmus_metadata.pmi_reset_value));

end:
	return;
	// __restore_and_enable_pmc_on_this_cpu(&state);
}

int setup_hw_events_on_system(pmc_evt_code *hw_events_codes, unsigned cnt)
{
	u64 mask;
	unsigned b, i;
	struct hw_events *hw_events;

	if (gbl_nr_hw_evts_groups == MAX_GBL_HW_EVTS) {
		pr_warn("Cannot register more hw_events sets\n");
		return -ENOBUFS;
	}

	pr_debug("Expected %u hw_events_codes\n", cnt);
	if (!hw_events_codes || !cnt) {
		pr_warn("Invalid hw_events. Cannot proceed with setup\n");
		return -EINVAL;
	}

	mask = compute_hw_events_mask(hw_events_codes, cnt);

	/* Check if the mask is already registered */
	for (i = 0; i < gbl_nr_hw_evts_groups; ++i) {
		if (gbl_hw_evts_groups[i]->mask == mask) {
			pr_debug("Mask %llx is already used. Just setup\n",
				 mask);
			hw_events = gbl_hw_evts_groups[i];
			goto setup_done;
		}
	}

	hw_events = kzalloc(sizeof(struct hw_events) +
				    (sizeof(struct pmc_evt_sel) * cnt),
			    GFP_KERNEL);

	if (!hw_events) {
		pr_warn("Cannot allocate memory for hw_events\n");
		return -ENOMEM;
	}

	/* Remove duplicates */
	for (i = 0, b = 0; b < 64; ++b) {
		if (!(mask & BIT(b)))
			continue;

		hw_events->cfgs[i].perf_evt_sel = gbl_raw_events[b].raw;
		/* PMC setup */
		hw_events->cfgs[i].usr = !!(params_cpl_usr);
		hw_events->cfgs[i].os = !!(params_cpl_os);
		hw_events->cfgs[i].pmi = 0;
		hw_events->cfgs[i].en = 1;
		pr_debug("Configure HW_EVENT %llx\n",
			 hw_events->cfgs[i].perf_evt_sel);
		++i;
	}

	hw_events->cnt = i;
	hw_events->mask = mask;

	gbl_hw_evts_groups[gbl_nr_hw_evts_groups++] = hw_events;

setup_done:
	pr_info("HW_MASK: %llx\n", hw_events->mask);
	on_each_cpu(setup_hw_events_on_cpu, hw_events, 1);

	return 0;
}

u64 compute_hw_events_mask(pmc_evt_code *hw_events_codes, unsigned cnt)
{
	u64 mask = 0;
	unsigned e, k;

	for (e = 0; e < cnt; ++e) {
		for (k = 0; k < NR_HW_EVENTS; ++k) {
			if (hw_events_codes[e].raw == gbl_raw_events[k].raw) {
				mask |= BIT(k);
				break;
			}
		}
	}

	return mask;
}