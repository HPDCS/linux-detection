#include <linux/slab.h>
#include <linux/dynamic-mitigations.h>
#include <linux/sched/signal.h>

#include "dependencies.h"

#include "pmu/hw_events.h"
#include "recode_security.h"

enum tuning_type tuning_type;

#define MSK_16(v) (v & (BIT(16) - 1))

/* The last value is the number of samples while tuning the system */
struct sc_thresholds sc_ths_fr = { .nr = 4,
				   .cnt = 1,
				   .ths = { 950, 950, 0, 0 } };

struct sc_thresholds sc_ths_xl = { .nr = 1, .cnt = 1, .ths = { 950 } };

#define SKIP_SAMPLES_BEFORE 1000
#define DEFAULT_TS_PRECISION 1000

atomic_t detected_theads = ATOMIC_INIT(0);

unsigned ts_precision = DEFAULT_TS_PRECISION;
unsigned ts_precision_5 = (DEFAULT_TS_PRECISION * 0.05);

bool signal_detected = false;
unsigned ts_window = 100;
unsigned ts_alpha = 1;
unsigned ts_beta = 1;

int recode_security_init(void)
{
#define NR_SC_HW_EVTS 6

	unsigned k = 0;

	pmc_evt_code *SC_HW_EVTS =
		kmalloc(sizeof(pmc_evt_code *) * NR_SC_HW_EVTS, GFP_KERNEL);

	if (!SC_HW_EVTS) {
		pr_err("Cannot allocate memory for SC_HW_EVTS\n");
		return -ENOMEM;
	}

	SC_HW_EVTS[k++].raw = HW_EVT_COD(l2_all_data);
	SC_HW_EVTS[k++].raw = HW_EVT_COD(l2_data_misses);
	SC_HW_EVTS[k++].raw = HW_EVT_COD(l3_miss_data);
	SC_HW_EVTS[k++].raw = HW_EVT_COD(tlb_page_walk);

	if (gbl_nr_pmc_general >= 6) {
		SC_HW_EVTS[k++].raw = HW_EVT_COD(l2_wb);
		SC_HW_EVTS[k++].raw = HW_EVT_COD(l2_in_all);
	} else {
		pr_warn("*** NOT ENOUGH PMCs ***");
		pr_warn("*** SKIPPING EVENTS: l2_wb l2_in_all ***");
	}

	recode_callbacks.on_pmi = on_pmi;
	recode_callbacks.on_ctx = on_ctx;
	recode_callbacks.on_state_change = on_state_change;

	setup_hw_events_on_system(SC_HW_EVTS, k);

	return 0;
}

void recode_security_fini(void)
{
}

static void tuning_finish_callback(void *dummy)
{
	unsigned k;
	recode_set_state(OFF);

	if ((tuning_type == FR && sc_ths_fr.cnt < (2 * SKIP_SAMPLES_BEFORE)) ||
	    (tuning_type == XL && sc_ths_xl.cnt < (2 * SKIP_SAMPLES_BEFORE))) {
		pr_warn("Not enough samples... Tuning not completed\n");
		goto skip;
	}

	if (tuning_type == FR) {
		for (k = 0; k < sc_ths_fr.nr; ++k) {
			sc_ths_fr.ths[k] /= (sc_ths_fr.cnt - SKIP_SAMPLES_BEFORE);
			pr_warn("THS [FR] [%u]: %lld\n", k, sc_ths_fr.ths[k]);
		}
	}

	if (tuning_type == XL) {
		for (k = 0; k < sc_ths_xl.nr; ++k) {
			sc_ths_xl.ths[k] /= (sc_ths_xl.cnt - SKIP_SAMPLES_BEFORE);
			pr_warn("THS [XL] [%u]: %lld\n", k, sc_ths_xl.ths[k]);
		}
	}

skip:
	pr_warn("THS SAMPLES [FR] %u - used: %u\n", sc_ths_fr.cnt,
		(sc_ths_fr.cnt - SKIP_SAMPLES_BEFORE));
	sc_ths_fr.cnt = 1;
	pr_warn("THS SAMPLES [XL] %u - used: %u\n", sc_ths_xl.cnt,
		(sc_ths_xl.cnt - SKIP_SAMPLES_BEFORE));
	sc_ths_xl.cnt = 1;
	pr_warn("THS PRECISION %u\n", ts_precision);

	set_exit_callback(NULL);
	pr_warn("Tuning finished\n");
}

