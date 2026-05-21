/**
 * @file pomdp64.hpp
 * @brief Phase 1 Bitboard Register Specification - The Markovian Void Protocol
 * @version 1.0.0
 * * COMPLIANCE MANDATE:
 * - Strict 128-Byte total footprint spanning exactly two 64-byte L1 cache lines.
 * - Little-Endian Rank-File Mapping (LERF): Square a1 = Bit 0, h8 = Bit 63.
 * - Flat memory execution bypassing high-level heap containers or pointer indirection.
 */

#ifndef POMDP64_HPP
#define POMDP64_HPP

#include <cstdint>

namespace pomdp64 {

// Color Array Lookups
constexpr int WHITE = 0;
constexpr int BLACK = 1;

// Fine-grained Piece Register Mapping Positions
constexpr int PAWN   = 0;
constexpr int KNIGHT = 1;
constexpr int BISHOP = 2;
constexpr int ROOK   = 3;
constexpr int QUEEN  = 4;
constexpr int KING   = 5;

/**
 * @struct GameState
 * @brief Memory-aligned core simulation matrix optimized for L1 cache locality.
 */
struct alignas(64) GameState {
    // Cache Line 1 (Offset Bytes 0 - 63)
    // Sequential layout maps: White (P, N, B, R, Q, K) followed by Black (P, N)
    uint64_t pieces[2][6]; 

    // Cache Line 2 (Offset Bytes 64 - 127)
    // Initialized via manual structural alignments to guarantee contiguous mapping
    alignas(8) uint64_t white_occ;    ///< Compressed structural bitmask of all White pieces
    alignas(8) uint64_t black_occ;    ///< Compressed structural bitmask of all Black pieces
    alignas(8) uint64_t total_occ;    ///< Global physical collision boundary mask
    alignas(8) uint64_t metadata;     ///< System runtime flags (Bit 0: Active Turn)
};

/**
 * @class Simulator
 * @brief Headless logic factory executing pure bitwise operations over the state matrix.
 */
class Simulator {
public:
    Simulator();
    ~Simulator() = default;

    /**
     * @brief Blasts the matrix back to the standard LERF initial layout boundaries.
     */
    void reset_board(GameState& state);

    /**
     * @brief Flattens granular piece bitboards into fast global occupancy barrier masks.
     */
    inline void squash_occupancy(GameState& state);

    /**
     * @brief Linear projection to isolate a specific coordinate bitmask.
     * @param file Index horizontal [0-7] (a to h)
     * @param rank Index vertical [0-7] (1 to 8)
     */
    static inline uint64_t get_square_mask(int file, int rank) {
        return 1ULL << ((rank * 8) + file);
    }

    /**
     * @brief Visualizes any raw 64-bit integer channel as a clean 8x8 matrix grid.
     */
    void print_bitboard(uint64_t bitboard) const;
};

} // namespace pomdp64

#endif // POMDP64_HPP