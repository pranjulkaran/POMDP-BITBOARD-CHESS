/**
 * @file pomdp64.cpp
 * @brief Compact Bitboard Core for POMDP-64
 */

#include "pomdp64.hpp"
#include <iostream>

namespace pomdp64 {

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

        for (auto& d : knight_delta) {

            int nf = f + d[0];
            int nr = r + d[1];

            if (nf >= 0 && nf < 8 &&
                nr >= 0 && nr < 8)
                knight_attacks[s] |=
                    1ULL << (nr * 8 + nf);
        }

        for (auto& d : king_delta) {

            int nf = f + d[0];
            int nr = r + d[1];

            if (nf >= 0 && nf < 8 &&
                nr >= 0 && nr < 8)
                king_attacks[s] |=
                    1ULL << (nr * 8 + nf);
        }
    }
}

// ============================================================
// RESET
// ============================================================

void Simulator::reset_board(GameState& s) {

    for (int c = 0; c < 2; ++c)
        for (int p = 0; p < 6; ++p)
            s.pieces[c][p] = 0ULL;

    s.pieces[WHITE][PAWN]   = 0x000000000000FF00ULL;
    s.pieces[WHITE][KNIGHT] = 0x0000000000000042ULL;
    s.pieces[WHITE][BISHOP] = 0x0000000000000024ULL;
    s.pieces[WHITE][ROOK]   = 0x0000000000000081ULL;
    s.pieces[WHITE][QUEEN]  = 0x0000000000000008ULL;
    s.pieces[WHITE][KING]   = 0x0000000000000010ULL;

    s.pieces[BLACK][PAWN]   = 0x00FF000000000000ULL;
    s.pieces[BLACK][KNIGHT] = 0x4200000000000000ULL;
    s.pieces[BLACK][BISHOP] = 0x2400000000000000ULL;
    s.pieces[BLACK][ROOK]   = 0x8100000000000000ULL;
    s.pieces[BLACK][QUEEN]  = 0x0800000000000000ULL;
    s.pieces[BLACK][KING]   = 0x1000000000000000ULL;

    squash_occupancy(s);

    s.metadata = 0ULL;
}

// ============================================================
// OCCUPANCY
// ============================================================

