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
    char** cpe_sams;
    int *sam_lens;
    const mem_pestat_t *pes0;
    int *s_ids;

    // for cross_copy
    int *tag;
    void *new_gp;
    unsigned long offset_seg;
    void *priv_addr;
    int tls_size;
    unsigned long *tls_content;
    char* big_buffer;
    long long cpe_buffer_size;
} Para_worker12_s;

typedef struct{
    char* big_buffer;
    long long cpe_buffer_size;
} Para_malloc;



/***********************************************************/
extern unsigned long segment1;
extern unsigned long segment1_len;
extern unsigned long segment2;
extern unsigned long segment2_len;
/***********************************************************/


#define use_cgs_mode

#ifdef use_cgs_mode
#define cpe_num_slave 384
#define global_pen (_CGN * 64 + _PEN)
#else
#define cpe_num_slave 64
#define global_pen (_PEN)
#endif

#define csr_copy_size (2 << 20)
#define private_start 0x400000000000



__thread Para_worker12_s *pp_slave;

void state_init(Para_malloc *para) {
 
    //printf("cpe set buffer %p %lld\n", para->big_buffer, para->cpe_buffer_size);
    set_big_buffer(para->big_buffer, para->cpe_buffer_size);
    //htmalloccount_init();
}

void state_print() {
    //fprintf(stderr, "%d state print start\n", _MYID);
    //htmalloccount_print();
    //fprintf(stderr, "%d state print done\n", _MYID);
}

void state_print_cross() {
    fprintf(stderr, "%d state print start\n", _MYID);
    htmalloccount_print();
    fprintf(stderr, "%d state print done\n", _MYID);
    pp_slave->tag[global_pen] = 1;
    flush_slave_cache();
    while(1);
}


void pass_para(Para_worker12_s *ptr)
{
    pp_slave = ptr;
    if(global_pen == 0 || global_pen == cpe_num_slave - 1) {
        printf("%d -- slave ptr1 para: %p, new_gp: %p, tag: %p, w: %p\n", global_pen, pp_slave, pp_slave->new_gp, pp_slave->tag, pp_slave->data);
    }
}

void copy_priv_segment(Para_worker12_s *ptr) {
    void* my_priv_addr = ptr->priv_addr + csr_copy_size * _MYID;
    if(_MYID == 0) printf("cpe %d, copy to %p start\n", _MYID, my_priv_addr);
    memcpy(my_priv_addr, (void*)private_start, csr_copy_size);
    if(_MYID == 0) printf("cpe %d, copy to %p done\n", _MYID, my_priv_addr);
}

void change_priv_segment(Para_worker12_s *ptr) {
    int tls_size = ptr->tls_size;
    unsigned long *tls_content = ptr->tls_content;
    void* my_priv_addr = ptr->priv_addr + csr_copy_size * _MYID;
    int pre_instr_size = 8;
    if(_MYID == 0) printf("cpe %d, change %p start\n", _MYID, my_priv_addr);
    for(unsigned long i = 0; i < (csr_copy_size) / pre_instr_size; ++i) {
        unsigned long disp = i * pre_instr_size;
        unsigned long *addr = (unsigned long*)((unsigned long)my_priv_addr + disp);
        unsigned long content = *addr;
        if(content >= segment1 + segment1_len) continue;
        if(content < segment1) continue;

        // check tls
        for(int j = 0; j < tls_size; j++) {
            unsigned long content2 = *((unsigned long *)tls_content[j]);
            //TODO
            if(content == content2) {
                *addr = content + ptr->offset_seg;
                if(_MYID == 0) printf("cpe change %p (%lu) from %lx to %lx\n", addr, disp, content, content + ptr->offset_seg);
                break;
            }
        }
    }
    if(_MYID == 0) printf("cpe %d, change %p done\n", _MYID, my_priv_addr);

}


__uncached long lock_s;

__thread_local_fix long t1 = 0;
__thread_local_fix long t2 = 0;
__thread_local_fix long t3 = 0;
__thread_local_fix long t4 = 0;


void init_icache_info() {
    penv_slave0_cycle_init();
    penv_slave2_l1ic_access_init();
    penv_slave3_l1ic_miss_init();
    penv_slave4_l1ic_misstime_init();
}

