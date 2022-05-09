#ifndef _PMU_STRUCTS_H
#define _PMU_STRUCTS_H

#include <linux/types.h>

typedef u64 pmc_ctr;

typedef union {
	u32 raw;
	struct {
		u8 evt;
		u8 umask;
		u8 reserved;
		u8 cmask;
	};
} pmc_evt_code;

/* Recode structs */
struct pmc_evt_sel {
	union {
		u64 perf_evt_sel;
		struct {
			u64 evt : 8, umask : 8, usr : 1, os : 1, edge : 1,
				pc : 1, pmi : 1, any : 1, en : 1, inv : 1,
				cmask : 8, reserved : 32;
		};
	};
} __attribute__((packed));

struct pmcs_snapshot {
	u64 tsc;
	union {
		pmc_ctr pmcs[11];
		struct {
			pmc_ctr fixed[3];
			pmc_ctr general[8];
		};
	};
};

struct pmcs_collection {
	u64 mask;
	unsigned cnt;
	bool complete;
	/* [ FIXED0 - FIXED1 - ... - GENERAL0 - GENERAL1 - ... ] */
	pmc_ctr pmcs[];
};

struct hw_events {
	u64 mask;
	unsigned cnt;
	struct pmc_evt_sel cfgs[];
};

struct pmus_metadata {
	u64 fixed_ctrl;
	u64 perf_global_ctrl;
	bool pmcs_active;

	pmc_ctr pmi_reset_value;
	unsigned pmi_partial_cnt;

	u64 sample_tsc;
	pmc_ctr last_tsc;

	unsigned hw_events_index;
	struct hw_events *hw_events;
	bool multiplexing;
	pmc_ctr *hw_pmcs;
	struct pmcs_collection *pmcs_collection;
};

#endif /* _PMU_STRUCTS_H */