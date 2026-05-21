/**
 * @file pomdp64.hpp
 * @brief POMDP-64 Master Interface - The Markovian Void Protocol
 * @version 1.4.0
 */

#ifndef POMDP64_HPP
#define POMDP64_HPP

#include <cstdint>

namespace pomdp64 {

// Color Dimensions
constexpr int WHITE = 0;
constexpr int BLACK = 1;

// Granular Register Allocations
constexpr int PAWN   = 0;
constexpr int KNIGHT = 1;
constexpr int BISHOP = 2;
constexpr int ROOK   = 3;
constexpr int QUEEN  = 4;
constexpr int KING   = 5;

/**
 * @struct GameState
 * @brief 128-Byte total footprint explicitly aligned to L1/L2 cache boundary constraints.
 */
struct alignas(64) GameState {
    // Cache Line 1 (Offset Bytes 0 - 63)
    uint64_t pieces[2][6]; 

    // Cache Line 2 (Offset Bytes 64 - 127)
    alignas(8) uint64_t white_occ;    
    alignas(8) uint64_t black_occ;    
    alignas(8) uint64_t total_occ;    
    alignas(8) uint64_t metadata;     // Bit 0: Active Side (0=White, 1=Black)
};

/**
 * @struct Move
 * @brief 32-bit highly packed state change vector.
 */
struct Move {
    uint8_t from;
    uint8_t to;
    uint8_t piece_type;
    uint8_t flag; 
};

class Simulator {
private:
    // Precomputed Sliding Paths (64 Squares Each)
    uint64_t ray_up[64];
    uint64_t ray_down[64];
    uint64_t ray_right[64];
    uint64_t ray_left[64];
    uint64_t ray_up_right[64];
    uint64_t ray_up_left[64];
    uint64_t ray_down_right[64];
    uint64_t ray_down_left[64];

    // Precomputed Discrete Step Paths
    uint64_t knight_attacks[64];

    void init_ray_masks();
    void init_step_masks();

public:
    Simulator();
    ~Simulator() = default;

    // Static Edge and Rank Guard Filters
    static constexpr uint64_t FILE_A = 0x0101010101010101ULL;
    static constexpr uint64_t FILE_H = 0x8080808080808080ULL;
    static constexpr uint64_t NOT_FILE_A = ~FILE_A;
    static constexpr uint64_t NOT_FILE_H = ~FILE_H;

    static constexpr uint64_t RANK_2 = 0x000000000000FF00ULL;
    static constexpr uint64_t RANK_3 = 0x0000000000FF0000ULL;
    static constexpr uint64_t RANK_6 = 0x0000FF0000000000ULL;
    static constexpr uint64_t RANK_7 = 0x00FF000000000000ULL;

    void reset_board(GameState& state);
    inline void squash_occupancy(GameState& state);
    void make_move(GameState& state, int active_color, int piece_type, int source_square, int target_square);
    
    uint64_t get_rook_vision(int square, uint64_t total_occ) const;
    uint64_t get_bishop_vision(int square, uint64_t total_occ) const;
    
    inline uint64_t get_queen_vision(int square, uint64_t total_occ) const {
        return get_rook_vision(square, total_occ) | get_bishop_vision(square, total_occ);
    }

    inline uint64_t get_knight_attacks(int square) const {
        return knight_attacks[square];
    }

    int generate_pseudo_moves(const GameState& state, int active_color, Move* move_list);
    void print_bitboard(uint64_t bitboard) const;
};

} // namespace pomdp64

#endif // POMDP64_HPP