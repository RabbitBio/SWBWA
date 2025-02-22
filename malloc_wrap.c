#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifdef USE_MALLOC_WRAPPERS
/* Don't wrap ourselves */
#  undef USE_MALLOC_WRAPPERS
#endif

#include "malloc_wrap.h"


#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include <mpi.h>
#include <assert.h>


//#define assert(x)

//#define use_std_malloc

inline double get_time() {
    struct timeval t_get_time;
    gettimeofday(&t_get_time, NULL);
    return t_get_time.tv_sec * 1000000 + t_get_time.tv_usec;
}

typedef struct Timer_block {

    size_t calls_free;
    double time_avg_free;
    double time_max_free;
    double time_min_free;
    double time_sum_free;

    double size_avg_free;
    size_t size_max_free;
    size_t size_min_free;
    size_t size_sum_free;



    size_t calls_calloc;
    double time_avg_calloc;
    double time_max_calloc;
    double time_min_calloc;
    double time_sum_calloc;

    double size_avg_calloc;
    size_t size_max_calloc;
    size_t size_min_calloc;
    size_t size_sum_calloc;



    size_t calls_valloc;
    double time_avg_valloc;
    double time_max_valloc;
    double time_min_valloc;
    double time_sum_valloc;

    double size_avg_valloc;
    size_t size_max_valloc;
    size_t size_min_valloc;
    size_t size_sum_valloc;



    size_t calls_realloc;
    double time_avg_realloc;
    double time_max_realloc;
    double time_min_realloc;
    double time_sum_realloc;

    double size_avg_realloc;
    size_t size_max_realloc;
    size_t size_min_realloc;
    size_t size_sum_realloc;



    size_t calls_mem;
    double time_avg_mem;
    double time_max_mem;
    double time_min_mem;
    double time_sum_mem;

    double size_avg_mem;
    size_t size_max_mem;
    size_t size_min_mem;
    size_t size_sum_mem;



    size_t calls;
    double time_avg;
    double time_max;
    double time_min;
    double time_sum;

    double size_avg;
    size_t size_max;
    size_t size_min;
    size_t size_sum;



    size_t calls1;
    double time_avg1;
    double time_max1;
    double time_min1;
    double time_sum1;

    double size_avg1;
    size_t size_max1;
    size_t size_min1;
    size_t size_sum1;



    size_t calls2;
    double time_avg2;
    double time_max2;
    double time_min2;
    double time_sum2;

    double size_avg2;
    size_t size_max2;
    size_t size_min2;
    size_t size_sum2;




    size_t calls4;
    double time_avg4;
    double time_max4;
    double time_min4;
    double time_sum4;

    double size_avg4;
    size_t size_max4;
    size_t size_min4;
    size_t size_sum4;



    size_t calls8;
    double time_avg8;
    double time_max8;
    double time_min8;
    double time_sum8;

    double size_avg8;
    size_t size_max8;
    size_t size_min8;
    size_t size_sum8;



    size_t calls16;
    double time_avg16;
    double time_max16;
    double time_min16;
    double time_sum16;

    double size_avg16;
    size_t size_max16;
    size_t size_min16;
    size_t size_sum16;



    size_t calls32;
    double time_avg32;
    double time_max32;
    double time_min32;
    double time_sum32;

    double size_avg32;
    size_t size_max32;
    size_t size_min32;
    size_t size_sum32;



    size_t calls64;
    double time_avg64;
    double time_max64;
    double time_min64;
    double time_sum64;

    double size_avg64;
    size_t size_max64;
    size_t size_min64;
    size_t size_sum64;



    size_t callsbig;
    double time_avgbig;
    double time_maxbig;
    double time_minbig;
    double time_sumbig;

    double size_avgbig;
    size_t size_maxbig;
    size_t size_minbig;
    size_t size_sumbig;


} timer_block;

timer_block tb;
int timer_flag = 0;

void update(double t, size_t size) {
    tb.calls++;
    if (t > tb.time_max) {
        tb.time_max = t;
    }
    if (t < tb.time_min) {
        tb.time_min = t;
    }
    if (size > tb.size_max) {
        tb.size_max = size;
    }
    if (size < tb.size_min) {
        tb.size_min = size;
    }
    tb.size_sum += size;
    tb.time_sum += t;
}

void update1(double t, size_t size) {
    tb.calls1++;
    if (t > tb.time_max1) {
        tb.time_max1 = t;
    }
    if (t < tb.time_min1) {
        tb.time_min1 = t;
    }
    if (size > tb.size_max1) {
        tb.size_max1 = size;
    }
    if (size < tb.size_min1) {
        tb.size_min1 = size;
    }
    tb.size_sum1 += size;
    tb.time_sum1 += t;

}

void update2(double t, size_t size) {
    tb.calls2++;
    if (t > tb.time_max2) {
        tb.time_max2 = t;
    }
    if (t < tb.time_min2) {
        tb.time_min2 = t;
    }
    if (size > tb.size_max2) {
        tb.size_max2 = size;
    }
    if (size < tb.size_min2) {
        tb.size_min2 = size;
    }
    tb.size_sum2 += size;
    tb.time_sum2 += t;

}


