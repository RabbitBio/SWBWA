#ifndef SCALAR_SSE_H
#define SCALAR_SSE_H

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "simd.h"

//#define use_float16_vec

# ifdef use_float16_vec

typedef union m128i {
    float16v32 val;
} __m128i;


static inline __m128i _mm_set1_epi32(int32_t n) {
	assert(n >= 0 && n <= 255);
	__m128i r;
    r.val = 1.0f * n;
    return r;
}

int is_valid_address(void* ptr) {
    uintptr_t addr = (uintptr_t)ptr;
    if ((addr & 0xFF0000000000) != 0x400000000000) {
        return 0;
    }
    if ((addr & 0x3F) != 0) {
        return 0;
    }
    return 1;
}

static inline __m128i _mm_load_si128(const __m128i *ptr) {
	_Float16* new_ptr = (_Float16*)ptr;
	//if(is_valid_address(new_ptr) == 0) {
	//	fprintf(stderr, "GG load ptr %p\n", new_ptr);
	//}
    __m128i r;
    simd_load(r.val, new_ptr); 
    return r; 
}
static inline void _mm_store_si128(__m128i *ptr, __m128i a) { 
	//_Float16* new_ptr = (_Float16*)ptr;
	//if(is_valid_address(new_ptr) == 0) {
	//	fprintf(stderr, "GG store ptr %p\n", new_ptr);
	//}
    simd_store(a.val, ptr);
}

static inline int m128i_allzero(__m128i a) {
    //intv16 xx = a.val;
    //xx = simd_vbisw(xx, simd_sllx(xx, 8 * 32));
    //xx = simd_vbisw(xx, simd_sllx(xx, 4 * 32));
    //xx = simd_vbisw(xx, simd_sllx(xx, 2 * 32));
    //xx = simd_vbisw(xx, simd_sllx(xx, 1 * 32));
    //return simd_vextw15(xx) == 0;

    _Float16 val[32];
    simd_store(a.val, &(val[0]));
    for(int i = 0; i < 32; i++) {
        if(fabs(val[i]) > 1e-3) return 0;
    }
	return 1;
}

static inline __m128i _mm_slli_si128(__m128i a, int n) {
    _Float16 val[32];
    simd_store(a.val, &(val[0]));
    memmove(&val[n], &val[0], sizeof(int) * (32 - n));
    for (int i = 0; i < n; i++) val[i] = 0;
    __m128i r;
    simd_load(r.val, &val[0]);
    //__m128i r;
    //r.val = simd_sllx(a.val, n * 32);
	return r;
}

static inline __m128i _mm_max_epu8(__m128i a, __m128i b) {
    a.val = simd_smaxh(a.val, b.val);
	return a;
}

static inline __m128i _mm_min_epu8(__m128i a, __m128i b) {
    a.val = simd_sminh(a.val, b.val);
    return a;
}


static inline uint8_t m128i_max_u8(__m128i a) {
    _Float16 val[32];
    simd_store(a.val, &(val[0]));
	_Float16 max = 0;
	for (int i = 0; i < 32; i++)
		if (max < val[i]) max = val[i];
	return (uint8_t)max;
}

static inline __m128i _mm_set1_epi8(int8_t n) {
	__m128i r;
    r.val = 1.0f * n;
    return r;
}

static inline __m128i _mm_adds_epu8(__m128i a, __m128i b) {
    static float16v32 con_255 = 255.0f;
    a.val = simd_vaddh(a.val, b.val);
    a.val = simd_sminh(a.val, con_255);
	return a;
}

static inline __m128i _mm_subs_epu8(__m128i a, __m128i b) {
    static float16v32 con_0 = 0.0f;
    a.val = simd_vsubh(a.val, b.val);
    a.val = simd_smaxh(a.val, con_0);
	return a;
}


# else
typedef union m128i {
    intv16 val;
} __m128i;

//#define use_2_int8

uintv16 v_min = 0;
uintv16 v_max = 0xFFFFFFFF;

intv16 v_low8 = 255;
intv16 v_mid8 = 255 << 16;

#  define my_int int16_t

#  ifdef use_2_int8
static inline __m128i _mm_set1_epi32(int32_t n) {
	assert(n >= 0 && n <= 255);
	__m128i r;
    r.val = (n << 16) | n;
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

    //int val[16];
    //simd_store(a.val, &(val[0]));
    //for(int i = 0; i < 16; i++) 
    //    if(val[i]) return 0;
    //return 1;

    intv16 xx = a.val;
    xx = simd_vbisw(xx, simd_sllx(xx, 8 * 32));
    xx = simd_vbisw(xx, simd_sllx(xx, 4 * 32));
    xx = simd_vbisw(xx, simd_sllx(xx, 2 * 32));
    xx = simd_vbisw(xx, simd_sllx(xx, 1 * 32));
    return simd_vextw15(xx) == 0;
}

