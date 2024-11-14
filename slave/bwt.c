/* The MIT License

   Copyright (c) 2008 Genome Research Ltd (GRL).

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

/* Contact: Heng Li <lh3@sanger.ac.uk> */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <limits.h>
#include "utils.h"
#include "bwt.h"
#include "kvec.h"

#include "lwpf3_my_cpe.h"

#ifdef USE_MALLOC_WRAPPERS
#  include "malloc_wrap.h"
#endif

#define use_my_prefetch
#define my_prefetch_dis 8

extern double t_extend;
extern double t_bwt_sa;

extern double t_work1_1;
extern double t_work1_2;


void bwt_gen_cnt_table(bwt_t *bwt)
{
	int i, j;
	for (i = 0; i != 256; ++i) {
		uint32_t x = 0;
		for (j = 0; j != 4; ++j)
			x |= (((i&3) == j) + ((i>>2&3) == j) + ((i>>4&3) == j) + (i>>6 == j)) << (j<<3);
		bwt->cnt_table[i] = x;
	}
}

static inline bwtint_t bwt_invPsi(const bwt_t *bwt, bwtint_t k) // compute inverse CSA
{
	bwtint_t x = k - (k > bwt->primary);
	x = bwt_B0(bwt, x);
	x = bwt->L2[x] + bwt_occ(bwt, k, x);
	return k == bwt->primary? 0 : x;
}

// bwt->bwt and bwt->occ must be precalculated
void bwt_cal_sa(bwt_t *bwt, int intv)
{
	bwtint_t isa, sa, i; // S(isa) = sa
	int intv_round = intv;

	kv_roundup32(intv_round);
	xassert(intv_round == intv, "SA sample interval is not a power of 2.");
	xassert(bwt->bwt, "bwt_t::bwt is not initialized.");

	if (bwt->sa) free(bwt->sa);
	bwt->sa_intv = intv;
	bwt->n_sa = (bwt->seq_len + intv) / intv;
	bwt->sa = (bwtint_t*)calloc(bwt->n_sa, sizeof(bwtint_t));
	// calculate SA value
	isa = 0; sa = bwt->seq_len;
	for (i = 0; i < bwt->seq_len; ++i) {
		if (isa % intv == 0) bwt->sa[isa/intv] = sa;
		--sa;
		isa = bwt_invPsi(bwt, isa);
	}
	if (isa % intv == 0) bwt->sa[isa/intv] = sa;
	bwt->sa[0] = (bwtint_t)-1; // before this line, bwt->sa[0] = bwt->seq_len
}

bwtint_t bwt_sa(const bwt_t *bwt, bwtint_t k)
{
    //static long sa_cnt = 0;
    //static int cntt_1 = 0;
    //cntt_1++;

	bwtint_t sa = 0, mask = bwt->sa_intv - 1;
	//bwtint_t sa = 0, mask = 31;
	while (k & mask) {
		++sa;
		k = bwt_invPsi(bwt, k);
        //if(_PEN == 0 && cntt_1 < 100) fprintf(stderr, "%llu ", k);
        //sa_cnt++;
	}
    //if(_PEN == 0 && cntt_1 < 100) fprintf(stderr, "\n");
    //if(cntt_1 % 100000 == 0) fprintf(stderr, "%d [sa_cnt] %.3f\n", cntt_1, 1.0 * sa_cnt / cntt_1);
	/* without setting bwt->sa[0] = -1, the following line should be
	   changed to (sa + bwt->sa[k/bwt->sa_intv]) % (bwt->seq_len + 1) */
    bwtint_t res = sa + bwt->sa[k/bwt->sa_intv];
    //bwtint_t res = sa + bwt->sa[k >> 5];
	return res;
}

static inline int __occ_aux(uint64_t y, int c)
{
	// reduce nucleotide counting to bits counting
	y = ((c&2)? y : ~y) >> 1 & ((c&1)? y : ~y) & 0x5555555555555555ull;
	// count the number of 1s in y
	y = (y & 0x3333333333333333ull) + (y >> 2 & 0x3333333333333333ull);
	return ((y + (y >> 4)) & 0xf0f0f0f0f0f0f0full) * 0x101010101010101ull >> 56;
}


