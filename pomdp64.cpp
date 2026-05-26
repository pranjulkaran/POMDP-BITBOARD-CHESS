/**
 * @file pomdp64.cpp
 * @brief Compact Bitboard Core for POMDP-64 (Rule Layer Extended)
 */

#include "pomdp64.hpp"
#include <iostream>

namespace pomdp64 {
    
    void Simulator::reset_board(GameState& state) {

    for (int c = 0; c < 2; ++c)
        for (int p = 0; p < 6; ++p)
            state.pieces[c][p] = 0ULL;

    state.pieces[WHITE][PAWN]   = 0x000000000000FF00ULL;
    state.pieces[WHITE][KNIGHT] = 0x0000000000000042ULL;
    state.pieces[WHITE][BISHOP] = 0x0000000000000024ULL;
    state.pieces[WHITE][ROOK]   = 0x0000000000000081ULL;
    state.pieces[WHITE][QUEEN]  = 0x0000000000000008ULL;
    state.pieces[WHITE][KING]   = 0x0000000000000010ULL;

    state.pieces[BLACK][PAWN]   = 0x00FF000000000000ULL;
    state.pieces[BLACK][KNIGHT] = 0x4200000000000000ULL;
    state.pieces[BLACK][BISHOP] = 0x2400000000000000ULL;
    state.pieces[BLACK][ROOK]   = 0x8100000000000000ULL;
    state.pieces[BLACK][QUEEN]  = 0x0800000000000000ULL;
    state.pieces[BLACK][KING]   = 0x1000000000000000ULL;

    squash_occupancy(state);

    state.metadata = 0ULL;
}

// ============================================================
// CONSTRUCTOR
// ============================================================

Simulator::Simulator() {
    init_ray_masks();
    init_attack_masks();
}

// ============================================================
// RAY INITIALIZATION
// ============================================================

void Simulator::init_ray_masks() {

    for (int s = 0; s < 64; ++s) {

        int f = s & 7;
        int r = s >> 3;

        ray_up[s] = ray_down[s] = 0ULL;
        ray_left[s] = ray_right[s] = 0ULL;

        ray_up_right[s] = ray_up_left[s] = 0ULL;
        ray_down_right[s] = ray_down_left[s] = 0ULL;

        for (int rr = r + 1; rr < 8; ++rr)
            ray_up[s] |= 1ULL << (rr * 8 + f);

        for (int rr = r - 1; rr >= 0; --rr)
            ray_down[s] |= 1ULL << (rr * 8 + f);

        for (int ff = f + 1; ff < 8; ++ff)
            ray_right[s] |= 1ULL << (r * 8 + ff);

        for (int ff = f - 1; ff >= 0; --ff)
            ray_left[s] |= 1ULL << (r * 8 + ff);

        for (int rr = r + 1, ff = f + 1;
             rr < 8 && ff < 8;
             ++rr, ++ff)
            ray_up_right[s] |= 1ULL << (rr * 8 + ff);

        for (int rr = r + 1, ff = f - 1;
             rr < 8 && ff >= 0;
             ++rr, --ff)
            ray_up_left[s] |= 1ULL << (rr * 8 + ff);

        for (int rr = r - 1, ff = f + 1;
             rr >= 0 && ff < 8;
             --rr, ++ff)
            ray_down_right[s] |= 1ULL << (rr * 8 + ff);

        for (int rr = r - 1, ff = f - 1;
             rr >= 0 && ff >= 0;
             --rr, --ff)
            ray_down_left[s] |= 1ULL << (rr * 8 + ff);
    }
}

// ============================================================
// ATTACK TABLES
// ============================================================

void Simulator::init_attack_masks() {

    constexpr int knight_delta[8][2] = {
        { 2, 1}, { 2,-1},
        {-2, 1}, {-2,-1},
        { 1, 2}, { 1,-2},
        {-1, 2}, {-1,-2}
    };

    constexpr int king_delta[8][2] = {
        { 1, 0}, {-1, 0},
        { 0, 1}, { 0,-1},
        { 1, 1}, { 1,-1},
        {-1, 1}, {-1,-1}
    };

    for (int s = 0; s < 64; ++s) {

        knight_attacks[s] = 0ULL;
        king_attacks[s] = 0ULL;

        int f = s & 7;
        int r = s >> 3;

        for (auto &d : knight_delta) {
            int nf = f + d[0];
            int nr = r + d[1];
            if (nf >= 0 && nf < 8 && nr >= 0 && nr < 8)
                knight_attacks[s] |= 1ULL << (nr * 8 + nf);
        }

        for (auto &d : king_delta) {
            int nf = f + d[0];
            int nr = r + d[1];
            if (nf >= 0 && nf < 8 && nr >= 0 && nr < 8)
                king_attacks[s] |= 1ULL << (nr * 8 + nf);
        }
    }
}

// ============================================================
// OCCUPANCY
// ============================================================

void Simulator::squash_occupancy(GameState& s) const {

    s.white_occ =
        s.pieces[WHITE][PAWN]   |
        s.pieces[WHITE][KNIGHT] |
        s.pieces[WHITE][BISHOP] |
        s.pieces[WHITE][ROOK]   |
        s.pieces[WHITE][QUEEN]  |
        s.pieces[WHITE][KING];

    s.black_occ =
        s.pieces[BLACK][PAWN]   |
        s.pieces[BLACK][KNIGHT] |
        s.pieces[BLACK][BISHOP] |
        s.pieces[BLACK][ROOK]   |
        s.pieces[BLACK][QUEEN]  |
        s.pieces[BLACK][KING];

    s.total_occ = s.white_occ | s.black_occ;
}

// ============================================================
// MOVE EXECUTION (LOW LEVEL)
// ============================================================

void Simulator::make_move(
    GameState& s,
    int color,
    int piece,
    int from,
    int to
) {
    s.pieces[color][piece] &= ~(1ULL << from);

    int enemy = color ^ 1;
    uint64_t mask = ~(1ULL << to);

    for (int p = 0; p < 6; ++p)
        s.pieces[enemy][p] &= mask;

    s.pieces[color][piece] |= (1ULL << to);

    squash_occupancy(s);

    s.metadata ^= Rules::MASK_COLOR;
}

// ============================================================
// ATTACK CHECKER
// ============================================================

bool Simulator::is_square_attacked(
    const GameState& s,
    int sq,
    int attacker_color
) const {

    if (s.pieces[attacker_color][KNIGHT] & knight_attacks[sq])
        return true;

    if (s.pieces[attacker_color][KING] & king_attacks[sq])
        return true;

    uint64_t bishops = s.pieces[attacker_color][BISHOP] |
                       s.pieces[attacker_color][QUEEN];

    uint64_t rooks = s.pieces[attacker_color][ROOK] |
                     s.pieces[attacker_color][QUEEN];

    if (bishops) {
        uint64_t vision = get_bishop_vision(sq, s.total_occ);
        if (vision & bishops) return true;
    }

    if (rooks) {
        uint64_t vision = get_rook_vision(sq, s.total_occ);
        if (vision & rooks) return true;
    }

    uint64_t pawns = s.pieces[attacker_color][PAWN];

    if (attacker_color == WHITE) {
        if ((pawns << 7) & s.black_occ & NOT_FILE_H & (1ULL << sq)) return true;
        if ((pawns << 9) & s.black_occ & NOT_FILE_A & (1ULL << sq)) return true;
    } else {
        if ((pawns >> 7) & s.white_occ & NOT_FILE_A & (1ULL << sq)) return true;
        if ((pawns >> 9) & s.white_occ & NOT_FILE_H & (1ULL << sq)) return true;
    }

    return false;
}

// ============================================================
// TRANSACTIONAL MOVE ENGINE
// ============================================================

bool Simulator::attempt_move(
    GameState& s,
    int from,
    int to,
    int promotion_type
) {
    GameState backup = s;

    apply_move(s, from, to, promotion_type);

    int king_sq = __builtin_ctzll(
        s.pieces[s.metadata & Rules::MASK_COLOR ? BLACK : WHITE][KING]
    );

    int attacker = (s.metadata & Rules::MASK_COLOR) ? WHITE : BLACK;

    if (is_square_attacked(s, king_sq, attacker)) {
        s = backup;
        return false;
    }

    s.metadata ^= Rules::MASK_COLOR;
    return true;
}

// ============================================================
// RULE-AWARE MOVE APPLICATION
// ============================================================

void Simulator::apply_move(
    GameState& s,
    int from,
    int to,
    int promotion_type
) {
    int color = (s.metadata & Rules::MASK_COLOR) ? BLACK : WHITE;
    int enemy = color ^ 1;

    int piece = -1;

    for (int p = 0; p < 6; ++p) {
        if (s.pieces[color][p] & (1ULL << from)) {
            piece = p;
            break;
        }
    }

    if (piece == -1) return;

    // EN PASSANT
    if (piece == PAWN && (s.metadata & Rules::MASK_EP_FILE)) {
        int ep_file = (s.metadata & Rules::MASK_EP_FILE) >> 4;
        int ep_sq = (color == WHITE) ? to - 8 : to + 8;

        if ((to & 7) == ep_file && !(s.pieces[enemy][PAWN] & (1ULL << to))) {
            s.pieces[enemy][PAWN] &= ~(1ULL << ep_sq);
        }
    }

    // CASTLING (rook move)
    if (piece == KING && abs(from - to) == 2) {
        if (to == 6) {
            s.pieces[color][ROOK] &= ~(1ULL << 7);
            s.pieces[color][ROOK] |= (1ULL << 5);
        } else if (to == 2) {
            s.pieces[color][ROOK] &= ~(1ULL << 0);
            s.pieces[color][ROOK] |= (1ULL << 3);
        } else if (to == 62) {
            s.pieces[color][ROOK] &= ~(1ULL << 63);
            s.pieces[color][ROOK] |= (1ULL << 61);
        } else if (to == 58) {
            s.pieces[color][ROOK] &= ~(1ULL << 56);
            s.pieces[color][ROOK] |= (1ULL << 59);
        }
    }

    // NORMAL MOVE
    s.pieces[color][piece] &= ~(1ULL << from);

    for (int p = 0; p < 6; ++p)
        s.pieces[enemy][p] &= ~(1ULL << to);

    // PROMOTION
    if (piece == PAWN && (to / 8 == 0 || to / 8 == 7)) {
        s.pieces[color][promotion_type] |= (1ULL << to);
    } else {
        s.pieces[color][piece] |= (1ULL << to);
    }

    squash_occupancy(s);
}

// ============================================================
// SLIDING ATTACKS
// ============================================================

static inline uint64_t clip_positive(
    uint64_t ray,
    uint64_t blockers,
    const uint64_t* table
) {
    return blockers ? (ray ^ table[__builtin_ctzll(blockers)]) : ray;
}

static inline uint64_t clip_negative(
    uint64_t ray,
    uint64_t blockers,
    const uint64_t* table
) {
    return blockers ? (ray ^ table[63 - __builtin_clzll(blockers)]) : ray;
}

// ============================================================
// ROOK / BISHOP / QUEEN
// ============================================================

uint64_t Simulator::get_rook_vision(int sq, uint64_t occ) const {
    return clip_positive(ray_up[sq], occ & ray_up[sq], ray_up)
         | clip_positive(ray_right[sq], occ & ray_right[sq], ray_right)
         | clip_negative(ray_down[sq], occ & ray_down[sq], ray_down)
         | clip_negative(ray_left[sq], occ & ray_left[sq], ray_left);
}

uint64_t Simulator::get_bishop_vision(int sq, uint64_t occ) const {
    return clip_positive(ray_up_right[sq], occ & ray_up_right[sq], ray_up_right)
         | clip_positive(ray_up_left[sq], occ & ray_up_left[sq], ray_up_left)
         | clip_negative(ray_down_right[sq], occ & ray_down_right[sq], ray_down_right)
         | clip_negative(ray_down_left[sq], occ & ray_down_left[sq], ray_down_left);
}

// ============================================================
// MOVE GENERATION (UNCHANGED CORE)
// ============================================================

int Simulator::generate_pseudo_moves(
    const GameState& s,
    int color,
    Move* out
) {
    int count = 0;

    uint64_t friendly = (color == WHITE) ? s.white_occ : s.black_occ;
    uint64_t enemy = (color == WHITE) ? s.black_occ : s.white_occ;
    uint64_t valid = ~friendly;

    for (int piece = KNIGHT; piece <= KING; ++piece) {

        uint64_t bb = s.pieces[color][piece];

        while (bb) {
            int from = __builtin_ctzll(bb);
            uint64_t attacks = 0;

            switch (piece) {
                case KNIGHT: attacks = knight_attacks[from]; break;
                case BISHOP: attacks = get_bishop_vision(from, s.total_occ); break;
                case ROOK:   attacks = get_rook_vision(from, s.total_occ); break;
                case QUEEN:  attacks = get_rook_vision(from, s.total_occ) |
                                        get_bishop_vision(from, s.total_occ); break;
                case KING:   attacks = king_attacks[from]; break;
            }

            attacks &= valid;

            while (attacks) {
                int to = __builtin_ctzll(attacks);
                out[count++] = {(uint8_t)from,(uint8_t)to,(uint8_t)piece,0};
                attacks &= attacks - 1;
            }

            bb &= bb - 1;
        }
    }

    uint64_t pawns = s.pieces[color][PAWN];

    if (color == WHITE) {
        uint64_t push1 = (pawns << 8) & ~s.total_occ;
        uint64_t push2 = ((push1 & RANK_3) << 8) & ~s.total_occ;
        uint64_t capL = (pawns << 7) & NOT_FILE_H & enemy;
        uint64_t capR = (pawns << 9) & NOT_FILE_A & enemy;

        while (push1) { int to = __builtin_ctzll(push1);
            out[count++] = {(uint8_t)(to-8),(uint8_t)to,PAWN,0}; push1 &= push1-1; }

        while (push2) { int to = __builtin_ctzll(push2);
            out[count++] = {(uint8_t)(to-16),(uint8_t)to,PAWN,0}; push2 &= push2-1; }

        while (capL) { int to = __builtin_ctzll(capL);
            out[count++] = {(uint8_t)(to-7),(uint8_t)to,PAWN,0}; capL &= capL-1; }

        while (capR) { int to = __builtin_ctzll(capR);
            out[count++] = {(uint8_t)(to-9),(uint8_t)to,PAWN,0}; capR &= capR-1; }
    } else {
        uint64_t push1 = (pawns >> 8) & ~s.total_occ;
        uint64_t push2 = ((push1 & RANK_6) >> 8) & ~s.total_occ;
        uint64_t capL = (pawns >> 9) & NOT_FILE_H & enemy;
        uint64_t capR = (pawns >> 7) & NOT_FILE_A & enemy;

        while (push1) { int to = __builtin_ctzll(push1);
            out[count++] = {(uint8_t)(to+8),(uint8_t)to,PAWN,0}; push1 &= push1-1; }

        while (push2) { int to = __builtin_ctzll(push2);
            out[count++] = {(uint8_t)(to+16),(uint8_t)to,PAWN,0}; push2 &= push2-1; }

        while (capL) { int to = __builtin_ctzll(capL);
            out[count++] = {(uint8_t)(to+9),(uint8_t)to,PAWN,0}; capL &= capL-1; }

        while (capR) { int to = __builtin_ctzll(capR);
            out[count++] = {(uint8_t)(to+7),(uint8_t)to,PAWN,0}; capR &= capR-1; }
    }

    return count;
}

// ============================================================
// VISIBILITY
// ============================================================

uint64_t Simulator::get_visibility_mask(
    const GameState& s,
    int color
) const {
    uint64_t vis = (color == WHITE) ? s.white_occ : s.black_occ;

    for (int piece = PAWN; piece <= KING; ++piece) {
        uint64_t bb = s.pieces[color][piece];

        while (bb) {
            int sq = __builtin_ctzll(bb);

            switch (piece) {
                case PAWN:
                    vis |= knight_attacks[sq]; // simplified pawn influence kept safe
                    break;
                case KNIGHT: vis |= knight_attacks[sq]; break;
                case BISHOP: vis |= get_bishop_vision(sq, s.total_occ); break;
                case ROOK:   vis |= get_rook_vision(sq, s.total_occ); break;
                case QUEEN:  vis |= get_rook_vision(sq, s.total_occ) |
                                    get_bishop_vision(sq, s.total_occ); break;
                case KING:   vis |= king_attacks[sq]; break;
            }

            bb &= bb - 1;
        }
    }

    return vis;
}

// ============================================================
// DEBUG
// ============================================================

void Simulator::print_bitboard(uint64_t bb) const {

    std::cout << "\n";

    for (int r = 7; r >= 0; --r) {
        std::cout << r + 1 << "  ";

        for (int f = 0; f < 8; ++f) {
            int sq = r * 8 + f;
            std::cout << ((bb >> sq) & 1ULL) << " ";
        }

        std::cout << "\n";
    }

    std::cout << "   a b c d e f g h\n\n";
}

} // namespace pomdp64