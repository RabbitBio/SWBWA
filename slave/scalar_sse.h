#ifndef SCALAR_SSE_H
#define SCALAR_SSE_H

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "simd.h"

typedef union m128i {
    intv16 val;
} __m128i;

uintv16 v_min = 0;
uintv16 v_max = 0xFFFFFFFF;

static inline __m128i _mm_set1_epi32(int32_t n) {
	assert(n >= 0 && n <= 255);
	__m128i r;
    r.val = n;
    return r;
}

static inline __m128i _mm_load_si128(const __m128i *ptr) {
    __m128i r;
    simd_load(r.val, (int*)ptr); 
    return r; 
}
static inline void _mm_store_si128(__m128i *ptr, __m128i a) { 
    simd_store(a.val, ptr);
}

static inline int m128i_allzero(__m128i a) {
    intv16 xx = a.val;
    xx = simd_vbisw(xx, simd_sllx(xx, 8 * 32));
    xx = simd_vbisw(xx, simd_sllx(xx, 4 * 32));
    xx = simd_vbisw(xx, simd_sllx(xx, 2 * 32));
    xx = simd_vbisw(xx, simd_sllx(xx, 1 * 32));
    return simd_vextw15(xx) == 0;

    //static const char zero[] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
    //return memcmp(&a, zero, sizeof a) == 0;
    //int val[16];
    //simd_store(a.val, &(val[0]));
    //for(int i = 0; i < 16; i++) {
    //    if(val[i] != 0) return 0;
    //}
	//return 1;
}

static inline __m128i _mm_slli_si128(__m128i a, int n) {
    //int val[16];
    //simd_store(a.val, &(val[0]));
    //memmove(&val[n], &val[0], sizeof(int) * (16 - n));
    //for (int i = 0; i < n; i++) val[i] = 0;
    //__m128i r;
    //simd_load(r.val, &val[0]);
    __m128i r;
    r.val = simd_sllx(a.val, n * 32);
	return r;
}

static inline intv16 _mm_max_intv16(intv16 a, intv16 b) {
    intv16 mask = simd_vcmpltw(a, b);
    uintv16 extended_mask = simd_vsubw(v_min, mask);
    intv16 b_selected = simd_vandw(extended_mask, b);
    extended_mask = simd_vxorw(extended_mask, v_max);
    intv16 a_selected = simd_vandw(extended_mask, a);
    intv16 r = simd_vaddw(a_selected, b_selected);
	return r;
}

static inline intv16 _mm_min_intv16(intv16 a, intv16 b) {
    intv16 mask = simd_vcmpltw(a, b);
    uintv16 extended_mask = simd_vsubw(v_min, mask);
    intv16 a_selected = simd_vandw(extended_mask, a);
    extended_mask = simd_vxorw(extended_mask, v_max);
    intv16 b_selected = simd_vandw(extended_mask, b);
    intv16 r = simd_vaddw(a_selected, b_selected);
	return r;
}


static inline __m128i _mm_max_epu8(__m128i a, __m128i b) {
    
    intv16 mask = simd_vcmpltw(a.val, b.val);
    uintv16 extended_mask = simd_vsubw(v_min, mask);
    intv16 b_selected = simd_vandw(extended_mask, b.val);
    extended_mask = simd_vxorw(extended_mask, v_max);
    intv16 a_selected = simd_vandw(extended_mask, a.val);
    __m128i r;
    r.val = simd_vaddw(a_selected, b_selected);
    //int val1[16];
    //simd_store(a.val, &(val1[0]));
    //int val2[16];
    //simd_store(b.val, &(val2[0]));
    //for(int i = 0; i < 16; i++) {
    //    if(val1[i] < val2[i]) val1[i] = val2[i];
    //}
    //__m128i r;
    //simd_load(r.val, &(val1[0])); 
	return r;
}

