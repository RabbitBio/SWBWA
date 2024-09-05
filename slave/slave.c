#include <slave.h>
#include <crts.h>

#include "bwt.h"
#include "bwamem.h"


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

void worker1_s_init(Para_worker1_s *para) {
    //if(_PEN) return;
    for(long i = _PEN; i < para->nn; i += 64) {
    //for(long i = _PEN; i < para->nn; i++) {
        worker1_init(para->data, i, _PEN);
    }
}

void worker1_s(Para_worker1_s *para) {
    for(long i = _PEN; i < para->nn; i += 64) {
        worker1(para->data, i, _PEN);
    }
}

void worker1_s_fast(Para_worker1_s *para) {
    for(long i = _PEN; i < para->nn; i += 64) {
        worker1_fast(para->data, i, _PEN, para->cpe_regs);
    }
}


void worker1_s_pre(Para_worker1_s *para) {
    for(long i = _PEN; i < para->nn; i += 64) {
        worker1_pre(para->data, i, _PEN, para->real_sizes);
    }
}

void worker1_s_pre_fast(Para_worker1_s *para) {
    for(long i = _PEN; i < para->nn; i += 64) {
        worker1_pre_fast(para->data, i, _PEN, para->cpe_regs);
    }
}



void worker2_s(Para_worker2_s *para) {
    //if(_PEN) return;
    for(long i = _PEN; i < para->nn; i += 64) {
    //for(long i = _PEN; i < para->nn; i++) {
        worker2(para->data, i, _PEN);
    }
}
