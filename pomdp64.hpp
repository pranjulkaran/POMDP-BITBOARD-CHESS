#ifndef POMDP64_HPP
#define POMDP64_HPP

#include <cstdint>

namespace pomdp64 {

// ============================================================
// COLORS
// ============================================================

constexpr int WHITE = 0;
constexpr int BLACK = 1;

// ============================================================
// PIECE TYPES
// ============================================================

constexpr int PAWN   = 0;
constexpr int KNIGHT = 1;
constexpr int BISHOP = 2;
constexpr int ROOK   = 3;
constexpr int QUEEN  = 4;
constexpr int KING   = 5;

// ============================================================
// MOVE FLAGS
// ============================================================

constexpr uint8_t MOVE_QUIET            = 0;
constexpr uint8_t MOVE_CAPTURE          = 1;
constexpr uint8_t MOVE_DOUBLE_PAWN_PUSH = 2;
constexpr uint8_t MOVE_KING_CASTLE      = 3;
constexpr uint8_t MOVE_QUEEN_CASTLE     = 4;
constexpr uint8_t MOVE_EN_PASSANT       = 5;
constexpr uint8_t MOVE_PROMOTE_KNIGHT   = 8;
constexpr uint8_t MOVE_PROMOTE_BISHOP   = 9;
constexpr uint8_t MOVE_PROMOTE_ROOK     = 10;
constexpr uint8_t MOVE_PROMOTE_QUEEN    = 11;

// ============================================================
// LIMITS
// ============================================================

constexpr int MAX_MOVES = 256;

// ============================================================
// GAME STATE
// EXACTLY 128 BYTES
// ============================================================

struct alignas(64) GameState {

uint64_t pieces[2][6];

uint64_t white_occ;
uint64_t black_occ;
uint64_t total_occ;

uint64_t metadata;

};

static_assert(sizeof(GameState) == 128);

// ============================================================
// MOVE
// ============================================================

struct Move {

uint8_t from;
uint8_t to;
uint8_t piece;
uint8_t flag;

};

// ============================================================
// SIMULATOR
// ============================================================

class Simulator {

private:

// ========================================================
// RAYS
// ========================================================

uint64_t ray_up[64];
uint64_t ray_down[64];
uint64_t ray_left[64];
uint64_t ray_right[64];

uint64_t ray_ur[64];
uint64_t ray_ul[64];
uint64_t ray_dr[64];
uint64_t ray_dl[64];

// ========================================================
// ATTACK TABLES
// ========================================================

uint64_t knight_attacks[64];
uint64_t king_attacks[64];

uint64_t pawn_attacks[2][64];

// ========================================================
// INTERNAL INITIALIZATION
// ========================================================

void init_ray_masks();
void init_attack_masks();

// ========================================================
// INTERNAL MOVE EXECUTION
// ========================================================

void apply_move(
    GameState& s,
    const Move& move
);
public:

// ========================================================
// FILE MASKS
// ========================================================

static constexpr uint64_t FILE_A =
    0x0101010101010101ULL;

static constexpr uint64_t FILE_H =
    0x8080808080808080ULL;

static constexpr uint64_t NOT_FILE_A =
    ~FILE_A;

static constexpr uint64_t NOT_FILE_H =
    ~FILE_H;

// ========================================================
// RANK MASKS
// ========================================================

static constexpr uint64_t RANK_2 =
    0x000000000000FF00ULL;

static constexpr uint64_t RANK_3 =
    0x0000000000FF0000ULL;

static constexpr uint64_t RANK_6 =
    0x0000FF0000000000ULL;

static constexpr uint64_t RANK_7 =
    0x00FF000000000000ULL;

// ========================================================
// CONSTRUCTION
// ========================================================

Simulator();
~Simulator() = default;

// ========================================================
// BOARD MANAGEMENT
// ========================================================

void reset_board(GameState& s);

void squash_occupancy(
    GameState& s
) const;

// ========================================================
// MOVE EXECUTION
// ========================================================

void make_move(
    GameState& s,
    int color,
    int piece,
    int from,
    int to
);

bool attempt_move(
    GameState& s,
    const Move& move
);

// ========================================================
// SLIDING ATTACKS
// ========================================================

uint64_t get_rook_vision(
    int sq,
    uint64_t occ
) const;

uint64_t get_bishop_vision(
    int sq,
    uint64_t occ
) const;

inline uint64_t get_queen_vision(
    int sq,
    uint64_t occ
) const {

    return
        get_rook_vision(sq, occ) |
        get_bishop_vision(sq, occ);
}

// ========================================================
// STEP ATTACKS
// ========================================================

inline uint64_t get_knight_attacks(
    int sq
) const {

    return knight_attacks[sq];
}

inline uint64_t get_king_attacks(
    int sq
) const {

    return king_attacks[sq];
}

// ========================================================
// CHECK DETECTION
// ========================================================

bool is_square_attacked(
    const GameState& s,
    int sq,
    int attacker
) const;

bool is_in_check(
    const GameState& s,
    int color
) const;

// ========================================================
// PIN DETECTION
// ========================================================

uint64_t get_pinned_pieces(
    const GameState& s,
    int color
) const;

uint64_t legal_mask_from_pins(
    const GameState& s,
    int color,
    int from,
    uint64_t pinned
) const;

// ========================================================
// MOVE GENERATION
// ========================================================

int generate_pseudo_moves(
    const GameState& s,
    int color,
    Move* out
);

// ========================================================
// VISIBILITY
// ========================================================

uint64_t get_visibility_mask(
    const GameState& s,
    int color
) const;

// ========================================================
// DEBUG
// ========================================================

void print_bitboard(
    uint64_t bb
) const;

};

} // namespace pomdp64

#endif