void print_icache_info_cross() {
    asm volatile("mov %0, $29\n\t"::"r"(pp_slave->new_gp):);

    unsigned long init_csr_value = 0;
    asm volatile("rcsr %0, 0xc4" : "=r"(init_csr_value));
    if(init_csr_value < 0x500000000000) {
        unsigned long new_csr_value = pp_slave->priv_addr + csr_copy_size * _MYID + init_csr_value - private_start;
        if(_MYID == 0) {
            printf("init_csr_value %p\n", (void*)init_csr_value);
            //for(int i = 0; i < 16; i++) {
            //    printf("%lx %lx\n", *(unsigned long*)(init_csr_value + i * 8), *(unsigned long*)(new_csr_value + i * 8));
            //}
        }
        asm volatile("wcsr %0, 0xc4\n\t"::"r"(new_csr_value):);
    }
    athread_lock(&lock_s);
    //fprintf(stderr, "worker12 %d (%d %d) %.3f %lld %lld %lld %lld\n", _MYID, _CGN, _PEN, 1.0 * t4 / t1, t1, t2, t3, t4);
    printf("worker12 %d (%d %d) %.3f %lld %lld %lld %lld\n", _MYID, _CGN, _PEN, 1.0 * t4 / t1, t1, t2, t3, t4);
    athread_unlock(&lock_s);
    pp_slave->tag[global_pen] = 1;
    flush_slave_cache();
    while(1);
}


void print_icache_info() {
    athread_lock(&lock_s);
    //fprintf(stderr, "worker12 %d (%d %d) %.3f %lld %lld %lld %lld\n", _MYID, _CGN, _PEN, 1.0 * t4 / t1, t1, t2, t3, t4);
    printf("worker12 %d (%d %d) %.3f %lld %lld %lld %lld\n", _MYID, _CGN, _PEN, 1.0 * t4 / t1, t1, t2, t3, t4);
    athread_unlock(&lock_s);
}

#define read_num_pre_block 1

#define use_dynamic_task

#ifdef use_dynamic_task
//__uncached __cross long work_counter;
__uncached long work_counter;
int task_list[384][50 << 10];
int task_num[384];
int cur_id[384];

int acquire_task(int block_num) {
    asm volatile("faal %0, 0(%1)\n\t"
                 : "=r"(cur_id[global_pen])
                 : "r"(&work_counter)
                 : "memory");
    if (cur_id[global_pen] < block_num) {
        //assert(task_num[global_pen] < (50 << 10));
        //printf("CG %d, size %d\n", _CGN, task_num[global_pen]);
        task_list[global_pen][task_num[global_pen]++] = cur_id[global_pen];
    }
    return cur_id[global_pen];
}
#endif

void worker1_s_pre_fast_cross() {

    asm volatile("mov %0, $29\n\t"::"r"(pp_slave->new_gp):);

    unsigned long init_csr_value = 0;
    asm volatile("rcsr %0, 0xc4" : "=r"(init_csr_value));
    if(init_csr_value < 0x500000000000) {
        unsigned long new_csr_value = pp_slave->priv_addr + csr_copy_size * _MYID + init_csr_value - private_start;
        if(_MYID == 0) {
            printf("init_csr_value p1 %p\n", (void*)init_csr_value);
            //for(int i = 0; i < 16; i++) {
            //    printf("p1 %lx %lx\n", *(unsigned long*)(init_csr_value + i * 8), *(unsigned long*)(new_csr_value + i * 8));
            //}
        }
        asm volatile("wcsr %0, 0xc4\n\t"::"r"(new_csr_value):);
    }

 
    asm volatile("memb\n\t":::);
    Para_worker12_s *para = pp_slave;

    lwpf_enter(TEST);
    lwpf_start(l_worker1_1);

#ifdef use_dynamic_task
    if(global_pen == 0) work_counter = 0;
# ifdef use_cgs_mode
    athread_ssync_node();
# else
    athread_ssync_array();
# endif
    task_num[global_pen] = 0;
    int block_num = ceil(1.0 * para->nn / read_num_pre_block);
    for(int i = acquire_task(block_num); i < block_num; i = acquire_task(block_num)) {
        int range_l = i * read_num_pre_block;
        int range_r = range_l + read_num_pre_block;
        if(range_r > para->nn) range_r = para->nn;
        for(int j = range_l; j < range_r; j++) {
            worker1_pre_fast(para->data, j, global_pen, para->cpe_regs);
        }
    }
#else
    for(long i = global_pen; i < para->nn; i += cpe_num_slave) {
        worker1_pre_fast(para->data, i, global_pen, para->cpe_regs);
    }
#endif

    lwpf_stop(l_worker1_1);
    lwpf_exit(TEST);

    pp_slave->tag[global_pen] = 1;
    flush_slave_cache();
    while(1);

}