//volatile __thread_local int reply;
//__thread_local_fix uint32_t p_tmp[16];
bwtint_t bwt_occ(const bwt_t *bwt, bwtint_t k, ubyte_t c)
{
	bwtint_t n1, n2 = 0;
	uint32_t *p, *end;

	if (k == bwt->seq_len) return bwt->L2[c+1] - bwt->L2[c];
	if (k == (bwtint_t)(-1)) return 0;
	k -= (k >= bwt->primary); // because $ is not in bwt

    //lwpf_start(l_my_2ooc1);
	// retrieve Occ at k/OCC_INTERVAL
    //reply=0;
    //athread_dma_iget(&n1, ((bwtint_t*)bwt_occ_intv(bwt, k)) + c, sizeof(bwtint_t), &reply);
    //assert(n == ((bwtint_t*)bwt_occ_intv(bwt, k))[c]);
    n1 = ((bwtint_t*)bwt_occ_intv(bwt, k))[c];

    //int p_size = (((k>>5) - ((k&~OCC_INTV_MASK)>>5))<<1) * sizeof(uint32_t);
    //p_size += 2 * sizeof(uint32_t);
    //assert(p_size <= 40);
    //lwpf_stop(l_my_2ooc1);

    //lwpf_start(l_my_2ooc2);
    p = bwt_occ_intv(bwt, k) + sizeof(bwtint_t);
    //p = p_tmp;
    //athread_dma_get(p, bwt_occ_intv(bwt, k) + sizeof(bwtint_t), p_size);

    //uint32_t *pp = p;
	// calculate Occ up to the last k/32
	end = p + (((k>>5) - ((k&~OCC_INTV_MASK)>>5))<<1);
	for (; p < end; p += 2) n2 += __occ_aux((uint64_t)p[0]<<32 | p[1], c);

	// calculate Occ
	n2 += __occ_aux(((uint64_t)p[0]<<32 | p[1]) & ~((1ull<<((~k&31)<<1)) - 1), c);
	if (c == 0) n2 -= ~k&31; // corrected for the masked bits

    //assert((p + 2 - pp) * sizeof(uint32_t) <= p_size);
    //lwpf_stop(l_my_2ooc2);
    //athread_dma_wait_value(&reply,1);
	return n1 + n2;
}

// an analogy to bwt_occ() but more efficient, requiring k <= l
void bwt_2occ(const bwt_t *bwt, bwtint_t k, bwtint_t l, ubyte_t c, bwtint_t *ok, bwtint_t *ol)
{


	bwtint_t _k, _l;
	_k = (k >= bwt->primary)? k-1 : k;
	_l = (l >= bwt->primary)? l-1 : l;
	if (_l/OCC_INTERVAL != _k/OCC_INTERVAL || k == (bwtint_t)(-1) || l == (bwtint_t)(-1)) {
	//if (1) {
		*ok = bwt_occ(bwt, k, c);
		*ol = bwt_occ(bwt, l, c);
	} else {
		bwtint_t m1, m2 = 0, n1, n2 = 0, i, j;
		uint32_t *p;
        
		if (k >= bwt->primary) --k;
		if (l >= bwt->primary) --l;

        //lwpf_start(l_my_2ooc3);
        //reply=0;
        //athread_dma_iget(&n1, ((bwtint_t*)bwt_occ_intv(bwt, k)) + c, sizeof(bwtint_t), &reply);
        //assert(n == ((bwtint_t*)bwt_occ_intv(bwt, k))[c]);
        n1 = ((bwtint_t*)bwt_occ_intv(bwt, k))[c];

        //int p_size = (((l>>5) - ((k&~OCC_INTV_MASK)>>5))<<1) * sizeof(uint32_t);
        //p_size += 2 * sizeof(uint32_t);
        //assert(p_size <= 40);
        //lwpf_stop(l_my_2ooc3);

        //lwpf_start(l_my_2ooc4);
        p = bwt_occ_intv(bwt, k) + sizeof(bwtint_t);
        //p = p_tmp;
        //athread_dma_get(p, bwt_occ_intv(bwt, k) + sizeof(bwtint_t), p_size);
		// calculate *ok
        //uint32_t *pp = p;
		j = k >> 5 << 5;
		for (i = k/OCC_INTERVAL*OCC_INTERVAL; i < j; i += 32, p += 2) {
			n2 += __occ_aux((uint64_t)p[0]<<32 | p[1], c);
        }
		m1 = n2;
		n2 += __occ_aux(((uint64_t)p[0]<<32 | p[1]) & ~((1ull<<((~k&31)<<1)) - 1), c);
		if (c == 0) n2 -= ~k&31; // corrected for the masked bits
		// calculate *ol
		j = l >> 5 << 5;
		for (; i < j; i += 32, p += 2)
			m2 += __occ_aux((uint64_t)p[0]<<32 | p[1], c);
		m2 += __occ_aux(((uint64_t)p[0]<<32 | p[1]) & ~((1ull<<((~l&31)<<1)) - 1), c);
		if (c == 0) m2 -= ~l&31; // corrected for the masked bits
        //athread_dma_wait_value(&reply,1);
		*ok = n1 + n2;
		*ol = n1 + m1 + m2;
        //assert((p + 2 - pp) * sizeof(uint32_t) <= p_size);
        //lwpf_stop(l_my_2ooc4);
	}
}