inline void Simulator::squash_occupancy(GameState& s) const {

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
// MOVE EXECUTION
// ============================================================

void Simulator::make_move(
    GameState& s,
    int color,
    int piece,
    int from,
    int to
) {

    s.pieces[color][piece] &=
        ~(1ULL << from);

    int enemy = color ^ 1;

    uint64_t clear_mask =
        ~(1ULL << to);

    for (int p = 0; p < 6; ++p)
        s.pieces[enemy][p] &= clear_mask;

    s.pieces[color][piece] |=
        (1ULL << to);

    squash_occupancy(s);

    s.metadata ^= 1ULL;
}

// ============================================================
// SLIDING HELPERS
// ============================================================

static inline uint64_t clip_positive(
    uint64_t ray,
    uint64_t blockers,
    const uint64_t* table
) {
    return blockers
        ? (ray ^ table[__builtin_ctzll(blockers)])
        : ray;
}

static inline uint64_t clip_negative(
    uint64_t ray,
    uint64_t blockers,
    const uint64_t* table
) {
    return blockers
        ? (ray ^ table[63 - __builtin_clzll(blockers)])
        : ray;
}

// ============================================================
// ROOK
// ============================================================

uint64_t Simulator::get_rook_vision(
    int sq,
    uint64_t occ
) const {

    return
        clip_positive(
            ray_up[sq],
            occ & ray_up[sq],
            ray_up
        )

        |

        clip_positive(
            ray_right[sq],
            occ & ray_right[sq],
            ray_right
        )

        |

        clip_negative(
            ray_down[sq],
            occ & ray_down[sq],
            ray_down
        )

        |

        clip_negative(
            ray_left[sq],
            occ & ray_left[sq],
            ray_left
        );
}

// ============================================================
// BISHOP
// ============================================================

uint64_t Simulator::get_bishop_vision(
    int sq,
    uint64_t occ
) const {

    return
        clip_positive(
            ray_up_right[sq],
            occ & ray_up_right[sq],
            ray_up_right
        )

        |

        clip_positive(
            ray_up_left[sq],
            occ & ray_up_left[sq],
            ray_up_left
        )

        |

        clip_negative(
            ray_down_right[sq],
            occ & ray_down_right[sq],
            ray_down_right
        )

        |

        clip_negative(
            ray_down_left[sq],
            occ & ray_down_left[sq],
            ray_down_left
        );
}

// ============================================================
// MOVE GENERATION
// ============================================================

int Simulator::generate_pseudo_moves(
    const GameState& s,
    int color,
    Move* out
) {

    int count = 0;

    uint64_t friendly =
        (color == WHITE)
        ? s.white_occ
        : s.black_occ;

    uint64_t enemy =
        (color == WHITE)
        ? s.black_occ
        : s.white_occ;

    uint64_t valid = ~friendly;

    // ========================================================
    // KNIGHT / BISHOP / ROOK / QUEEN / KING
    // ========================================================

    for (int piece = KNIGHT;
         piece <= KING;
         ++piece) {

        uint64_t bb =
            s.pieces[color][piece];

        while (bb) {

            int from =
                __builtin_ctzll(bb);

            uint64_t attacks = 0ULL;

            switch (piece) {

                case KNIGHT:
                    attacks =
                        knight_attacks[from];
                    break;

                case BISHOP:
                    attacks =
                        get_bishop_vision(
                            from,
                            s.total_occ
                        );
                    break;

                case ROOK:
                    attacks =
                        get_rook_vision(
                            from,
                            s.total_occ
                        );
                    break;

                case QUEEN:
                    attacks =
                        get_queen_vision(
                            from,
                            s.total_occ
                        );
                    break;

                case KING:
                    attacks =
                        king_attacks[from];
                    break;
            }

            attacks &= valid;

            while (attacks) {

                int to =
                    __builtin_ctzll(attacks);

                out[count++] = {
                    (uint8_t)from,
                    (uint8_t)to,
                    (uint8_t)piece,
                    0
                };

                attacks &= attacks - 1;
            }

            bb &= bb - 1;
        }
    }

    // ========================================================
    // PAWNS
    // ========================================================

    uint64_t pawns =
        s.pieces[color][PAWN];

    if (color == WHITE) {

        uint64_t push1 =
            (pawns << 8) &
            ~s.total_occ;

        uint64_t push2 =
            ((push1 & RANK_3) << 8) &
            ~s.total_occ;

        uint64_t capL =
            (pawns << 7) &
            NOT_FILE_H &
            enemy;

        uint64_t capR =
            (pawns << 9) &
            NOT_FILE_A &
            enemy;

        while (push1) {

            int to =
                __builtin_ctzll(push1);

            out[count++] = {
                (uint8_t)(to - 8),
                (uint8_t)to,
                PAWN,
                0
            };

            push1 &= push1 - 1;
        }

        while (push2) {

            int to =
                __builtin_ctzll(push2);

            out[count++] = {
                (uint8_t)(to - 16),
                (uint8_t)to,
                PAWN,
                0
            };

            push2 &= push2 - 1;
        }

        while (capL) {

            int to =
                __builtin_ctzll(capL);

            out[count++] = {
                (uint8_t)(to - 7),
                (uint8_t)to,
                PAWN,
                0
            };

            capL &= capL - 1;
        }

        while (capR) {

            int to =
                __builtin_ctzll(capR);

            out[count++] = {
                (uint8_t)(to - 9),
                (uint8_t)to,
                PAWN,
                0
            };

            capR &= capR - 1;
        }
    }

    else {

        uint64_t push1 =
            (pawns >> 8) &
            ~s.total_occ;

        uint64_t push2 =
            ((push1 & RANK_6) >> 8) &
            ~s.total_occ;

        uint64_t capL =
            (pawns >> 9) &
            NOT_FILE_H &
            enemy;

        uint64_t capR =
            (pawns >> 7) &
            NOT_FILE_A &
            enemy;

        while (push1) {

            int to =
                __builtin_ctzll(push1);

            out[count++] = {
                (uint8_t)(to + 8),
                (uint8_t)to,
                PAWN,
                0
            };

            push1 &= push1 - 1;
        }

        while (push2) {

            int to =
                __builtin_ctzll(push2);

            out[count++] = {
                (uint8_t)(to + 16),
                (uint8_t)to,
                PAWN,
                0
            };

            push2 &= push2 - 1;
        }

        while (capL) {

            int to =
                __builtin_ctzll(capL);

            out[count++] = {
                (uint8_t)(to + 9),
                (uint8_t)to,
                PAWN,
                0
            };

            capL &= capL - 1;
        }

        while (capR) {

            int to =
                __builtin_ctzll(capR);

            out[count++] = {
                (uint8_t)(to + 7),
                (uint8_t)to,
                PAWN,
                0
            };

            capR &= capR - 1;
        }
    }

    return count;
}

// ============================================================
// VISIBILITY MASK
// ============================================================

uint64_t Simulator::get_visibility_mask(
    const GameState& s,
    int color
) const {

    uint64_t visibility =
        (color == WHITE)
        ? s.white_occ
        : s.black_occ;

    for (int piece = PAWN;
         piece <= KING;
         ++piece) {

        uint64_t bb =
            s.pieces[color][piece];

        while (bb) {

            int sq =
                __builtin_ctzll(bb);

            switch (piece) {

                case PAWN:

                    if (color == WHITE) {

                        visibility |=
                            ((1ULL << sq) << 7 &
                             NOT_FILE_H);

                        visibility |=
                            ((1ULL << sq) << 9 &
                             NOT_FILE_A);
                    }
                    else {

                        visibility |=
                            ((1ULL << sq) >> 9 &
                             NOT_FILE_H);

                        visibility |=
                            ((1ULL << sq) >> 7 &
                             NOT_FILE_A);
                    }

                    break;

                case KNIGHT:
                    visibility |=
                        knight_attacks[sq];
                    break;

                case BISHOP:
                    visibility |=
                        get_bishop_vision(
                            sq,
                            s.total_occ
                        );
                    break;

                case ROOK:
                    visibility |=
                        get_rook_vision(
                            sq,
                            s.total_occ
                        );
                    break;

                case QUEEN:
                    visibility |=
                        get_queen_vision(
                            sq,
                            s.total_occ
                        );
                    break;

                case KING:
                    visibility |=
                        king_attacks[sq];
                    break;
            }

            bb &= bb - 1;
        }
    }

    return visibility;
}

// ============================================================
// DEBUG
// ============================================================

void Simulator::print_bitboard(
    uint64_t bb
) const {

    std::cout << "\n";

    for (int r = 7; r >= 0; --r) {

        std::cout << r + 1 << "  ";

        for (int f = 0; f < 8; ++f) {

            int sq = r * 8 + f;

            std::cout
                << ((bb >> sq) & 1ULL)
                << " ";
        }

        std::cout << "\n";
    }

    std::cout
        << "   a b c d e f g h\n\n";
}

} // namespace pomdp64