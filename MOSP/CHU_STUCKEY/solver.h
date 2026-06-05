#ifndef SOLVER_H
#define SOLVER_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <assert.h>
#include <new>
#include <time.h>
#include <algorithm>

static inline long long wallClockTime(void) {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return (long long) tp.tv_sec * 1000000 + tp.tv_usec;
}

/******************************* MULTI THREADING CHANGES ****************************/
#define MAXTHREAD 32

long long ccc[MAXTHREAD];
int loopcount[MAXTHREAD];
int bitscount[MAXTHREAD];
int reuse[MAXTHREAD];
int definite[MAXTHREAD];
long long LIMIT[MAXTHREAD];

int process_id = 0;

int total_ccc = 0;
long long total_optccc = 0; 
int total_loopcount = 0;
int total_bitscount = 0;
int total_reuse = 0;
int total_definite = 0;
int total_LIMIT = 0;

// =====================================================================
// LIMITES EXPANDIDOS PARA SUPORTAR INSTÂNCIAS DE ATÉ 1024x1024
// =====================================================================
#define ARRAYPRODUCTS 1024
#define MAXPRODUCTS   ARRAYPRODUCTS
#define MAXCUSTOMERS  ARRAYPRODUCTS
#define MAXBIT 64

/******************************* CLASS DEFINITION ****************************/

class SET {
public:
    // Expandido de 8 para 16 long longs (16 * 64 = 1024 bits)
    long long a[16]; 
    
    SET() {}
    SET(long long a0, long long a1, long long a2, long long a3, 
        long long a4, long long a5, long long a6, long long a7,
        long long a8, long long a9, long long a10, long long a11,
        long long a12, long long a13, long long a14, long long a15) {
        a[0]=a0; a[1]=a1; a[2]=a2; a[3]=a3;
        a[4]=a4; a[5]=a5; a[6]=a6; a[7]=a7;
        a[8]=a8; a[9]=a9; a[10]=a10; a[11]=a11;
        a[12]=a12; a[13]=a13; a[14]=a14; a[15]=a15;
    }

    bool operator == (SET S) const {
        for (int i = 0; i < 16; i++) if (a[i] != S.a[i]) return false;
        return true;
    }
    bool operator != (SET S) const {
        return !(*this == S);
    }

    bool operator < (SET S) const {
        for (int i = 15; i >= 0; i--) {
            if (a[i] < S.a[i]) return true;
            if (a[i] > S.a[i]) return false;
        }
        return false;
    }

    SET operator + (SET S) const {
        return SET(a[0]|S.a[0], a[1]|S.a[1], a[2]|S.a[2], a[3]|S.a[3],
                   a[4]|S.a[4], a[5]|S.a[5], a[6]|S.a[6], a[7]|S.a[7],
                   a[8]|S.a[8], a[9]|S.a[9], a[10]|S.a[10], a[11]|S.a[11],
                   a[12]|S.a[12], a[13]|S.a[13], a[14]|S.a[14], a[15]|S.a[15]);
    }
    SET operator - (SET S) const {
        return SET(a[0]&~S.a[0], a[1]&~S.a[1], a[2]&~S.a[2], a[3]&~S.a[3],
                   a[4]&~S.a[4], a[5]&~S.a[5], a[6]&~S.a[6], a[7]&~S.a[7],
                   a[8]&~S.a[8], a[9]&~S.a[9], a[10]&~S.a[10], a[11]&~S.a[11],
                   a[12]&~S.a[12], a[13]&~S.a[13], a[14]&~S.a[14], a[15]&~S.a[15]);
    }
    SET operator & (SET S) const {
        return SET(a[0]&S.a[0], a[1]&S.a[1], a[2]&S.a[2], a[3]&S.a[3],
                   a[4]&S.a[4], a[5]&S.a[5], a[6]&S.a[6], a[7]&S.a[7],
                   a[8]&S.a[8], a[9]&S.a[9], a[10]&S.a[10], a[11]&S.a[11],
                   a[12]&S.a[12], a[13]&S.a[13], a[14]&S.a[14], a[15]&S.a[15]);
    }

    inline void addbit(int i) {
        a[i/64] |= ((long long) 1 << (i%64));
    }
    inline void removebit(int i) {
        a[i/64] &= ~((long long) 1 << (i%64));
    }
    inline bool subset(SET S) const {
        for (int i = 0; i < 16; i++) if (a[i] != (a[i] & S.a[i])) return false;
        return true;
    }

    int nbitsp() const {
        int c = 0;
        // Otimização Extrema (Intrinsic Hardware)
        // Substitui o "word &= word - 1" por contagem nativa do processador
        for (int i = 0; i < 16; i++) {
            c += __builtin_popcountll(a[i]);
        }
        bitscount[process_id]++;
        return c;
    }

    inline bool in(int i) const {
        return ((a[i/64] & ((long long) 1 << (i%64))) != 0);
    }
};

const SET emptyset = SET(0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0);

#define in(e,S) (S.in(e)) 

long long bit[64];

inline int safe_max(int a, int b) { return (a > b) ? a : b; }

/******************************* LINEAR CHAINING ****************************/

#define TABLESIZE        16777216  /* 2^24 */
#define MAXENTRIES       TABLESIZE

int collisions = 0;
int hash_entries = 0;