#define __occ_aux4(bwt, b)											\
	((bwt)->cnt_table[(b)&0xff] + (bwt)->cnt_table[(b)>>8&0xff]		\
	 + (bwt)->cnt_table[(b)>>16&0xff] + (bwt)->cnt_table[(b)>>24])

void bench_bwt_occ4(const bwt_t *bwt, bwtint_t k, int *cnt)
{
	*cnt = *(bwt_occ_intv(bwt, k));
}


void bwt_occ4(const bwt_t *bwt, bwtint_t k, bwtint_t cnt[4])
{
	bwtint_t x;
	uint32_t *p, tmp, *end;
	if (k == (bwtint_t)(-1)) {
		memset(cnt, 0, 4 * sizeof(bwtint_t));
		return;
	}
	k -= (k >= bwt->primary); // because $ is not in bwt
	p = bwt_occ_intv(bwt, k);
	memcpy(cnt, p, 4 * sizeof(bwtint_t));
	p += sizeof(bwtint_t); // sizeof(bwtint_t) = 4*(sizeof(bwtint_t)/sizeof(uint32_t))
	end = p + ((k>>4) - ((k&~OCC_INTV_MASK)>>4)); // this is the end point of the following loop
	for (x = 0; p < end; ++p) x += __occ_aux4(bwt, *p);
	tmp = *p & ~((1U<<((~k&15)<<1)) - 1);
	x += __occ_aux4(bwt, tmp) - (~k&15);
	cnt[0] += x&0xff; cnt[1] += x>>8&0xff; cnt[2] += x>>16&0xff; cnt[3] += x>>24;
}

// an analogy to bwt_occ4() but more efficient, requiring k <= l
void bwt_2occ4(const bwt_t *bwt, bwtint_t k, bwtint_t l, bwtint_t cntk[4], bwtint_t cntl[4])
{
	bwtint_t _k, _l;
	_k = k - (k >= bwt->primary);
	_l = l - (l >= bwt->primary);
	if (_l>>OCC_INTV_SHIFT != _k>>OCC_INTV_SHIFT || k == (bwtint_t)(-1) || l == (bwtint_t)(-1)) {
		bwt_occ4(bwt, k, cntk);
		bwt_occ4(bwt, l, cntl);
	} else {
		bwtint_t x, y;
		uint32_t *p, tmp, *endk, *endl;
		k -= (k >= bwt->primary); // because $ is not in bwt
		l -= (l >= bwt->primary);
		p = bwt_occ_intv(bwt, k);
		memcpy(cntk, p, 4 * sizeof(bwtint_t));
		p += sizeof(bwtint_t); // sizeof(bwtint_t) = 4*(sizeof(bwtint_t)/sizeof(uint32_t))
		// prepare cntk[]
		endk = p + ((k>>4) - ((k&~OCC_INTV_MASK)>>4));
		endl = p + ((l>>4) - ((l&~OCC_INTV_MASK)>>4));
		for (x = 0; p < endk; ++p) x += __occ_aux4(bwt, *p);
		y = x;
		tmp = *p & ~((1U<<((~k&15)<<1)) - 1);
		x += __occ_aux4(bwt, tmp) - (~k&15);
		// calculate cntl[] and finalize cntk[]
		for (; p < endl; ++p) y += __occ_aux4(bwt, *p);
		tmp = *p & ~((1U<<((~l&15)<<1)) - 1);
		y += __occ_aux4(bwt, tmp) - (~l&15);
		memcpy(cntl, cntk, 4 * sizeof(bwtint_t));
		cntk[0] += x&0xff; cntk[1] += x>>8&0xff; cntk[2] += x>>16&0xff; cntk[3] += x>>24;
		cntl[0] += y&0xff; cntl[1] += y>>8&0xff; cntl[2] += y>>16&0xff; cntl[3] += y>>24;
	}
}

int bwt_match_exact(const bwt_t *bwt, int len, const ubyte_t *str, bwtint_t *sa_begin, bwtint_t *sa_end)
{
	bwtint_t k, l, ok, ol;
	int i;
	k = 0; l = bwt->seq_len;
	for (i = len - 1; i >= 0; --i) {
		ubyte_t c = str[i];
		if (c > 3) return 0; // no match
		bwt_2occ(bwt, k - 1, l, c, &ok, &ol);
		k = bwt->L2[c] + ok + 1;
		l = bwt->L2[c] + ol;
		if (k > l) break; // no match
	}
	if (k > l) return 0; // no match
	if (sa_begin) *sa_begin = k;
	if (sa_end)   *sa_end = l;
	return l - k + 1;
}

