//#define use_lwpf3

#ifdef use_lwpf3

#define EVT_PC0 PC0_CYCLE
#define EVT_PC1 PC1_INST
#define EVT_PC2 PC2_L1IC_ACCESS
#define EVT_PC3 PC3_LDM_PIPE
#define EVT_PC4 PC4_L1IC_MISSTIME
#define EVT_PC5 PC5_INST_L0IC_READ
#define EVT_PC6 PC6_CYC_LAUNHCNONE_BUFFER
#define EVT_PC7 PC7_INST_L0IC_READ

#define LWPF_KERNELS K(l_worker1_1) K(l_worker1_2) K(l_worker2) K(l_2_pre) K(l_2_mem) K(l_2_after) K(l_2_after_1)  K(l_2_after_2) K(l_2_after_3) K(l_2_mem_1) K(l_2_mem_2) K(l_2_mem_3) K(l_2_mem_4) K(l_2_mem_5) K(l_mem_gen_alt) K(l_mem_reg2aln) K(l_mem_matesw1) K(l_mem_matesw2) K(l_bns_fetch_seq) K(l_mem_sort_dedup_patch) K(l_ksw_1) K(l_ksw_2) K(l_ksw_3) K(l_ksw_4) K(l_sort_1) K(l_sort_2) K(l_ksw_global2) K(l_bwa_gen_cigar2)
#define LWPF_UNIT U(TEST)
#include "lwpf.h"

#endif


