#include <slave.h>
#include <crts.h>

#include "bwt.h"
#include "bwamem.h"


typedef struct{
    long nn;
    void* data;
    int* real_sizes;
} Para_worker1_s;

void worker1_s(Para_worker1_s *para) {
    for(long i = _PEN; i < para->nn; i += 64) {
        worker1(para->data, i, _PEN);
    }
}

void worker1_s_pre(Para_worker1_s *para) {
    for(long i = _PEN; i < para->nn; i += 64) {
        worker1_pre(para->data, i, _PEN, para->real_sizes);
    }
}