int bwt_match_exact_alt(const bwt_t *bwt, int len, const ubyte_t *str, bwtint_t *k0, bwtint_t *l0)
{
	int i;
	bwtint_t k, l, ok, ol;
	k = *k0; l = *l0;
	for (i = len - 1; i >= 0; --i) {
		ubyte_t c = str[i];
		if (c > 3) return 0; // there is an N here. no match
		bwt_2occ(bwt, k - 1, l, c, &ok, &ol);
		k = bwt->L2[c] + ok + 1;
		l = bwt->L2[c] + ol;
		if (k > l) return 0; // no match
	}
	*k0 = k; *l0 = l;
	return l - k + 1;
}

/*********************
 * Bidirectional BWT *
 *********************/

void bwt_extend_forward_base(const bwt_t *bwt, const bwtintv_t *ik, bwtintv_t ok[4], ubyte_t c)
{
	bwtint_t tk, tl;

    lwpf_start(l_my_extend3);
    bwt_2occ(bwt, ik->x[0] - 1, ik->x[0] - 1 + ik->x[2], c, &tk, &tl);
    lwpf_stop(l_my_extend3);

    lwpf_start(l_my_extend4);
    ok[c].x[0] = bwt->L2[c] + 1 + tk;
    ok[c].x[2] = tl - tk;
    lwpf_stop(l_my_extend4);
}

void bwt_extend_backward_base(const bwt_t *bwt, const bwtintv_t *ik, bwtintv_t ok[4], ubyte_t c)
{
	bwtint_t tk[4], tl[4];
	int i;

    lwpf_start(l_my_extend5);
    for(i = 3; i >= c; i--) {
        bwt_2occ(bwt, ik->x[1] - 1, ik->x[1] - 1 + ik->x[2], i, &tk[i], &tl[i]);
    }
    lwpf_stop(l_my_extend5);

    lwpf_start(l_my_extend6);
	for (i = 3; i >= c; i--) {
		ok[i].x[1] = bwt->L2[i] + 1 + tk[i];
		ok[i].x[2] = tl[i] - tk[i];
	}
	ok[3].x[0] = ik->x[0] + (ik->x[1] <= bwt->primary && ik->x[1] + ik->x[2] - 1 >= bwt->primary);
    for(i = 2; i >= c; i--) {
        ok[i].x[0] = ok[i + 1].x[0] + ok[i + 1].x[2];
    }
    lwpf_stop(l_my_extend6);
}


void bwt_extend(const bwt_t *bwt, const bwtintv_t *ik, bwtintv_t ok[4], int is_back)
{
	bwtint_t tk[4], tl[4];
	int i;

    lwpf_start(l_my_extend1);
	bwt_2occ4(bwt, ik->x[!is_back] - 1, ik->x[!is_back] - 1 + ik->x[2], tk, tl);
    lwpf_stop(l_my_extend1);

    lwpf_start(l_my_extend2);
	for (i = 0; i != 4; ++i) {
		ok[i].x[!is_back] = bwt->L2[i] + 1 + tk[i];
		ok[i].x[2] = tl[i] - tk[i];
	}
	ok[3].x[is_back] = ik->x[is_back] + (ik->x[!is_back] <= bwt->primary && ik->x[!is_back] + ik->x[2] - 1 >= bwt->primary);
	ok[2].x[is_back] = ok[3].x[is_back] + ok[3].x[2];
	ok[1].x[is_back] = ok[2].x[is_back] + ok[2].x[2];
	ok[0].x[is_back] = ok[1].x[is_back] + ok[1].x[2];
    lwpf_stop(l_my_extend2);
}

static void bwt_reverse_intvs(bwtintv_v *p)
{
	if (p->n > 1) {
		int j;
		for (j = 0; j < p->n>>1; ++j) {
			bwtintv_t tmp = p->a[p->n - 1 - j];
			p->a[p->n - 1 - j] = p->a[j];
			p->a[j] = tmp;
		}
	}
}



#ifdef use_2_pass_batch
__thread_local_fix bwtintv_t p1[8][128];
__thread_local_fix bwtintv_t p2[8][128];

