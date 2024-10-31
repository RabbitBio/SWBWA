#include <slave.h>
#include <crts.h>

#include "bwt.h"
#include "bwamem.h"

#include "lwpf3_my_cpe.h"

typedef struct{
    long nn;
    void* data;
    int* real_sizes;
    mem_alnreg_v* cpe_regs;
} Para_worker1_s;

typedef struct{
    long nn;
    void* data;
} Para_worker2_s;

void get_occ_bench(Para_worker1_s *para) {
    occ_bench(para->data);
}

//#define use_cgs_mode

#ifdef use_cgs_mode
#define cpe_num_slave 384
#define global_pen (_CGN * 64 + _PEN)
#else
#define cpe_num_slave 64
#define global_pen (_PEN)
#endif


void worker1_s_init(Para_worker1_s *para) {
    for(long i = global_pen; i < para->nn; i += cpe_num_slave) {
        worker1_init(para->data, i, global_pen);
    }
}

void worker1_s(Para_worker1_s *para) {
    for(long i = global_pen; i < para->nn; i += cpe_num_slave) {
        worker1(para->data, i, global_pen);
    }
}

void worker1_s_fast(Para_worker1_s *para) {
#ifdef use_lwpf3
    lwpf_enter(TEST);
    lwpf_start(l_worker1_2);
#endif
    //if(_PEN) return;
    for(long i = global_pen; i < para->nn; i += cpe_num_slave) {
    //for(long i = 0; i < para->nn; i++) {
        worker1_fast(para->data, i, global_pen, para->cpe_regs);
    }
#ifdef use_lwpf3
    lwpf_stop(l_worker1_2);
    lwpf_exit(TEST);
#endif
}


void worker1_s_pre(Para_worker1_s *para) {
    for(long i = global_pen; i < para->nn; i += cpe_num_slave) {
        worker1_pre(para->data, i, global_pen, para->real_sizes);
    }
}

void worker1_s_pre_fast(Para_worker1_s *para) {

#ifdef use_lwpf3
    lwpf_enter(TEST);
    lwpf_start(l_worker1_1);
#endif
    //if(_PEN) return;
    for(long i = global_pen; i < para->nn; i += cpe_num_slave) {
    //for(long i = 0; i < para->nn; i++) {
        worker1_pre_fast(para->data, i, global_pen, para->cpe_regs);
    }
#ifdef use_lwpf3
    lwpf_stop(l_worker1_1);
    lwpf_exit(TEST);
#endif
}



void worker2_s(Para_worker2_s *para) {
#ifdef use_lwpf3
    lwpf_enter(TEST);
    lwpf_start(l_worker2);
#endif
    for(long i = global_pen; i < para->nn; i += cpe_num_slave) {
        worker2(para->data, i, global_pen);
    }
#ifdef use_lwpf3
    lwpf_stop(l_worker2);
    lwpf_exit(TEST);
#endif
}