void worker1_s_fast_cross() {
    asm volatile("mov %0, $29\n\t"::"r"(pp_slave->new_gp):);

    unsigned long init_csr_value = 0;
    asm volatile("rcsr %0, 0xc4" : "=r"(init_csr_value));
    if(init_csr_value < 0x500000000000) {
        unsigned long new_csr_value = pp_slave->priv_addr + csr_copy_size * _MYID + init_csr_value - private_start;
        if(_MYID == 0) {
            printf("init_csr_value %p\n", (void*)init_csr_value);
            //for(int i = 0; i < 16; i++) {
            //    printf("%lx %lx\n", *(unsigned long*)(init_csr_value + i * 8), *(unsigned long*)(new_csr_value + i * 8));
            //}
        }
        asm volatile("wcsr %0, 0xc4\n\t"::"r"(new_csr_value):);
    }

    Para_worker12_s *para = pp_slave;

    lwpf_enter(TEST);
    lwpf_start(l_worker1_2);

#ifdef use_dynamic_task
    for(long i = 0; i < task_num[global_pen]; i++) {
        int range_l = task_list[global_pen][i] * read_num_pre_block;
        int range_r = range_l + read_num_pre_block;
        if(range_r > para->nn) range_r = para->nn;
        for(int j = range_l; j < range_r; j++) {
            worker1_fast(para->data, j, global_pen, para->cpe_regs);
        }
    }
#else
    for(long i = global_pen; i < para->nn; i += cpe_num_slave) {
        worker1_fast(para->data, i, global_pen, para->cpe_regs);
    }
#endif
    lwpf_stop(l_worker1_2);
    lwpf_exit(TEST);

    pp_slave->tag[global_pen] = 1;
    flush_slave_cache();
    while(1);

}


void worker2_s_pre_fast_cross() {
    asm volatile("mov %0, $29\n\t"::"r"(pp_slave->new_gp):);

    unsigned long init_csr_value = 0;
    asm volatile("rcsr %0, 0xc4" : "=r"(init_csr_value));
    if(init_csr_value < 0x500000000000) {
        unsigned long new_csr_value = pp_slave->priv_addr + csr_copy_size * _MYID + init_csr_value - private_start;
        if(_MYID == 0) {
            printf("init_csr_value %p\n", (void*)init_csr_value);
            //for(int i = 0; i < 16; i++) {
            //    printf("%lx %lx\n", *(unsigned long*)(init_csr_value + i * 8), *(unsigned long*)(new_csr_value + i * 8));
            //}
        }
        asm volatile("wcsr %0, 0xc4\n\t"::"r"(new_csr_value):);
    }

    Para_worker12_s *para = pp_slave;

    lwpf_enter(TEST);
    lwpf_start(l_worker2_1);

#ifdef use_dynamic_task
    if(global_pen == 0) work_counter = 0;
#ifdef use_cgs_mode
    athread_ssync_node();
#else
    athread_ssync_array();
#endif
    task_num[global_pen] = 0;
    int block_num = ceil(1.0 * para->nn / read_num_pre_block);
    for(int i = acquire_task(block_num); i < block_num; i = acquire_task(block_num)) {
        int range_l = i * read_num_pre_block;
        int range_r = range_l + read_num_pre_block;
        if(range_r > para->nn) range_r = para->nn;
        for(int j = range_l; j < range_r; j++) {
            worker2_pre_fast(para->data, j, global_pen, para->sam_lens, para->cpe_sams);
        }
    }
#else
    for(long i = global_pen; i < para->nn; i += cpe_num_slave) {
        worker2_pre_fast(para->data, i, global_pen, para->sam_lens, para->cpe_sams);
    }
#endif
    lwpf_stop(l_worker2_1);
    lwpf_exit(TEST);


    pp_slave->tag[global_pen] = 1;
    flush_slave_cache();
    while(1);

}


