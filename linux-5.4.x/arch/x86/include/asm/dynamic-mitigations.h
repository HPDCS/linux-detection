/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_DYNAMIC_MITIGATIONS_H
#define _ASM_X86_DYNAMIC_MITIGATIONS_H

#include <asm/percpu.h>

#ifndef __ASSEMBLY__

DECLARE_PER_CPU(u32, pcpu_dynamic_mitigations);

#define cpu_has_dynamic_flag(f)                                              \
	(!!(this_cpu_read_stable(pcpu_dynamic_mitigations) & f))

#endif /* __ASSEMBLY__ */

/* Dynamic CPU Patch flags */
#define DM_PTI				0
#define DM_FENCE_SWAP_USER		1
#define DM_FENCE_SWAP_KERNEL		2
#define DM_RETPOLINE			3
#define DM_RSB_CTXSW			4
#define DM_USE_IBPB			5

#define DM_COND_STIBP			6
#define DM_COND_IBPB			7
#define DM_ALWAYS_IBPB			8
#define DM_MDS_CLEAR			9

#define DM_USE_IBRS_FW			10

#define DM_ENABLED			14

#define DM_RETPOLINE_SHIFT		_BITUL(DM_RETPOLINE)
#define DM_RSB_CTXSW_SHIFT		_BITUL(DM_RSB_CTXSW)
#define DM_USE_IBPB_SHIFT		_BITUL(DM_USE_IBPB)

#define DM_COND_STIBP_SHIFT		_BITUL(DM_COND_STIBP)
#define DM_COND_IBPB_SHIFT		_BITUL(DM_COND_IBPB)
#define DM_ALWAYS_IBPB_SHIFT		_BITUL(DM_ALWAYS_IBPB)
#define DM_MDS_CLEAR_SHIFT		_BITUL(DM_MDS_CLEAR)


/* Global flags: activte mitigations */

/* LLC Flush at CTX Switch */
#define DM_G_LLC_FLUSH			0	
/* Change Task CPU affinity */
#define DM_G_CPU_EXILE			1	
/* Transient Execution mitigations */
#define DM_G_TE_MITIGATE		2
/* Enable verbose mode */	
#define DM_G_VERBOSE			18	


#define DM_G_LLC_FLUSH_SHIFT		_BITUL(DM_G_LLC_FLUSH)
#define DM_G_CPU_EXILE_SHIFT		_BITUL(DM_G_CPU_EXILE)
#define DM_G_TE_MITIGATE_SHIFT		_BITUL(DM_G_TE_MITIGATE)
#define DM_G_VERBOSE_SHIFT		_BITUL(DM_G_VERBOSE)

#define skip_mitigation(type)				\
	(!cpu_has_dynamic_flag(_BITUL(type)))

#define skip_switch_to_cond_stibp			\
	(!cpu_has_dynamic_flag(DM_COND_STIBP_SHIFT))

#define skip_switch_mm_cond_ibpb			\
	(!cpu_has_dynamic_flag(DM_COND_IBPB_SHIFT))

#define skip_switch_mm_always_ibpb			\
	(!cpu_has_dynamic_flag(DM_ALWAYS_IBPB_SHIFT))

#define skip_mds_clear				\
	(!cpu_has_dynamic_flag(DM_MDS_CLEAR_SHIFT))
				


#endif /* _ASM_X86_DYNAMIC_PATCHES_H */