void bwt_smem1a_batch(int bs, const bwt_t *bwt, int len, const uint8_t *q, int *b_x, int *b_min_intv, uint64_t max_intv, bwtintv_v *mem)
{
	bwtintv_t b_ik[bs];
	bwtintv_v *b_prev[bs], *b_curr[bs], *b_swap[bs];
    assert(bs <= 8);
    for(int id = 0; id < bs; id++) {
        b_prev[id] = p1[id];
        b_curr[id] = p2[id];
    }


    lwpf_start(l_forward_batch);

	for(int id = 0; id < bs; id++) if (b_min_intv[id] < 1) b_min_intv[id] = 1; // the interval size should be at least 1

    for(int id = 0; id < bs; id++) {
        bwt_set_intv(bwt, q[b_x[id]], b_ik[id]); // the initial interval of a single base
        b_ik[id].info = b_x[id] + 1;
    }

    for(int id = 0; id < bs; id++) {
    //for(int id = bs - 1; id >= 0; id--) {
        bwtintv_v *prev = b_prev[id];
        bwtintv_v *curr = b_curr[id];
        bwtintv_v *swap = b_swap[id];

        bwtintv_t ik = b_ik[id];
        bwtintv_t ok[4];
        int c, i, j;
        int x = b_x[id];
        int ret;
        int min_intv = b_min_intv[id];
        for (i = x + 1, curr->n = 0; i < len; ++i) { // forward search
            if (ik.x[2] < max_intv) { // an interval small enough
                kv_push(bwtintv_t, *curr, ik);
                break;
            } else if (q[i] < 4) { // an A/C/G/T base
                c = 3 - q[i]; // complement of q[i]
                //bwt_extend(bwt, &ik, ok, 0);
                bwt_extend_backward_base(bwt, &ik, ok, c);
                if (ok[c].x[2] != ik.x[2]) { // change of the interval size
                    kv_push(bwtintv_t, *curr, ik);
                    if (ok[c].x[2] < min_intv) {
                        break; // the interval size is too small to be extended further
                    }
                }
                ik = ok[c]; ik.info = i + 1;
#ifdef use_my_prefetch
                bwtint_t k1 = ok[c].x[0] - 1;
                if(k1 != -1) __builtin_prefetch(bwt_occ_intv(bwt, k1 - (k1 >= bwt->primary)), 0, 3);
                bwtint_t l1 = ok[c].x[0] - 1 + ok[c].x[2];
                if(l1 != -1) __builtin_prefetch(bwt_occ_intv(bwt, l1 - (l1 >= bwt->primary)), 0, 3);
                bwtint_t k2 = ok[c].x[1] - 1;
                if(k2 != -1) __builtin_prefetch(bwt_occ_intv(bwt, k2 - (k2 >= bwt->primary)), 0, 3);
                bwtint_t l2 = ok[c].x[1] - 1 + ok[c].x[2];
                if(l2 != -1) __builtin_prefetch(bwt_occ_intv(bwt, l2 - (l2 >= bwt->primary)), 0, 3);
#endif
            } else { // an ambiguous base
                kv_push(bwtintv_t, *curr, ik);
                break; // always terminate extension at an ambiguous base; in this case, i<len always stands
            }
        }
        if (i == len) kv_push(bwtintv_t, *curr, ik); // push the last interval if we reach the end
        bwt_reverse_intvs(curr); // s.t. smaller intervals (i.e. longer matches) visited first
        ret = curr->a[0].info; // this will be the returned value
        swap = curr; curr = prev; prev = swap;


        b_ik[id] = ik;
        b_prev[id] = prev;
        b_curr[id] = curr;
        b_swap[id] = swap;
    }
    lwpf_stop(l_forward_batch);

    mem->n = 0;
    lwpf_start(l_backward_batch);
    for(int id = 0; id < bs; id++) {
    //for(int id = bs - 1; id >= 0; id--) {
        int this_mem_n = 0;
        bwtintv_v *prev = b_prev[id];
        bwtintv_v *curr = b_curr[id];
        bwtintv_v *swap = b_swap[id];
        bwtintv_t ik = b_ik[id];
        bwtintv_t ok[4];
        int c, i, j;
        int x = b_x[id];
        int min_intv = b_min_intv[id];
        for (i = x - 1; i >= -1; --i) { // backward search for MEMs
            c = i < 0? -1 : q[i] < 4? q[i] : -1; // c==-1 if i<0 or q[i] is an ambiguous base
            for (j = 0, curr->n = 0; j < prev->n; ++j) {
                bwtintv_t *p = &prev->a[j];
                if (c >= 0 && ik.x[2] >= max_intv) bwt_extend_forward_base(bwt, p, ok, c);
                //if (c >= 0 && ik.x[2] >= max_intv) bwt_extend(bwt, p, ok, 1);
                if (c < 0 || ik.x[2] < max_intv || ok[c].x[2] < min_intv) { // keep the hit if reaching the beginning or an ambiguous base or the intv is small enough
                    if (curr->n == 0) { // test curr->n>0 to make sure there are no longer matches
                        if (this_mem_n == 0 || i + 1 < mem->a[mem->n-1].info>>32) { // skip contained matches
                            ik = *p; ik.info |= (uint64_t)(i + 1)<<32;
                            kv_push(bwtintv_t, *mem, ik);
                            this_mem_n++;
                        }
                    } // otherwise the match is contained in another longer match
                } else if (curr->n == 0 || ok[c].x[2] != curr->a[curr->n-1].x[2]) {
                    ok[c].info = p->info;
                    kv_push(bwtintv_t, *curr, ok[c]);
#ifdef use_my_prefetch
                    bwtint_t k1 = ok[c].x[0] - 1;
                    if(k1 != -1) __builtin_prefetch(bwt_occ_intv(bwt, k1 - (k1 >= bwt->primary)), 0, 3);
                    bwtint_t l1 = ok[c].x[0] - 1 + ok[c].x[2];
                    if(l1 != -1) __builtin_prefetch(bwt_occ_intv(bwt, l1 - (l1 >= bwt->primary)), 0, 3);
#endif
                }
            }
            if (curr->n == 0) break;
            swap = curr; curr = prev; prev = swap;
        }
    }
    bwt_reverse_intvs(mem); // s.t. sorted by the start coordinate

    lwpf_stop(l_backward_batch);
}
#endif