void worker2_s_fast_cross() {
    asm volatile("mov %0, $29\n\t"::"r"(pp_slave->new_gp):);

    unsigned long init_csr_value = 0;
    asm volatile("rcsr %0, 0xc4" : "=r"(init_csr_value));
    if(init_csr_value < 0x500000000000) {
        unsigned long new_csr_value = pp_slave->priv_addr + csr_copy_size * _MYID + init_csr_value - private_start;
        if(_MYID == 0) {
            printf("init_csr_value %p\n", (void*)init_csr_value);
            //for(int i = 0; i < 16; i++) {
            //    printf("%lx %lx\n", *(unsigned long*)(init_csr_value + i * 8), *(unsigned long*)(new_csr_value + i * 8));
            //}
        }
        asm volatile("wcsr %0, 0xc4\n\t"::"r"(new_csr_value):);
    }

    Para_worker12_s *para = pp_slave;
 
    lwpf_enter(TEST);
    lwpf_start(l_worker2_2);
#ifdef use_dynamic_task
    for(long i = 0; i < task_num[global_pen]; i++) {
        int range_l = task_list[global_pen][i] * read_num_pre_block;
        int range_r = range_l + read_num_pre_block;
        if(range_r > para->nn) range_r = para->nn;
        for(int j = range_l; j < range_r; j++) {
            worker2_fast(para->data, j, global_pen, para->sam_lens, para->cpe_sams);
        }
    }
#else
    for(long i = global_pen; i < para->nn; i += cpe_num_slave) {
        worker2_fast(para->data, i, global_pen, para->sam_lens, para->cpe_sams);
    }
#endif
    lwpf_stop(l_worker2_2);
    lwpf_exit(TEST);

 
    pp_slave->tag[global_pen] = 1;
    flush_slave_cache();
    while(1);

}



void worker12_s_pre_fast_cross() {
    asm volatile("mov %0, $29\n\t"::"r"(pp_slave->new_gp):);

    unsigned long init_csr_value = 0;
    asm volatile("rcsr %0, 0xc4" : "=r"(init_csr_value));
    if(init_csr_value < 0x500000000000) {
    //if(0) {
        unsigned long new_csr_value = pp_slave->priv_addr + csr_copy_size * _MYID + init_csr_value - private_start;
        if(_MYID == 0) {
            printf("init_csr_value %p\n", (void*)init_csr_value);
            //for(int i = 0; i < 16; i++) {
            //    printf("%lx %lx\n", *(unsigned long*)(init_csr_value + i * 8), *(unsigned long*)(new_csr_value + i * 8));
            //}
        }
        asm volatile("wcsr %0, 0xc4\n\t"::"r"(new_csr_value):);
    }

    //printf("cpe set buffer %p %lld\n", pp_slave->big_buffer, pp_slave->cpe_buffer_size);
    //set_big_buffer(pp_slave->big_buffer, pp_slave->cpe_buffer_size);

    Para_worker12_s *para = pp_slave;
    lwpf_enter(TEST);
    lwpf_start(l_worker12_1);
#ifdef use_dynamic_task
    if(global_pen == 0) work_counter = 0;
#ifdef use_cgs_mode
    athread_ssync_node();
#else
    athread_ssync_array();
#endif
    task_num[global_pen] = 0;
    int block_num = ceil(1.0 * para->nn / read_num_pre_block);
    for(int i = acquire_task(block_num); i < block_num; i = acquire_task(block_num)) {
        int l_pos = i * read_num_pre_block;
        int r_pos = l_pos + read_num_pre_block;
        if(r_pos > para->nn) r_pos = para->nn;
        lwpf_start(l_worker12i_1);
        worker12_pre_fast(para->data, l_pos, r_pos, global_pen, para->sam_lens, para->cpe_sams, para->pes0, para->s_ids);
        lwpf_stop(l_worker12i_1);
    }
#else
    int pre_n = ceil(1.0 * para->nn / cpe_num_slave);
    int l_pos = global_pen * pre_n;
    int r_pos = l_pos + pre_n;
    if(r_pos > para->nn) r_pos = para->nn;
    worker12_pre_fast(para->data, l_pos, r_pos, global_pen, para->sam_lens, para->cpe_sams, para->pes0, para->s_ids);
#endif

    lwpf_stop(l_worker12_1);
    lwpf_exit(TEST);

    pp_slave->tag[global_pen] = 1;
    flush_slave_cache();
    while(1);
}


