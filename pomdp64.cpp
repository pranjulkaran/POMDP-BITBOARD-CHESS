#include "pomdp64.hpp"
#include <iostream>

namespace pomdp64 {

// ============================================================
// INIT
// ============================================================

Simulator::Simulator() {
    init_ray_masks();
    init_attack_masks();
}

// ============================================================
// RAYS
// ============================================================

void Simulator::init_ray_masks() {

    for (int s = 0; s < 64; ++s) {

        int f = s & 7;
        int r = s >> 3;

        ray_up[s] = ray_down[s] = ray_left[s] = ray_right[s] = 0ULL;
        ray_ur[s] = ray_ul[s] = ray_dr[s] = ray_dl[s] = 0ULL;

        for (int rr = r + 1; rr < 8; ++rr)
            ray_up[s] |= 1ULL << (rr * 8 + f);

        for (int rr = r - 1; rr >= 0; --rr)
            ray_down[s] |= 1ULL << (rr * 8 + f);

        for (int ff = f + 1; ff < 8; ++ff)
            ray_right[s] |= 1ULL << (r * 8 + ff);

        for (int ff = f - 1; ff >= 0; --ff)
            ray_left[s] |= 1ULL << (r * 8 + ff);

        for (int rr = r + 1, ff = f + 1; rr < 8 && ff < 8; ++rr, ++ff)
            ray_ur[s] |= 1ULL << (rr * 8 + ff);

        for (int rr = r + 1, ff = f - 1; rr < 8 && ff >= 0; ++rr, --ff)
            ray_ul[s] |= 1ULL << (rr * 8 + ff);

        for (int rr = r - 1, ff = f + 1; rr >= 0 && ff < 8; --rr, ++ff)
            ray_dr[s] |= 1ULL << (rr * 8 + ff);

        for (int rr = r - 1, ff = f - 1; rr >= 0 && ff >= 0; --rr, --ff)
            ray_dl[s] |= 1ULL << (rr * 8 + ff);
    }
}

// ============================================================
// ATTACKS
// ============================================================

void Simulator::init_attack_masks() {

    static constexpr int N[8][2] = {
        {2,1},{2,-1},{-2,1},{-2,-1},
        {1,2},{1,-2},{-1,2},{-1,-2}
    };

    static constexpr int K[8][2] = {
        {1,0},{-1,0},{0,1},{0,-1},
        {1,1},{1,-1},{-1,1},{-1,-1}
    };

    for (int s = 0; s < 64; ++s) {

        knight_attacks[s] = 0ULL;
        king_attacks[s] = 0ULL;

        int f = s & 7, r = s >> 3;

        for (auto &d : N) {
            int nf = f + d[0], nr = r + d[1];
            if (nf>=0 && nf<8 && nr>=0 && nr<8)
                knight_attacks[s] |= 1ULL << (nr*8+nf);
        }

        for (auto &d : K) {
            int nf = f + d[0], nr = r + d[1];
            if (nf>=0 && nf<8 && nr>=0 && nr<8)
                king_attacks[s] |= 1ULL << (nr*8+nf);
        }
    }
}

// ============================================================
// BOARD
// ============================================================

void Simulator::reset_board(GameState& s) {

    for (auto &c : s.pieces)
        for (auto &p : c)
            p = 0ULL;

    s.pieces[WHITE][PAWN]   = 0xFF00ULL;
    s.pieces[WHITE][KNIGHT] = 0x42ULL;
    s.pieces[WHITE][BISHOP] = 0x24ULL;
    s.pieces[WHITE][ROOK]   = 0x81ULL;
    s.pieces[WHITE][QUEEN]  = 0x08ULL;
    s.pieces[WHITE][KING]   = 0x10ULL;

    s.pieces[BLACK][PAWN]   = 0xFF000000000000ULL;
    s.pieces[BLACK][KNIGHT] = 0x4200000000000000ULL;
    s.pieces[BLACK][BISHOP] = 0x2400000000000000ULL;
    s.pieces[BLACK][ROOK]   = 0x8100000000000000ULL;
    s.pieces[BLACK][QUEEN]  = 0x0800000000000000ULL;
    s.pieces[BLACK][KING]   = 0x1000000000000000ULL;

    squash_occupancy(s);
    s.metadata = 0;
}

void Simulator::squash_occupancy(GameState& s) const {

    s.white_occ = s.black_occ = s.total_occ = 0;

    for (int p=0;p<6;++p) {
        s.white_occ |= s.pieces[WHITE][p];
        s.black_occ |= s.pieces[BLACK][p];
    }

    s.total_occ = s.white_occ | s.black_occ;
}

// ============================================================
// ATTACK HELPERS
// ============================================================

static inline uint64_t clip(uint64_t ray,uint64_t blockers,const uint64_t* t){
    return blockers ? (ray ^ t[__builtin_ctzll(blockers)]) : ray;
}

static inline uint64_t clip_r(uint64_t ray,uint64_t blockers,const uint64_t* t){
    return blockers ? (ray ^ t[63-__builtin_clzll(blockers)]) : ray;
}

// ============================================================
// VISION
// ============================================================

uint64_t Simulator::get_rook_vision(int sq,uint64_t occ) const{
    return clip(ray_up[sq],occ&ray_up[sq],ray_up)
         | clip(ray_right[sq],occ&ray_right[sq],ray_right)
         | clip_r(ray_down[sq],occ&ray_down[sq],ray_down)
         | clip_r(ray_left[sq],occ&ray_left[sq],ray_left);
}

uint64_t Simulator::get_bishop_vision(int sq,uint64_t occ) const{
    return clip(ray_ur[sq],occ&ray_ur[sq],ray_ur)
         | clip(ray_ul[sq],occ&ray_ul[sq],ray_ul)
         | clip_r(ray_dr[sq],occ&ray_dr[sq],ray_dr)
         | clip_r(ray_dl[sq],occ&ray_dl[sq],ray_dl);
}

uint64_t Simulator::get_queen_vision(int sq,uint64_t occ) const{
    return get_rook_vision(sq,occ)|get_bishop_vision(sq,occ);
}

// ============================================================
// ATTACK CHECK
// ============================================================

bool Simulator::is_square_attacked(const GameState& s,int sq,int atk) const{

    if (s.pieces[atk][KNIGHT] & knight_attacks[sq]) return true;
    if (s.pieces[atk][KING] & king_attacks[sq]) return true;

    uint64_t bishops = s.pieces[atk][BISHOP] | s.pieces[atk][QUEEN];
    uint64_t rooks   = s.pieces[atk][ROOK]   | s.pieces[atk][QUEEN];

    if (bishops && (get_bishop_vision(sq,s.total_occ)&bishops)) return true;
    if (rooks   && (get_rook_vision(sq,s.total_occ)&rooks)) return true;

    return false;
}

bool Simulator::is_in_check(const GameState& s,int color) const{
    int ksq = __builtin_ctzll(s.pieces[color][KING]);
    return is_square_attacked(s,ksq,color^1);
}

// ============================================================
// PIN DETECTION
// ============================================================

uint64_t Simulator::get_pinned_pieces(const GameState& s,int color) const{

    uint64_t pinned = 0;
    int king = __builtin_ctzll(s.pieces[color][KING]);
    uint64_t occ = s.total_occ;

    uint64_t rays[8] = {
        ray_up[king],ray_down[king],
        ray_left[king],ray_right[king],
        ray_ur[king],ray_ul[king],
        ray_dr[king],ray_dl[king]
    };

    for (auto r : rays) {
        uint64_t b = r & occ;
        if (!b) continue;

        int first = __builtin_ctzll(b);
        uint64_t sq = 1ULL << first;

        uint64_t friendly = s.white_occ | s.black_occ;
        if (!(sq & friendly)) continue;

        pinned |= sq;
    }

    return pinned;
}

// ============================================================
// LEGAL MASK
// ============================================================

uint64_t Simulator::legal_mask_from_pins(const GameState& s,int color,int from) const{

    uint64_t pinned = get_pinned_pieces(s,color);

    if (!(pinned & (1ULL<<from)))
        return ~0ULL;

    int king = __builtin_ctzll(s.pieces[color][KING]);

    uint64_t line =
        ray_up[king]|ray_down[king]|
        ray_left[king]|ray_right[king]|
        ray_ur[king]|ray_ul[king]|
        ray_dr[king]|ray_dl[king];

    return line;
}

// ============================================================
// MOVE GENERATION (FINAL INTEGRATED)
// ============================================================

int Simulator::generate_pseudo_moves(const GameState& s,int color,Move* out){

    int count=0;

    uint64_t friendly = (color==WHITE)?s.white_occ:s.black_occ;

    uint64_t pinned = get_pinned_pieces(s,color);

    for(int p=PAWN;p<=KING;++p){

        uint64_t bb=s.pieces[color][p];

        while(bb){

            int from=__builtin_ctzll(bb);
            bb&=bb-1;

            uint64_t pin_mask =
                (pinned & (1ULL<<from))
                ? legal_mask_from_pins(s,color,from)
                : ~friendly;

            uint64_t att=0;

            if(p==KNIGHT) att=knight_attacks[from];
            else if(p==KING) att=king_attacks[from];
            else if(p==BISHOP) att=get_bishop_vision(from,s.total_occ);
            else if(p==ROOK) att=get_rook_vision(from,s.total_occ);
            else if(p==QUEEN) att=get_queen_vision(from,s.total_occ);
            else if(p==PAWN) att=0;

            att &= pin_mask;

            while(att){

                int to=__builtin_ctzll(att);
                att&=att-1;

                out[count++]={(uint8_t)from,(uint8_t)to,(uint8_t)p,0};
            }
        }
    }

    return count;
}

// ============================================================
// VISIBILITY
// ============================================================

uint64_t Simulator::get_visibility_mask(const GameState& s,int color) const{

    uint64_t v=(color==WHITE)?s.white_occ:s.black_occ;

    for(int p=PAWN;p<=KING;++p){

        uint64_t bb=s.pieces[color][p];

        while(bb){

            int sq=__builtin_ctzll(bb);
            bb&=bb-1;

            if(p==KNIGHT) v|=knight_attacks[sq];
            else if(p==KING) v|=king_attacks[sq];
            else if(p==BISHOP) v|=get_bishop_vision(sq,s.total_occ);
            else if(p==ROOK) v|=get_rook_vision(sq,s.total_occ);
            else if(p==QUEEN) v|=get_queen_vision(sq,s.total_occ);
        }
    }

    return v;
}

// ============================================================
// PRINT
// ============================================================

void Simulator::print_bitboard(uint64_t bb) const{

    for(int r=7;r>=0;--r){
        for(int f=0;f<8;++f)
            std::cout<<((bb>>(r*8+f))&1)<<" ";
        std::cout<<"\n";
    }
}

} // namespace pomdp64