// NOTE: $max_intv is not currently used in BWA-MEM
int bwt_smem1a(const bwt_t *bwt, int len, const uint8_t *q, int x, int min_intv, uint64_t max_intv, bwtintv_v *mem, bwtintv_v *tmpvec[2])
{
	int i, j, c, ret;
	bwtintv_t ik, ok[4];
	bwtintv_v a[2], *prev, *curr, *swap;

	mem->n = 0;
	if (q[x] > 3) return x + 1;


    lwpf_start(l_smem1a_for);

	if (min_intv < 1) min_intv = 1; // the interval size should be at least 1
	kv_init(a[0]); kv_init(a[1]);
	prev = tmpvec && tmpvec[0]? tmpvec[0] : &a[0]; // use the temporary vector if provided
	curr = tmpvec && tmpvec[1]? tmpvec[1] : &a[1];
	bwt_set_intv(bwt, q[x], ik); // the initial interval of a single base
	ik.info = x + 1;
    

	for (i = x + 1, curr->n = 0; i < len; ++i) { // forward search
		if (ik.x[2] < max_intv) { // an interval small enough
			kv_push(bwtintv_t, *curr, ik);
			break;
		} else if (q[i] < 4) { // an A/C/G/T base
			c = 3 - q[i]; // complement of q[i]
            //bwt_extend(bwt, &ik, ok, 0);
			bwt_extend_backward_base(bwt, &ik, ok, c);
			if (ok[c].x[2] != ik.x[2]) { // change of the interval size
				kv_push(bwtintv_t, *curr, ik);
				if (ok[c].x[2] < min_intv) {
                    break; // the interval size is too small to be extended further
                }
			}
			ik = ok[c]; ik.info = i + 1;
#ifdef use_my_prefetch
            bwtint_t k1 = ok[c].x[0] - 1;
            if(k1 != -1) __builtin_prefetch(bwt_occ_intv(bwt, k1 - (k1 >= bwt->primary)), 0, 3);
            bwtint_t l1 = ok[c].x[0] - 1 + ok[c].x[2];
            if(l1 != -1) __builtin_prefetch(bwt_occ_intv(bwt, l1 - (l1 >= bwt->primary)), 0, 3);
            bwtint_t k2 = ok[c].x[1] - 1;
            if(k2 != -1) __builtin_prefetch(bwt_occ_intv(bwt, k2 - (k2 >= bwt->primary)), 0, 3);
            bwtint_t l2 = ok[c].x[1] - 1 + ok[c].x[2];
            if(l2 != -1) __builtin_prefetch(bwt_occ_intv(bwt, l2 - (l2 >= bwt->primary)), 0, 3);
#endif
		} else { // an ambiguous base
			kv_push(bwtintv_t, *curr, ik);
			break; // always terminate extension at an ambiguous base; in this case, i<len always stands
		}
	}
    lwpf_stop(l_smem1a_for);

	if (i == len) kv_push(bwtintv_t, *curr, ik); // push the last interval if we reach the end
	bwt_reverse_intvs(curr); // s.t. smaller intervals (i.e. longer matches) visited first
	ret = curr->a[0].info; // this will be the returned value
	swap = curr; curr = prev; prev = swap;

    lwpf_start(l_smem1a_back);
	for (i = x - 1; i >= -1; --i) { // backward search for MEMs
		c = i < 0? -1 : q[i] < 4? q[i] : -1; // c==-1 if i<0 or q[i] is an ambiguous base
		for (j = 0, curr->n = 0; j < prev->n; ++j) {
			bwtintv_t *p = &prev->a[j];
//#ifdef use_my_prefetch
//            if(c >= 0 && j + my_prefetch_dis < prev->n) {
//                bwtintv_t *pp = &prev->a[j + my_prefetch_dis];
//                bwtint_t k1 = pp->x[0] - 1;
//                if(k1 != -1) __builtin_prefetch(bwt_occ_intv(bwt, k1 - (k1 >= bwt->primary)), 0, 3);
//                bwtint_t l1 = pp->x[0] - 1 + pp->x[2];
//                if(l1 != -1) __builtin_prefetch(bwt_occ_intv(bwt, l1 - (l1 >= bwt->primary)), 0, 3);
//            }
//#endif
			if (c >= 0 && ik.x[2] >= max_intv) bwt_extend_forward_base(bwt, p, ok, c);
            //if (c >= 0 && ik.x[2] >= max_intv) bwt_extend(bwt, p, ok, 1);
			if (c < 0 || ik.x[2] < max_intv || ok[c].x[2] < min_intv) { // keep the hit if reaching the beginning or an ambiguous base or the intv is small enough
				if (curr->n == 0) { // test curr->n>0 to make sure there are no longer matches
					if (mem->n == 0 || i + 1 < mem->a[mem->n-1].info>>32) { // skip contained matches
						ik = *p; ik.info |= (uint64_t)(i + 1)<<32;
						kv_push(bwtintv_t, *mem, ik);
					}
				} // otherwise the match is contained in another longer match
			} else if (curr->n == 0 || ok[c].x[2] != curr->a[curr->n-1].x[2]) {
				ok[c].info = p->info;
				kv_push(bwtintv_t, *curr, ok[c]);
#ifdef use_my_prefetch
                bwtint_t k1 = ok[c].x[0] - 1;
                if(k1 != -1) __builtin_prefetch(bwt_occ_intv(bwt, k1 - (k1 >= bwt->primary)), 0, 3);
                bwtint_t l1 = ok[c].x[0] - 1 + ok[c].x[2];
                if(l1 != -1) __builtin_prefetch(bwt_occ_intv(bwt, l1 - (l1 >= bwt->primary)), 0, 3);
#endif
			}
		}
		if (curr->n == 0) break;
		swap = curr; curr = prev; prev = swap;
	}
    lwpf_stop(l_smem1a_back);
	bwt_reverse_intvs(mem); // s.t. sorted by the start coordinate

	if (tmpvec == 0 || tmpvec[0] == 0) free(a[0].a);
	if (tmpvec == 0 || tmpvec[1] == 0) free(a[1].a);
	return ret;
}