void worker12_s_fast_cross() {
    asm volatile("mov %0, $29\n\t"::"r"(pp_slave->new_gp):);

    unsigned long init_csr_value = 0;
    asm volatile("rcsr %0, 0xc4" : "=r"(init_csr_value));
    if(init_csr_value < 0x500000000000) {
    //if(0) {
        unsigned long new_csr_value = pp_slave->priv_addr + csr_copy_size * _MYID + init_csr_value - private_start;
        if(_MYID == 0) {
            printf("init_csr_value %p\n", (void*)init_csr_value);
            //for(int i = 0; i < 16; i++) {
            //    printf("%lx %lx\n", *(unsigned long*)(init_csr_value + i * 8), *(unsigned long*)(new_csr_value + i * 8));
            //}
        }
        asm volatile("wcsr %0, 0xc4\n\t"::"r"(new_csr_value):);
    }

    Para_worker12_s *para = pp_slave;
    lwpf_enter(TEST);
    lwpf_start(l_worker12_2);
#ifdef use_dynamic_task
    for(long i = 0; i < task_num[global_pen]; i++) {
        int l_pos = task_list[global_pen][i] * read_num_pre_block;
        int r_pos = l_pos + read_num_pre_block;
        if(r_pos > para->nn) r_pos = para->nn;
        worker12_fast(para->data, l_pos, r_pos, global_pen, para->sam_lens, para->cpe_sams, para->s_ids);
    }
#else
    int pre_n = ceil(1.0 * para->nn / cpe_num_slave);
    int l_pos = global_pen * pre_n;
    int r_pos = l_pos + pre_n;
    if(r_pos > para->nn) r_pos = para->nn;
    worker12_fast(para->data, l_pos, r_pos, global_pen, para->sam_lens, para->cpe_sams, para->s_ids);
#endif

    lwpf_stop(l_worker12_2);
    lwpf_exit(TEST);

    pp_slave->tag[global_pen] = 1;
    flush_slave_cache();
    while(1);
}


void worker1_s_pre_fast(Para_worker12_s *para) {
 
    lwpf_enter(TEST);
    lwpf_start(l_worker1_1);
#ifdef use_dynamic_task
# ifdef use_cgs_mode
    if(global_pen == 0) work_counter = 0;
    athread_ssync_node();
# else
    if(_PEN == 0) work_counter = 0;
    athread_ssync_array();
# endif
    task_num[global_pen] = 0;
    int block_num = ceil(1.0 * para->nn / read_num_pre_block);
    for(int i = acquire_task(block_num); i < block_num; i = acquire_task(block_num)) {
        int range_l = i * read_num_pre_block;
        int range_r = range_l + read_num_pre_block;
        if(range_r > para->nn) range_r = para->nn;
        for(int j = range_l; j < range_r; j++) {
            worker1_pre_fast(para->data, j, global_pen, para->cpe_regs);
        }
    }
#else
    for(long i = global_pen; i < para->nn; i += cpe_num_slave) {
        worker1_pre_fast(para->data, i, global_pen, para->cpe_regs);
    }
#endif
    lwpf_stop(l_worker1_1);
    lwpf_exit(TEST);
}

void worker1_s_fast(Para_worker12_s *para) {
    lwpf_enter(TEST);
    lwpf_start(l_worker1_2);
#ifdef use_dynamic_task
    for(long i = 0; i < task_num[global_pen]; i++) {
        int range_l = task_list[global_pen][i] * read_num_pre_block;
        int range_r = range_l + read_num_pre_block;
        if(range_r > para->nn) range_r = para->nn;
        for(int j = range_l; j < range_r; j++) {
            worker1_fast(para->data, j, global_pen, para->cpe_regs);
        }
    }
#else
    for(long i = global_pen; i < para->nn; i += cpe_num_slave) {
        worker1_fast(para->data, i, global_pen, para->cpe_regs);
    }
#endif
    lwpf_stop(l_worker1_2);
    lwpf_exit(TEST);

}



