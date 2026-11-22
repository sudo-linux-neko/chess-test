// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Vaibhav Agarwal

#include "chessclaw/bitboard.h"
#include <cstring>
#include <vector>
#include <algorithm>
#include <ctime>

namespace chessclaw {

Bitboard kKnightAttacks[64], kKingAttacks[64], kPawnAttacks[2][64];
Bitboard gBishopAttacks[5248];
Bitboard gRookAttacks[102400];

// Now non-const to allow runtime initialization
Magic kBishopMagics[64];
Magic kRookMagics[64];

static Bitboard slider(int sq, Bitboard occ, const int* df, const int* dr, int n) {
    Bitboard a = 0; int f = sq % 8, r = sq / 8;
    for (int i = 0; i < n; ++i) {
        int ff = f + df[i], rr = r + dr[i];
        while (ff >= 0 && ff < 8 && rr >= 0 && rr < 8) {
            Bitboard b = Bitboard(1) << (rr * 8 + ff);
            a |= b;
            if (occ & b) break;
            ff += df[i]; rr += dr[i];
        }
    }
    return a;
}

static void findMagic(int sq, const int* df, const int* dr, int n, int bits, Bitboard* table, Magic& m) {
    Bitboard mask = 0; int f = sq % 8, r = sq / 8;
    for (int i = 0; i < n; ++i) {
        int ff = f + df[i], rr = r + dr[i];
        while (ff >= 0 && ff < 8 && rr >= 0 && rr < 8) {
            mask |= Bitboard(1) << (rr * 8 + ff);
            ff += df[i]; rr += dr[i];
        }
    }
    int N = 1 << bits;
    std::vector<Bitboard> occ(N), atk(N);
    for (int i = 0; i < N; ++i) {
        Bitboard o = 0; int tmp = i;
        for (int b = 0; b < 64; ++b) {
            if (mask & (Bitboard(1) << b)) {
                if (tmp & 1) o |= (Bitboard(1) << b);
                tmp >>= 1;
            }
        }
        occ[i] = o; atk[i] = slider(sq, o, df, dr, n);
    }
    srand(time(NULL));
    while(true) {
        std::uint64_t magic = 0;
        for(int i=0;i<4;i++) magic |= (1ULL << (rand()%64));
        if (popcount((mask * magic) & 0xFF00000000000000ULL) < 6) continue;
        
        std::vector<Bitboard> tbl(N, 0);
        bool ok = true;
        for (int i = 0; i < N && ok; ++i) {
            unsigned idx = (unsigned)((occ[i] * magic) >> (64 - bits));
            if (tbl[idx] != 0 && tbl[idx] != atk[i]) ok = false;
            else tbl[idx] = atk[i];
        }
        if (ok) {
            m.mask = mask; m.magic = magic; m.shift = 64 - bits;
            for (int i = 0; i < N; ++i) table[i] = tbl[i];
            return;
        }
    }
}

void initAttacks() {
    for(int s=0;s<64;++s){
        int f=s&7,r=s>>3;
        Bitboard kn=0, ki=0;
        const int kd[8][2]={{1,2},{2,1},{2,-1},{1,-2},{-1,-2},{-2,-1},{-2,1},{-1,2}};
        for(auto &d:kd){int ff=f+d[0],rr=r+d[1]; if(ff>=0&&ff<8&&rr>=0&&rr<8) kn|=Bitboard(1)<<(rr*8+ff);}
        for(int df=-1;df<=1;++df)for(int dr=-1;dr<=1;++dr){if(!df&&!dr)continue; int ff=f+df,rr=r+dr; if(ff>=0&&ff<8&&rr>=0&&rr<8) ki|=Bitboard(1)<<(rr*8+ff);}
        kKnightAttacks[s]=kn; kKingAttacks[s]=ki;
        Bitboard wp=0,bp=0;
        if(r<7){ if(f>0)wp|=Bitboard(1)<<((r+1)*8+f-1); if(f<7)wp|=Bitboard(1)<<((r+1)*8+f+1); }
        if(r>0){ if(f>0)bp|=Bitboard(1)<<((r-1)*8+f-1); if(f<7)bp|=Bitboard(1)<<((r-1)*8+f+1); }
        kPawnAttacks[0][s]=wp; kPawnAttacks[1][s]=bp;
    }

    static Bitboard btbl[64][512], rtbl[64][4096];
    const int bdf[4]={1,1,-1,-1}, bdr[4]={1,-1,1,-1};
    const int rdf[4]={1,-1,0,0}, rdr[4]={0,0,1,-1};
    int bOff = 0, rOff = 0;
    for(int s=0;s<64;++s){
        Bitboard mask=0; for(int i=0;i<4;++i){ int ff=s%8+bdf[i],rr=s/8+bdr[i]; while(ff>=0&&ff<8&&rr>=0&&rr<8){ mask|=Bitboard(1)<<(rr*8+ff); ff+=bdf[i]; rr+=bdr[i]; } }
        int bits=popcount(mask);
        findMagic(s,bdf,bdr,4,bits,btbl[s],kBishopMagics[s]);
        kBishopMagics[s].offset = bOff;
        bOff += (1<<bits);
    }
    for(int s=0;s<64;++s){
        Bitboard mask=0; for(int i=0;i<4;++i){ int ff=s%8+rdf[i],rr=s/8+rdr[i]; while(ff>=0&&ff<8&&rr>=0&&rr<8){ mask|=Bitboard(1)<<(rr*8+ff); ff+=rdf[i]; rr+=rdr[i]; } }
        int bits=popcount(mask);
        findMagic(s,rdf,rdr,4,bits,rtbl[s],kRookMagics[s]);
        kRookMagics[s].offset = rOff;
        rOff += (1<<bits);
    }
    for(int s=0;s<64;++s){ for(int i=0;i<(1<<(64-kBishopMagics[s].shift));++i) gBishopAttacks[kBishopMagics[s].offset+i]=btbl[s][i]; }
    for(int s=0;s<64;++s){ for(int i=0;i<(1<<(64-kRookMagics[s].shift));++i) gRookAttacks[kRookMagics[s].offset+i]=rtbl[s][i]; }
}

std::string moveToUci(Move m) { return "0000"; }
Move uciToMove(const std::string& s) { return {}; }
std::string moveToSan(const std::string& u) { return u; }

} // namespace chessclaw
