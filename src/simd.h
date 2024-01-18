#ifndef PEACEKEEPER_SIMD
#define PEACEKEEPER_SIMD

#if defined(__AVX__) || defined(__AVX2__) || (defined(__AVX512F__) && defined(__AVX512BW__) && defined(__AVX512DQ__))
#include <immintrin.h>
#define SIMD
#endif

#if defined(__AVX512F__) && defined(__AVX512BW__) && defined(__AVX512DQ__)
#define BIT_ALIGNMENT 512
#elif defined(__AVX2__) || defined(__AVX__)
#define BIT_ALIGNMENT 256
#endif

#define I16_STRIDE (BIT_ALIGNMENT / 16)
#define ALIGNMENT (BIT_ALIGNMENT / 8)

#if defined(__AVX512F__) && defined(__AVX512BW__) && defined(__AVX512DQ__)
using register_type = __m512i;
#define register_madd_16 _mm512_madd_epi16
#define register_add_32 _mm512_add_epi32
#define register_sub_32 _mm512_sub_epi32
#define register_add_16 _mm512_add_epi16
#define register_sub_16 _mm512_sub_epi16
#define register_min_16 _mm512_min_epi16
#define register_max_16 _mm512_max_epi16
#define register_set_16 _mm512_set1_epi16
#define register_mul_16 _mm512_mullo_epi16
#elif defined(__AVX2__) || defined(__AVX__)
using register_type = __m256i;
#define register_madd_16 _mm256_madd_epi16
#define register_add_32 _mm256_add_epi32
#define register_sub_32 _mm256_sub_epi32
#define register_add_16 _mm256_add_epi16
#define register_sub_16 _mm256_sub_epi16
#define register_min_16 _mm256_min_epi16
#define register_max_16 _mm256_max_epi16
#define register_set_16 _mm256_set1_epi16
#define register_mul_16 _mm256_mullo_epi16
#endif

#ifdef SIMD
inline int32_t register_sum_32(register_type& reg) {
#if defined(__AVX512F__) && defined(__AVX512BW__) && defined(__AVX512DQ__)
    const __m256i reduced_8 = _mm256_add_epi32(_mm512_castsi512_si256(reg), _mm512_extracti32x8_epi32(reg, 1));
#elif defined(__AVX2__) || defined(__AVX__)
    const __m256i reduced_8 = reg;
#endif
    const __m128i reduced_4 = _mm_add_epi32(_mm256_castsi256_si128(reduced_8), _mm256_extractf128_si256(reduced_8, 1));
    __m128i vsum = _mm_add_epi32(reduced_4, _mm_srli_si128(reduced_4, 8));
    vsum = _mm_add_epi32(vsum, _mm_srli_si128(vsum, 4));
    int32_t sums = _mm_cvtsi128_si32(vsum);
    return sums;
}
#endif

#endif