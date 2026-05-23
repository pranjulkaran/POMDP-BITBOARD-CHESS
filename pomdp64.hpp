/**
 * @file pomdp64.hpp
 * @brief POMDP-64 Master Interface
 * @version 2.0.0
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
// GAME STATE
// EXACTLY 128 BYTES
// ============================================================

struct alignas(64) GameState {

    // 96 bytes
    uint64_t pieces[2][6];

    // 32 bytes
    uint64_t white_occ;
    uint64_t black_occ;
    uint64_t total_occ;
    uint64_t metadata;
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
    // SLIDING RAYS
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
    // INITIALIZATION
    // ========================================================

    void init_ray_masks();
    void init_attack_masks();

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

        return
            get_rook_vision(square, occupancy) |
            get_bishop_vision(square, occupancy);
    }

    // ========================================================
    // STEP ATTACKS
    // ========================================================

    inline uint64_t get_knight_attacks(
        int square
    ) const {

        return knight_attacks[square];
    }

    inline uint64_t get_king_attacks(
        int square
    ) const {

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

    void print_bitboard(
        uint64_t bitboard
    ) const;
};

} // namespace pomdp64

#endif