#ifdef use_2_pass_batch
void bwt_smem1_batch(int bs, const bwt_t *bwt, int len, const uint8_t *q, int *x, int *min_intv, bwtintv_v *mem)
{
	bwt_smem1a_batch(bs, bwt, len, q, x, min_intv, 0, mem);
}
#endif

int bwt_smem1(const bwt_t *bwt, int len, const uint8_t *q, int x, int min_intv, bwtintv_v *mem, bwtintv_v *tmpvec[2])
{
	return bwt_smem1a(bwt, len, q, x, min_intv, 0, mem, tmpvec);
}

int bwt_seed_strategy1(const bwt_t *bwt, int len, const uint8_t *q, int x, int min_len, int max_intv, bwtintv_t *mem)
{
	int i, c;
	bwtintv_t ik, ok[4];

	memset(mem, 0, sizeof(bwtintv_t));
	if (q[x] > 3) return x + 1;
	bwt_set_intv(bwt, q[x], ik); // the initial interval of a single base
	for (i = x + 1; i < len; ++i) { // forward search
		if (q[i] < 4) { // an A/C/G/T base
			c = 3 - q[i]; // complement of q[i]
			bwt_extend(bwt, &ik, ok, 0);
			//bwt_extend_backward_base(bwt, &ik, ok, c);
            if (ok[c].x[2] < max_intv && i - x >= min_len) {
				*mem = ok[c];
				mem->info = (uint64_t)x<<32 | (i + 1);
				return i + 1;
			}
            if (ok[c].x[2] == 0) {
                return min_len + x + 1 < len ? min_len + x + 1 : len;
            }
			
//#ifdef use_my_prefetch
//            bwtint_t k1 = ok[c].x[0] - 1;
//            if(k1 != -1) __builtin_prefetch(bwt_occ_intv(bwt, k1 - (k1 >= bwt->primary)), 0, 3);
//            bwtint_t l1 = ok[c].x[0] - 1 + ok[c].x[2];
//            if(l1 != -1) __builtin_prefetch(bwt_occ_intv(bwt, l1 - (l1 >= bwt->primary)), 0, 3);
//            bwtint_t k2 = ok[c].x[1] - 1;
//            if(k2 != -1) __builtin_prefetch(bwt_occ_intv(bwt, k2 - (k2 >= bwt->primary)), 0, 3);
//            bwtint_t l2 = ok[c].x[1] - 1 + ok[c].x[2];
//            if(l2 != -1) __builtin_prefetch(bwt_occ_intv(bwt, l2 - (l2 >= bwt->primary)), 0, 3);
//#endif
	
			ik = ok[c];
		} else return i + 1;
	}
	return len;
}