bool on_state_change(enum recode_state state)
{
	if (state == TUNING) {
		char *type;
		unsigned k;
		switch (tuning_type) {
		case FR:
			type = "Flush+Reload";
			k = sc_ths_fr.nr;
			while (k--)
				sc_ths_fr.ths[k] = 0;
			break;
		case XL:
			type = "XLate on Pagetables";
			k = sc_ths_xl.nr;
			while (k--)
				sc_ths_xl.ths[k] = 0;
			break;
		default:
			pr_err("No tuning type defined... Skip\n");
			return false;
		}
		set_exit_callback(tuning_finish_callback);
		pr_warn("Recode ready for TUNING (%s)\n", type);

		return true;
	} else {
		set_exit_callback(NULL);
	}

	return false;
}

void print_spy(char *s)
{
	if (current->comm[0] == 's' && current->comm[1] == 'p' &&
	    current->comm[2] == 'y') {
		pr_info("[%u] SPY ON %s\n", current->pid, s);
	}
}

void on_ctx(struct task_struct *prev, bool prev_on, bool curr_on)
{
	unsigned cpu = get_cpu();

	if (!prev_on)
		goto no_log;

	/* Disable PMUs, hence PMI */
	disable_pmc_on_this_cpu(false);

	// We may skip sample at ctx
	// if (pmc_generate_collection(cpu))
	// 	pmc_evaluate_activity(
	// 		prev,
	// 		this_cpu_read(pcpu_pmus_metadata.pmcs_collection));

	// CLEAN PMCs
	pmc_generate_collection(cpu);

	/* Enable PMUs */
	enable_pmc_on_this_cpu(false);

no_log:
	/* Activate on previous task */
	if (has_pending_mitigations(prev))
		enable_mitigations_on_task(prev);

	/* Enable mitigations */
	mitigations_switch(prev, current);
	LLC_flush(current);

	put_cpu();
}

static void enable_detection_statistics(struct task_struct *tsk)
{
	struct detect_stats *ds;

	/* Skif if already allocated */
	if (tsk->monitor_state & BIT(DM_ACTIVE))
		return;

	ds = kmalloc(sizeof(struct detect_stats), GFP_ATOMIC);

	if (!ds) {
		pr_warn("@%u] Cannot allocate statistics for pid %u\n",
			smp_processor_id(), tsk->pid);
		return;
	}

	/* Copy and safe truncate the task name */
	memcpy(ds->comm, tsk->comm, sizeof(ds->comm));
	ds->comm[sizeof(ds->comm) - 1] = '\0';

	ds->pid = tsk->pid;
	ds->tgid = tsk->tgid;

	ds->utime = tsk->utime;
	ds->stime = tsk->stime;

	ds->s1 = 0;
	ds->p4 = 0;

	ds->ticks = 0;
	ds->score = 0;

	/* Attach stats to task */
	tsk->monitor_state |= BIT(DM_ACTIVE);
	tsk->monitor_data = (void *)ds;
}

static void kill_detected_process(struct task_struct *tsk)
{
	int ret;
	if (!signal_detected)
		return;

	ret = send_sig_info(SIGINT, SEND_SIG_NOINFO, tsk);
	if (ret < 0)
		pr_warn("Error sending signal to process: %u\n", tsk->pid);
}

static void finalize_detection_statisitcs(struct task_struct *tsk)
{
	struct detect_stats *ds;

	/* Skip KThreads */
	if (!tsk->mm)
		return;

	/* Something went wrong */
	if (!(tsk->monitor_state & BIT(DM_ACTIVE)))
		return;

	ds = (struct detect_stats *)tsk->monitor_data;

	ds->utime = tsk->utime - ds->utime;
	ds->stime = tsk->stime - ds->stime;

	tsk->monitor_state &= ~BIT(DM_ACTIVE);
	tsk->monitor_data = NULL;

	/* Print all the values */
	pr_warn("DETECTED pid %u (tgid %u): %s\n", ds->pid, ds->tgid, ds->comm);
	pr_info("[!!] %s %u %u\n", ds->comm, ds->pid, ds->tgid);

	// printk(" **  UTIME --> %llu\n", ds->utime);
	// printk(" **  STIME --> %llu\n", ds->stime);
	printk(" **  SCORE --> %u\n", ds->score);
	printk(" **  TICKS --> %u\n", ds->ticks);
	printk(" **  	S1 --> %u\n", ds->s1);
	printk(" **  	P4 --> %u\n", ds->p4);
	printk(" ****  END OF REPORT  ****\n");

	kfree(ds);
}

static bool evaluate_pmcs(struct task_struct *tsk,
			  struct pmcs_collection *snapshot)
{
	struct detect_stats *ds = NULL;
	bool p1, p2, p3, p4, p5;

	/* Skip KThreads */
	if (!tsk->mm)
		goto end;

	if (recode_state == IDLE)
		goto end;

