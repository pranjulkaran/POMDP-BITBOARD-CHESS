/**
 * @file pomdp64.hpp
 * @brief Phase 4 Interface - Precomputed Step Offsets & Move Generation
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

/**
 * @struct Move
 * @brief Unmanaged 32-bit compact move payload representation.
 */
struct Move {
    uint8_t from;
    uint8_t to;
    uint8_t piece_type;
    uint8_t flag; // Reserved for special actions (promotions, etc.)
};

class Simulator {
private:
    // Sliding Ray Tables
    uint64_t ray_up[64];
    uint64_t ray_down[64];
    uint64_t ray_right[64];
    uint64_t ray_left[64];
    uint64_t ray_up_right[64];
    uint64_t ray_up_left[64];
    uint64_t ray_down_right[64];
    uint64_t ray_down_left[64];

    // Non-sliding Step Tables
    uint64_t knight_attacks[64];

    void init_ray_masks();
    void init_step_masks();

public:
    Simulator();
    ~Simulator() = default;

    void reset_board(GameState& state);
    inline void squash_occupancy(GameState& state);
    void make_move(GameState& state, int active_color, int piece_type, int source_square, int target_square);
    
    uint64_t get_rook_vision(int square, uint64_t total_occ) const;
    uint64_t get_bishop_vision(int square, uint64_t total_occ) const;
    
    inline uint64_t get_queen_vision(int square, uint64_t total_occ) const {
        return get_rook_vision(square, total_occ) | get_bishop_vision(square, total_occ);
    }

    /**
     * @brief Pulls the precomputed Knight step matrix for a given square index.
     */
    inline uint64_t get_knight_attacks(int square) const {
        return knight_attacks[square];
    }

    /**
     * @brief Populates a continuous stack list of valid pseudo-legal targets for an active side.
     * @return Total count of moves stored inside the buffer pipeline.
     */
    int generate_pseudo_moves(const GameState& state, int active_color, Move* move_list);

    void print_bitboard(uint64_t bitboard) const;
};

} // namespace pomdp64

#endif // POMDP64_HPP