void update4(double t, size_t size) {
    tb.calls4++;
    if (t > tb.time_max4) {
        tb.time_max4 = t;
    }
    if (t < tb.time_min4) {
        tb.time_min4 = t;
    }
    if (size > tb.size_max4) {
        tb.size_max4 = size;
    }
    if (size < tb.size_min4) {
        tb.size_min4 = size;
    }
    tb.size_sum4 += size;
    tb.time_sum4 += t;

}


void update8(double t, size_t size) {
    tb.calls8++;
    if (t > tb.time_max8) {
        tb.time_max8 = t;
    }
    if (t < tb.time_min8) {
        tb.time_min8 = t;
    }
    if (size > tb.size_max8) {
        tb.size_max8 = size;
    }
    if (size < tb.size_min8) {
        tb.size_min8 = size;
    }
    tb.size_sum8 += size;
    tb.time_sum8 += t;

}

void update16(double t, size_t size) {
    tb.calls16++;
    if (t > tb.time_max16) {
        tb.time_max16 = t;
    }
    if (t < tb.time_min16) {
        tb.time_min16 = t;
    }
    if (size > tb.size_max16) {
        tb.size_max16 = size;
    }
    if (size < tb.size_min16) {
        tb.size_min16 = size;
    }
    tb.size_sum16 += size;
    tb.time_sum16 += t;

}

void update32(double t, size_t size) {
    tb.calls32++;
    if (t > tb.time_max32) {
        tb.time_max32 = t;
    }
    if (t < tb.time_min32) {
        tb.time_min32 = t;
    }
    if (size > tb.size_max32) {
        tb.size_max32 = size;
    }
    if (size < tb.size_min32) {
        tb.size_min32 = size;
    }
    tb.size_sum32 += size;
    tb.time_sum32 += t;

}

void update64(double t, size_t size) {
    tb.calls64++;
    if (t > tb.time_max64) {
        tb.time_max64 = t;
    }
    if (t < tb.time_min64) {
        tb.time_min64 = t;
    }
    if (size > tb.size_max64) {
        tb.size_max64 = size;
    }
    if (size < tb.size_min64) {
        tb.size_min64 = size;
    }
    tb.size_sum64 += size;
    tb.time_sum64 += t;

}

void updatebig(double t, size_t size) {
    tb.callsbig++;
    if (t > tb.time_maxbig) {
        tb.time_maxbig = t;
    }
    if (t < tb.time_minbig) {
        tb.time_minbig = t;
    }
    if (size > tb.size_maxbig) {
        tb.size_maxbig = size;
    }
    if (size < tb.size_minbig) {
        tb.size_minbig = size;
    }
    tb.size_sumbig += size;
    tb.time_sumbig += t;

}

void update_free(double t, size_t size) {
    tb.calls_free++;
    if (t > tb.time_max_free) {
        tb.time_max_free = t;
    }
    if (t < tb.time_min_free) {
        tb.time_min_free = t;
    }
    if (size > tb.size_max_free) {
        tb.size_max_free = size;
    }
    if (size < tb.size_min_free) {
        tb.size_min_free = size;
    }
    tb.size_sum_free += size;
    tb.time_sum_free += t;


}

void update_calloc(double t, size_t size) {
    tb.calls_calloc++;
    if (t > tb.time_max_calloc) {
        tb.time_max_calloc = t;
    }
    if (t < tb.time_min_calloc) {
        tb.time_min_calloc = t;
    }
    if (size > tb.size_max_calloc) {
        tb.size_max_calloc = size;
    }
    if (size < tb.size_min_calloc) {
        tb.size_min_calloc = size;
    }
    tb.size_sum_calloc += size;
    tb.time_sum_calloc += t;


}

void update_valloc(double t, size_t size) {
    tb.calls_valloc++;
    if (t > tb.time_max_valloc) {
        tb.time_max_valloc = t;
    }
    if (t < tb.time_min_valloc) {
        tb.time_min_valloc = t;
    }
    if (size > tb.size_max_valloc) {
        tb.size_max_valloc = size;
    }
    if (size < tb.size_min_valloc) {
        tb.size_min_valloc = size;
    }
    tb.size_sum_valloc += size;
    tb.time_sum_valloc += t;


}

void update_realloc(double t, size_t size) {
    tb.calls_realloc++;
    if (t > tb.time_max_realloc) {
        tb.time_max_realloc = t;
    }
    if (t < tb.time_min_realloc) {
        tb.time_min_realloc = t;
    }
    if (size > tb.size_max_realloc) {
        tb.size_max_realloc = size;
    }
    if (size < tb.size_min_realloc) {
        tb.size_min_realloc = size;
    }
    tb.size_sum_realloc += size;
    tb.time_sum_realloc += t;


}

void update_mem(double t, size_t size) {
    tb.calls_mem++;
    if (t > tb.time_max_mem) {
        tb.time_max_mem = t;
    }
    if (t < tb.time_min_mem) {
        tb.time_min_mem = t;
    }
    if (size > tb.size_max_mem) {
        tb.size_max_mem = size;
    }
    if (size < tb.size_min_mem) {
        tb.size_min_mem = size;
    }
    tb.size_sum_mem += size;
    tb.time_sum_mem += t;


}



