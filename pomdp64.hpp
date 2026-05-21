/**
 * @file pomdp64.hpp
 * @brief Phase 2 Updated Phase 2 Definition - Sliding Ray Integration
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

constexpr int WHITE = 0;
constexpr int BLACK = 1;

constexpr int PAWN   = 0;
constexpr int KNIGHT = 1;
constexpr int BISHOP = 2;
constexpr int ROOK   = 3;
constexpr int QUEEN  = 4;
constexpr int KING   = 5;

struct alignas(64) GameState {
    uint64_t pieces[2][6]; 
    alignas(8) uint64_t white_occ;    
    alignas(8) uint64_t black_occ;    
    alignas(8) uint64_t total_occ;    
    alignas(8) uint64_t metadata;     
};

class Simulator {
private:
    // 64-element precomputed direction lookup maps
    uint64_t ray_up[64];
    uint64_t ray_down[64];
    uint64_t ray_right[64];
    uint64_t ray_left[64];

    uint64_t ray_up_right[64];
    uint64_t ray_up_left[64];
    uint64_t ray_down_right[64];
    uint64_t ray_down_left[64];

    /**
     * @brief One-time generation execution loop mapping directional bits.
     */
    void init_ray_masks();

public:
    Simulator();
    ~Simulator() = default;

    void reset_board(GameState& state);
    inline void squash_occupancy(GameState& state);
    void print_bitboard(uint64_t bitboard) const;

    /**
     * @brief Resolves orthogonal vision masks looplessly using hardware bit manipulation opcodes.
     */
    uint64_t get_rook_vision(int square, uint64_t total_occ) const;

    /**
     * @brief Resolves diagonal vision masks looplessly using hardware bit manipulation opcodes.
     */
    uint64_t get_bishop_vision(int square, uint64_t total_occ) const;

    /**
     * @brief Combines orthogonal and diagonal paths to resolve compound queen vision profiles.
     */
    inline uint64_t get_queen_vision(int square, uint64_t total_occ) const {
        return get_rook_vision(square, total_occ) | get_bishop_vision(square, total_occ);
    }

    static inline uint64_t get_square_mask(int file, int rank) {
        return 1ULL << ((rank * 8) + file);
    }
};

} // namespace pomdp64

#endif // POMDP64_HPP