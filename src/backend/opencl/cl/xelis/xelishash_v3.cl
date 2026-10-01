#define CHACHA_QR(a,b,c,d) do { a+=b;d^=a;d=rotl32_16(d);c+=d;b^=c;b=rotl32(b,12);a+=b;d^=a;d=rotl32_8(d);c+=d;b^=c;b=rotl32(b,7); } while(0);
inline float native_reciprocal(float x){return 1.0f/x;} 
#define __ldg(p) (*(p))
#define U128_LT(a,b,c,d) ((a)<(c)||((a)==(c)&&(b)<(d)))
#define U128_GT(a,b,c,d) ((a)>(c)||((a)==(c)&&(b)>(d)))
// Generated OpenCL XelisHash v3 mono kernel.
#pragma OPENCL EXTENSION cl_khr_fp64 : enable
typedef uchar uint8_t;
typedef ushort uint16_t;typedef uint uint32_t;typedef ulong uint64_t;typedef long int64_t;
#define NEWTON_LUT_SHIFT 22
#define NEWTON_LUT_BASE 512
#define XELIS_HOT
inline ulong xelis_mul_hi(ulong a,ulong b){return mul_hi(a,b);} inline uint xelis_mul_hi32(uint a,uint b){return mul_hi(a,b);} inline int xelis_ffsll(ulong x){return x?(ctz(x)+1):0;}
inline uint32_t rotl32(uint32_t x,uint32_t r){r&=31;return(x<<r)|(x>>((32-r)&31));} inline uint32_t rotl32_16(uint32_t x){return(x<<16)|(x>>16);} inline uint32_t rotl32_8(uint32_t x){return(x<<8)|(x>>24);}


inline ulong load_scratch(__generic const ulong*p){return*p;} inline void store_scratch(__generic ulong*p,ulong v){*p=v;} inline void store_streaming_u256(__generic ulong*p,ulong a,ulong b,ulong c,ulong d){p[0]=a;p[1]=b;p[2]=c;p[3]=d;}
typedef struct{ulong q;ulong r;} divmod_result; const size_t XELIS_MEMORY_SIZE_V3=531*128,XELIS_BUFFER_SIZE_V3=XELIS_MEMORY_SIZE_V3/2; const uint16_t XELIS_SCRATCHPAD_ITERS_V3=2; const size_t XELIS_HASH_SIZE=32,XELIS_TEMPLATE_SIZE=112,XELIS_CHUNK_SIZE=32,XELIS_OUTPUT_SIZE_V3=XELIS_MEMORY_SIZE_V3*8,XELIS_BYTES_PER_CHUNK_V3=XELIS_OUTPUT_SIZE_V3/4;
#define NEWTON_LUT_SHIFT 22
#define NEWTON_LUT_BASE 512
__constant uint64_t XELIS_MURMUR_CONST1=0xff51afd7ed558ccdUL,XELIS_MURMUR_CONST2=0xc4ceb9fe1a85ec53UL,XELIS_GOLDEN_RATIO=0x9e3779b97f4a7c15UL,XELIS_SCATTER_CONST=0xd2b74407b1ce6e93UL,XELIS_PICK_HALF_BIT=(1UL<<58);__constant uint8_t AES_KEY[16]={'x','e','l','i','s','h','a','s','h','-','p','o','w','-','v','3'};inline ulong ROTL(ulong x,uint r){r&=63;return(x<<r)|(x>>((-r)&63));}inline ulong ROTR(ulong x,uint r){r&=63;return(x>>r)|(x<<((-r)&63));}
#pragma once

// blake3-inline.hip.inc - Optimized for AMD HIP and NVIDIA CUDA
// Platform-specific rotation optimizations

#define BLAKE3_KEY_LEN 32
#define BLAKE3_OUT_LEN 32
#define BLAKE3_BLOCK_LEN 64
#define BLAKE3_CHUNK_LEN 1024
#define BLAKE3_MAX_DEPTH 54

// Split IV constants (enables direct constant embedding)
#define IV_0 0x6A09E667UL
#define IV_1 0xBB67AE85UL
#define IV_2 0x3C6EF372UL
#define IV_3 0xA54FF53AUL
#define IV_4 0x510E527FUL
#define IV_5 0x9B05688CUL
#define IV_6 0x1F83D9ABUL
#define IV_7 0x5BE0CD19UL

// Flags
#define CHUNK_START (1 << 0)
#define CHUNK_END   (1 << 1)
#define PARENT      (1 << 2)
#define ROOT        (1 << 3)

// ============================================================================
// Message schedule constants (compile-time lookup)
// ============================================================================
#define REF_Z00 0
#define REF_Z01 1
#define REF_Z02 2
#define REF_Z03 3
#define REF_Z04 4
#define REF_Z05 5
#define REF_Z06 6
#define REF_Z07 7
#define REF_Z08 8
#define REF_Z09 9
#define REF_Z0A 10
#define REF_Z0B 11
#define REF_Z0C 12
#define REF_Z0D 13
#define REF_Z0E 14
#define REF_Z0F 15

#define REF_Z10 2
#define REF_Z11 6
#define REF_Z12 3
#define REF_Z13 10
#define REF_Z14 7
#define REF_Z15 0
#define REF_Z16 4
#define REF_Z17 13
#define REF_Z18 1
#define REF_Z19 11
#define REF_Z1A 12
#define REF_Z1B 5
#define REF_Z1C 9
#define REF_Z1D 14
#define REF_Z1E 15
#define REF_Z1F 8

#define REF_Z20 3
#define REF_Z21 4
#define REF_Z22 10
#define REF_Z23 12
#define REF_Z24 13
#define REF_Z25 2
#define REF_Z26 7
#define REF_Z27 14
#define REF_Z28 6
#define REF_Z29 5
#define REF_Z2A 9
#define REF_Z2B 0
#define REF_Z2C 11
#define REF_Z2D 15
#define REF_Z2E 8
#define REF_Z2F 1

#define REF_Z30 10
#define REF_Z31 7
#define REF_Z32 12
#define REF_Z33 9
#define REF_Z34 14
#define REF_Z35 3
#define REF_Z36 13
#define REF_Z37 15
#define REF_Z38 4
#define REF_Z39 0
#define REF_Z3A 11
#define REF_Z3B 2
#define REF_Z3C 5
#define REF_Z3D 8
#define REF_Z3E 1
#define REF_Z3F 6

#define REF_Z40 12
#define REF_Z41 13
#define REF_Z42 9
#define REF_Z43 11
#define REF_Z44 15
#define REF_Z45 10
#define REF_Z46 14
#define REF_Z47 8
#define REF_Z48 7
#define REF_Z49 2
#define REF_Z4A 5
#define REF_Z4B 3
#define REF_Z4C 0
#define REF_Z4D 1
#define REF_Z4E 6
#define REF_Z4F 4

#define REF_Z50 9
#define REF_Z51 14
#define REF_Z52 11
#define REF_Z53 5
#define REF_Z54 8
#define REF_Z55 12
#define REF_Z56 15
#define REF_Z57 1
#define REF_Z58 13
#define REF_Z59 3
#define REF_Z5A 0
#define REF_Z5B 10
#define REF_Z5C 2
#define REF_Z5D 6
#define REF_Z5E 4
#define REF_Z5F 7

#define REF_Z60 11
#define REF_Z61 15
#define REF_Z62 5
#define REF_Z63 0
#define REF_Z64 1
#define REF_Z65 9
#define REF_Z66 8
#define REF_Z67 6
#define REF_Z68 14
#define REF_Z69 10
#define REF_Z6A 2
#define REF_Z6B 12
#define REF_Z6C 3
#define REF_Z6D 4
#define REF_Z6E 7
#define REF_Z6F 13

#if defined(__HIPCC_RTC__) || defined(__CUDACC_RTC__)
#include "hiprtc_types.hip.h"
#elif defined(__CUDACC__)
#include <stdint.h>
#endif

// ============================================================================
// Rotation primitives - Platform-specific optimizations
// ============================================================================

// 16-bit rotation using byte permute (both platforms)
 inline uint32_t blake3_rotr16(uint32_t x) {
#if defined(__HIP_DEVICE_COMPILE__) || defined(__CUDA_ARCH__)
    return __byte_perm(x, x, 0x1032);
#else
    return (x >> 16) | (x << 16);
#endif
}

