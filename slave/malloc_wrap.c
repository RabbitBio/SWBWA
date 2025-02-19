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

#include <assert.h>

#include <slave.h>

//#define global_val_attr __thread
#define global_val_attr


#define assert(x)

//#define use_std_malloc

// ===================================== warp malloc, free, realloc ========================================




struct segTree {
    int *tree;
    int seg_size;
    int N;
    long *start_address;
    long *end_address;
};

#define lengthNum  17
#define maxEdgeListSize 1000


int bindLength[lengthNum] = {1 << 2, 1 << 3, 1 << 4, 1 << 5, 1 << 6, 1 << 7, 1 << 8, 1 << 9, 1 << 10, 1 << 11, 1 << 12, 1 << 13, 1 << 14, 1 << 15, 1 << 16, 1 << 17, 1 << 18};

#ifdef use_std_malloc
int initListsSize[lengthNum] = {8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8};
#else
int initListsSize[lengthNum] = {8, 8, 8, 32, 8, 1024, 32, 8192, 128, 8, 8, 8, 8, 8, 8, 8, 8};
#endif

global_val_attr int hasInit[384] = {0};
global_val_attr struct segTree *lists[384][lengthNum][maxEdgeListSize];
global_val_attr int listsSize[384][lengthNum];
global_val_attr int nextListsSize[384][lengthNum];


//global_val_attr int hasInit;
//global_val_attr struct segTree *lists[lengthNum][maxEdgeListSize];
//global_val_attr int listsSize[lengthNum];
//global_val_attr int nextListsSize[lengthNum];


#define hasInit hasInit[_MYID]
#define lists lists[_MYID]
#define listsSize listsSize[_MYID]
#define nextListsSize nextListsSize[_MYID]

#define use_tree

#ifdef use_tree

#define MAX_THREADS 384

char* big_buffer[MAX_THREADS];
long long now_pos[MAX_THREADS];
long long tot_size[MAX_THREADS];

void set_big_buffer(char* buffer, long long t_size) {
    big_buffer[_MYID] = buffer + _MYID * t_size;
    now_pos[_MYID] = 0;
    tot_size[_MYID] = t_size;
    //printf("cpe %d set %lld, %p\n", _MYID, tot_size[_MYID], big_buffer[_MYID]);
}

void *l_calloc(size_t nmemb, size_t size) {
    //return calloc(nmemb, size);
    size_t t_size = nmemb * size;
    if (now_pos[_MYID] + t_size > tot_size[_MYID]) {
        printf("ERROR: Out of memory %lld %lld\n", now_pos[_MYID] + t_size, tot_size[_MYID]);
        exit(EXIT_FAILURE);
    }
    void *ptr = big_buffer[_MYID] + now_pos[_MYID];
    now_pos[_MYID] += t_size;
    memset(ptr, 0, t_size);
    return ptr;
}



__uncached long lock_s;
struct segTree *buildSetTree(int bind_length, int seg_size) {
    void *new_address = l_calloc(bind_length * seg_size, 1);
    struct segTree *now_tree = (struct segTree *) l_calloc(sizeof(struct segTree), 1);
    now_tree->start_address = (long *) new_address;
    //now_tree->end_address = (long *)((char *)new_address + bind_length * seg_size);
    long p1 = now_tree->start_address;
    now_tree->end_address = (long *) (p1 + 1ll * bind_length * seg_size);
    //int real_size = 1 << __lg(seg_size + 5) + 1;
    //TODO
    now_tree->N = seg_size - 1;
    now_tree->seg_size = seg_size;
    int tree_size = seg_size << 1;
    now_tree->tree = (int *) l_calloc(tree_size * sizeof(int), 1);
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
    void *new_address = calloc(bind_length * seg_size, 1);
    assert(new_address);
    struct segTree *now_tree = (struct segTree *) calloc(sizeof(struct segTree), 1);
    now_tree->start_address = (long *) new_address;
    long p1 = now_tree->start_address;

