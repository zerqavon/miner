typedef unsigned char u8;
typedef unsigned int u32;

static inline u32 rr(const u32 x, const u32 n) { return (x >> n) | (x << (32 - n)); }
static inline u32 ch(const u32 x, const u32 y, const u32 z) { return (x & y) ^ (~x & z); }
static inline u32 maj(const u32 x, const u32 y, const u32 z) { return (x & y) ^ (x & z) ^ (y & z); }
static inline u32 bs0(const u32 x) { return rr(x,2) ^ rr(x,13) ^ rr(x,22); }
static inline u32 bs1(const u32 x) { return rr(x,6) ^ rr(x,11) ^ rr(x,25); }
static inline u32 ss0(const u32 x) { return rr(x,7) ^ rr(x,18) ^ (x >> 3); }
static inline u32 ss1(const u32 x) { return rr(x,17) ^ rr(x,19) ^ (x >> 10); }

__constant u32 K[64] = {
  0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
  0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
  0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
  0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
  0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
  0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
  0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
  0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

static inline u32 be32(const u8 *p) { return ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | p[3]; }
static inline void putbe(u8 *p, u32 x) { p[0]=(u8)(x>>24); p[1]=(u8)(x>>16); p[2]=(u8)(x>>8); p[3]=(u8)x; }

static inline void sha256_block(const u8 *msg, u32 h[8])
{
    u32 w[64];
    for (u32 i=0;i<16;i++) w[i]=be32(msg+i*4);
    for (u32 i=16;i<64;i++) w[i]=ss1(w[i-2])+w[i-7]+ss0(w[i-15])+w[i-16];
    u32 a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],x=h[7];
    for (u32 i=0;i<64;i++) { const u32 t1=x+bs1(e)+ch(e,f,g)+K[i]+w[i]; const u32 t2=bs0(a)+maj(a,b,c); x=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2; }
    h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;h[4]+=e;h[5]+=f;h[6]+=g;h[7]+=x;
}

static inline void sha256_41(const u8 *in, u8 *out)
{
    u8 m[64]={0}; for (u32 i=0;i<41;i++) m[i]=in[i]; m[41]=0x80; m[62]=1; m[63]=0x48;
    u32 h[8]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    sha256_block(m,h); for (u32 i=0;i<8;i++) putbe(out+i*4,h[i]);
}

static inline void sha256_32(const u8 *in, u8 *out)
{
    u8 m[64]={0}; for (u32 i=0;i<32;i++) m[i]=in[i]; m[32]=0x80; m[62]=1;
    u32 h[8]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    sha256_block(m,h); for (u32 i=0;i<8;i++) putbe(out+i*4,h[i]);
}

static inline void sha256_msg(const u8 *in, u32 len, u8 *out)
{
    u8 m[256]={0};
    for (u32 i=0;i<len;i++) m[i]=in[i];
    m[len]=0x80;
    const u32 bits=len*8;
    m[((len+9+63)/64)*64-4]=(u8)(bits>>24);
    m[((len+9+63)/64)*64-3]=(u8)(bits>>16);
    m[((len+9+63)/64)*64-2]=(u8)(bits>>8);
    m[((len+9+63)/64)*64-1]=(u8)bits;
    u32 h[8]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    const u32 blocks=(len+9+63)/64;
    for (u32 i=0;i<blocks;i++) sha256_block(m+i*64,h);
    for (u32 i=0;i<8;i++) putbe(out+i*4,h[i]);
}

static inline void hmac256(const u8 *key, const u8 *data, u32 len, u8 *out)
{
    u8 inner[177]={0};
    u8 outer[96]={0};
    for (u32 i=0;i<64;i++) inner[i]=(u8)(key[i%32]^0x36);
    for (u32 i=0;i<len;i++) inner[64+i]=data[i];
    u8 ih[32]; sha256_msg(inner,64+len,ih);
    for (u32 i=0;i<64;i++) outer[i]=(u8)(key[i%32]^0x5c);
    for (u32 i=0;i<32;i++) outer[64+i]=ih[i];
    sha256_msg(outer,96,out);
}

static inline void words_from_be(const u8 *in, u32 *out)
{
    for (u32 i=0;i<8;i++) {
        const u32 p=31-i*4;
        out[i]=(u32)in[p]|((u32)in[p-1]<<8)|((u32)in[p-2]<<16)|((u32)in[p-3]<<24);
    }
}

static inline void words_to_be(const u32 *in, u8 *out)
{
    for (u32 i=0;i<8;i++) {
        const u32 v=in[7-i];
        out[i*4]=(u8)(v>>24); out[i*4+1]=(u8)(v>>16);
        out[i*4+2]=(u8)(v>>8); out[i*4+3]=(u8)v;
    }
}