// 8-bit rotation using byte permute (both platforms)
 inline uint32_t blake3_rotr8(uint32_t x) {
#if defined(__HIP_DEVICE_COMPILE__) || defined(__CUDA_ARCH__)
    return __byte_perm(x, x, 0x0321);
#else
    return (x >> 8) | (x << 24);
#endif
}

#if defined(__HIP_PLATFORM_NVIDIA__) || defined(__CUDACC_RTC__) || defined(__CUDACC__)

// NVIDIA: Use funnel shifts for 12-bit and 7-bit rotations
 inline uint32_t blake3_rotr12(uint32_t x) {
    uint32_t result;
    asm("shf.r.wrap.b32 %0, %1, %2, 12;" : "=r"(result) : "r"(x), "r"(x));
    return result;
}

 inline uint32_t blake3_rotr7(uint32_t x) {
    uint32_t result;
    asm("shf.r.wrap.b32 %0, %1, %2, 7;" : "=r"(result) : "r"(x), "r"(x));
    return result;
}

// Generic rotation for other values (shouldn't be needed in BLAKE3)
 inline uint32_t blake3_rotr32(uint32_t x, uint32_t n) {
    uint32_t result;
    asm("shf.r.wrap.b32 %0, %1, %2, %3;" : "=r"(result) : "r"(x), "r"(x), "r"(n));
    return result;
}

#else

// AMD: Let compiler optimize (it handles alignbit automatically)
 inline uint32_t blake3_rotr12(uint32_t x) {
    return (x >> 12) | (x << 20);
}

 inline uint32_t blake3_rotr7(uint32_t x) {
    return (x >> 7) | (x << 25);
}

 inline uint32_t blake3_rotr32(uint32_t x, uint32_t n) {
    return (x >> n) | (x << (32 - n));
}

#endif

// ============================================================================
// Byte loading (explicit, avoids alignment issues)
// ============================================================================

 inline uint32_t blake3_load32(__generic const uint8_t * src) {
    return (uint32_t)src[0] |
           ((uint32_t)src[1] << 8) |
           ((uint32_t)src[2] << 16) |
           ((uint32_t)src[3] << 24);
}

 inline void blake3_store32(uint8_t* dst, uint32_t val) {
    dst[0] = (uint8_t)(val);
    dst[1] = (uint8_t)(val >> 8);
    dst[2] = (uint8_t)(val >> 16);
    dst[3] = (uint8_t)(val >> 24);
}

// ============================================================================
// G function macro - uses specialized rotations
// ============================================================================

#define BLAKE3_G(a, b, c, d, x, y)                      \
    do {                                                \
        state[a] = state[a] + state[b] + (x);           \
        state[d] = blake3_rotr16(state[d] ^ state[a]);  \
        state[c] = state[c] + state[d];                 \
        state[b] = blake3_rotr12(state[b] ^ state[c]);  \
        state[a] = state[a] + state[b] + (y);           \
        state[d] = blake3_rotr8(state[d] ^ state[a]);   \
        state[c] = state[c] + state[d];                 \
        state[b] = blake3_rotr7(state[b] ^ state[c]);   \
    } while (0)

// Message word lookup macro
#define BLAKE3_MSG(r, i) (block_words[REF_Z##r##i])

// Full round macro
#define BLAKE3_ROUND(r)                                             \
    do {                                                            \
        BLAKE3_G(0, 4, 8,  12, BLAKE3_MSG(r, 0), BLAKE3_MSG(r, 1)); \
        BLAKE3_G(1, 5, 9,  13, BLAKE3_MSG(r, 2), BLAKE3_MSG(r, 3)); \
        BLAKE3_G(2, 6, 10, 14, BLAKE3_MSG(r, 4), BLAKE3_MSG(r, 5)); \
        BLAKE3_G(3, 7, 11, 15, BLAKE3_MSG(r, 6), BLAKE3_MSG(r, 7)); \
        BLAKE3_G(0, 5, 10, 15, BLAKE3_MSG(r, 8), BLAKE3_MSG(r, 9)); \
        BLAKE3_G(1, 6, 11, 12, BLAKE3_MSG(r, A), BLAKE3_MSG(r, B)); \
        BLAKE3_G(2, 7, 8,  13, BLAKE3_MSG(r, C), BLAKE3_MSG(r, D)); \
        BLAKE3_G(3, 4, 9,  14, BLAKE3_MSG(r, E), BLAKE3_MSG(r, F)); \
    } while (0)

// ============================================================================
// CV state initialization
// ============================================================================

 inline void blake3_cv_init(uint32_t cv[8]) {
    cv[0] = IV_0;
    cv[1] = IV_1;
    cv[2] = IV_2;
    cv[3] = IV_3;
    cv[4] = IV_4;
    cv[5] = IV_5;
    cv[6] = IV_6;
    cv[7] = IV_7;
}

// ============================================================================
// Core compression function
// ============================================================================

 inline void blake3_compress_in_place(
    uint32_t cv[8],
    __generic const uint8_t block[BLAKE3_BLOCK_LEN],
    uint8_t block_len,
    uint64_t counter,
    uint8_t flags)
{
    uint32_t state[16];
    uint32_t block_words[16];
    
    // Load block words
    for (int i = 0; i < 16; i++) {
        block_words[i] = blake3_load32(block + i * 4);
    }
    
    // Initialize state
    state[0] = cv[0];
    state[1] = cv[1];
    state[2] = cv[2];
    state[3] = cv[3];
    state[4] = cv[4];
    state[5] = cv[5];
    state[6] = cv[6];
    state[7] = cv[7];
    state[8] = IV_0;
    state[9] = IV_1;
    state[10] = IV_2;
    state[11] = IV_3;
    state[12] = (uint32_t)counter;
    state[13] = (uint32_t)(counter >> 32);
    state[14] = (uint32_t)block_len;
    state[15] = (uint32_t)flags;
    
    // 7 rounds using compile-time message schedule
    BLAKE3_ROUND(0);
    BLAKE3_ROUND(1);
    BLAKE3_ROUND(2);
    BLAKE3_ROUND(3);
    BLAKE3_ROUND(4);
    BLAKE3_ROUND(5);
    BLAKE3_ROUND(6);
    
    // Finalize
    cv[0] = state[0] ^ state[8];
    cv[1] = state[1] ^ state[9];
    cv[2] = state[2] ^ state[10];
    cv[3] = state[3] ^ state[11];
    cv[4] = state[4] ^ state[12];
    cv[5] = state[5] ^ state[13];
    cv[6] = state[6] ^ state[14];
    cv[7] = state[7] ^ state[15];
}

// ============================================================================
// Single-chunk hash (input_len <= 1024) - No streaming stores needed
// ============================================================================

 inline void blake3_hash_single_chunk(
    __generic const uint8_t * input,
    size_t input_len,
    uint8_t output[BLAKE3_OUT_LEN])
{
    uint32_t cv[8];
    blake3_cv_init(cv);
    
    uint8_t block[BLAKE3_BLOCK_LEN];
    size_t offset = 0;
    
    while (offset < input_len || offset == 0) {
        size_t take = (input_len - offset >= BLAKE3_BLOCK_LEN) ? 
                      BLAKE3_BLOCK_LEN : (input_len - offset);
        
        // Copy and zero-pad
        for (size_t i = 0; i < BLAKE3_BLOCK_LEN; i++) {
            block[i] = (i < take) ? input[offset + i] : 0;
        }
        
        uint8_t flags = 0;
        if (offset == 0) flags |= CHUNK_START;
        if (offset + BLAKE3_BLOCK_LEN >= input_len) flags |= CHUNK_END | ROOT;
        
        blake3_compress_in_place(cv, block, take, 0, flags);
        
        offset += BLAKE3_BLOCK_LEN;
        if (input_len == 0) break;
    }
    for (int i = 0; i < 8; i++) {
        blake3_store32(output + i * 4, cv[i]);
    }
}