static inline __m128i _mm_slli_si128(__m128i a, int n) {
    __m128i r;
    r.val = simd_sllx(a.val, n * 16);
	return r;
}


static inline __m128i _mm_max_epu8(__m128i a, __m128i b) {
    intv16 mask = simd_vcmpltw(a.val, b.val);
    uintv16 extended_mask = simd_vsubw(v_min, mask);
    intv16 b_selected2 = simd_vandw(extended_mask2, b.val);
    extended_mask2 = simd_vxorw(extended_mask2, v_max);
    intv16 a_selected2 = simd_vandw(extended_mask2, a.val);


    intv16 a1 = simd_vandw(v_low8, a.val);
    intv16 b1 = simd_vandw(v_low8, b.val);
 
    intv16 mask1 = simd_vcmpltw(a1, b1);
    uintv16 extended_mask1 = simd_vsubw(v_min, mask1);
    intv16 b_selected1 = simd_vandw(extended_mask1, b1);
    extended_mask1 = simd_vxorw(extended_mask1, v_max);
    intv16 a_selected1 = simd_vandw(extended_mask1, a1);
 
    intv16 mask2 = simd_vcmpltw(a.val, b.val);
    uintv16 extended_mask2 = simd_vsubw(v_min, mask2);
    intv16 b_selected2 = simd_vandw(extended_mask2, b.val);
    extended_mask2 = simd_vxorw(extended_mask2, v_max);
    intv16 a_selected2 = simd_vandw(extended_mask2, a.val);

    a.val = simd_vaddw(simd_vaddw(a_selected1, b_selected1), simd_vandw(v_mid8, simd_vaddw(a_selected2, b_selected2)));

    return a;

    
 
    //int val1[16];
    //simd_store(a.val, &(val1[0]));
    //int val2[16];
    //simd_store(b.val, &(val2[0]));
    //for(int i = 0; i < 16; i++) {
    //    my_int v1 = val1[i] & 0xFF;
    //    my_int v2 = (val1[i] >> 16) & 0xFF;
    //    my_int v3 = val2[i] & 0xFF;
    //    my_int v4 = (val2[i] >> 16) & 0xFF;
    //    val1[i] = (v1 > v3 ? v1 : v3) + ((v2 > v4 ? v2 : v4) << 16);
    //}
    //__m128i r;
    //simd_load(r.val, &(val1[0])); 
	//return r;
}

static inline __m128i _mm_min_epu8(__m128i a, __m128i b) {
    intv16 a1 = simd_vandw(v_low8, a.val);
    intv16 b1 = simd_vandw(v_low8, b.val);
    intv16 a2 = simd_vandw(v_mid8, a.val);
    intv16 b2 = simd_vandw(v_mid8, b.val);

    intv16 mask1 = simd_vcmpltw(a1, b1);
    uintv16 extended_mask1 = simd_vsubw(v_min, mask1);
    intv16 a_selected1 = simd_vandw(extended_mask1, a1);
    extended_mask1 = simd_vxorw(extended_mask1, v_max);
    intv16 b_selected1 = simd_vandw(extended_mask1, b1);
    a1 = simd_vaddw(a_selected1, b_selected1);
 
    intv16 mask2 = simd_vcmpltw(a2, b2);
    uintv16 extended_mask2 = simd_vsubw(v_min, mask2);
    intv16 a_selected2 = simd_vandw(extended_mask2, a2);
    extended_mask2 = simd_vxorw(extended_mask2, v_max);
    intv16 b_selected2 = simd_vandw(extended_mask2, b2);
    a2 = simd_vaddw(a_selected2, b_selected2);

    a.val = simd_vaddw(a1, a2);

    return a;
    

    //int val1[16];
    //simd_store(a.val, &(val1[0]));
    //int val2[16];
    //simd_store(b.val, &(val2[0]));
    //for(int i = 0; i < 16; i++) {
    //    my_int v1 = val1[i] & 0xFF;
    //    my_int v2 = (val1[i] >> 16) & 0xFF;
    //    my_int v3 = val2[i] & 0xFF;
    //    my_int v4 = (val2[i] >> 16) & 0xFF;
    //    val1[i] = (v1 < v3 ? v1 : v3) + ((v2 < v4 ? v2 : v4) << 16);
    //}
    //__m128i r;
    //simd_load(r.val, &(val1[0])); 
	//return r;
}


