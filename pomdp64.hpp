/**
 * @file pomdp64.hpp
 * @brief POMDP-64 Master Interface
 * @version 2.1.0 (Rule Layer Extension)
 */

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
// MOVE FLAGS (EXTENSION LAYER)
// ============================================================

namespace MoveFlag {
    constexpr uint8_t QUIET       = 0;
    constexpr uint8_t CAPTURE     = 1;
    constexpr uint8_t DOUBLE_PUSH = 2;
    constexpr uint8_t EN_PASSANT  = 3;
    constexpr uint8_t CASTLE      = 4;
    constexpr uint8_t PROMOTION   = 5;
}

// ============================================================
// RULE METADATA (BIT PACKING)
// ============================================================

namespace Rules {
    constexpr uint64_t MASK_CASTLE_WK = 1ULL << 0;
    constexpr uint64_t MASK_CASTLE_WQ = 1ULL << 1;
    constexpr uint64_t MASK_CASTLE_BK = 1ULL << 2;
    constexpr uint64_t MASK_CASTLE_BQ = 1ULL << 3;

    constexpr uint64_t MASK_EP_FILE   = 0x7ULL << 4; // 3-bit file encoding
    constexpr uint64_t MASK_COLOR     = 1ULL << 7;
}

// ============================================================
// GAME STATE (128 BYTES EXACT)
// ============================================================

struct alignas(64) GameState {

    uint64_t pieces[2][6];   // 96 bytes

    uint64_t white_occ;
    uint64_t black_occ;
    uint64_t total_occ;

    uint64_t metadata;       // rule + turn + ep + castling
};

static_assert(sizeof(GameState) == 128,
              "GameState must remain exactly 128 bytes");

// ============================================================
// MOVE PACKET
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
    // RAY TABLES
    // ========================================================

    uint64_t ray_up[64];
    uint64_t ray_down[64];
    uint64_t ray_left[64];
    uint64_t ray_right[64];

    uint64_t ray_up_right[64];
    uint64_t ray_up_left[64];
    uint64_t ray_down_right[64];
    uint64_t ray_down_left[64];

    // ========================================================
    // STEP ATTACK TABLES
    // ========================================================

    uint64_t knight_attacks[64];
    uint64_t king_attacks[64];

    // ========================================================
    // INTERNAL INIT
    // ========================================================

    void init_ray_masks();
    void init_attack_masks();

public:

    // ========================================================
    // FILE MASKS
    // ========================================================

    static constexpr uint64_t FILE_A = 0x0101010101010101ULL;
    static constexpr uint64_t FILE_H = 0x8080808080808080ULL;

    static constexpr uint64_t NOT_FILE_A = ~FILE_A;
    static constexpr uint64_t NOT_FILE_H = ~FILE_H;

    // ========================================================
    // RANK MASKS
    // ========================================================

    static constexpr uint64_t RANK_2 = 0x000000000000FF00ULL;
    static constexpr uint64_t RANK_3 = 0x0000000000FF0000ULL;
    static constexpr uint64_t RANK_6 = 0x0000FF0000000000ULL;
    static constexpr uint64_t RANK_7 = 0x00FF000000000000ULL;

    // ========================================================
    // LIFECYCLE
    // ========================================================

    Simulator();
    ~Simulator() = default;

    // ========================================================
    // STATE CORE
    // ========================================================

    void reset_board(GameState& state);

    inline void squash_occupancy(GameState& state) const;

    void make_move(
        GameState& state,
        int active_color,
        int piece_type,
        int source_square,
        int target_square
    );

    // ========================================================
    // TRANSACTIONAL MOVE ENGINE (NEW)
    // ========================================================

    bool attempt_move(
        GameState& state,
        int from,
        int to,
        int promotion_type = 0
    );

    void apply_move(
        GameState& state,
        int from,
        int to,
        int promotion_type
    );

    // ========================================================
    // LEGALITY VALIDATION (NEW)
    // ========================================================

    bool is_square_attacked(
        const GameState& state,
        int square,
        int attacker_color
    ) const;

    // ========================================================
    // SLIDING ATTACKS
    // ========================================================

    uint64_t get_rook_vision(
        int square,
        uint64_t occupancy
    ) const;

    uint64_t get_bishop_vision(
        int square,
        uint64_t occupancy
    ) const;

    inline uint64_t get_queen_vision(
        int square,
        uint64_t occupancy
    ) const {
        return get_rook_vision(square, occupancy)
             | get_bishop_vision(square, occupancy);
    }

    // ========================================================
    // STEP ATTACKS
    // ========================================================

    inline uint64_t get_knight_attacks(int square) const {
        return knight_attacks[square];
    }

    inline uint64_t get_king_attacks(int square) const {
        return king_attacks[square];
    }

    // ========================================================
    // MOVE GENERATION
    // ========================================================

    int generate_pseudo_moves(
        const GameState& state,
        int active_color,
        Move* move_buffer
    );

    // ========================================================
    // POMDP VISIBILITY
    // ========================================================

    uint64_t get_visibility_mask(
        const GameState& state,
        int color
    ) const;

    // ========================================================
    // DEBUG
    // ========================================================

    void print_bitboard(uint64_t bitboard) const;
};

} // namespace pomdp64

#endif