// ===================================== warp malloc, free, realloc ========================================




struct segTree {
    int *tree;
    int seg_size;
    int N;
    long *start_address;
    long *end_address;
};

#define lengthNum  13
#define maxEdgeListSize 1000


int bindLength[lengthNum] = {1 << 4, 1 << 5, 1 << 6, 1 << 7, 1 << 8, 1 << 9, 1 << 10, 1 << 11, 1 << 12,
                             1 << 13, 1 << 14, 1 << 15, 1 << 16};

#ifdef use_std_malloc
int initListsSize[lengthNum] = {8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8};
#else
int initListsSize[lengthNum] = {8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8};
#endif

int hasInit = 0;

struct segTree *lists[lengthNum][maxEdgeListSize];
int listsSize[lengthNum];

int nextListsSize[lengthNum];

#define use_tree

#ifdef use_tree

struct segTree *buildSetTree(int bind_length, int seg_size) {
    void *new_address = malloc(bind_length * seg_size);
    struct segTree *now_tree = (struct segTree *) malloc(sizeof(struct segTree));
    now_tree->start_address = (long *) new_address;
    //now_tree->end_address = (long *)((char *)new_address + bind_length * seg_size);
    long p1 = now_tree->start_address;
    now_tree->end_address = (long *) (p1 + 1ll * bind_length * seg_size);
    //int real_size = 1 << __lg(seg_size + 5) + 1;
    //TODO
    now_tree->N = seg_size - 1;
    now_tree->seg_size = seg_size;
    int tree_size = seg_size << 1;
    now_tree->tree = (int *) malloc(tree_size * sizeof(int));
    for (int i = 1; i <= seg_size; ++i)
        now_tree->tree[i + now_tree->N] = 0;
    for (int i = now_tree->N; i; --i)
        now_tree->tree[i] = now_tree->tree[i << 1] + now_tree->tree[i << 1 | 1];
    return now_tree;
}

void updateTree(struct segTree *now_tree, int pos, int value) {
    for (int i = pos + now_tree->N; i; i >>= 1)
        now_tree->tree[i] += value;
}

int queryOne(struct segTree *now_tree, int pos) {
    return now_tree->tree[pos + now_tree->N];
}

int queryTree(struct segTree *now_tree) {
    int l = 1, r = now_tree->seg_size;
    assert(now_tree->tree[1] <= now_tree->seg_size);
    if (now_tree->tree[1] == now_tree->seg_size) return -1;
    for (int i = 1; l != r;) {
        int mid = (l + r) / 2;
        //TODO
        if (now_tree->tree[i << 1] < (mid - l + 1)) {
            r = mid;
            i = i << 1;
        } else if (now_tree->tree[i << 1 | 1] < (r - mid)) {
            l = mid + 1;
            i = i << 1 | 1;
        } else {
            assert(0);
        }
    }
    return l;
}

#else
struct segTree *buildSetTree(int bind_length, int seg_size) {
    void *new_address = malloc(bind_length * seg_size);
    assert(new_address);
    struct segTree *now_tree = (struct segTree *) malloc(sizeof(struct segTree));
    now_tree->start_address = (long *) new_address;
    long p1 = now_tree->start_address;

    now_tree->end_address = (long *)(p1 + bind_length * seg_size);
    now_tree->seg_size = seg_size;
    int tree_size = seg_size << 1;
    now_tree->tree = (int *) malloc(tree_size * sizeof(int));
    for(int i = 0; i < tree_size; i++) now_tree->tree[i] = 999;
    for (int i = 1; i <= seg_size; ++i)
        now_tree->tree[i] = 0;
    return now_tree;
}

void updateTree(struct segTree *now_tree, int pos, int value) {
    now_tree->tree[pos] += value;
}

int queryOne(struct segTree *now_tree, int pos) {
    return now_tree->tree[pos];
}

int queryTree(struct segTree *now_tree) {
    int res = -1;
    for(int i = 1; i <= now_tree->seg_size; i++) {
        if(now_tree->tree[i] == 0) {
            res = i;
            break;
        }
    }
    return res;
}
#endif

void init() {
    for (int i = 0; i < lengthNum; i++) {
        listsSize[i] = 0;
        struct segTree *now_tree = buildSetTree(bindLength[i], initListsSize[i]);
        lists[i][listsSize[i]++] = now_tree;
        nextListsSize[i] = initListsSize[i] << 1;
    }
    hasInit = 1;
}

void *getAddress(struct segTree *now_tree, int bind_length, int seg_pos) {
    assert(seg_pos >= 1);
    long p1 = now_tree->start_address;
    long add_len = bind_length * (seg_pos - 1);
    return (void *) (p1 + add_len);
}

int getSegPos(struct segTree *now_tree, int bind_length, void *free_address) {
    long p1 = (long *) free_address;
    long p2 = now_tree->start_address;
    return (p1 - p2) / bind_length + 1;
}