// Nexa-specific fixed-base multiplication.  This uses an 8-bit signed NAF
// and a constant table containing odd multiples 1G..127G.  It is independent
// of WildRig and avoids rebuilding the generator table per work item.
static inline int nexapow_naf8(u32 *naf, const u32 *k)
{
    u32 n[9]={0,k[7],k[6],k[5],k[4],k[3],k[2],k[1],k[0]};
    int start=0;
    for (int i=0;i<=256;i++) {
        if (n[8]&1U) {
            int d=(int)(n[8]&0xffU); int v=d;
            if (d>=128) { d-=256; v=257-v; }
            naf[i>>2] |= (u32)(v&0xff) << ((i&3)<<3);
            u32 old=n[8]; n[8]-=(u32)d; u32 j=8;
            if (d>0) { while (n[j]>old) { if (!j) break; --j; old=n[j]; --n[j]; } }
            else { while (old>n[j]) { if (!j) break; --j; old=n[j]; ++n[j]; } }
            start=i;
        }
        for (int j=8;j>0;j--) n[j]=(n[j]>>1)|(n[j-1]<<31);
        n[0]>>=1;
        int nonzero=0; for (int j=1;j<9;j++) nonzero|=n[j]!=0;
        if (!nonzero) break;
    }
    return start;
}

static inline void nexapow_point_mul_xy(u32 *x1, u32 *y1, const u32 *k)
{
    u32 naf[33]={0}; const int start=nexapow_naf8(naf,k);
    u32 digit=(naf[start>>2]>>((start&3)<<3))&0xffU;
    if (!digit) digit=1;
    const u32 odd=digit&1U, pos=((digit-1U+odd)>>1)*24U;
    u32 x[8],y[8],z[8]={1,0,0,0,0,0,0,0};
    for (u32 i=0;i<8;i++) { x[i]=nexapow_g_table[pos+i]; y[i]=nexapow_g_table[pos+(odd?8:16)+i]; }
    for (int bit=start-1;bit>=0;bit--) {
        point_double(x,y,z);
        digit=(naf[bit>>2]>>((bit&3)<<3))&0xffU;
        if (digit) {
            const u32 o=digit&1U, p=((digit-1U+o)>>1)*24U;
            u32 x2[8],y2[8];
            for (u32 i=0;i<8;i++) { x2[i]=nexapow_g_table[p+i]; y2[i]=nexapow_g_table[p+(o?8:16)+i]; }
            point_add(x,y,z,x2,y2);
        }
    }
    point_to_affine(x,y,z);
    for (u32 i=0;i<8;i++) { x1[i]=x[i]; y1[i]=y[i]; }
}

static inline int n_ge(const u32 *a, const u32 *b)
{
    for (int i=7;i>=0;i--) { if (a[i]!=b[i]) return a[i]>b[i]; }
    return 1;
}

static inline void n_sub(u32 *r, const u32 *a, const u32 *b)
{
    ulong borrow=0;
    for (u32 i=0;i<8;i++) { const ulong v=(ulong)a[i]-(ulong)b[i]-borrow; r[i]=(u32)v; borrow=(v>>63)&1; }
}

static inline void n_add(u32 *r, const u32 *a, const u32 *b)
{
    const u32 N[8]={0xd0364141,0xbfd25e8c,0xaf48a03b,0xbaaedce6,0xfffffffe,0xffffffff,0xffffffff,0xffffffff};
    ulong carry=0;
    for (u32 i=0;i<8;i++) { const ulong v=(ulong)a[i]+b[i]+carry; r[i]=(u32)v; carry=v>>32; }
    if (carry || n_ge(r,N)) { u32 t[8]; n_sub(t,r,N); for (u32 i=0;i<8;i++) r[i]=t[i]; }
}

static inline void n_mul(u32 *r, const u32 *a, const u32 *b)
{
    // Left-to-right 4-bit window multiplication.  The previous bit-at-a-time
    // implementation performed 256 doublings plus up to 256 additions.  Four
    // doublings per nibble and at most three additions cuts the modular work
    // substantially while retaining the exact scalar arithmetic required by
    // Nexa's Schnorr signature.
    u32 y[8]={0};
    for (int limb=7; limb>=0; --limb) {
        for (int shift=28; shift>=0; shift-=4) {
            u32 t[8];
            n_add(t,y,y); for (u32 i=0;i<8;i++) y[i]=t[i];
            n_add(t,y,y); for (u32 i=0;i<8;i++) y[i]=t[i];
            n_add(t,y,y); for (u32 i=0;i<8;i++) y[i]=t[i];
            n_add(t,y,y); for (u32 i=0;i<8;i++) y[i]=t[i];
            const u32 digit=(b[limb]>>(u32)shift)&15U;
            u32 addend[8]={0};
            for (u32 j=0;j<3;j++) {
                if (j<digit) { n_add(t,addend,a); for (u32 i=0;i<8;i++) addend[i]=t[i]; }
            }
            n_add(t,y,addend); for (u32 i=0;i<8;i++) y[i]=t[i];
        }
    }
    for (u32 i=0;i<8;i++) r[i]=y[i];
}