	if (recode_state == TUNING) {
		switch (tuning_type) {
		case FR:
			if (sc_ths_fr.cnt > SKIP_SAMPLES_BEFORE) {
				sc_ths_fr.ths[0] += DM0(ts_precision, snapshot);
				sc_ths_fr.ths[1] += DM1(ts_precision, snapshot);
				sc_ths_fr.ths[2] += DM2(ts_precision, snapshot);
				sc_ths_fr.ths[3] += DM3(ts_precision, snapshot);
			}
			sc_ths_fr.cnt++;
			break;
		case XL:
			if (sc_ths_xl.cnt > SKIP_SAMPLES_BEFORE)
				sc_ths_xl.ths[0] += DM3(ts_precision, snapshot);
			sc_ths_xl.cnt++;
			break;
		}
		goto end;
	}

	if (has_mitigations(tsk))
		goto end;

	ds = (struct detect_stats *)tsk->monitor_data;

	// TODO Remove - Init task_struct
	if (MSK_16(tsk->monitor_state) > ts_window + 1)
		tsk->monitor_state &= ~(BIT(16) - 1);

	p1 = LESS_THAN_TS(sc_ths_fr.ths[0], DM0(ts_precision, snapshot),
			  ts_precision_5);

	p2 = LESS_THAN_TS(sc_ths_fr.ths[1], DM1(ts_precision, snapshot),
			  ts_precision_5);

	p3 = MORE_THAN_TS(sc_ths_fr.ths[2], DM2(ts_precision, snapshot),
			  ts_precision_5);

	p5 = MORE_THAN_TS(sc_ths_fr.ths[3], DM3(ts_precision, snapshot),
			  ts_precision_5);

	p4 = LESS_THAN_TS(sc_ths_xl.ths[0], DM3(ts_precision, snapshot),
			  ts_precision_5);

	bool s1 = p1 && p2 && p5;

	if (recode_state == PROFILE && query_tracker(tsk)) {
		pr_info("P1: %llu, P2: %llu, P3: %llu, P4: %llu\n",
			DM0(ts_precision, snapshot),
			DM1(ts_precision, snapshot),
			DM2(ts_precision, snapshot),
			DM3(ts_precision, snapshot));
		pr_info("P3: %llu/%llu\n", pmcs_general(snapshot->pmcs)[3],
			pmcs_general(snapshot->pmcs)[4]);

		pr_info("P4: %u\n", p4);
	}

	if (ds)
		ds->ticks++;

	if (s1 || p4) {
		tsk->monitor_state += ts_alpha;
		/* Enable statitics for this task */
		enable_detection_statistics(tsk);

		if (!ds) {
			ds = (struct detect_stats *)tsk->monitor_data;
		}

		if (ds) {
			ds->s1 += s1 ? 1 : 0;
			ds->p4 += p4 ? 1 : 0;
		}

		if (ds && ds->score < MSK_16(tsk->monitor_state)) {
			ds->score = MSK_16(tsk->monitor_state);
			pr_info("[++] %s %u %u %u %u %u %u %u %u %u %u\n",
				tsk->comm, tsk->pid, ds->score, ds->ticks + 1,
				ds->s1, ds->p4, p1, p2, p3, p4, p5);
			pr_info("[WATCH] %s (PID %u): %u/%u [s1:%u - p4:%u]\n",
				tsk->comm, tsk->pid, ds->score, ds->ticks + 1,
				ds->s1, ds->p4);
		}
	} else if (MSK_16(tsk->monitor_state) > 0) {
		tsk->monitor_state -= ts_beta;
		// pr_info("[--] %s (PID %u): %x\n", tsk->comm,
		// 	tsk->pid, tsk->monitor_state);
	}

	if (MSK_16(tsk->monitor_state) > ts_window) {
		pr_warn("[FLAG] Detected %s (PID %u): %u\n", tsk->comm,
			tsk->pid, tsk->monitor_state);

		/* Close statitics for this task */
		atomic_inc(&detected_theads);
		finalize_detection_statisitcs(tsk);
		kill_detected_process(tsk);

		return true;
	}

end:
	return false;
}

void pmc_evaluate_activity(unsigned cpu, struct task_struct *tsk,
			   struct pmcs_collection *pmcs)
{
	// get_cpu();

	if (unlikely(!pmcs)) {
		pr_warn("Called pmc_evaluate_activity without pmcs... skip");
		goto end;
	}

	if (evaluate_pmcs(tsk, pmcs)) {
		/* Delay activation if we are inside the PMI */
		request_mitigations_on_task(tsk, true);
	}

end:
	return;
	// put_cpu();
}

void on_pmi(unsigned cpu, struct pmus_metadata *pmus_metadata)
{
	pmc_evaluate_activity(cpu, current, pmus_metadata->pmcs_collection);
}