    now_tree->end_address = (long *)(p1 + bind_length * seg_size);
    now_tree->seg_size = seg_size;
    int tree_size = seg_size << 1;
    now_tree->tree = (int *) calloc(tree_size * sizeof(int), 1);
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
    long long tot_size = 0;
    for (int i = 0; i < lengthNum; i++) {
        listsSize[i] = 0;
        struct segTree *now_tree = buildSetTree(bindLength[i], initListsSize[i]);
        tot_size += bindLength[i] * initListsSize[i];
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

    int length_type = (int) ceil(log2(size)) - 2;

    assert(length_type >= 0 && length_type < lengthNum);

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
        struct segTree *now_tree = buildSetTree(bindLength[length_type], nextListsSize[length_type]);
        nextListsSize[length_type] = nextListsSize[length_type] << 1;
        lists[length_type][listsSize[length_type]++] = now_tree;
        find_pos = listsSize[length_type] - 1;
        first_zero_pos = 1;
    }

    assert(find_pos != -1 && first_zero_pos != -1);

    // Find the first_zero_pos of the find_pos tree which is 0, return this address and modify the tree
    void *res_address = getAddress(lists[length_type][find_pos], bindLength[length_type], first_zero_pos);
    //TODO
    assert(queryOne(lists[length_type][find_pos], first_zero_pos) == 0);
    updateTree(lists[length_type][find_pos], first_zero_pos, 1);

    return res_address;
}


void *my_malloc(size_t size) {
    if(size < 4) size = 4;

    void *ptr;
#ifdef use_std_malloc
    ptr = malloc(size);
#else
    if (size > (1 << (lengthNum + 1))) {
        printf("malloc size too big: %zu\n", size); 
        ptr = malloc(size);
    }
    else ptr = mannual_malloc(size);
#endif
    return ptr;
}


void my_free(void *ptr) {

#ifdef use_std_malloc
    free(ptr);
#else
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
        return;
    }
    int free_seg_pos = getSegPos(lists[pos1][pos2], bindLength[pos1], ptr);
    //TODO free 2?
    assert(queryOne(lists[pos1][pos2], free_seg_pos) == 1);
    updateTree(lists[pos1][pos2], free_seg_pos, -1);
#endif
}

void *my_realloc(void *ptr, size_t size) {
#ifdef use_std_malloc
    void* res = realloc(ptr, size);
    return res;
#else
    if (hasInit == 0) init();
    //TODO
    if (ptr == NULL && size == 0) {
        //void *res = realloc(ptr, size);
        //if (timer_flag) {
        //    t2 = get_time();
        //    t = t2 - t1;

        //    update_realloc(t, size);
        //}
        //return res;
        return NULL;
    }

    if (ptr == NULL) {
        void *res = my_malloc(size);
        //if (timer_flag) {
        //    t2 = get_time();
        //    t = t2 - t1;

        //    update_realloc(t, size);
        //}
        return res;
    }

    if (size == 0) {
        my_free(ptr);
        //if (timer_flag) {
        //    t2 = get_time();
        //    t = t2 - t1;

        //    update_realloc(t, size);
        //}
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
        printf("realloc GG %p %zu\n", ptr, size);
        void *res = realloc(ptr, size);
        return res;
    }

    size_t old_size = bindLength[pos1];
    if (size > old_size) {
        void *new_ptr = my_malloc(size);
        memcpy(new_ptr, ptr, old_size);
        int free_seg_pos = getSegPos(lists[pos1][pos2], bindLength[pos1], ptr);
        //TODO free 2?
        assert(queryOne(lists[pos1][pos2], free_seg_pos) == 1);
        updateTree(lists[pos1][pos2], free_seg_pos, -1);
        return new_ptr;
    } else {
        return ptr;
    }

#endif
}