void *mannual_malloc(size_t size) {

    if (hasInit == 0) init();

    int length_type = (int) ceil(log2(size)) - 4;

    assert(length_type >= 0 && length_type < 13);

    int now_tree_num = listsSize[length_type];
    int find_pos = -1;
    int first_zero_pos = -1;
    for (int i = 0; i < now_tree_num; i++) {
        struct segTree *now_tree = lists[length_type][i];
        first_zero_pos = queryTree(now_tree);
        if (first_zero_pos != -1) {
            find_pos = i;
            break;
        }
    }

    // The trees that are already occupied, open a new one
    if (find_pos == -1) {
//        printf("GG no node\n");
        struct segTree *now_tree = buildSetTree(bindLength[length_type], nextListsSize[length_type]);
        nextListsSize[length_type] = nextListsSize[length_type] << 1;
        lists[length_type][listsSize[length_type]++] = now_tree;
        find_pos = listsSize[length_type] - 1;
        first_zero_pos = 1;
    }

    assert(find_pos != -1 && first_zero_pos != -1);

    // Find the first_zero_pos of the find_pos tree which is 0, return this address and modify the tree
    void *res_address = getAddress(lists[length_type][find_pos], bindLength[length_type], first_zero_pos);
    assert(queryOne(lists[length_type][find_pos], first_zero_pos) == 0);
    updateTree(lists[length_type][find_pos], first_zero_pos, 1);

    return res_address;
}


void *my_malloc(size_t size) {
    double t, t1, t2;
    if (timer_flag) {
        t1 = get_time();
    }

    void *ptr;
#ifdef use_std_malloc
    ptr = malloc(size);
#else
    if (size > (1 << 16) || size <= 8)
        ptr = malloc(size);
    else ptr = mannual_malloc(size);
#endif
    assert(ptr);

    if (timer_flag) {
        t2 = get_time();
        t = t2 - t1;

        update(t, size);
        if (size <= 32) {
            update1(t, size);
        } else if (size <= 128) {
            update2(t, size);
        } else if (size <= 512) {
            update4(t, size);
        } else if (size <= 2048) {
            update8(t, size);
        } else if (size <= 1024 * 8) {
            update16(t, size);
        } else if (size <= 1024 * 32) {
            update32(t, size);
        } else if (size <= 64 * 1024) {
            update64(t, size);
        } else {
            updatebig(t, size);
        }
    }

    return ptr;
}


void my_free(void *ptr) {

#ifdef use_std_malloc

    double t, t1, t2;
    if(timer_flag) {
        t1 = get_time();
    }

    free(ptr);

    if(timer_flag) {
        t2 = get_time();
        t = t2 - t1;
        
        update_free(t, 0);
    }

#else

    double t, t1, t2;
    if (timer_flag) {
        t1 = get_time();
    }

    //init
    if (hasInit == 0) init();

    //TODO
    int pos1 = -1;
    int pos2 = -1;
    for (int i = 0; i < lengthNum; i++) {
        int now_tree_num = listsSize[i];
        for (int j = 0; j < now_tree_num; j++) {
            long p1 = lists[i][j]->start_address;
            long p2 = lists[i][j]->end_address;
            long p3 = ptr;
            if (p3 >= p1 && p3 < p2) {
                pos1 = i;
                pos2 = j;
                break;
            }
        }
        if (pos1 != -1) break;
    }
    if (pos1 == -1 && pos2 == -1) {
        free(ptr);
        if (timer_flag) {
            t2 = get_time();
            t = t2 - t1;

            update_free(t, 0);
        }
        return;
    }
    int free_seg_pos = getSegPos(lists[pos1][pos2], bindLength[pos1], ptr);
    //TODO free 2?
    assert(queryOne(lists[pos1][pos2], free_seg_pos) == 1);
    updateTree(lists[pos1][pos2], free_seg_pos, -1);
    if (timer_flag) {
        t2 = get_time();
        t = t2 - t1;

        update_free(t, 0);
    }
#endif
}

void *my_realloc(void *ptr, size_t size) {

#ifdef use_std_malloc

    double t, t1, t2;
    if(timer_flag) {
        t1 = get_time();
    }

    void* res = realloc(ptr, size);

    if(timer_flag) {
        t2 = get_time();
            t = t2 - t1;
        
        update_realloc(t, size);
    }
    return res;

#else
    double t, t1, t2;
    if (timer_flag) {
        t1 = get_time();
    }


    if (hasInit == 0) init();

    if (ptr == NULL && size == 0) {
        void *res = realloc(ptr, size);
        if (timer_flag) {
            t2 = get_time();
            t = t2 - t1;

            update_realloc(t, size);
        }
        return res;
    }

    //if(ptr == NULL) return malloc(size);
    if (ptr == NULL) {
        void *res = my_malloc(size);
        if (timer_flag) {
            t2 = get_time();
            t = t2 - t1;

            update_realloc(t, size);
        }
        return res;
    }

    if (size == 0) {
        my_free(ptr);
        if (timer_flag) {
            t2 = get_time();
            t = t2 - t1;

            update_realloc(t, size);
        }
        return NULL;
    }

    int pos1 = -1;
    int pos2 = -1;
    for (int i = 0; i < lengthNum; i++) {
        int now_tree_num = listsSize[i];
        for (int j = 0; j < now_tree_num; j++) {
            long p1 = lists[i][j]->start_address;
            long p2 = lists[i][j]->end_address;
            long p3 = ptr;
            if (p3 >= p1 && p3 < p2) {
                pos1 = i;
                pos2 = j;
                break;
            }
        }
        if (pos1 != -1) break;
    }

    if (pos1 == -1 && pos2 == -1) {
        void *res = realloc(ptr, size);
        if (timer_flag) {
            t2 = get_time();
            t = t2 - t1;

            update_realloc(t, size);
        }
        return res;
    }

    size_t old_size = bindLength[pos1];
    if (size > old_size) {
        void *new_ptr = malloc(size);
        memcpy(new_ptr, ptr, old_size);
        int free_seg_pos = getSegPos(lists[pos1][pos2], bindLength[pos1], ptr);
        //TODO free 2?
        assert(queryOne(lists[pos1][pos2], free_seg_pos) == 1);
        updateTree(lists[pos1][pos2], free_seg_pos, -1);
        if (timer_flag) {
            t2 = get_time();
            t = t2 - t1;

            update_realloc(t, size);
        }
        return new_ptr;
    } else {
        if (timer_flag) {
            t2 = get_time();
            t = t2 - t1;

            update_realloc(t, size);
        }
        return ptr;
    }

#endif
}