static inline __m128i _mm_min_epu8(__m128i a, __m128i b) {
    
    intv16 mask = simd_vcmpltw(a.val, b.val);
    uintv16 extended_mask = simd_vsubw(v_min, mask);
    intv16 a_selected = simd_vandw(extended_mask, a.val);
    extended_mask = simd_vxorw(extended_mask, v_max);
    intv16 b_selected = simd_vandw(extended_mask, b.val);
    __m128i r;
    r.val = simd_vaddw(a_selected, b_selected);
    //int val1[16];
    //simd_store(a.val, &(val1[0]));
    //int val2[16];
    //simd_store(b.val, &(val2[0]));
    //for(int i = 0; i < 16; i++) {
    //    if(val1[i] < val2[i]) val1[i] = val2[i];
    //}
    //__m128i r;
    //simd_load(r.val, &(val1[0])); 
	return r;
}


static inline uint8_t m128i_max_u8(__m128i a) {
    int val[16];
    simd_store(a.val, &(val[0]));
	int max = 0;
	for (int i = 0; i < 16; i++)
		if (max < val[i]) max = val[i];
	return max;
}

static inline __m128i _mm_set1_epi8(int8_t n) {
	__m128i r;
    r.val = n;
    return r;
}

static inline __m128i _mm_adds_epu8(__m128i a, __m128i b) {
    static intv16 con_255 = 255;
    a.val = simd_vaddw(a.val, b.val);
    intv16 mask = simd_vcmpltw(a.val, con_255);
    uintv16 extended_mask = simd_vsubw(v_min, mask);
    intv16 a_selected = simd_vandw(extended_mask, a.val);
    extended_mask = simd_vxorw(extended_mask, v_max);
    intv16 b_selected = simd_vandw(extended_mask, con_255);
    a.val = simd_vaddw(a_selected, b_selected);

    //a.val = _mm_min_intv16(a.val, con_255);
    //int val[16];
    //simd_store(a.val, &(val[0]));
	//for (int i = 0; i < 16; i++) {
    //    val[i] = val[i] > 255 ? 255 : val[i];
    //}
    //simd_load(a.val, &(val[0])); 
	return a;
}
static inline __m128i _mm_subs_epu8(__m128i a, __m128i b) {
    static intv16 con_0 = 0;
    a.val = simd_vsubw(a.val, b.val);
    intv16 mask = simd_vcmpltw(a.val, con_0);
    uintv16 extended_mask = simd_vsubw(v_min, mask);
    intv16 b_selected = simd_vandw(extended_mask, con_0);
    extended_mask = simd_vxorw(extended_mask, v_max);
    intv16 a_selected = simd_vandw(extended_mask, a.val);
    a.val = simd_vaddw(a_selected, b_selected);
	
    //a.val = _mm_max_intv16(a.val, con_0);
    //int val[16];
    //simd_store(a.val, &(val[0]));
	//for (int i = 0; i < 16; i++) {
    //    val[i] = val[i] < 0 ? 0 : val[i];
    //}
    //simd_load(a.val, &(val[0])); 
	return a;
}

static inline __m128i _mm_adds_epi16(__m128i a, __m128i b) {
    fprintf(stderr, "TODO\n");
    exit(0);
	return a;
}

static inline __m128i _mm_cmpgt_epi16(__m128i a, __m128i b) {
    fprintf(stderr, "TODO\n");
    exit(0);
	return a;
}

static inline __m128i _mm_max_epi16(__m128i a, __m128i b) {
    fprintf(stderr, "TODO\n");
    exit(0);
	return a;
}

static inline __m128i _mm_set1_epi16(int16_t n) {
    fprintf(stderr, "TODO\n");
    exit(0);
	__m128i r;
	return r;
}

static inline int16_t m128i_max_s16(__m128i a) {
    fprintf(stderr, "TODO\n");
    exit(0);
	int16_t max = -32768;
	return max;
}

static inline __m128i _mm_subs_epu16(__m128i a, __m128i b) {
    fprintf(stderr, "TODO\n");
    exit(0);
	return a;
}

#endif
