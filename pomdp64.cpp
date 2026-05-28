/**

* @file pomdp64.cpp
* @brief POMDP-64 Core Engine
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

    ray_up[s]    = 0ULL;
    ray_down[s]  = 0ULL;
    ray_left[s]  = 0ULL;
    ray_right[s] = 0ULL;

    ray_ur[s] = 0ULL;
    ray_ul[s] = 0ULL;
    ray_dr[s] = 0ULL;
    ray_dl[s] = 0ULL;

    // UP
    for (int rr = r + 1; rr < 8; ++rr)
        ray_up[s] |= 1ULL << (rr * 8 + f);

    // DOWN
    for (int rr = r - 1; rr >= 0; --rr)
        ray_down[s] |= 1ULL << (rr * 8 + f);

    // RIGHT
    for (int ff = f + 1; ff < 8; ++ff)
        ray_right[s] |= 1ULL << (r * 8 + ff);

    // LEFT
    for (int ff = f - 1; ff >= 0; --ff)
        ray_left[s] |= 1ULL << (r * 8 + ff);

    // UP RIGHT
    for (int rr = r + 1, ff = f + 1;
         rr < 8 && ff < 8;
         ++rr, ++ff)
        ray_ur[s] |= 1ULL << (rr * 8 + ff);

    // UP LEFT
    for (int rr = r + 1, ff = f - 1;
         rr < 8 && ff >= 0;
         ++rr, --ff)
        ray_ul[s] |= 1ULL << (rr * 8 + ff);

    // DOWN RIGHT
    for (int rr = r - 1, ff = f + 1;
         rr >= 0 && ff < 8;
         --rr, ++ff)
        ray_dr[s] |= 1ULL << (rr * 8 + ff);

    // DOWN LEFT
    for (int rr = r - 1, ff = f - 1;
         rr >= 0 && ff >= 0;
         --rr, --ff)
        ray_dl[s] |= 1ULL << (rr * 8 + ff);
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
    king_attacks[s]   = 0ULL;

    pawn_attacks[WHITE][s] = 0ULL;
    pawn_attacks[BLACK][s] = 0ULL;

    int f = s & 7;
    int r = s >> 3;

    // KNIGHT
    for (const auto& d : knight_delta) {

        int nf = f + d[0];
        int nr = r + d[1];

        if (nf >= 0 && nf < 8 &&
            nr >= 0 && nr < 8) {

            knight_attacks[s] |=
                1ULL << (nr * 8 + nf);
        }
    }

    // KING
    for (const auto& d : king_delta) {

        int nf = f + d[0];
        int nr = r + d[1];

        if (nf >= 0 && nf < 8 &&
            nr >= 0 && nr < 8) {

            king_attacks[s] |=
                1ULL << (nr * 8 + nf);
        }
    }

    // WHITE PAWN
    if (r < 7) {

        if (f > 0)
            pawn_attacks[WHITE][s] |=
                1ULL << ((r + 1) * 8 + (f - 1));

        if (f < 7)
            pawn_attacks[WHITE][s] |=
                1ULL << ((r + 1) * 8 + (f + 1));
    }

    // BLACK PAWN
    if (r > 0) {

        if (f > 0)
            pawn_attacks[BLACK][s] |=
                1ULL << ((r - 1) * 8 + (f - 1));

        if (f < 7)
            pawn_attacks[BLACK][s] |=
                1ULL << ((r - 1) * 8 + (f + 1));
    }
}


}

// ============================================================
// RESET BOARD
// ============================================================

void Simulator::reset_board(GameState& s) {

for (int c = 0; c < 2; ++c)
    for (int p = 0; p < 6; ++p)
        s.pieces[c][p] = 0ULL;

// WHITE
s.pieces[WHITE][PAWN]   = 0x000000000000FF00ULL;
s.pieces[WHITE][KNIGHT] = 0x0000000000000042ULL;
s.pieces[WHITE][BISHOP] = 0x0000000000000024ULL;
s.pieces[WHITE][ROOK]   = 0x0000000000000081ULL;
s.pieces[WHITE][QUEEN]  = 0x0000000000000008ULL;
s.pieces[WHITE][KING]   = 0x0000000000000010ULL;

// BLACK
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

s.total_occ =
    s.white_occ |
    s.black_occ;

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
// APPLY MOVE
// ============================================================

void Simulator::apply_move(
GameState& s,
int from,
int to,
int piece,
int promotion
) {

int color =
    (s.metadata & 1ULL)
    ? BLACK
    : WHITE;

int enemy = color ^ 1;

s.pieces[color][piece] &=
    ~(1ULL << from);

for (int p = 0; p < 6; ++p)
    s.pieces[enemy][p] &=
        ~(1ULL << to);

// PROMOTION
if (piece == PAWN) {

    if ((color == WHITE && to >= 56) ||
        (color == BLACK && to <= 7)) {

        s.pieces[color][promotion] |=
            (1ULL << to);
    }
    else {
        s.pieces[color][PAWN] |=
            (1ULL << to);
    }
}
else {

    s.pieces[color][piece] |=
        (1ULL << to);
}

squash_occupancy(s);

}

// ============================================================
// ATTEMPT MOVE
// ============================================================

bool Simulator::attempt_move(
GameState& s,
int from,
int to,
int piece,
int promotion
) {

GameState backup = s;

int moving_color =
    (s.metadata & 1ULL)
    ? BLACK
    : WHITE;

apply_move(
    s,
    from,
    to,
    piece,
    promotion
);

if (is_in_check(s, moving_color)) {

    s = backup;
    return false;
}

s.metadata ^= 1ULL;

return true;

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
// ROOK VISION
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
// BISHOP VISION
// ============================================================

uint64_t Simulator::get_bishop_vision(
int sq,
uint64_t occ
) const {

return
    clip_positive(
        ray_ur[sq],
        occ & ray_ur[sq],
        ray_ur
    )

    |

    clip_positive(
        ray_ul[sq],
        occ & ray_ul[sq],
        ray_ul
    )

    |

    clip_negative(
        ray_dr[sq],
        occ & ray_dr[sq],
        ray_dr
    )

    |

    clip_negative(
        ray_dl[sq],
        occ & ray_dl[sq],
        ray_dl
    );

}

// ============================================================
// ATTACK DETECTION
// ============================================================

bool Simulator::is_square_attacked(
const GameState& s,
int sq,
int attacker
) const {

// PAWNS
if (s.pieces[attacker][PAWN] &
    pawn_attacks[attacker ^ 1][sq])
    return true;

// KNIGHTS
if (s.pieces[attacker][KNIGHT] &
    knight_attacks[sq])
    return true;

// KING
if (s.pieces[attacker][KING] &
    king_attacks[sq])
    return true;

// BISHOP / QUEEN
if (get_bishop_vision(sq, s.total_occ) &
    (s.pieces[attacker][BISHOP] |
     s.pieces[attacker][QUEEN]))
    return true;

// ROOK / QUEEN
if (get_rook_vision(sq, s.total_occ) &
    (s.pieces[attacker][ROOK] |
     s.pieces[attacker][QUEEN]))
    return true;

return false;

}

// ============================================================
// CHECK DETECTION
// ============================================================

bool Simulator::is_in_check(
const GameState& s,
int color
) const {

uint64_t king =
    s.pieces[color][KING];

if (!king)
    return false;

int king_sq =
    __builtin_ctzll(king);

return is_square_attacked(
    s,
    king_sq,
    color ^ 1
);

}

// ============================================================
// PIN DETECTION
// ============================================================

uint64_t Simulator::get_pinned_pieces(
const GameState& s,
int color
) const {

uint64_t pinned = 0ULL;

uint64_t king_bb =
    s.pieces[color][KING];

if (!king_bb)
    return 0ULL;

int king_sq =
    __builtin_ctzll(king_bb);

uint64_t friendly =
    (color == WHITE)
    ? s.white_occ
    : s.black_occ;

uint64_t enemy_bq =
    s.pieces[color ^ 1][BISHOP] |
    s.pieces[color ^ 1][QUEEN];

uint64_t enemy_rq =
    s.pieces[color ^ 1][ROOK] |
    s.pieces[color ^ 1][QUEEN];

const uint64_t directions[8] = {
    ray_up[king_sq],
    ray_down[king_sq],
    ray_left[king_sq],
    ray_right[king_sq],
    ray_ur[king_sq],
    ray_ul[king_sq],
    ray_dr[king_sq],
    ray_dl[king_sq]
};

for (int i = 0; i < 8; ++i) {

    uint64_t ray = directions[i];

    uint64_t blockers =
        ray & s.total_occ;

    int friendly_sq = -1;
    int enemy_sq = -1;

    while (blockers) {

        int sq;

        if (i == 0 || i == 3 || i == 4 || i == 5)
            sq = __builtin_ctzll(blockers);
        else
            sq = 63 - __builtin_clzll(blockers);

        uint64_t bit =
            1ULL << sq;

        if (bit & friendly) {

            if (friendly_sq == -1)
                friendly_sq = sq;
            else
                break;
        }
        else {

            enemy_sq = sq;
            break;
        }

        blockers &= ~(1ULL << sq);
    }

    if (friendly_sq == -1 ||
        enemy_sq == -1)
        continue;

    uint64_t enemy_bit =
        1ULL << enemy_sq;

    bool diagonal =
        i >= 4;

    if (diagonal) {

        if (enemy_bit & enemy_bq)
            pinned |= (1ULL << friendly_sq);
    }
    else {

        if (enemy_bit & enemy_rq)
            pinned |= (1ULL << friendly_sq);
    }
}

return pinned;

}

// ============================================================
// LEGAL MASK FROM PINS
// ============================================================

uint64_t Simulator::legal_mask_from_pins(
const GameState& s,
int color,
int from,
uint64_t pinned
) const {

uint64_t from_bb =
    1ULL << from;

if (!(pinned & from_bb))
    return ~0ULL;

uint64_t king_bb =
    s.pieces[color][KING];

int king_sq =
    __builtin_ctzll(king_bb);

uint64_t mask =
    from_bb;

if (ray_up[king_sq] & from_bb)
    mask |= ray_up[king_sq];

else if (ray_down[king_sq] & from_bb)
    mask |= ray_down[king_sq];

else if (ray_left[king_sq] & from_bb)
    mask |= ray_left[king_sq];

else if (ray_right[king_sq] & from_bb)
    mask |= ray_right[king_sq];

else if (ray_ur[king_sq] & from_bb)
    mask |= ray_ur[king_sq];

else if (ray_ul[king_sq] & from_bb)
    mask |= ray_ul[king_sq];

else if (ray_dr[king_sq] & from_bb)
    mask |= ray_dr[king_sq];

else if (ray_dl[king_sq] & from_bb)
    mask |= ray_dl[king_sq];

return mask;

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

uint64_t pinned =
    get_pinned_pieces(s, color);

// ========================================================
// PIECES
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

        attacks &= ~friendly;

        // PIN FILTER
        if (pinned & (1ULL << from))
            attacks &=
                legal_mask_from_pins(
                    s,
                    color,
                    from,
                    pinned
                );

        while (attacks) {

            if (count >= MAX_MOVES)
                return count;

            int to =
                __builtin_ctzll(attacks);

            uint8_t flag =
                (enemy & (1ULL << to))
                ? MOVE_CAPTURE
                : MOVE_QUIET;

            out[count++] = {
                (uint8_t)from,
                (uint8_t)to,
                (uint8_t)piece,
                flag
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

while (pawns) {

int from =
    __builtin_ctzll(pawns);

uint64_t moves = 0ULL;

if (color == WHITE) {

    int push = from + 8;

    if (push < 64 &&
        !(s.total_occ & (1ULL << push)))
        moves |= (1ULL << push);

    if ((1ULL << from) & RANK_2) {

        int dbl = from + 16;

        if (!(s.total_occ & (1ULL << push)) &&
            !(s.total_occ & (1ULL << dbl)))
            moves |= (1ULL << dbl);
    }

    moves |=
        pawn_attacks[WHITE][from] &
        enemy;
}
else {

    int push = from - 8;

    if (push >= 0 &&
        !(s.total_occ & (1ULL << push)))
        moves |= (1ULL << push);

    if ((1ULL << from) & RANK_7) {

        int dbl = from - 16;

        if (!(s.total_occ & (1ULL << push)) &&
            !(s.total_occ & (1ULL << dbl)))
            moves |= (1ULL << dbl);
    }

    moves |=
        pawn_attacks[BLACK][from] &
        enemy;
}

// PIN FILTER
if (pinned & (1ULL << from))
    moves &=
        legal_mask_from_pins(
            s,
            color,
            from,
            pinned
        );

while (moves) {

    if (count >= MAX_MOVES)
        return count;

    int to =
        __builtin_ctzll(moves);

    uint8_t flag =
        MOVE_QUIET;

    if (enemy & (1ULL << to))
        flag =
            MOVE_CAPTURE;

    if ((color == WHITE && to >= 56) ||
        (color == BLACK && to <= 7))
        flag =
            MOVE_PROMOTE_QUEEN;

    else if (color == WHITE &&
             to - from == 16)
        flag =
            MOVE_DOUBLE_PAWN_PUSH;

    else if (color == BLACK &&
             from - to == 16)
        flag =
            MOVE_DOUBLE_PAWN_PUSH;

    out[count++] = {
        (uint8_t)from,
        (uint8_t)to,
        PAWN,
        flag
    };

    moves &= moves - 1;
}

pawns &= pawns - 1;

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
                visibility |=
                    pawn_attacks[color][sq];
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

    std::cout
        << r + 1
        << "  ";

    for (int f = 0; f < 8; ++f) {

        int sq =
            r * 8 + f;

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