static inline void rfc6979_nonce(const u8 *key, const u8 *msg, u32 *out)
{
    u8 K0[32]={0}, V[32]; for (u32 i=0;i<32;i++) V[i]=1;
    u8 seed[80]={0}; for (u32 i=0;i<32;i++) seed[i]=key[i]; for (u32 i=0;i<32;i++) seed[32+i]=msg[i];
    const u8 tag[16]={'S','c','h','n','o','r','r','+','S','H','A','2','5','6',' ',' '};
    for (u32 i=0;i<16;i++) seed[64+i]=tag[i];
    u8 q[113]={0}, tmp[32];
    for (u32 i=0;i<32;i++) q[i]=V[i]; q[32]=0; for (u32 i=0;i<80;i++) q[33+i]=seed[i]; hmac256(K0,q,113,K0);
    hmac256(K0,V,32,V);
    for (u32 i=0;i<32;i++) q[i]=V[i]; q[32]=1; for (u32 i=0;i<80;i++) q[33+i]=seed[i]; hmac256(K0,q,113,K0);
    hmac256(K0,V,32,V); hmac256(K0,V,32,tmp); words_from_be(tmp,out);
}

static inline int nexapow_finalize(const u8 *mining, u8 *out, SECP256K1_TMPS_TYPE const secp256k1_t *tmps)
{
    const u32 N[8]={0xd0364141,0xbfd25e8c,0xaf48a03b,0xbaaedce6,0xfffffffe,0xffffffff,0xffffffff,0xffffffff};
    u32 priv[8]; words_from_be(mining,priv);
    int zero=1; for (u32 i=0;i<8;i++) if (priv[i]) zero=0;
    if (zero || n_ge(priv,N)) return 0;
    u8 h1[32]; sha256_32(mining,h1);
    u32 k[8]; rfc6979_nonce(mining,h1,k); if (n_ge(k,N)) n_sub(k,k,N);
    u32 x[8], y[8]; nexapow_point_mul_xy(x,y,k);
    u32 p[8]={0xfffffc2f,0xfffffffe,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff};
    u32 leg[8]; for (u32 i=0;i<8;i++) leg[i]=y[i];
    u32 exp[8]={0xfffffe17,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0x7fffffff};
    u32 acc[8]={1,0,0,0,0,0,0,0}, base[8]; for (u32 i=0;i<8;i++) base[i]=leg[i];
    for (int bit=255;bit>=0;bit--) { u32 t[8]; mul_mod(t,acc,acc); for (u32 i=0;i<8;i++) acc[i]=t[i]; if ((exp[bit>>5]>>(bit&31))&1) { mul_mod(t,acc,base); for (u32 i=0;i<8;i++) acc[i]=t[i]; } }
    const int invert=!((acc[0]==1) && !acc[1]&&!acc[2]&&!acc[3]&&!acc[4]&&!acc[5]&&!acc[6]&&!acc[7]);
    u8 pub[33]; words_to_be(x,pub+1); pub[0]=(u8)(0x02|(y[0]&1)^(invert?1:0));
    u8 challenge[97]; for (u32 i=0;i<32;i++) challenge[i]=pub[i+1]; for (u32 i=0;i<33;i++) challenge[32+i]=pub[i]; for (u32 i=0;i<32;i++) challenge[65+i]=h1[i];
    u8 eh[32]; sha256_msg(challenge,97,eh); u32 e[8]; words_from_be(eh,e); if (n_ge(e,N)) n_sub(e,e,N);
    u32 ep[8], s[8]; n_mul(ep,e,priv); n_add(s,ep,k);
    u8 sig[64]; words_to_be(x,sig); words_to_be(s,sig+32); sha256_msg(sig,64,out); return 1;
}

__kernel void nexapow_sha_kernel(__global const u8 *header, __global u8 *hashes, const ulong start)
{
    const ulong id=(ulong)get_global_id(0); const ulong nonce=start+id;
    __local secp256k1_t tmps;
    if (get_local_id(0) == 0) set_precomputed_basepoint_g(&tmps);
    barrier(CLK_LOCAL_MEM_FENCE);
    u8 candidate[41]={0};
    // Stratum gives the commitment in display (big-endian) order, whereas
    // Nexa serializes uint256 little-endian before the nonce vector.
    for (u32 i=0;i<32;i++) candidate[i]=header[31-i];
    candidate[32]=8;
    for (u32 i=0;i<4;i++) candidate[33+i]=header[32+i];
    for (u32 i=0;i<4;i++) candidate[37+i]=(u8)(nonce >> (24-i*8));
    u8 first[32]; u8 mining[32]; u8 finalHash[32]; sha256_41(candidate,first); sha256_32(first,mining);
    if (nexapow_finalize(mining,finalHash,&tmps)) for (u32 i=0;i<32;i++) hashes[id*32+i]=finalHash[i];
    else for (u32 i=0;i<32;i++) hashes[id*32+i]=0;
}