void worker2_s_pre_fast(Para_worker12_s *para) {
    lwpf_enter(TEST);
    lwpf_start(l_worker2_1);
#ifdef use_dynamic_task
    if(global_pen == 0) work_counter = 0;
#ifdef use_cgs_mode
    athread_ssync_node();
#else
    athread_ssync_array();
#endif
    task_num[global_pen] = 0;
    int block_num = ceil(1.0 * para->nn / read_num_pre_block);
    for(int i = acquire_task(block_num); i < block_num; i = acquire_task(block_num)) {
        int range_l = i * read_num_pre_block;
        int range_r = range_l + read_num_pre_block;
        if(range_r > para->nn) range_r = para->nn;
        for(int j = range_l; j < range_r; j++) {
            worker2_pre_fast(para->data, j, global_pen, para->sam_lens, para->cpe_sams);
        }
    }
#else
    for(long i = global_pen; i < para->nn; i += cpe_num_slave) {
        worker2_pre_fast(para->data, i, global_pen, para->sam_lens, para->cpe_sams);
    }
#endif
    lwpf_stop(l_worker2_1);
    lwpf_exit(TEST);

}


void worker2_s_fast(Para_worker12_s *para) {
    lwpf_enter(TEST);
    lwpf_start(l_worker2_2);
#ifdef use_dynamic_task
    for(long i = 0; i < task_num[global_pen]; i++) {
        int range_l = task_list[global_pen][i] * read_num_pre_block;
        int range_r = range_l + read_num_pre_block;
        if(range_r > para->nn) range_r = para->nn;
        for(int j = range_l; j < range_r; j++) {
            worker2_fast(para->data, j, global_pen, para->sam_lens, para->cpe_sams);
        }
    }
#else
    for(long i = global_pen; i < para->nn; i += cpe_num_slave) {
        worker2_fast(para->data, i, global_pen, para->sam_lens, para->cpe_sams);
    }
#endif
    lwpf_stop(l_worker2_2);
    lwpf_exit(TEST);

}



void worker12_s_pre_fast(Para_worker12_s *para) {

    //printf("cpe set buffer %p %lld\n", para->big_buffer, para->cpe_buffer_size);
    //set_big_buffer(para->big_buffer, para->cpe_buffer_size);

    lwpf_enter(TEST);
    lwpf_start(l_worker12_1);
#ifdef use_dynamic_task
    if(global_pen == 0) work_counter = 0;
#ifdef use_cgs_mode
    athread_ssync_node();
#else
    athread_ssync_array();
#endif
    task_num[global_pen] = 0;
    int block_num = ceil(1.0 * para->nn / read_num_pre_block);
    for(int i = acquire_task(block_num); i < block_num; i = acquire_task(block_num)) {
        int l_pos = i * read_num_pre_block;
        int r_pos = l_pos + read_num_pre_block;
        if(r_pos > para->nn) r_pos = para->nn;
        lwpf_start(l_worker12i_1);
        worker12_pre_fast(para->data, l_pos, r_pos, global_pen, para->sam_lens, para->cpe_sams, para->pes0, para->s_ids);
        lwpf_stop(l_worker12i_1);
    }
#else
    int pre_n = ceil(1.0 * para->nn / cpe_num_slave);
    int l_pos = global_pen * pre_n;
    int r_pos = l_pos + pre_n;
    if(r_pos > para->nn) r_pos = para->nn;
    worker12_pre_fast(para->data, l_pos, r_pos, global_pen, para->sam_lens, para->cpe_sams, para->pes0, para->s_ids);
#endif
    lwpf_stop(l_worker12_1);
    lwpf_exit(TEST);
}


void worker12_s_fast(Para_worker12_s *para) {
    lwpf_enter(TEST);
    lwpf_start(l_worker12_2);
#ifdef use_dynamic_task
    for(long i = 0; i < task_num[global_pen]; i++) {
        int l_pos = task_list[global_pen][i] * read_num_pre_block;
        int r_pos = l_pos + read_num_pre_block;
        if(r_pos > para->nn) r_pos = para->nn;
        worker12_fast(para->data, l_pos, r_pos, global_pen, para->sam_lens, para->cpe_sams, para->s_ids);
    }
#else
    int pre_n = ceil(1.0 * para->nn / cpe_num_slave);
    int l_pos = global_pen * pre_n;
    int r_pos = l_pos + pre_n;
    if(r_pos > para->nn) r_pos = para->nn;
    worker12_fast(para->data, l_pos, r_pos, global_pen, para->sam_lens, para->cpe_sams, para->s_ids);
#endif
    lwpf_stop(l_worker12_2);
    lwpf_exit(TEST);
}