// ============================================================================
// Chunk CV computation (for multi-chunk)
// ============================================================================

 inline void blake3_chunk_cv(
    __generic const uint8_t * input,
    size_t chunk_len,
    uint64_t chunk_counter,
    uint8_t cv_out[BLAKE3_OUT_LEN],
    bool is_root)
{
    uint32_t cv[8];
    blake3_cv_init(cv);
    
    uint8_t block[BLAKE3_BLOCK_LEN];
    size_t offset = 0;
    
    while (offset < chunk_len || offset == 0) {
        size_t take = (chunk_len - offset >= BLAKE3_BLOCK_LEN) ?
                      BLAKE3_BLOCK_LEN : (chunk_len - offset);
        for (size_t i = 0; i < BLAKE3_BLOCK_LEN; i++) {
            block[i] = (i < take) ? input[offset + i] : 0;
        }
        
        uint8_t flags = 0;
        if (offset == 0) flags |= CHUNK_START;
        
        bool is_last_block = (offset + BLAKE3_BLOCK_LEN >= chunk_len) || (chunk_len == 0);
        if (is_last_block) {
            flags |= CHUNK_END;
            if (is_root) flags |= ROOT;
        }
        
        blake3_compress_in_place(cv, block, take, chunk_counter, flags);
        
        offset += BLAKE3_BLOCK_LEN;
        if (chunk_len == 0) break;
    }
    for (int i = 0; i < 8; i++) {
        blake3_store32(cv_out + i * 4, cv[i]);
    }
}

// ============================================================================
// Parent node hash
// ============================================================================

 inline void blake3_hash_parent(
    const uint8_t left_cv[BLAKE3_OUT_LEN],
    const uint8_t right_cv[BLAKE3_OUT_LEN],
    uint8_t output[BLAKE3_OUT_LEN],
    bool is_root)
{
    uint32_t cv[8];
    blake3_cv_init(cv);
    
    uint8_t block[BLAKE3_BLOCK_LEN];
    for (int i = 0; i < 32; i++) {
        block[i] = left_cv[i];
        block[i + 32] = right_cv[i];
    }
    
    uint8_t flags = PARENT;
    if (is_root) flags |= ROOT;
    
    blake3_compress_in_place(cv, block, BLAKE3_BLOCK_LEN, 0, flags);
    for (int i = 0; i < 8; i++) {
        blake3_store32(output + i * 4, cv[i]);
    }
}

// ============================================================================
// Streaming store helpers for final output (platform-specific)
// ============================================================================
// AES Tables
// ============================================================================

__constant uint8_t AES_SBOX[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16};

__constant uint8_t GF_MUL2[256] = {
    0x00, 0x02, 0x04, 0x06, 0x08, 0x0a, 0x0c, 0x0e, 0x10, 0x12, 0x14, 0x16, 0x18, 0x1a, 0x1c, 0x1e,
    0x20, 0x22, 0x24, 0x26, 0x28, 0x2a, 0x2c, 0x2e, 0x30, 0x32, 0x34, 0x36, 0x38, 0x3a, 0x3c, 0x3e,
    0x40, 0x42, 0x44, 0x46, 0x48, 0x4a, 0x4c, 0x4e, 0x50, 0x52, 0x54, 0x56, 0x58, 0x5a, 0x5c, 0x5e,
    0x60, 0x62, 0x64, 0x66, 0x68, 0x6a, 0x6c, 0x6e, 0x70, 0x72, 0x74, 0x76, 0x78, 0x7a, 0x7c, 0x7e,
    0x80, 0x82, 0x84, 0x86, 0x88, 0x8a, 0x8c, 0x8e, 0x90, 0x92, 0x94, 0x96, 0x98, 0x9a, 0x9c, 0x9e,
    0xa0, 0xa2, 0xa4, 0xa6, 0xa8, 0xaa, 0xac, 0xae, 0xb0, 0xb2, 0xb4, 0xb6, 0xb8, 0xba, 0xbc, 0xbe,
    0xc0, 0xc2, 0xc4, 0xc6, 0xc8, 0xca, 0xcc, 0xce, 0xd0, 0xd2, 0xd4, 0xd6, 0xd8, 0xda, 0xdc, 0xde,
    0xe0, 0xe2, 0xe4, 0xe6, 0xe8, 0xea, 0xec, 0xee, 0xf0, 0xf2, 0xf4, 0xf6, 0xf8, 0xfa, 0xfc, 0xfe,
    0x1b, 0x19, 0x1f, 0x1d, 0x13, 0x11, 0x17, 0x15, 0x0b, 0x09, 0x0f, 0x0d, 0x03, 0x01, 0x07, 0x05,
    0x3b, 0x39, 0x3f, 0x3d, 0x33, 0x31, 0x37, 0x35, 0x2b, 0x29, 0x2f, 0x2d, 0x23, 0x21, 0x27, 0x25,
    0x5b, 0x59, 0x5f, 0x5d, 0x53, 0x51, 0x57, 0x55, 0x4b, 0x49, 0x4f, 0x4d, 0x43, 0x41, 0x47, 0x45,
    0x7b, 0x79, 0x7f, 0x7d, 0x73, 0x71, 0x77, 0x75, 0x6b, 0x69, 0x6f, 0x6d, 0x63, 0x61, 0x67, 0x65,
    0x9b, 0x99, 0x9f, 0x9d, 0x93, 0x91, 0x97, 0x95, 0x8b, 0x89, 0x8f, 0x8d, 0x83, 0x81, 0x87, 0x85,
    0xbb, 0xb9, 0xbf, 0xbd, 0xb3, 0xb1, 0xb7, 0xb5, 0xab, 0xa9, 0xaf, 0xad, 0xa3, 0xa1, 0xa7, 0xa5,
    0xdb, 0xd9, 0xdf, 0xdd, 0xd3, 0xd1, 0xd7, 0xd5, 0xcb, 0xc9, 0xcf, 0xcd, 0xc3, 0xc1, 0xc7, 0xc5,
    0xfb, 0xf9, 0xff, 0xfd, 0xf3, 0xf1, 0xf7, 0xf5, 0xeb, 0xe9, 0xef, 0xed, 0xe3, 0xe1, 0xe7, 0xe5};

__constant uint8_t GF_MUL3[256] = {
    0x00, 0x03, 0x06, 0x05, 0x0c, 0x0f, 0x0a, 0x09, 0x18, 0x1b, 0x1e, 0x1d, 0x14, 0x17, 0x12, 0x11,
    0x30, 0x33, 0x36, 0x35, 0x3c, 0x3f, 0x3a, 0x39, 0x28, 0x2b, 0x2e, 0x2d, 0x24, 0x27, 0x22, 0x21,
    0x60, 0x63, 0x66, 0x65, 0x6c, 0x6f, 0x6a, 0x69, 0x78, 0x7b, 0x7e, 0x7d, 0x74, 0x77, 0x72, 0x71,
    0x50, 0x53, 0x56, 0x55, 0x5c, 0x5f, 0x5a, 0x59, 0x48, 0x4b, 0x4e, 0x4d, 0x44, 0x47, 0x42, 0x41,
    0xc0, 0xc3, 0xc6, 0xc5, 0xcc, 0xcf, 0xca, 0xc9, 0xd8, 0xdb, 0xde, 0xdd, 0xd4, 0xd7, 0xd2, 0xd1,
    0xf0, 0xf3, 0xf6, 0xf5, 0xfc, 0xff, 0xfa, 0xf9, 0xe8, 0xeb, 0xee, 0xed, 0xe4, 0xe7, 0xe2, 0xe1,
    0xa0, 0xa3, 0xa6, 0xa5, 0xac, 0xaf, 0xaa, 0xa9, 0xb8, 0xbb, 0xbe, 0xbd, 0xb4, 0xb7, 0xb2, 0xb1,
    0x90, 0x93, 0x96, 0x95, 0x9c, 0x9f, 0x9a, 0x99, 0x88, 0x8b, 0x8e, 0x8d, 0x84, 0x87, 0x82, 0x81,
    0x9b, 0x98, 0x9d, 0x9e, 0x97, 0x94, 0x91, 0x92, 0x83, 0x80, 0x85, 0x86, 0x8f, 0x8c, 0x89, 0x8a,
    0xab, 0xa8, 0xad, 0xae, 0xa7, 0xa4, 0xa1, 0xa2, 0xb3, 0xb0, 0xb5, 0xb6, 0xbf, 0xbc, 0xb9, 0xba,
    0xfb, 0xf8, 0xfd, 0xfe, 0xf7, 0xf4, 0xf1, 0xf2, 0xe3, 0xe0, 0xe5, 0xe6, 0xef, 0xec, 0xe9, 0xea,
    0xcb, 0xc8, 0xcd, 0xce, 0xc7, 0xc4, 0xc1, 0xc2, 0xd3, 0xd0, 0xd5, 0xd6, 0xdf, 0xdc, 0xd9, 0xda,
    0x5b, 0x58, 0x5d, 0x5e, 0x57, 0x54, 0x51, 0x52, 0x43, 0x40, 0x45, 0x46, 0x4f, 0x4c, 0x49, 0x4a,
    0x6b, 0x68, 0x6d, 0x6e, 0x67, 0x64, 0x61, 0x62, 0x73, 0x70, 0x75, 0x76, 0x7f, 0x7c, 0x79, 0x7a,
    0x3b, 0x38, 0x3d, 0x3e, 0x37, 0x34, 0x31, 0x32, 0x23, 0x20, 0x25, 0x26, 0x2f, 0x2c, 0x29, 0x2a,
    0x0b, 0x08, 0x0d, 0x0e, 0x07, 0x04, 0x01, 0x02, 0x13, 0x10, 0x15, 0x16, 0x1f, 0x1c, 0x19, 0x1a};

    // ============================================================================