/*************************
 * Read/write BWT and SA *
 *************************/

void bwt_dump_bwt(const char *fn, const bwt_t *bwt)
{
	FILE *fp;
	fp = xopen(fn, "wb");
	err_fwrite(&bwt->primary, sizeof(bwtint_t), 1, fp);
	err_fwrite(bwt->L2+1, sizeof(bwtint_t), 4, fp);
	err_fwrite(bwt->bwt, 4, bwt->bwt_size, fp);
	err_fflush(fp);
	err_fclose(fp);
}

void bwt_dump_sa(const char *fn, const bwt_t *bwt)
{
	FILE *fp;
	fp = xopen(fn, "wb");
	err_fwrite(&bwt->primary, sizeof(bwtint_t), 1, fp);
	err_fwrite(bwt->L2+1, sizeof(bwtint_t), 4, fp);
	err_fwrite(&bwt->sa_intv, sizeof(bwtint_t), 1, fp);
	err_fwrite(&bwt->seq_len, sizeof(bwtint_t), 1, fp);
	err_fwrite(bwt->sa + 1, sizeof(bwtint_t), bwt->n_sa - 1, fp);
	err_fflush(fp);
	err_fclose(fp);
}

static bwtint_t fread_fix(FILE *fp, bwtint_t size, void *a)
{ // Mac/Darwin has a bug when reading data longer than 2GB. This function fixes this issue by reading data in small chunks
	const int bufsize = 0x1000000; // 16M block
	bwtint_t offset = 0;
	while (size) {
		int x = bufsize < size? bufsize : size;
		if ((x = err_fread_noeof(a + offset, 1, x, fp)) == 0) break;
		size -= x; offset += x;
	}
	return offset;
}

void bwt_restore_sa(const char *fn, bwt_t *bwt)
{
	char skipped[256];
	FILE *fp;
	bwtint_t primary;

	fp = xopen(fn, "rb");
	err_fread_noeof(&primary, sizeof(bwtint_t), 1, fp);
	xassert(primary == bwt->primary, "SA-BWT inconsistency: primary is not the same.");
	err_fread_noeof(skipped, sizeof(bwtint_t), 4, fp); // skip
	err_fread_noeof(&bwt->sa_intv, sizeof(bwtint_t), 1, fp);
	err_fread_noeof(&primary, sizeof(bwtint_t), 1, fp);
	xassert(primary == bwt->seq_len, "SA-BWT inconsistency: seq_len is not the same.");

	bwt->n_sa = (bwt->seq_len + bwt->sa_intv) / bwt->sa_intv;
	bwt->sa = (bwtint_t*)calloc(bwt->n_sa, sizeof(bwtint_t));
	bwt->sa[0] = -1;

	fread_fix(fp, sizeof(bwtint_t) * (bwt->n_sa - 1), bwt->sa + 1);
	err_fclose(fp);
}

bwt_t *bwt_restore_bwt(const char *fn)
{
	bwt_t *bwt;
	FILE *fp;

	bwt = (bwt_t*)calloc(1, sizeof(bwt_t));
	fp = xopen(fn, "rb");
	err_fseek(fp, 0, SEEK_END);
	bwt->bwt_size = (err_ftell(fp) - sizeof(bwtint_t) * 5) >> 2;
	bwt->bwt = (uint32_t*)calloc(bwt->bwt_size, 4);
	err_fseek(fp, 0, SEEK_SET);
	err_fread_noeof(&bwt->primary, sizeof(bwtint_t), 1, fp);
	err_fread_noeof(bwt->L2+1, sizeof(bwtint_t), 4, fp);
	fread_fix(fp, bwt->bwt_size<<2, bwt->bwt);
	bwt->seq_len = bwt->L2[4];
	err_fclose(fp);
	bwt_gen_cnt_table(bwt);

	return bwt;
}

void bwt_destroy(bwt_t *bwt)
{
	if (bwt == 0) return;
	free(bwt->sa); free(bwt->bwt);
	free(bwt);
}
