/**
 * @file pomdp64.hpp
 * @brief POMDP-64 Unified Structural Interface - The Markovian Void Protocol
 * @version 1.3.0
 * * SPECIFICATION COMPLIANCE:
 * - 128-Byte total GameState allocation split across two 64-byte L1 cache lines.
 * - Little-Endian Rank-File Mapping (LERF): Square a1 = Bit 0, h8 = Bit 63.
 * - Loopless, branch-free hardware accelerated sliding ray-cast resolution.
 * - Branch-free atomic state move-execution with unconditional capture gates.
 */

#ifndef POMDP64_HPP
#define POMDP64_HPP

#include <cstdint>

namespace pomdp64 {

// Color Identifiers
constexpr int WHITE = 0;
constexpr int BLACK = 1;

// Granular Piece Array Registers
constexpr int PAWN   = 0;
constexpr int KNIGHT = 1;
constexpr int BISHOP = 2;
constexpr int ROOK   = 3;
constexpr int QUEEN  = 4;
constexpr int KING   = 5;

/**
 * @struct GameState
 * @brief 128-Byte contiguous state matrix aligned perfectly for L1/L2 cache locality.
 */
struct alignas(64) GameState {
    // Cache Line 1 (Offset Bytes 0 - 63)
    uint64_t pieces[2][6]; 

    // Cache Line 2 (Offset Bytes 64 - 127)
    alignas(8) uint64_t white_occ;    ///< Flattened bitmask footprint of all White pieces
    alignas(8) uint64_t black_occ;    ///< Flattened bitmask footprint of all Black pieces
    alignas(8) uint64_t total_occ;    ///< Global collision barrier map (white_occ | black_occ)
    alignas(8) uint64_t metadata;     ///< System flags (Bit 0: Active Turn. 0=White, 1=Black)
};

/**
 * @class Simulator
 * @brief Headless logic factory executing pure bitwise transformations over state space.
 */
class Simulator {
private:
    // Precomputed directional sliding ray tables (64 squares each)
    uint64_t ray_up[64];
    uint64_t ray_down[64];
    uint64_t ray_right[64];
    uint64_t ray_left[64];

    uint64_t ray_up_right[64];
    uint64_t ray_up_left[64];
    uint64_t ray_down_right[64];
    uint64_t ray_down_left[64];

    /**
     * @brief Populates the static 64-bit lookups once on simulation compilation instantiation.
     */
    void init_ray_masks();

public:
    Simulator();
    ~Simulator() = default;

    /**
     * @brief Blasts the GameState registers back to the standard LERF initial layout.
     */
    void reset_board(GameState& state);

    /**
     * @brief Collapses fine-grained registers into compressed global blocking barriers.
     */
    inline void squash_occupancy(GameState& state);

    /**
     * @brief Mutates state space registers atomically via bitwise masking lanes.
     */
    void make_move(GameState& state, int active_color, int piece_type, int source_square, int target_square);

    /**
     * @brief Resolves orthogonal sliding lines looplessly via hardware bit-scan lookups.
     */
    uint64_t get_rook_vision(int square, uint64_t total_occ) const;

    /**
     * @brief Resolves diagonal sliding lines looplessly via hardware bit-scan lookups.
     */
    uint64_t get_bishop_vision(int square, uint64_t total_occ) const;

    /**
     * @brief Combines orthogonal and diagonal fields to yield true compound queen visibility.
     */
    inline uint64_t get_queen_vision(int square, uint64_t total_occ) const {
        return get_rook_vision(square, total_occ) | get_bishop_vision(square, total_occ);
    }

    /**
     * @brief Utility coordinator conversion projecting File/Rank pairs to a raw LERF bit position index.
     */
    static inline uint64_t get_square_mask(int file, int rank) {
        return 1ULL << ((rank * 8) + file);
    }

    /**
     * @brief Stream rendering utility to print 64-bit blocks as an 8x8 matrix display.
     */
    void print_bitboard(uint64_t bitboard) const;
};

} // namespace pomdp64

#endif // POMDP64_HPP