void *my_calloc(size_t nmemb, size_t size) {
    double t, t1, t2;
    if (timer_flag) {
        t1 = get_time();
    }

    void *ptr = calloc(nmemb, size);
    assert(ptr);
    if (timer_flag) {
        t2 = get_time();
        t = t2 - t1;

        update_calloc(t, nmemb * size);
    }

    return ptr;
}

char *my_strdup(const char *s) {
    char *copy = my_malloc(strlen(s) + 1);
    if (copy) {
        strcpy(copy, s);
    }
    return copy;
}

void htmalloccount_init() {
    char folder[256] = "htmalloccount_data";
    if (access(folder, F_OK) == -1) {
        mkdir(folder, 0777);
    }

    tb.calls = 0;
    tb.time_avg = 0;
    tb.time_max = 0;
    tb.time_min = 99999999.0;
    tb.size_avg = 0;
    tb.size_max = 0;
    tb.size_min = 99999999.0;
    tb.size_sum = 0;
    tb.time_sum = 0;


    tb.calls1 = 0;
    tb.time_avg1 = 0;
    tb.time_max1 = 0;
    tb.time_min1 = 99999999.0;
    tb.size_avg1 = 0;
    tb.size_max1 = 0;
    tb.size_min1 = 99999999.0;
    tb.size_sum1 = 0;
    tb.time_sum1 = 0;


    tb.calls2 = 0;
    tb.time_avg2 = 0;
    tb.time_max2 = 0;
    tb.time_min2 = 99999999.0;
    tb.size_avg2 = 0;
    tb.size_max2 = 0;
    tb.size_min2 = 99999999.0;
    tb.size_sum2 = 0;
    tb.time_sum2 = 0;


    tb.calls4 = 0;
    tb.time_avg4 = 0;
    tb.time_max4 = 0;
    tb.time_min4 = 99999999.0;
    tb.size_avg4 = 0;
    tb.size_max4 = 0;
    tb.size_min4 = 99999999.0;
    tb.size_sum4 = 0;
    tb.time_sum4 = 0;


    tb.calls8 = 0;
    tb.time_avg8 = 0;
    tb.time_max8 = 0;
    tb.time_min8 = 99999999.0;
    tb.size_avg8 = 0;
    tb.size_max8 = 0;
    tb.size_min8 = 99999999.0;
    tb.size_sum8 = 0;
    tb.time_sum8 = 0;


    tb.calls16 = 0;
    tb.time_avg16 = 0;
    tb.time_max16 = 0;
    tb.time_min16 = 99999999.0;
    tb.size_avg16 = 0;
    tb.size_max16 = 0;
    tb.size_min16 = 99999999.0;
    tb.size_sum16 = 0;
    tb.time_sum16 = 0;


    tb.calls32 = 0;
    tb.time_avg32 = 0;
    tb.time_max32 = 0;
    tb.time_min32 = 99999999.0;
    tb.size_avg32 = 0;
    tb.size_max32 = 0;
    tb.size_min32 = 99999999.0;
    tb.size_sum32 = 0;
    tb.time_sum32 = 0;


    tb.calls64 = 0;
    tb.time_avg64 = 0;
    tb.time_max64 = 0;
    tb.time_min64 = 99999999.0;
    tb.size_avg64 = 0;
    tb.size_max64 = 0;
    tb.size_min64 = 99999999.0;
    tb.size_sum64 = 0;
    tb.time_sum64 = 0;


    tb.callsbig = 0;
    tb.time_avgbig = 0;
    tb.time_maxbig = 0;
    tb.time_minbig = 99999999.0;
    tb.size_avgbig = 0;
    tb.size_maxbig = 0;
    tb.size_minbig = 99999999.0;
    tb.size_sumbig = 0;
    tb.time_sumbig = 0;


    tb.calls_free = 0;
    tb.time_avg_free = 0;
    tb.time_max_free = 0;
    tb.time_min_free = 99999999.0;
    tb.size_avg_free = 0;
    tb.size_max_free = 0;
    tb.size_min_free = 99999999.0;
    tb.size_sum_free = 0;
    tb.time_sum_free = 0;


    tb.calls_calloc = 0;
    tb.time_avg_calloc = 0;
    tb.time_max_calloc = 0;
    tb.time_min_calloc = 99999999.0;
    tb.size_avg_calloc = 0;
    tb.size_max_calloc = 0;
    tb.size_min_calloc = 99999999.0;
    tb.size_sum_calloc = 0;
    tb.time_sum_calloc = 0;


    tb.calls_valloc = 0;
    tb.time_avg_valloc = 0;
    tb.time_max_valloc = 0;
    tb.time_min_valloc = 99999999.0;
    tb.size_avg_valloc = 0;
    tb.size_max_valloc = 0;
    tb.size_min_valloc = 99999999.0;
    tb.size_sum_valloc = 0;
    tb.time_sum_valloc = 0;


    tb.calls_realloc = 0;
    tb.time_avg_realloc = 0;
    tb.time_max_realloc = 0;
    tb.time_min_realloc = 99999999.0;
    tb.size_avg_realloc = 0;
    tb.size_max_realloc = 0;
    tb.size_min_realloc = 99999999.0;
    tb.size_sum_realloc = 0;
    tb.time_sum_realloc = 0;


    tb.calls_mem = 0;
    tb.time_avg_mem = 0;
    tb.time_max_mem = 0;
    tb.time_min_mem = 99999999.0;
    tb.size_avg_mem = 0;
    tb.size_max_mem = 0;
    tb.size_min_mem = 99999999.0;
    tb.size_sum_mem = 0;
    tb.time_sum_mem = 0;


    //timer_flag = 1;
}