typedef struct entry {
  SET p;
  SET q;
  int time_stamp;
  struct entry *next;
} ENTRY, *EP;

EP hsh[TABLESIZE];

#define mix(a,b,c) \
{ \
  a -= b; a -= c; a ^= (c>>13); \
  b -= c; b -= a; b ^= (a<<8); \
  c -= a; c -= b; c ^= (b>>13); \
  a -= b; a -= c; a ^= (c>>12);  \
  b -= c; b -= a; b ^= (a<<16); \
  c -= a; c -= b; c ^= (b>>5); \
  a -= b; a -= c; a ^= (c>>3);  \
  b -= c; b -= a; b ^= (a<<10); \
  c -= a; c -= b; c ^= (b>>15); \
}

// =====================================================================
// HASHING EXPANDIDO PARA MISTURAR OS 16 BLOCOS DE 64-BITS
// =====================================================================
unsigned int ghash(SET p) {
   register unsigned int a,b,c;

   a = b = 0x9e3779b9;    
   c = 392842354;

   a += p.a[0]; b += (p.a[0]>>32); mix(a,b,c);
   c += p.a[1]; a += (p.a[1]>>32); mix(a,b,c);
   a += p.a[2]; b += (p.a[2]>>32); mix(a,b,c);
   c += p.a[3]; a += (p.a[3]>>32); mix(a,b,c);
   a += p.a[4]; b += (p.a[4]>>32); mix(a,b,c);
   c += p.a[5]; a += (p.a[5]>>32); mix(a,b,c);
   a += p.a[6]; b += (p.a[6]>>32); mix(a,b,c);
   c += p.a[7]; a += (p.a[7]>>32); mix(a,b,c);
   a += p.a[8]; b += (p.a[8]>>32); mix(a,b,c);
   c += p.a[9]; a += (p.a[9]>>32); mix(a,b,c);
   a += p.a[10]; b += (p.a[10]>>32); mix(a,b,c);
   c += p.a[11]; a += (p.a[11]>>32); mix(a,b,c);
   a += p.a[12]; b += (p.a[12]>>32); mix(a,b,c);
   c += p.a[13]; a += (p.a[13]>>32); mix(a,b,c);
   a += p.a[14]; b += (p.a[14]>>32); mix(a,b,c);
   c += p.a[15]; a += (p.a[15]>>32); mix(a,b,c);

   return c;
}

void printarr(int SIZE, int a[]) {
  int i;
  for (i = 0; i < SIZE; i++) printf("%5d",a[i]);
  printf("\n");
}

void printbit(int nbits, SET n) {
  int  i;
  for (i = nbits-1; i >= 0; i--)
    if (in(i,n)) printf("1");
    else printf("0");
}

int hash(SET p, SET q) {
  int i;
  EP  r;

    i = (ghash(p)>>8);

    assert(i >= 0);
    assert(i < TABLESIZE);
  r = hsh[i];
  while (r && (r->p != p))
    r = r->next;

    if (r) {
        while (r && (r->p == p)) {
            if ((r->q).subset(q)) {
                r->time_stamp = ccc[process_id];
                return 1;
            }
            r = r->next;
        }
    }
    return 0;
}

void hashset(SET p, SET q) {
  int i, j;
  EP  r, last = NULL;

    hash_entries++;

    if (hash_entries%100000 == 0) printf("Hash entries: %d\n", hash_entries);

    if (hash_entries >= MAXENTRIES) {
        printf("Clean hash table\n"); fflush(stdout);
        for (j = 0; j < TABLESIZE; j++) {
            EP t = hsh[j];
            while (t) {
                if (t->time_stamp < ccc[process_id]-MAXENTRIES/2) {
                    hsh[j] = t->next;
                    free(t);
                    hash_entries--;
                    t = hsh[j];
                } else break;
            }
            if (t) {
                EP prev = t;
                EP cur = t->next;
                while (cur) {
                    if (cur->time_stamp < ccc[process_id]-MAXENTRIES/2) {
                        prev->next = cur->next;
                        free(cur);
                        hash_entries--;
                        cur = prev->next;
                    } else {
                        prev = cur;
                        cur = cur->next;
                    }
                }
            }
        }
        printf("Remaining entries: %d\n", hash_entries); fflush(stdout);
    }

    i = (ghash(p)>>8);
    assert(i >= 0 && i < TABLESIZE);
    
    r = hsh[i];
    if (r) collisions++;

    while (r && (r->p != p)) {
        last = r;
        r = r->next;
    }
    if (!r) {
        r = (EP) malloc(sizeof(ENTRY));
        r->p = p;
        r->q = q;
        r->time_stamp = ccc[process_id];
        r->next = hsh[i];
        hsh[i] = r;
    } else {
        while (r && (r->p == p)) {
            if (q.subset(r->q)) {
                EP tmp = r;
                r = r->next;
                free(tmp);
                if (last) last->next = r;
                else hsh[i] = r;
            } else {
                last = r;
                r = r->next;
            }
        }
    }
}

void flushhashtable() {
    for (int i = 0; i < TABLESIZE; i++) {
        EP r = hsh[i];
        while (r) {
            EP next = r->next;
            free(r);
            r = next;
        }
        hsh[i] = NULL;
    }
}
#endif