#include "recode.h"
#include "pmu/pmu.h"
#include "recode_config.h"


#ifdef SECURITY_MODULE_ON
#define SAMPLING_PERIOD (BIT_ULL(20) - 1)
unsigned __read_mostly gbl_fixed_pmc_pmi = 1; // PMC with PMI active
enum recode_pmi_vector recode_pmi_vector = IRQ;
#elif defined(TMA_MODULE_ON)
#define SAMPLING_PERIOD (BIT_ULL(28) - 1)
unsigned __read_mostly gbl_fixed_pmc_pmi = 2; // PMC with PMI active
enum recode_pmi_vector recode_pmi_vector = NMI;
#else
#define SAMPLING_PERIOD (BIT_ULL(24) - 1)
unsigned __read_mostly gbl_fixed_pmc_pmi = 2; // PMC with PMI active
enum recode_pmi_vector recode_pmi_vector = NMI;
#endif

// u64 __read_mostly gbl_reset_period = PMC_TRIM(~SAMPLING_PERIOD);
u64 __read_mostly gbl_reset_period = SAMPLING_PERIOD;

unsigned params_cpl_os = 1;
unsigned params_cpl_usr = 1;