long cal(int id, int n) {
    long res = 0;
    long now = initListsSize[id];
    for (int i = 0; i < n; i++) {
        res += now;
        now = now << 1;
    }
    return res;
}

void htmalloccount_print() {

    if(tb.calls) tb.time_avg = 1.0 * tb.time_sum / tb.calls, tb.size_avg = 1.0 * tb.size_sum / tb.calls; else tb.time_min = 0, tb.size_min = 0;
    if(tb.calls1) tb.time_avg1 = 1.0 * tb.time_sum1 / tb.calls1, tb.size_avg1 = 1.0 * tb.size_sum1 / tb.calls1; else tb.time_min1 = 0, tb.size_min1 = 0;
    if(tb.calls2) tb.time_avg2 = 1.0 * tb.time_sum2 / tb.calls2, tb.size_avg2 = 1.0 * tb.size_sum2 / tb.calls2; else tb.time_min2 = 0, tb.size_min2 = 0;
    if(tb.calls4) tb.time_avg4 = 1.0 * tb.time_sum4 / tb.calls4, tb.size_avg4 = 1.0 * tb.size_sum4 / tb.calls4; else tb.time_min4 = 0, tb.size_min4 = 0;
    if(tb.calls8) tb.time_avg8 = 1.0 * tb.time_sum8 / tb.calls8, tb.size_avg8 = 1.0 * tb.size_sum8 / tb.calls8; else tb.time_min8 = 0, tb.size_min8 = 0;
    if(tb.calls16) tb.time_avg16 = 1.0 * tb.time_sum16 / tb.calls16, tb.size_avg16 = 1.0 * tb.size_sum16 / tb.calls16; else tb.time_min16 = 0, tb.size_min16 = 0;
    if(tb.calls32) tb.time_avg32 = 1.0 * tb.time_sum32 / tb.calls32, tb.size_avg32 = 1.0 * tb.size_sum32 / tb.calls32; else tb.time_min32 = 0, tb.size_min32 = 0;
    if(tb.calls64) tb.time_avg64 = 1.0 * tb.time_sum64 / tb.calls64, tb.size_avg64 = 1.0 * tb.size_sum64 / tb.calls64; else tb.time_min64 = 0, tb.size_min64 = 0;
    if(tb.callsbig) tb.time_avgbig = 1.0 * tb.time_sumbig / tb.callsbig, tb.size_avgbig = 1.0 * tb.size_sumbig / tb.callsbig; else tb.time_minbig = 0, tb.size_minbig = 0;
    if(tb.calls_calloc) tb.time_avg_calloc = 1.0 * tb.time_sum_calloc / tb.calls_calloc, tb.size_avg_calloc = 1.0 * tb.size_sum_calloc / tb.calls_calloc; else tb.time_min_calloc = 0, tb.size_min_calloc = 0;
    if(tb.calls_free) tb.time_avg_free = 1.0 * tb.time_sum_free / tb.calls_free, tb.size_avg_free = 1.0 * tb.size_sum_free / tb.calls_free; else tb.time_min_free = 0, tb.size_min_free = 0;
    if(tb.calls_mem) tb.time_avg_mem = 1.0 * tb.time_sum_mem / tb.calls_mem, tb.size_avg_mem = 1.0 * tb.size_sum_mem / tb.calls_mem; else tb.time_min_mem = 0, tb.size_min_mem = 0;
    if(tb.calls_realloc) tb.time_avg_realloc = 1.0 * tb.time_sum_realloc / tb.calls_realloc, tb.size_avg_realloc = 1.0 * tb.size_sum_realloc / tb.calls_realloc; else tb.time_min_realloc = 0, tb.size_min_realloc = 0;
    if(tb.calls_valloc) tb.time_avg_valloc = 1.0 * tb.time_sum_valloc / tb.calls_valloc, tb.size_avg_valloc = 1.0 * tb.size_sum_valloc / tb.calls_valloc; else tb.time_min_valloc = 0, tb.size_min_valloc = 0;



    int my_mpi_id;
    char filename[256];
    FILE *fp;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_mpi_id);

    sprintf(filename, "htmalloccount_data/htmalloccount_rank%06d.dat", my_mpi_id);
    fp = fopen(filename, "w");


    fprintf(fp, "%-13s%-15s%-18s%-18s%-18s%-18s%-18s%-18s%-18s\n", "TYPE", "CALLED", "TIME_TOT", "TIME_AVG", "TIME_MAX", "TIME_MIN",
            "SIZE_AVG", "SIZE_MAX", "SIZE_MIN");

    fprintf(fp, "%-13s%15zu%18.6e%18.6e%18.6e%18.6e%18.6e%18zu%18zu\n", "all", tb.calls, tb.time_sum, tb.time_avg, tb.time_max,
            tb.time_min, tb.size_avg, tb.size_max, tb.size_min);

    fprintf(fp, "%-13s%15zu%18.6e%18.6e%18.6e%18.6e%18.6e%18zu%18zu\n", "(8B, 32B]", tb.calls1, tb.time_sum1, tb.time_avg1, tb.time_max1,
            tb.time_min1, tb.size_avg1, tb.size_max1, tb.size_min1);

    fprintf(fp, "%-13s%15zu%18.6e%18.6e%18.6e%18.6e%18.6e%18zu%18zu\n", "(32B, 128B]", tb.calls2, tb.time_sum2, tb.time_avg2, tb.time_max2,
            tb.time_min2, tb.size_avg2, tb.size_max2, tb.size_min2);

    fprintf(fp, "%-13s%15zu%18.6e%18.6e%18.6e%18.6e%18.6e%18zu%18zu\n", "(128B, 512B]", tb.calls4, tb.time_sum4, tb.time_avg4, tb.time_max4,
            tb.time_min4, tb.size_avg4, tb.size_max4, tb.size_min4);

    fprintf(fp, "%-13s%15zu%18.6e%18.6e%18.6e%18.6e%18.6e%18zu%18zu\n", "(512B, 2k]", tb.calls8, tb.time_sum8, tb.time_avg8, tb.time_max8,
            tb.time_min8, tb.size_avg8, tb.size_max8, tb.size_min8);

    fprintf(fp, "%-13s%15zu%18.6e%18.6e%18.6e%18.6e%18.6e%18zu%18zu\n", "(2k, 8k]", tb.calls16, tb.time_sum16, tb.time_avg16, tb.time_max16,
            tb.time_min16, tb.size_avg16, tb.size_max16, tb.size_min16);

    fprintf(fp, "%-13s%15zu%18.6e%18.6e%18.6e%18.6e%18.6e%18zu%18zu\n", "(8k, 32k]", tb.calls32, tb.time_sum32, tb.time_avg32, tb.time_max32,
            tb.time_min32, tb.size_avg32, tb.size_max32, tb.size_min32);

    fprintf(fp, "%-13s%15zu%18.6e%18.6e%18.6e%18.6e%18.6e%18zu%18zu\n", "(32k, 64k]", tb.calls64, tb.time_sum64, tb.time_avg64, tb.time_max64,
            tb.time_min64, tb.size_avg64, tb.size_max64, tb.size_min64);

    fprintf(fp, "%-13s%15zu%18.6e%18.6e%18.6e%18.6e%18.6e%18zu%18zu\n", "[64k, -]", tb.callsbig, tb.time_sumbig, tb.time_avgbig, tb.time_maxbig,
            tb.time_minbig, tb.size_avgbig, tb.size_maxbig, tb.size_minbig);

    fprintf(fp, "%-13s%15zu%18.6e%18.6e%18.6e%18.6e%18.6e%18zu%18zu\n", "free", tb.calls_free, tb.time_sum_free, tb.time_avg_free,
            tb.time_max_free, tb.time_min_free, tb.size_avg_free, tb.size_max_free, tb.size_min_free);

    fprintf(fp, "%-13s%15zu%18.6e%18.6e%18.6e%18.6e%18.6e%18zu%18zu\n", "calloc", tb.calls_calloc, tb.time_sum_calloc, tb.time_avg_calloc,
            tb.time_max_calloc, tb.time_min_calloc, tb.size_avg_calloc, tb.size_max_calloc, tb.size_min_calloc);

    fprintf(fp, "%-13s%15zu%18.6e%18.6e%18.6e%18.6e%18.6e%18zu%18zu\n", "valloc", tb.calls_valloc, tb.time_sum_valloc, tb.time_avg_valloc,
            tb.time_max_valloc, tb.time_min_valloc, tb.size_avg_valloc, tb.size_max_valloc, tb.size_min_valloc);

    fprintf(fp, "%-13s%15zu%18.6e%18.6e%18.6e%18.6e%18.6e%18zu%18zu\n", "realloc", tb.calls_realloc, tb.time_sum_realloc, tb.time_avg_realloc,
            tb.time_max_realloc, tb.time_min_realloc, tb.size_avg_realloc, tb.size_max_realloc, tb.size_min_realloc);

    fprintf(fp, "%-13s%15zu%18.6e%18.6e%18.6e%18.6e%18.6e%18zu%18zu\n", "mem", tb.calls_mem, tb.time_sum_mem, tb.time_avg_mem,
            tb.time_max_mem, tb.time_min_mem, tb.size_avg_mem, tb.size_max_mem, tb.size_min_mem);


    fprintf(fp, "list size info : \n");
    fprintf(fp, "(8B ,16B]      == list size %8d  tot %12d ele\n", listsSize[0], cal(0, listsSize[0]));
    fprintf(fp, "(16B ,32B]     == list size %8d  tot %12d ele\n", listsSize[1], cal(1, listsSize[1]));
    fprintf(fp, "(32B ,64B]     == list size %8d  tot %12d ele\n", listsSize[2], cal(2, listsSize[2]));
    fprintf(fp, "(64B ,128B]    == list size %8d  tot %12d ele\n", listsSize[3], cal(3, listsSize[3]));
    fprintf(fp, "(128B ,256B]   == list size %8d  tot %12d ele\n", listsSize[4], cal(4, listsSize[4]));
    fprintf(fp, "(256B ,512B]   == list size %8d  tot %12d ele\n", listsSize[5], cal(5, listsSize[5]));
    fprintf(fp, "(512B ,1K]     == list size %8d  tot %12d ele\n", listsSize[6], cal(6, listsSize[6]));
    fprintf(fp, "(1K ,2K]       == list size %8d  tot %12d ele\n", listsSize[7], cal(7, listsSize[7]));
    fprintf(fp, "(2K ,4K]       == list size %8d  tot %12d ele\n", listsSize[8], cal(8, listsSize[8]));
    fprintf(fp, "(4K ,8K]       == list size %8d  tot %12d ele\n", listsSize[9], cal(9, listsSize[9]));
    fprintf(fp, "(8K ,16K]      == list size %8d  tot %12d ele\n", listsSize[10], cal(10, listsSize[10]));
    fprintf(fp, "(16K ,32K]     == list size %8d  tot %12d ele\n", listsSize[11], cal(11, listsSize[11]));
    fprintf(fp, "(32K ,64K]     == list size %8d  tot %12d ele\n", listsSize[12], cal(12, listsSize[12]));
    //fprintf(fp, "(64K ,-]       == list size %8d  tot %12d ele\n", listsSize[13], cal(listsSize[13]));

    //fprintf(fp, "align address infp : \n");

    //for(int i = 0; i < test_address_size; i++) {
    //    fprintf(fp, "%p   ",valloc_addss[i]);
    //}
    //fprintf(fp, "\n\n");

    //fprintf(fp, "align address infp : \n");
    //for(int i = 0; i < test_address_size; i++) {
    //    fprintf(fp, "%p   ", memalign_addss[i]);
    //}
    //fprintf(fp, "\n\n");



    fclose(fp);
}


