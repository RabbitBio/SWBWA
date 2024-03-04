#include <slave.h>
#include <crts.h>

#include "bwt.h"
#include "bwamem.h"

typedef struct{
    bwt_t *bwt;
    bwtint_t *rbegs;
    int loop_size;
    bwtint_t* k_ids;
} Para_bwa_sa;

void bwt_sa_s(Para_bwa_sa *para) {
    flush_slave_cache(); 
    for(int i = _PEN; i < para->loop_size; i += 64) {
        para->rbegs[i] = bwt_sa(para->bwt, para->k_ids[i]);
    }
    flush_slave_cache();
}

typedef struct{
    long nn;
    void* data;
} Para_worker1_s;

void worker1_s(Para_worker1_s *para) {
    //flush_slave_cache(); 
    for(long i = _PEN; i < para->nn; i += 64) {
        worker1(para->data, i, _PEN);
    }
    //flush_slave_cache();
}
