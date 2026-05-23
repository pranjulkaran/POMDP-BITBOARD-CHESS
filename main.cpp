/**
 * @file main.cpp
 * @brief POMDP-64 Verification Bench
 */

#include "pomdp64.hpp"
#include <iostream>

int main() {

    using namespace pomdp64;

    Simulator sim;
    GameState state;

    sim.reset_board(state);

    std::cout << "=====================================\n";
    std::cout << "POMDP-64 :: MARKOVIAN VOID PROTOCOL\n";
    std::cout << "=====================================\n\n";

    // =====================================================
    // STRUCTURE VALIDATION
    // =====================================================

    std::cout
        << "[STRUCT] GameState Size : "
        << sizeof(GameState)
        << " bytes\n";

    // =====================================================
    // INITIAL MOVE GENERATION
    // =====================================================

    Move buffer[256];

    int white_moves =
        sim.generate_pseudo_moves(
            state,
            WHITE,
            buffer
        );

    int black_moves =
        sim.generate_pseudo_moves(
            state,
            BLACK,
            buffer
        );

    std::cout
        << "[MOVES] White Initial Moves : "
        << white_moves
        << "\n";

    std::cout
        << "[MOVES] Black Initial Moves : "
        << black_moves
        << "\n";

    // =====================================================
    // VISIBILITY TEST
    // =====================================================

    uint64_t white_visibility =
        sim.get_visibility_mask(
            state,
            WHITE
        );

    std::cout
        << "\n[VISIBILITY] White Sight Mask\n";

    sim.print_bitboard(white_visibility);

    // =====================================================
    // ROOK VISION TEST
    // =====================================================

    uint64_t rook_vision =
        sim.get_rook_vision(
            0,
            state.total_occ
        );

    std::cout
        << "[VISION] White Rook a1\n";

    sim.print_bitboard(rook_vision);

    // =====================================================
    // MOVE EXECUTION TEST
    // =====================================================

    sim.make_move(
        state,
        WHITE,
        PAWN,
        12,
        28
    );

    std::cout
        << "[MOVE] Executed e2 -> e4\n";

    std::cout
        << "[OCCUPANCY] Total Occupancy\n";

    sim.print_bitboard(state.total_occ);

    // =====================================================
    // TERMINATION
    // =====================================================

    std::cout
        << "[STATUS] Diagnostics Completed.\n";

    return 0;
}