void *wrap_calloc(size_t nmemb, size_t size,
                  const char *file, unsigned int line, const char *func) {
    void *p = my_calloc(nmemb, size);
    if (NULL == p) {
        fprintf(stderr,
                "[%s] Failed to allocate %zu bytes at %s line %u: %s\n",
                func, nmemb * size, file, line, strerror(errno));
        exit(EXIT_FAILURE);
    }
    return p;
}

void *wrap_malloc(size_t size,
                  const char *file, unsigned int line, const char *func) {
    void *p = my_malloc(size);
    if (NULL == p) {
        fprintf(stderr,
                "[%s] Failed to allocate %zu bytes at %s line %u: %s\n",
                func, size, file, line, strerror(errno));
        exit(EXIT_FAILURE);
    }
    return p;
}

void *wrap_realloc(void *ptr, size_t size,
                   const char *file, unsigned int line, const char *func) {
    void *p = my_realloc(ptr, size);
    if (NULL == p) {
        fprintf(stderr,
                "[%s] Failed to allocate %zu bytes at %s line %u: %s\n",
                func, size, file, line, strerror(errno));
        exit(EXIT_FAILURE);
    }
    return p;
}

char *wrap_strdup(const char *s,
                  const char *file, unsigned int line, const char *func) {
    char *p = my_strdup(s);
    if (NULL == p) {
        fprintf(stderr,
                "[%s] Failed to allocate %zu bytes at %s line %u: %s\n",
                func, strlen(s), file, line, strerror(errno));
        exit(EXIT_FAILURE);
    }
    return p;
}

void wrap_free(void *ptr, const char *file, unsigned int line, const char *func) {
    if (ptr != NULL) {
        my_free(ptr);
//        fprintf(stderr, "[%s] Freed memory at %s line %u\n", func, file, line);
    } else {
//        fprintf(stderr, "[%s] Attempt to free NULL pointer at %s line %u\n", func, file, line);
    }
}



