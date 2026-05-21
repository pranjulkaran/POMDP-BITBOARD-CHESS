/**
 * @file pomdp64.cpp
 * @brief Phase 2 Loopless Implementation Logic for Sliding Piece Vision
 */

#include "pomdp64.hpp"
#include <iostream>

namespace pomdp64 {

Simulator::Simulator() {
    // Force precomputation pass on class construction
    init_ray_masks();
}

void Simulator::init_ray_masks() {
    for (int s = 0; s < 64; ++s) {
        ray_up[s] = ray_down[s] = ray_right[s] = ray_left[s] = 0ULL;
        ray_up_right[s] = ray_up_left[s] = ray_down_right[s] = ray_down_left[s] = 0ULL;

        int init_file = s % 8;
        int init_rank = s / 8;

        // Orthogonal Vectors
        for (int r = init_rank + 1; r < 8; ++r) ray_up[s] |= (1ULL << (r * 8 + init_file));
        for (int r = init_rank - 1; r >= 0; --r) ray_down[s] |= (1ULL << (r * 8 + init_file));
        for (int f = init_file + 1; f < 8; ++f)  ray_right[s] |= (1ULL << (init_rank * 8 + f));
        for (int f = init_file - 1; f >= 0; --f)  ray_left[s] |= (1ULL << (init_rank * 8 + f));

        // Diagonal Vectors
        for (int r = init_rank + 1, f = init_file + 1; r < 8 && f < 8; ++r, ++f)   ray_up_right[s] |= (1ULL << (r * 8 + f));
        for (int r = init_rank + 1, f = init_file - 1; r < 8 && f >= 0; ++r, --f)  ray_up_left[s] |= (1ULL << (r * 8 + f));
        for (int r = init_rank - 1, f = init_file + 1; r >= 0 && f < 8; --r, ++f)  ray_down_right[s] |= (1ULL << (r * 8 + f));
        for (int r = init_rank - 1, f = init_file - 1; r >= 0 && f >= 0; --r, --f) ray_down_left[s] |= (1ULL << (r * 8 + f));
    }
}

void Simulator::reset_board(GameState& state) {
    for (int color = 0; color < 2; ++color) {
        for (int type = 0; type < 6; ++type) {
            state.pieces[color][type] = 0ULL;
        }
    }

    state.pieces[WHITE][PAWN]   = 0x000000000000FF00ULL;
    state.pieces[WHITE][ROOK]   = 0x0000000000000081ULL;
    state.pieces[WHITE][KNIGHT] = 0x0000000000000042ULL;
    state.pieces[WHITE][BISHOP] = 0x0000000000000024ULL;
    state.pieces[WHITE][QUEEN]  = 0x0000000000000008ULL;
    state.pieces[WHITE][KING]   = 0x0000000000000010ULL;

    state.pieces[BLACK][PAWN]   = 0x00FF000000000000ULL;
    state.pieces[BLACK][ROOK]   = 0x8100000000000000ULL;
    state.pieces[BLACK][KNIGHT] = 0x4200000000000000ULL;
    state.pieces[BLACK][BISHOP] = 0x2400000000000000ULL;
    state.pieces[BLACK][QUEEN]  = 0x0800000000000000ULL;
    state.pieces[BLACK][KING]   = 0x1000000000000000ULL;

    squash_occupancy(state);
    state.metadata = 0ULL;
}

inline void Simulator::squash_occupancy(GameState& state) {
    state.white_occ = state.pieces[WHITE][PAWN]   | state.pieces[WHITE][KNIGHT] |
                      state.pieces[WHITE][BISHOP] | state.pieces[WHITE][ROOK]   |
                      state.pieces[WHITE][QUEEN]  | state.pieces[WHITE][KING];

    state.black_occ = state.pieces[BLACK][PAWN]   | state.pieces[BLACK][KNIGHT] |
                      state.pieces[BLACK][BISHOP] | state.pieces[BLACK][ROOK]   |
                      state.pieces[BLACK][QUEEN]  | state.pieces[BLACK][KING];

    state.total_occ = state.white_occ | state.black_occ;
}

uint64_t Simulator::get_rook_vision(int square, uint64_t occ) const {
    uint64_t vision = 0ULL;
    uint64_t blockers = 0ULL;

    // Positive Vector: Up
    blockers = occ & ray_up[square];
    if (blockers) {
        int blocker_sq = __builtin_ctzll(blockers); // Hardware bitscan forward
        vision |= (ray_up[square] ^ ray_up[blocker_sq]);
    } else {
        vision |= ray_up[square];
    }

    // Positive Vector: Right
    blockers = occ & ray_right[square];
    if (blockers) {
        int blocker_sq = __builtin_ctzll(blockers);
        vision |= (ray_right[square] ^ ray_right[blocker_sq]);
    } else {
        vision |= ray_right[square];
    }

    // Negative Vector: Down
    blockers = occ & ray_down[square];
    if (blockers) {
        int blocker_sq = 63 - __builtin_clzll(blockers); // Hardware bitscan reverse
        vision |= (ray_down[square] ^ ray_down[blocker_sq]);
    } else {
        vision |= ray_down[square];
    }

    // Negative Vector: Left
    blockers = occ & ray_left[square];
    if (blockers) {
        int blocker_sq = 63 - __builtin_clzll(blockers);
        vision |= (ray_left[square] ^ ray_left[blocker_sq]);
    } else {
        vision |= ray_left[square];
    }

    return vision;
}

uint64_t Simulator::get_bishop_vision(int square, uint64_t occ) const {
    uint64_t vision = 0ULL;
    uint64_t blockers = 0ULL;

    // Positive Vector: Up-Right
    blockers = occ & ray_up_right[square];
    if (blockers) {
        int blocker_sq = __builtin_ctzll(blockers);
        vision |= (ray_up_right[square] ^ ray_up_right[blocker_sq]);
    } else {
        vision |= ray_up_right[square];
    }

    // Positive Vector: Up-Left
    blockers = occ & ray_up_left[square];
    if (blockers) {
        int blocker_sq = __builtin_ctzll(blockers);
        vision |= (ray_up_left[square] ^ ray_up_left[blocker_sq]);
    } else {
        vision |= ray_up_left[square];
    }

    // Negative Vector: Down-Right
    blockers = occ & ray_down_right[square];
    if (blockers) {
        int blocker_sq = 63 - __builtin_clzll(blockers);
        vision |= (ray_down_right[square] ^ ray_down_right[blocker_sq]);
    } else {
        vision |= ray_down_right[square];
    }

    // Negative Vector: Down-Left
    blockers = occ & ray_down_left[square];
    if (blockers) {
        int blocker_sq = 63 - __builtin_clzll(blockers);
        vision |= (ray_down_left[square] ^ ray_down_left[blocker_sq]);
    } else {
        vision |= ray_down_left[square];
    }

    return vision;
}

void Simulator::print_bitboard(uint64_t bitboard) const {
    std::cout << "\n";
    for (int rank = 7; rank >= 0; --rank) {
        std::cout << rank + 1 << "  ";
        for (int file = 0; file < 8; ++file) {
            int sq = rank * 8 + file;
            std::cout << ((bitboard >> sq) & 1ULL) << " ";
        }
        std::cout << "\n";
    }
    std::cout << "   a b c d e f g h\n\n";
}

} // namespace pomdp64