static inline uint8_t m128i_max_u8(__m128i a) {
    int val[16];
    simd_store(a.val, &(val[0]));
	int max = 0;
	for (int i = 0; i < 16; i++) {
        my_int v1 = val[i] & 0xFF;
        my_int v2 = (val[i] >> 16) & 0xFF;
        if(v1 > max) max = v1;
        if(v2 > max) max = v2;
    }
	return max;
}

static inline __m128i _mm_set1_epi8(int8_t n) {
	__m128i r;
    r.val = (n << 16) | n; 
    return r;
}

static inline __m128i _mm_adds_epu8(__m128i a, __m128i b) {
    intv16 a1 = simd_vandw(v_low8, a.val);
    intv16 b1 = simd_vandw(v_low8, b.val);
    intv16 a2 = simd_vandw(v_mid8, a.val);
    intv16 b2 = simd_vandw(v_mid8, b.val);

    a1= simd_vaddw(a1, b1);
    intv16 mask1 = simd_vcmpltw(a1, v_low8);
    uintv16 extended_mask1 = simd_vsubw(v_min, mask1);
    intv16 a_selected1 = simd_vandw(extended_mask1, a1);
    extended_mask1 = simd_vxorw(extended_mask1, v_max);
    intv16 b_selected1 = simd_vandw(extended_mask1, v_low8);
    a1= simd_vaddw(a_selected1, b_selected1);

    a2= simd_vaddw(a2, b2);
    intv16 mask2 = simd_vcmpltw(a2, v_mid8);
    uintv16 extended_mask2 = simd_vsubw(v_min, mask2);
    intv16 a_selected2 = simd_vandw(extended_mask2, a2);
    extended_mask2 = simd_vxorw(extended_mask2, v_max);
    intv16 b_selected2 = simd_vandw(extended_mask2, v_mid8);
    a2= simd_vaddw(a_selected2, b_selected2);

    a.val = simd_vaddw(a1, a2);
    return a;



    //int val1[16];
    //simd_store(a.val, &(val1[0]));
    //int val2[16];
    //simd_store(b.val, &(val2[0]));
    //for(int i = 0; i < 16; i++) {
    //    my_int v1 = val1[i] & 0xFF;
    //    my_int v2 = (val1[i] >> 16) & 0xFF;
    //    my_int v3 = val2[i] & 0xFF;
    //    my_int v4 = (val2[i] >> 16) & 0xFF;
    //    v1 += v3;
    //    if(v1 > 255) v1 = 255;
    //    v2 += v4;
    //    if(v2 > 255) v2 = 255;
    //    val1[i] = v1 + (v2 << 16);
    //}
    //__m128i r;
    //simd_load(r.val, &(val1[0])); 
	//return r;
}

static inline __m128i _mm_subs_epu8(__m128i a, __m128i b) {
    intv16 a1 = simd_vandw(v_low8, a.val);
    intv16 b1 = simd_vandw(v_low8, b.val);
    intv16 a2 = simd_vandw(v_mid8, a.val);
    intv16 b2 = simd_vandw(v_mid8, b.val);

    a1= simd_vsubw(a1, b1);
    intv16 mask1 = simd_vcmpltw(a1, v_min);
    uintv16 extended_mask1 = simd_vsubw(v_min, mask1);
    intv16 b_selected1 = simd_vandw(extended_mask1, v_min);
    extended_mask1 = simd_vxorw(extended_mask1, v_max);
    intv16 a_selected1 = simd_vandw(extended_mask1, a1);
    a1= simd_vaddw(a_selected1, b_selected1);

    a2= simd_vsubw(a2, b2);
    intv16 mask2 = simd_vcmpltw(a2, v_min);
    uintv16 extended_mask2 = simd_vsubw(v_min, mask2);
    intv16 b_selected2 = simd_vandw(extended_mask2, v_min);
    extended_mask2 = simd_vxorw(extended_mask2, v_max);
    intv16 a_selected2 = simd_vandw(extended_mask2, a2);
    a2= simd_vaddw(a_selected2, b_selected2);

    a.val = simd_vaddw(a1, a2);
    return a;


    //int val1[16];
    //simd_store(a.val, &(val1[0]));
    //int val2[16];
    //simd_store(b.val, &(val2[0]));
    //for(int i = 0; i < 16; i++) {
    //    int v1 = val1[i] & 0xFF;
    //    int v2 = (val1[i] >> 16) & 0xFF;
    //    int v3 = val2[i] & 0xFF;
    //    int v4 = (val2[i] >> 16) & 0xFF;
    //    v1 -= v3;
    //    if(v1 < 0) v1 = 0;
    //    v2 -= v4;
    //    if(v2 < 0) v2 = 0;
    //    val1[i] = v1 + (v2 << 16);
    //}
    //__m128i r;
    //simd_load(r.val, &(val1[0])); 
	//return r;
}
#  else
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
#  endif
# endif

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
