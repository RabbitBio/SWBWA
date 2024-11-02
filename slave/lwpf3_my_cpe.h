#define use_lwpf3

#ifdef use_lwpf3

#define EVT_PC0 PC0_CYCLE
#define EVT_PC1 PC1_INST
#define EVT_PC2 PC2_L1IC_ACCESS
#define EVT_PC3 PC3_LDM_PIPE
#define EVT_PC4 PC4_L1IC_MISSTIME
#define EVT_PC5 PC5_INST_L0IC_READ
#define EVT_PC6 PC6_CYC_LAUNHCNONE_BUFFER
#define EVT_PC7 PC7_INST_L0IC_READ
#define LWPF_KERNELS K(l_worker1_1) K(l_mem_chain) K(l_collect_intv) K(l_collect_1) K(l_my_smem1) K(l_smem1a_for) K(l_smem1a_back) K(l_forward_batch) K(l_backward_batch) K(l_collect_2) K(l_my_smem2) K(l_my_seed_stra) K(l_my_extend1) K(l_my_extend2) K(l_my_extend3) K(l_my_extend4) K(l_my_extend5) K(l_my_extend6) K(l_collect_3) K(l_collect_4) K(l_my_cal_rep) K(l_my_btree_chain) K(l_my_btree_sa) K(l_my_btree_rid) K(l_my_btree_find) K(l_my_btree_merge) K(l_my_btree_add) K(l_my_btree_free) K(l_mem_chain_flt) K(l_mem_flt_chained_seeds) K(l_mem_chain2aln) K(l_mem_sort_dedup_patch1) K(l_worker1_2) K(l_worker2) K(l_worker2_1) K(l_worker2_2) K(l_2_pre) K(l_2_mem) K(l_2_after) K(l_2_after_1)  K(l_2_after_2) K(l_2_after_3) K(l_2_mem_1) K(l_2_mem_2) K(l_2_mem_3) K(l_2_mem_4) K(l_2_mem_5) K(l_mem_gen_alt) K(l_mem_reg2aln) K(l_mem_matesw1) K(l_mem_matesw2) K(l_bns_fetch_seq) K(l_mem_sort_dedup_patch) K(l_ksw_1) K(l_ksw_2) K(l_ksw_3) K(l_ksw_4) K(l_sort_1) K(l_sort_2) K(l_ksw_global2) K(l_bwa_gen_cigar2)

//K(l_my_2ooc1) K(l_my_2ooc2) K(l_my_2ooc3) K(l_my_2ooc4)
#define LWPF_UNIT U(TEST)
#include "lwpf.h"



#else
#define lwpf_start(x) ((void)0)
#define lwpf_stop(x) ((void)0)
#define lwpf_enter(x) ((void)0)
#define lwpf_exit(x) ((void)0)
#endif


