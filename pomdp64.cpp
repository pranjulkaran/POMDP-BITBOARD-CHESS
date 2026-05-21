/**
 * @file pomdp64.cpp
 * @brief Bitwise pipeline execution routines for POMDP-64 core state space.
 */

#include "pomdp64.hpp"
#include <iostream>

namespace pomdp64 {

Simulator::Simulator() {
    // Core engine initialization point
}

void Simulator::reset_board(GameState& state) {
    // 1. Zero out all fine-grained piece registers across both colors
    for (int color = 0; color < 2; ++color) {
        for (int type = 0; type < 6; ++type) {
            state.pieces[color][type] = 0ULL;
        }
    }

    // 2. Set hardcoded LERF base alignments (Hexadecimal Verification Bounds)
    state.pieces[WHITE][PAWN]   = 0x000000000000FF00ULL; // Rank 2 (Bits 8-15)
    state.pieces[WHITE][ROOK]   = 0x0000000000000081ULL; // Squares a1, h1
    state.pieces[WHITE][KNIGHT] = 0x0000000000000042ULL; // Squares b1, g1
    state.pieces[WHITE][BISHOP] = 0x0000000000000024ULL; // Squares c1, f1
    state.pieces[WHITE][QUEEN]  = 0x0000000000000008ULL; // Square d1
    state.pieces[WHITE][KING]   = 0x0000000000000010ULL; // Square e1

    state.pieces[BLACK][PAWN]   = 0x00FF000000000000ULL; // Rank 7 (Bits 48-55)
    state.pieces[BLACK][ROOK]   = 0x8100000000000000ULL; // Squares a8, h8
    state.pieces[BLACK][KNIGHT] = 0x4200000000000000ULL; // Squares b8, g8
    state.pieces[BLACK][BISHOP] = 0x2400000000000000ULL; // Squares c8, f8
    state.pieces[BLACK][QUEEN]  = 0x0800000000000000ULL; // Square d8
    state.pieces[BLACK][KING]   = 0x1000000000000000ULL; // Square e8

    // 3. Sync structural layers
    squash_occupancy(state);

    // 4. Metadata Reset: Clear clocks, set Turn to White (0)
    state.metadata = 0ULL;
}

inline void Simulator::squash_occupancy(GameState& state) {
    // Process white occupancy layer via parallel bitwise OR operations
    state.white_occ = state.pieces[WHITE][PAWN]   | state.pieces[WHITE][KNIGHT] |
                      state.pieces[WHITE][BISHOP] | state.pieces[WHITE][ROOK]   |
                      state.pieces[WHITE][QUEEN]  | state.pieces[WHITE][KING];

    // Process black occupancy layer via parallel bitwise OR operations
    state.black_occ = state.pieces[BLACK][PAWN]   | state.pieces[BLACK][KNIGHT] |
                      state.pieces[BLACK][BISHOP] | state.pieces[BLACK][ROOK]   |
                      state.pieces[BLACK][QUEEN]  | state.pieces[BLACK][KING];

    // Combine both active layers into a single physical block footprint
    state.total_occ = state.white_occ | state.black_occ;
}

void Simulator::print_bitboard(uint64_t bitboard) const {
    std::cout << "\n";
    for (int rank = 7; rank >= 0; --rank) {
        std::cout << rank + 1 << "  "; // Rank indicator display
        for (int file = 0; file < 8; ++file) {
            int sq = rank * 8 + file;
            // Shift target bit to index 0, extract it via AND gate
            std::cout << ((bitboard >> sq) & 1ULL) << " ";
        }
        std::cout << "\n";
    }
    std::cout << "   a b c d e f g h\n\n";
}

} // namespace pomdp64