// AES Round
// ============================================================================

 void aes_round(uint8_t *block, __constant const uint8_t *key)
{
  uint8_t tmp[16];
  for (int i = 0; i < 16; i++)
  {
    tmp[i] = AES_SBOX[block[i]];
  }

  uint8_t shifted[16];
  shifted[0] = tmp[0];
  shifted[1] = tmp[5];
  shifted[2] = tmp[10];
  shifted[3] = tmp[15];
  shifted[4] = tmp[4];
  shifted[5] = tmp[9];
  shifted[6] = tmp[14];
  shifted[7] = tmp[3];
  shifted[8] = tmp[8];
  shifted[9] = tmp[13];
  shifted[10] = tmp[2];
  shifted[11] = tmp[7];
  shifted[12] = tmp[12];
  shifted[13] = tmp[1];
  shifted[14] = tmp[6];
  shifted[15] = tmp[11];
  for (int col = 0; col < 4; col++)
  {
    int i = col * 4;
    uint8_t a0 = shifted[i], a1 = shifted[i + 1], a2 = shifted[i + 2], a3 = shifted[i + 3];
    block[i] = GF_MUL2[a0] ^ GF_MUL3[a1] ^ a2 ^ a3 ^ key[i];
    block[i + 1] = a0 ^ GF_MUL2[a1] ^ GF_MUL3[a2] ^ a3 ^ key[i + 1];
    block[i + 2] = a0 ^ a1 ^ GF_MUL2[a2] ^ GF_MUL3[a3] ^ key[i + 2];
    block[i + 3] = GF_MUL3[a0] ^ a1 ^ a2 ^ GF_MUL2[a3] ^ key[i + 3];
  }
}

 inline void aes_round_u64(uint64_t *lo, uint64_t *hi, __constant const uint8_t *key)
{
  uint8_t block[16];
  for (int i = 0; i < 8; i++)
  {
    block[i] = (*lo >> (i * 8)) & 0xFF;
    block[i + 8] = (*hi >> (i * 8)) & 0xFF;
  }

  aes_round(block, key);

  *lo = 0;
  *hi = 0;
  for (int i = 0; i < 8; i++)
  {
    *lo |= ((uint64_t)block[i]) << (i * 8);
    *hi |= ((uint64_t)block[i + 8]) << (i * 8);
  }
}
// ============================================================================
// Galois Field Multiplication (computed on-demand)
// ============================================================================

 inline uint8_t gf_mul2(uint8_t x) {
    // Multiply by 2 in GF(2^8): left shift with conditional XOR by 0x1B
    return (x << 1) ^ (((x >> 7) & 1) * 0x1B);
}
 inline uint64_t murmurhash3(uint64_t seed)
{
  seed ^= seed >> 55;
  seed *= XELIS_MURMUR_CONST1;
  seed ^= seed >> 32;
  seed *= XELIS_MURMUR_CONST2;
  seed ^= seed >> 15;
  return seed;
}
 inline uint64_t map_index(uint64_t x)
{
  x ^= x >> 33;
  x *= XELIS_MURMUR_CONST1;
  return xelis_mul_hi(x, XELIS_BUFFER_SIZE_V3);
}
 inline int pick_half(uint64_t v)
{
  return (murmurhash3(v) & XELIS_PICK_HALF_BIT) != 0;
}
 inline uint64_t isqrt(uint64_t a)
{
    if (a < 2) return a;

    uint64_t arg = a;

    // Normalize to even shift
    uint32_t scal = clz(a) & ~1;
    a <<= scal;
    uint32_t b = (uint32_t)(a >> 32);

    // rsqrt seed
    float fr = native_rsqrt((float)b);
    uint32_t r = (uint32_t)fma(1.407374884e14f, fr, -438.0f);

    // sqrt Ã¢â€°Ë† input Ãƒâ€” rsqrt
    uint32_t s = xelis_mul_hi32(r, b);  // 32Ãƒâ€”32Ã¢â€ â€™high32, works on both
    s = s * 2;

    // Newton-Raphson Ã¢â‚¬â€ compiler emits optimal wide multiply for (uint64_t)s * s
    uint64_t rem = a - (uint64_t)s * s;
    r = xelis_mul_hi32((uint32_t)(rem >> 32) + 1, r);
    s = s + r;

    // Denormalize
    s = s >> (scal >> 1);

    // Branchless correction Ã¢â‚¬â€ eliminates warp divergence
    uint64_t sq = (uint64_t)s * s;
    s -= (uint32_t)(sq > arg);
    sq = (uint64_t)(s + 1) * (s + 1);
    s += (uint32_t)(sq <= arg);

    return s;
}
 inline void isqrt_pair(uint64_t xa, uint64_t xb,
    uint64_t *restrict ra, uint64_t *restrict rb)
{
    uint64_t xa_arg = xa, xb_arg = xb;
    xa = xa < 2 ? 4 : xa;
    xb = xb < 2 ? 4 : xb;
    uint32_t sa_scal = clz(xa) & ~1u, sb_scal = clz(xb) & ~1u;
    uint64_t sa_n = xa << sa_scal, sb_n = xb << sb_scal;
    uint32_t sa_hi = (uint32_t)(sa_n >> 32), sb_hi = (uint32_t)(sb_n >> 32);
    float sa_fr = native_rsqrt((float)sa_hi), sb_fr = native_rsqrt((float)sb_hi);
    uint32_t sa_r = (uint32_t)fma(1.407374884e14f, sa_fr, -438.0f);
    uint32_t sb_r = (uint32_t)fma(1.407374884e14f, sb_fr, -438.0f);
    uint32_t sa_s = xelis_mul_hi32(sa_r, sa_hi) * 2, sb_s = xelis_mul_hi32(sb_r, sb_hi) * 2;
    uint64_t sa_rem = sa_n - (uint64_t)sa_s * sa_s, sb_rem = sb_n - (uint64_t)sb_s * sb_s;
    sa_r = xelis_mul_hi32((uint32_t)(sa_rem >> 32) + 1, sa_r);
    sb_r = xelis_mul_hi32((uint32_t)(sb_rem >> 32) + 1, sb_r);
    sa_s += sa_r; sb_s += sb_r;
    sa_s >>= (sa_scal >> 1); sb_s >>= (sb_scal >> 1);
    uint64_t sa_sq = (uint64_t)sa_s * sa_s, sb_sq = (uint64_t)sb_s * sb_s;
    uint32_t sa_high = (uint32_t)(sa_sq > xa_arg), sb_high = (uint32_t)(sb_sq > xb_arg);
    sa_s -= sa_high; sb_s -= sb_high;
    sa_sq -= (2 * (uint64_t)sa_s + 1) & -(uint64_t)sa_high;
    sb_sq -= (2 * (uint64_t)sb_s + 1) & -(uint64_t)sb_high;
    uint64_t sa_nxt = sa_sq + 2 * (uint64_t)sa_s + 1;
    uint64_t sb_nxt = sb_sq + 2 * (uint64_t)sb_s + 1;
    sa_s += (uint32_t)(sa_nxt <= xa_arg);
    sb_s += (uint32_t)(sb_nxt <= xb_arg);
    *ra = (xa_arg < 2) ? xa_arg : (uint64_t)sa_s;
    *rb = (xb_arg < 2) ? xb_arg : (uint64_t)sb_s;
}
 inline uint64_t barrett_reduce(
    uint64_t x_lo, uint64_t x_hi,
    uint64_t mod,
    uint64_t mu_lo, uint64_t mu_hi)
{
    // q = high 128 bits of (x * mu)
    uint64_t p0_hi = xelis_mul_hi(x_lo, mu_lo);
    
    uint64_t p1_lo = x_lo * mu_hi;
    uint64_t p1_hi = xelis_mul_hi(x_lo, mu_hi);
    
    uint64_t p2_lo = x_hi * mu_lo;
    uint64_t p2_hi = xelis_mul_hi(x_hi, mu_lo);
    
    uint64_t p3_lo = x_hi * mu_hi;
    
    // Middle column + carry
    uint64_t mid = p0_hi + p1_lo;
    uint64_t c1 = (mid < p0_hi);
    mid += p2_lo;
    uint64_t c2 = (mid < p2_lo);
    
    uint64_t q = p3_lo + p1_hi + p2_hi + c1 + c2;
    
    // r = x - q*mod (128-bit subtract)
    uint64_t qm_lo = q * mod;
    uint64_t qm_hi = xelis_mul_hi(q, mod);
    
    uint64_t r_lo = x_lo - qm_lo;
    uint64_t borrow = (x_lo < qm_lo);
    uint64_t r_hi = x_hi - qm_hi - borrow;
    
    // Single correction
    if (r_hi | (r_lo >= mod)) {
        r_lo -= mod;
    }
    
    return r_lo;
}
 inline uint32_t le_bytes_to_uint32(__generic const uint8_t *bytes)
{
  return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
         ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}
 inline void xelis_blake3_lean(__generic const uint8_t * input, uint8_t* output)
{
    const int NUM_CHUNKS = 531;
    const uint8_t FULL_BLEN = BLAKE3_BLOCK_LEN;

    uint32_t cv_stack[10][8];
    int stack_len = 0;

    uint8_t left_bytes[BLAKE3_OUT_LEN];
    uint8_t right_bytes[BLAKE3_OUT_LEN];
    uint8_t parent_bytes[BLAKE3_OUT_LEN];

    // Leaves + incremental merges
    for (uint64_t chunk_idx = 0; chunk_idx < (uint64_t)NUM_CHUNKS; chunk_idx++) {
        __generic const uint8_t * chunk_ptr = input + (size_t)chunk_idx * (size_t)BLAKE3_CHUNK_LEN;

        uint32_t cv[8];
        blake3_cv_init(cv);

        // 16 full blocks
        blake3_compress_in_place(cv, chunk_ptr, FULL_BLEN, chunk_idx, (uint8_t)CHUNK_START);
        for (int b = 1; b < 15; b++) {
            blake3_compress_in_place(cv, chunk_ptr + b * BLAKE3_BLOCK_LEN, FULL_BLEN, chunk_idx, (uint8_t)0);
        }

        blake3_compress_in_place(cv, chunk_ptr + 15 * BLAKE3_BLOCK_LEN, FULL_BLEN, chunk_idx, (uint8_t)CHUNK_END);

        // Push to stack
        for (int i = 0; i < 8; i++) cv_stack[stack_len][i] = cv[i];
        stack_len++;

        // Merges: ctz(chunk_idx+1), capped to 9
        uint32_t tz = xelis_ffsll(chunk_idx + 1ULL) - 1;
        tz = min(tz, 9u);

        // Fixed merge attempts
        for (int m = 0; m < 10; m++) {
            int do_merge = (m < (int)tz) & (stack_len >= 2);

            int idx_r = max(stack_len - 1, 0);
            int idx_l = max(stack_len - 2, 0);

            uint32_t right_w[8], left_w[8];
            for (int i = 0; i < 8; i++) {
                right_w[i] = cv_stack[idx_r][i];
                left_w[i]  = cv_stack[idx_l][i];
            }
            for (int i = 0; i < 8; i++) {
                blake3_store32(left_bytes  + i * 4, left_w[i]);
                blake3_store32(right_bytes + i * 4, right_w[i]);
            }

            blake3_hash_parent(left_bytes, right_bytes, parent_bytes, false);
            for (int i = 0; i < 8; i++) {
                uint32_t parent_w = blake3_load32(parent_bytes + i * 4);
                cv_stack[idx_l][i] = do_merge ? parent_w : cv_stack[idx_l][i];
            }

            stack_len -= do_merge;
        }
    }

    // Final reduction to 2
    for (int t = 0; t < 10; t++) {
        int do_merge = (stack_len > 2);

        int idx_r = max(stack_len - 1, 0);
        int idx_l = max(stack_len - 2, 0);

        uint32_t right_w[8], left_w[8];
        for (int i = 0; i < 8; i++) {
            right_w[i] = cv_stack[idx_r][i];
            left_w[i]  = cv_stack[idx_l][i];
        }
        for (int i = 0; i < 8; i++) {
            blake3_store32(left_bytes  + i * 4, left_w[i]);
            blake3_store32(right_bytes + i * 4, right_w[i]);
        }

        blake3_hash_parent(left_bytes, right_bytes, parent_bytes, false);
        for (int i = 0; i < 8; i++) {
            uint32_t parent_w = blake3_load32(parent_bytes + i * 4);
            cv_stack[idx_l][i] = do_merge ? parent_w : cv_stack[idx_l][i];
        }

        stack_len -= do_merge;
    }

    // Final ROOT merge
    for (int i = 0; i < 8; i++) {
        blake3_store32(left_bytes  + i * 4, cv_stack[0][i]);
        blake3_store32(right_bytes + i * 4, cv_stack[1][i]);
    }

    blake3_hash_parent(left_bytes, right_bytes, parent_bytes, true);
    for (int i = 0; i < 8; i++) {
        blake3_store32(output + i * 4, blake3_load32(parent_bytes + i * 4));
    }
}
 void chacha20_block(uint32_t *output, const uint32_t *input)
{
  uint32_t x[16];
  for (int i = 0; i < 16; i++)
    x[i] = input[i];
  for (int i = 0; i < 8; i += 2)
  {
    CHACHA_QR(x[0], x[4], x[8], x[12])
    CHACHA_QR(x[1], x[5], x[9], x[13])
    CHACHA_QR(x[2], x[6], x[10], x[14])
    CHACHA_QR(x[3], x[7], x[11], x[15])
    CHACHA_QR(x[0], x[5], x[10], x[15])
    CHACHA_QR(x[1], x[6], x[11], x[12])
    CHACHA_QR(x[2], x[7], x[8], x[13])
    CHACHA_QR(x[3], x[4], x[9], x[14])
  }
  for (int i = 0; i < 16; i++)
    output[i] = x[i] + input[i];
}
 inline void chacha20_init_state(uint32_t *state,
                                                    __generic const uint8_t *key,
                                                    __generic const uint8_t *nonce)
{
  state[0] = 0x61707865;
  state[1] = 0x3320646e;
  state[2] = 0x79622d32;
  state[3] = 0x6b206574;
  for (int i = 0; i < 8; i++)
    state[4 + i] = le_bytes_to_uint32(key + i * 4);

  state[12] = 0;
  state[13] = le_bytes_to_uint32(nonce);
  state[14] = le_bytes_to_uint32(nonce + 4);
  state[15] = le_bytes_to_uint32(nonce + 8);
}
 inline void stage_1(
    __generic const uint8_t *input,
    __global uint64_t *my_scratch,
    uint32_t block_size)
{
    uint8_t key[128];
    uint8_t K2[4][32];
    uint8_t nonces[4][12];
    uint8_t buffer[64];

    // Initialize key from input
    for (int i = 0; i < XELIS_TEMPLATE_SIZE; i++) {
        key[i] = input[i];
    }
    for (int i = XELIS_TEMPLATE_SIZE; i < 128; i++) {
        key[i] = 0;
    }

    // First hash to get nonce[0]
    blake3_hash_single_chunk(input, XELIS_TEMPLATE_SIZE, buffer);
    for (int i = 0; i < 12; i++) {
        nonces[0][i] = buffer[i];
    }

    // Derive K2[0]
    for (int i = 0; i < XELIS_CHUNK_SIZE; i++) {
        buffer[XELIS_CHUNK_SIZE + i] = key[i];
    }
    blake3_hash_single_chunk(buffer, XELIS_CHUNK_SIZE * 2, K2[0]);

    // Derive K2[1..3]
    for (int k = 1; k < 4; k++) {
        for (int i = 0; i < XELIS_CHUNK_SIZE; i++) {
            buffer[i] = K2[k-1][i];
            buffer[XELIS_CHUNK_SIZE + i] = key[k * XELIS_CHUNK_SIZE + i];
        }
        blake3_hash_single_chunk(buffer, XELIS_CHUNK_SIZE * 2, K2[k]);
    }

    // Derive nonces[1..3] using ChaCha20
    for (int k = 0; k < 3; k++) {
        uint32_t state[16];
        chacha20_init_state(state, K2[k], nonces[k]);
        state[12] = (XELIS_BYTES_PER_CHUNK_V3 / 64) - 1;

        uint32_t block[16];
        chacha20_block(block, state);

        // Extract nonce from last 12 bytes of block
        uint8_t* block_bytes = (uint8_t*)block;
        for (int i = 0; i < 12; i++) {
            nonces[k + 1][i] = block_bytes[52 + i];
        }
    }

    // Generate scratchpad - all in registers, stream to memory
    const uint32_t blocks_per_chunk = XELIS_BYTES_PER_CHUNK_V3 / 64;

    for (uint32_t chunk = 0; chunk < 4; chunk++) {
        for (uint32_t blk = 0; blk < blocks_per_chunk; blk++) {
            uint32_t state[16];
            chacha20_init_state(state, K2[chunk], nonces[chunk]);
            state[12] = blk;

            uint32_t chacha_out[16];
            chacha20_block(chacha_out, state);

            uint32_t global_block = chunk * blocks_per_chunk + blk;
            uint64_t* dest = my_scratch + global_block * 8;
            uint64_t* out64 = (uint64_t*)chacha_out;

            // Streaming stores - bypass L1 cache
            store_streaming_u256(dest, out64[0], out64[1], out64[2], out64[3]);
            store_streaming_u256(dest + 4, out64[4], out64[5], out64[6], out64[7]);
        }
    }
}
 inline divmod_result divmod64_split32(uint64_t a, uint64_t d) {
    if (d <= 0xFFFFFFFFULL) {
        uint32_t d32 = (uint32_t)d;
        uint32_t a_hi = (uint32_t)(a >> 32);
        uint32_t q_hi = a_hi / d32;
        uint32_t r_hi = a_hi - q_hi * d32;
        uint64_t mid = ((uint64_t)r_hi << 32) | (uint32_t)a;
        uint32_t q_lo = (uint32_t)(mid / d32);
        uint64_t rem = mid - (uint64_t)q_lo * d32;
        return (divmod_result){ ((uint64_t)q_hi << 32) | q_lo, rem };
    }
    int s = clz(d);
    uint64_t dn = d << s;
    uint32_t v = (uint32_t)(dn >> 32);
    uint64_t a_shifted = (s == 0) ? (a >> 32) : (a >> (32 - s));
    uint32_t q = (v == 0xFFFFFFFFu) ? (uint32_t)a_shifted
                                    : (uint32_t)(a_shifted / ((uint64_t)v + 1));
    uint64_t rem = a - (uint64_t)q * d;
    if (rem > a) { q--; rem += d; }
    else if (rem >= d) { q++; rem -= d; }
    return (divmod_result){ (uint64_t)q, rem };
}
 inline uint64_t mod64_split32(uint64_t a, uint64_t d) {
    return divmod64_split32(a, d).r;
}
 inline uint64_t div64_split32(uint64_t a, uint64_t d) {
    return divmod64_split32(a, d).q;
}
 inline double recip_norm(uint64_t dn) {
    double dd = (double)dn;
    double x  = (double)native_reciprocal((float)dn);
    x = x * fma(-dd, x, 2.0);
    return x;
}
 inline uint32_t div96by64_q32_frcp(
    uint64_t r, uint32_t x, uint64_t d, double recip, uint64_t *rem_out)
{
    uint64_t q = (uint64_t)(((double)r * 4294967296.0 + (double)x) * recip);
    if (q > 0xffffffffull) q = 0xffffffffull;
    uint64_t rem = (r << 32) | (uint64_t)x;
    uint64_t p_lo = q * d;
    int64_t hi = (int64_t)(r >> 32)
               - (int64_t)xelis_mul_hi(q, d)
               - (rem < p_lo);
    rem -= p_lo;
    uint64_t over = (uint64_t)(hi < 0);
    q   -= over;
    rem += d & -over;
    hi  += over & (rem < d);
    uint64_t under = (uint64_t)(hi > 0) | (uint64_t)(rem >= d);
    q   += under;
    rem -= d & -under;
    *rem_out = rem;
    return (uint32_t)q;
}
 inline divmod_result divmod128by64_split32(uint64_t hi, uint64_t lo, uint64_t d) {
    uint64_t r = mod64_split32(hi, d);
    int s = clz(d);
    uint64_t dn = d << s;
    double recip = recip_norm(dn);
    uint64_t n_hi = s ? ((r << s) | (lo >> (64 - s))) : r;
    uint64_t n_lo = lo << s;
    uint64_t rem;
    uint32_t q1 = div96by64_q32_frcp(n_hi, (uint32_t)(n_lo >> 32), dn, recip, &rem);
    uint32_t q0 = div96by64_q32_frcp(rem,  (uint32_t)n_lo,          dn, recip, &rem);
    return (divmod_result){ ((uint64_t)q1 << 32) | q0, rem >> s };
}
 inline uint64_t div128by64_split32(uint64_t hi, uint64_t lo, uint64_t d) {
    return divmod128by64_split32(hi, lo, d).q;
}
 inline uint64_t mod128by64_split32(uint64_t hi, uint64_t lo, uint64_t d) {
    return divmod128by64_split32(hi, lo, d).r;
}
 inline uint64_t modular_power_barrett(uint64_t base, uint64_t exp, uint64_t mod) {
    mod += (mod == 0);
    if (mod == 1) return 0;

    base %= mod;
    if (base < 2) return base;
    if (exp == 0) return 1;

    // Precompute mu = floor((2^128 - 1) / mod) using split32 (exact, CLZ-normalized)
    uint64_t mu_hi = div64_split32(0xFFFFFFFFFFFFFFFFULL, mod);
    uint64_t rem   = mod64_split32(0xFFFFFFFFFFFFFFFFULL, mod);
    uint64_t mu_lo = div128by64_split32(rem, 0xFFFFFFFFFFFFFFFFULL, mod);

    uint64_t result = 1;

    while (exp > 0) {
        if (exp & 1) {
            uint64_t lo = result * base;
            uint64_t hi = xelis_mul_hi(result, base);
            result = barrett_reduce(lo, hi, mod, mu_lo, mu_hi);
        }
        exp >>= 1;
        if (exp > 0) {
            uint64_t lo = base * base;
            uint64_t hi = xelis_mul_hi(base, base);
            base = barrett_reduce(lo, hi, mod, mu_lo, mu_hi);
            if (base == 0) return 0;
        }
    }
    return result;
}
 inline double lut_load(const double *lut, int vi) {
    return __ldg(&lut[vi]);
}
 inline uint64_t s3_mod64_newton(uint64_t a, uint64_t d,
                                                     const double *lut) {
    int s = clz(d); uint64_t dn = d << s; uint32_t v = (uint32_t)(dn >> 32);
    uint64_t ash = (s == 0) ? (a >> 32) : (a >> (32 - s));
    uint32_t vi = (v >> NEWTON_LUT_SHIFT) - NEWTON_LUT_BASE;
    double r = lut_load(lut, vi);
    r = r * fma(-(double)((uint64_t)v + 1), r, 2.0);  // unconditional FMA
    uint32_t q = (uint32_t)((double)ash * r);
    uint64_t rem = a - (uint64_t)q * d;
    if (rem > a) { rem += d; } else if (rem >= d) { rem -= d; }
    return rem;
}
 inline uint64_t s3_mod64(uint64_t a, uint64_t d) {
    if (d <= 0xFFFFFFFFULL) {
        uint32_t d32 = (uint32_t)d, ah = (uint32_t)(a >> 32);
        uint32_t rh = ah - (ah / d32) * d32;
        uint64_t mid = ((uint64_t)rh << 32) | (uint32_t)a;
        return mid - (uint64_t)((uint32_t)(mid / d32)) * d32;
    }
    int s = clz(d); uint64_t dn = d << s; uint32_t v = (uint32_t)(dn >> 32);
    uint64_t ash = (s == 0) ? (a >> 32) : (a >> (32 - s));
    uint32_t q = (v == 0xFFFFFFFFu) ? (uint32_t)ash : (uint32_t)(ash / ((uint64_t)v + 1));
    uint64_t rem = a - (uint64_t)q * d;
    if (rem > a) { rem += d; } else if (rem >= d) { rem -= d; }
    return rem;
}
 inline double s3_recip_norm(uint64_t dn) {
    double dd = (double)dn, x = (double)native_reciprocal((float)dn);
    return x * fma(-dd, x, 2.0);
}
 inline uint32_t s3_div96by64(uint64_t r, uint32_t x, uint64_t d,
    double recip, uint64_t *ro) {
    uint64_t q = (uint64_t)(((double)r * 4294967296.0 + (double)x) * recip);
    if (q > 0xffffffffull) q = 0xffffffffull;
    uint64_t rem = (r << 32) | (uint64_t)x, p_lo = q * d;
    int64_t hi = (int64_t)(r >> 32) - (int64_t)xelis_mul_hi(q, d) - (rem < p_lo); rem -= p_lo;
    uint64_t ov = (uint64_t)(hi < 0); q -= ov; rem += d & -ov; hi += ov & (rem < d);
    uint64_t un = (uint64_t)(hi > 0) | (uint64_t)(rem >= d); q += un; rem -= d & -un;
    *ro = rem; return (uint32_t)q;
}
 inline divmod_result s3_divmod128_newton(uint64_t hi, uint64_t lo,
                                                              uint64_t d, const double *lut) {
    uint64_t r = s3_mod64_newton(hi, d, lut);
    int s = clz(d); uint64_t dn = d << s; double recip = s3_recip_norm(dn);
    uint64_t n_hi = s ? ((r << s) | (lo >> (64 - s))) : r, n_lo = lo << s, rem;
    uint32_t q1 = s3_div96by64(n_hi, (uint32_t)(n_lo >> 32), dn, recip, &rem);
    uint32_t q0 = s3_div96by64(rem, (uint32_t)n_lo, dn, recip, &rem);
    return (divmod_result){ ((uint64_t)q1 << 32) | q0, rem >> s };
}
 inline uint64_t exec_cheap(
    uint32_t op, uint64_t a, uint64_t b, uint64_t c,
    uint32_t r, uint64_t result) {
    uint64_t m3  = -(uint64_t)(op==3),  m4  = -(uint64_t)(op==4);
    uint64_t m5  = -(uint64_t)(op==5),  m6  = -(uint64_t)(op==6);
    uint64_t m7  = -(uint64_t)(op==7),  m8  = -(uint64_t)(op==8);
    uint64_t m9  = -(uint64_t)(op==9);
    uint64_t m14 = -(uint64_t)(op==14), m15 = -(uint64_t)(op==15);
    uint64_t ab = a * b, ac = a * c, bc = b * c;
    uint64_t cx = (ac & (m3|m8)) | (ab & m4) | (c & m5) | (a & m6) | (bc & m7);
    uint64_t cy = (bc & m3)      | (b & m5)  | (c & m6) | (a & m7) | (b & m8);
    uint64_t cz = (ac & m4)      | (a & m5)  | (b & m6);
    uint64_t cheap = cx + cy - cz;
    cheap |= (ab * c) & m9;
    uint64_t mhi = m14 | m15;
    uint64_t hp = (a & m14) | (c & m15);
    uint64_t hq = (c & m14) | (b & m15);
    uint64_t hn = (bc & m14) | ((ab + c * ROTR(result, r)) & m15);
    cheap |= (xelis_mul_hi(hp, hq) + hn) & mhi;
    return cheap;
}
  uint64_t exec_op_split_newton(
    uint32_t op, uint64_t a, uint64_t b, uint64_t c,
    uint32_t r, uint64_t result, uint32_t i, uint32_t j,
    const double *lut) {

    bool is_expensive = (op <= 2) || (op >= 10 && op <= 13);
    if (!is_expensive) {
        return exec_cheap(op, a, b, c, r, result);
    }

    uint64_t t0 = ROTL(result, r), mur = murmurhash3(c ^ result ^ i ^ j) | 1;
    uint64_t m0  = -(uint64_t)(op==0),  m1  = -(uint64_t)(op==1),  m2  = -(uint64_t)(op==2);
    uint64_t m10 = -(uint64_t)(op==10), m11 = -(uint64_t)(op==11);
    uint64_t m12 = -(uint64_t)(op==12), m13 = -(uint64_t)(op==13);
    uint64_t pA = m0 | m10 | m12, pB = m1 | m11 | m13, pC = m2;

    uint64_t sq0_in = ((b + j) & pA) | ((b | 2) & pB) | ((a + i) & pC);
    uint64_t sq1_in = ((a + j) & pB) | ((c + j) & pC) | (4 & ~(pB | pC));
    uint64_t sq0, sq1;
    isqrt_pair(sq0_in, sq1_in, &sq0, &sq1);

    uint64_t hiA  = ((a + i) & m0) | (a & m10) | (c & m12) | ~pA;
    uint64_t loA  = (sq0 & m0) | (b & m10) | (a & m12);
    uint64_t dvA  = (mur & m0) | ((c | 1) & m10) | ((b | 4) & m12) | ~pA;
    uint64_t numB = ((c + i) & m1) | (b & m11) | (t0 & m13) | ~pB;
    uint64_t denB = (sq0 & m1) | (t0 & m11) | (a & m13) | ~pB;
    denB |= (uint64_t)(denB == 0);

    uint64_t dm_hi  = hiA & pA;
    uint64_t dm_lo  = (loA & pA) | (numB & pB) | (1 & ~(pA | pB));
    uint64_t dm_div = (dvA & pA) | (denB & pB) | (1 & ~(pA | pB));
    dm_div |= (uint64_t)(dm_div == 0);

    divmod_result dm = s3_divmod128_newton(dm_hi, dm_lo, dm_div, lut);

    uint64_t resA = (dm.r & (m0 | m10)) | (dm.q & m12);
    uint64_t res1 = ROTL(dm.r, i + j) * sq1;
    uint64_t t1 = a | 2;
    uint64_t c11 = -(uint64_t)U128_LT(b, c, t0, t1);
    uint64_t r11 = (c & c11) | ((c - t1 * dm.q) & ~c11);
    uint64_t c13 = -(uint64_t)U128_GT(t0, b, a, (c | 8));
    uint64_t r13 = (dm.q & c13) | ((a ^ b) & ~c13);
    uint64_t resB = (res1 & m1) | (r11 & m11) | (r13 & m13);
    uint64_t resC = (sq0 * sq1) ^ (b + i + j);
    return (resA & pA) | (resB & pB) | (resC & pC);
}
XELIS_HOT  void stage_3_hybrid_v2(__global uint64_t *my_scratch,
                                              __global const double *newton_lut)
{
  __global uint64_t *mem_buffer_a = my_scratch;
  __global uint64_t *mem_buffer_b = my_scratch + XELIS_BUFFER_SIZE_V3;

  uint64_t addr_a = mem_buffer_b[XELIS_BUFFER_SIZE_V3 - 1];
  uint64_t addr_b = mem_buffer_a[XELIS_BUFFER_SIZE_V3 - 1] >> 32;
  uint32_t r = 0;

  for (uint32_t i = 0; i < XELIS_SCRATCHPAD_ITERS_V3; i++)
  {
    // Random reads
    uint64_t mem_a = load_scratch(&mem_buffer_a[map_index(addr_a)]);
    uint64_t mem_b = load_scratch(&mem_buffer_b[map_index(mem_a ^ addr_b)]);

    uint64_t hash1 = mem_b;
    uint64_t hash2 = mem_a;
#if defined(__CUDA_ARCH__) || defined(__HIP_PLATFORM_NVIDIA__)
    aes_round_u64(&hash1, &hash2, AES_KEY);
#else
    aes_round_u64(&hash1, &hash2, AES_KEY);
#endif

    uint64_t result = ~(hash1 ^ hash2);

#if defined(__CUDA_ARCH__) || defined(__HIP_PLATFORM_NVIDIA__)
    // DEFER_FIX2: deferred stores with 3 software bypasses (apre, bv, c)
    // Maximizes ILP by issuing all loads before flushing deferred stores.
    uint64_t iar = map_index(result);
    uint64_t apre = load_scratch(&mem_buffer_a[iar]);

    uint64_t d_iaw = XELIS_BUFFER_SIZE_V3 + 1;
    uint64_t d_ibw = XELIS_BUFFER_SIZE_V3 + 1;
    uint64_t d_t = 0;
    uint32_t d_ij = 0;
    int ds = 0;

    for (uint32_t j = 0; j < XELIS_BUFFER_SIZE_V3; j++)
    {
      uint64_t a = (ds && d_iaw == iar) ? d_t : apre;

      uint64_t rot_res = ~ROTR(result, r);
      uint64_t bv_addr = map_index(a ^ rot_res);

      uint64_t bv = load_scratch(&mem_buffer_b[bv_addr]);
      uint64_t oa = 0, ob = 0, pvb = 0;
      if (ds)
      {
        oa = load_scratch(&mem_buffer_a[d_iaw]);
        ob = load_scratch(&mem_buffer_b[d_ibw]);
      }

      uint64_t c = my_scratch[r];

      if (ds) pvb = ob ^ oa ^ ROTR(d_t, d_ij);

      if (ds)
      {
        if (d_iaw == (uint64_t)r)
          c = d_t;
        else if ((d_ibw + XELIS_BUFFER_SIZE_V3) == (uint64_t)r)
          c = pvb;
      }

      r++;
      uint32_t op_idx = ROTL(result, (uint32_t)c) & 0xF;

      if (ds)
      {
        if (d_ibw == bv_addr) bv = pvb;
        store_scratch(&mem_buffer_b[d_ibw], pvb);
        store_scratch(&mem_buffer_a[d_iaw], d_t);
      }

      uint64_t v = exec_op_split_newton(op_idx, a, bv, c, r, result, i, j, newton_lut);

      uint64_t idx_seed = v ^ result;
      result = ROTL(idx_seed, r);

      uint64_t idx_t = map_index(idx_seed);
      iar = map_index(result);
      apre = load_scratch(&mem_buffer_a[iar]);

      uint64_t t = (pick_half(v) ? load_scratch(&mem_buffer_b[idx_t])
                                 : load_scratch(&mem_buffer_a[idx_t])) ^ result;

      uint64_t iaw = map_index(t ^ result ^ XELIS_GOLDEN_RATIO);
      uint64_t ibw = map_index(iaw ^ ~result ^ XELIS_SCATTER_CONST);

      d_iaw = iaw;
      d_ibw = ibw;
      d_t = t;
      d_ij = i + j;
      ds = 1;
    }

    if (ds)
    {
      uint64_t oa = load_scratch(&mem_buffer_a[d_iaw]);
      uint64_t ob = load_scratch(&mem_buffer_b[d_ibw]);
      store_scratch(&mem_buffer_b[d_ibw], ob ^ oa ^ ROTR(d_t, d_ij));
      store_scratch(&mem_buffer_a[d_iaw], d_t);
    }

#else
    // AMD: 1-bypass deferred store (v0.7.7 pattern).
    // LLVM's aggressive reordering makes the 3-bypass variant unreliable.
    uint64_t idx_a_read = map_index(result);

    int pending_store = 0;
    uint64_t pending_idx_a_write = 0, pending_idx_b_write = 0;
    uint64_t pending_store_a = 0, pending_store_b = 0;

    uint64_t a_preloaded = load_scratch(&mem_buffer_a[idx_a_read]);

    for (uint32_t j = 0; j < XELIS_BUFFER_SIZE_V3; j++)
    {
      if (pending_store)
      {
        store_scratch(&mem_buffer_b[pending_idx_b_write], pending_store_b);
        store_scratch(&mem_buffer_a[pending_idx_a_write], pending_store_a);
      }

      uint64_t a = (pending_store && pending_idx_a_write == idx_a_read)
                   ? pending_store_a
                   : a_preloaded;

      uint64_t c = my_scratch[r];
      uint64_t rot_res = ~ROTR(result, r);
      r++;
      uint32_t op_idx = ROTL(result, (uint32_t)c) & 0xF;

      uint64_t b_val = load_scratch(&mem_buffer_b[map_index(a ^ rot_res)]);

      uint64_t v = exec_op_split_newton(op_idx, a, b_val, c, r, result, i, j, newton_lut);

      uint64_t idx_seed = v ^ result;
      result = ROTL(idx_seed, r);

      idx_a_read = map_index(result);
      a_preloaded = load_scratch(&mem_buffer_a[idx_a_read]);

      uint64_t idx_t = map_index(idx_seed);
      uint64_t t = (pick_half(v) ? load_scratch(&mem_buffer_b[idx_t])
                                 : load_scratch(&mem_buffer_a[idx_t])) ^ result;

      uint64_t idx_a_write = map_index(t ^ result ^ XELIS_GOLDEN_RATIO);
      uint64_t idx_b_write = map_index(idx_a_write ^ ~result ^ XELIS_SCATTER_CONST);

      uint64_t old_mem_a = load_scratch(&mem_buffer_a[idx_a_write]);
      uint64_t old_b     = load_scratch(&mem_buffer_b[idx_b_write]);

      pending_idx_a_write = idx_a_write;
      pending_idx_b_write = idx_b_write;
      pending_store_a = t;
      pending_store_b = old_b ^ old_mem_a ^ ROTR(t, i + j);
      pending_store = 1;
    }

    if (pending_store)
    {
      store_scratch(&mem_buffer_b[pending_idx_b_write], pending_store_b);
      store_scratch(&mem_buffer_a[pending_idx_a_write], pending_store_a);
    }

#endif // DEFER_FIX2 platform gate

    addr_a = modular_power_barrett(addr_a, addr_b, result);
    uint64_t sq_r, sq_a;
    isqrt_pair(result, addr_a, &sq_r, &sq_a);
    addr_b = sq_r * (r + 1) * sq_a;
  }
}
 inline bool check_difficulty_gpu(__generic const uint8_t *hash, const uint64_t *target)
{
  // XMRig's verifier compares the little-endian u64 at the end of the
  // Xelis hash against Job::target(). Keep the GPU pre-filter identical;
  // lexicographically comparing all 32 bytes produces false candidates.
  return (*((__generic const uint64_t *)(hash + 24))) < target[0];
}
__kernel void xelis_hash_v3_kernel(__global const uint8_t*i,__global uint8_t*out,__global uint64_t*m,uint64_t ns,uint32_t n,__global const uint64_t*t,__global uint64_t*s,__global const double*l){uint g=get_global_id(0);if(g>=n)return;__global uint64_t*q=m+(size_t)g*XELIS_MEMORY_SIZE_V3;uint8_t in[112];for(uint j=0;j<112;j++)in[j]=i[j];uint64_t no=(ns&0xFFFF000000000000UL)|((ns+g)&0x0000FFFFFFFFFFFFUL);((__private uint64_t*)in)[5]=no;stage_1(in,q,get_local_size(0));stage_3_hybrid_v2(q,l);uint8_t h[32];xelis_blake3_lean((const uint8_t*)q,h);for(uint j=0;j<32;j++)out[g*32+j]=h[j];if(t&&s&&check_difficulty_gpu(h,t)){uint old=atomic_inc((volatile __global uint*)&s[0]);if(old<1024){s[1+old*5]=no;for(uint j=0;j<4;j++)s[2+old*5+j]=((__private uint64_t*)h)[j];}}}