void *my_calloc(size_t nmemb, size_t size) {
    void *ptr = calloc(nmemb, size);
    //assert(ptr);
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
    //timer_flag = 1;
    if (hasInit == 0) init();
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

    int my_cpe_id = _MYID;
    char filename[256];
    FILE *fp;

    sprintf(filename, "htmalloccount_data/htmalloccount_rank%06d.dat", my_cpe_id);
    fp = fopen(filename, "w");

    fprintf(fp, "list size info : \n");
    fprintf(fp, "[0B ,4B]       == list size %8d  tot %12d ele\n", listsSize[0], cal(0, listsSize[0]));
    fprintf(fp, "(4B ,8B]       == list size %8d  tot %12d ele\n", listsSize[1], cal(1, listsSize[1]));
    fprintf(fp, "(8B ,16B]      == list size %8d  tot %12d ele\n", listsSize[2], cal(2, listsSize[2]));
    fprintf(fp, "(16B ,32B]     == list size %8d  tot %12d ele\n", listsSize[3], cal(3, listsSize[3]));
    fprintf(fp, "(32B ,64B]     == list size %8d  tot %12d ele\n", listsSize[4], cal(4, listsSize[4]));
    fprintf(fp, "(64B ,128B]    == list size %8d  tot %12d ele\n", listsSize[5], cal(5, listsSize[5]));
    fprintf(fp, "(128B ,256B]   == list size %8d  tot %12d ele\n", listsSize[6], cal(6, listsSize[6]));
    fprintf(fp, "(256B ,512B]   == list size %8d  tot %12d ele\n", listsSize[7], cal(7, listsSize[7]));
    fprintf(fp, "(512B ,1K]     == list size %8d  tot %12d ele\n", listsSize[8], cal(8, listsSize[8]));
    fprintf(fp, "(1K ,2K]       == list size %8d  tot %12d ele\n", listsSize[9], cal(9, listsSize[9]));
    fprintf(fp, "(2K ,4K]       == list size %8d  tot %12d ele\n", listsSize[10], cal(10, listsSize[10]));
    fprintf(fp, "(4K ,8K]       == list size %8d  tot %12d ele\n", listsSize[11], cal(11, listsSize[11]));
    fprintf(fp, "(8K ,16K]      == list size %8d  tot %12d ele\n", listsSize[12], cal(12, listsSize[12]));
    fprintf(fp, "(16K ,32K]     == list size %8d  tot %12d ele\n", listsSize[13], cal(13, listsSize[13]));
    fprintf(fp, "(32K ,64K]     == list size %8d  tot %12d ele\n", listsSize[14], cal(14, listsSize[14]));
    fprintf(fp, "(64K ,128K]    == list size %8d  tot %12d ele\n", listsSize[15], cal(15, listsSize[15]));
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
    void *p = my_malloc(nmemb * size);
    memset(p, 0, nmemb * size);
    //void *p = calloc(nmemb, size);
    if (nmemb * size > 0 && NULL == p) {
        printf(
                "[%s] Failed to calloc %zu bytes at %s line %u: %s\n",
                func, nmemb * size, file, line, strerror(errno));
        exit(EXIT_FAILURE);
    }
    return p;
}

void *wrap_malloc(size_t size,
                  const char *file, unsigned int line, const char *func) {
    void *p = my_malloc(size);
    //void *p = malloc(size);
    if (size > 0 && NULL == p) {
        printf(
                "[%s] Failed to malloc %zu bytes at %s line %u: %s\n",
                func, size, file, line, strerror(errno));
        exit(EXIT_FAILURE);
    }
    return p;
}

void *wrap_realloc(void *ptr, size_t size,
                   const char *file, unsigned int line, const char *func) {
    void *p = my_realloc(ptr, size);
    //void *p = realloc(ptr, size);
    if (size > 0 && NULL == p) {
        printf(
                "[%s] Failed to realloc %zu bytes at %s line %u: %s\n",
                func, size, file, line, strerror(errno));
        exit(EXIT_FAILURE);
    }
    return p;
}

char *wrap_strdup(const char *s,
                  const char *file, unsigned int line, const char *func) {
    char *p = my_strdup(s);
    //char *p = strdup(s);
    if (NULL == p) {
        printf(
                "[%s] Failed to strdup %zu bytes at %s line %u: %s\n",
                func, strlen(s), file, line, strerror(errno));
        exit(EXIT_FAILURE);
    }
    return p;
}

void wrap_free(void *ptr, const char *file, unsigned int line, const char *func) {
    if (ptr != NULL) {
        my_free(ptr);
        //free(ptr);
//        fprintf(stderr, "[%s] Freed memory at %s line %u\n", func, file, line);
    } else {
//        fprintf(stderr, "[%s] Attempt to free NULL pointer at %s line %u\n", func, file, line);
    }
}


