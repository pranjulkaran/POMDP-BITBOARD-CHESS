/**
 * @file main.cpp
 * @brief Compilation validation loop and structural diagnostic checks.
 */

#include "pomdp64.hpp"
#include <iostream>

int main() {
    // Instantiate our unmanaged stack-allocated state matrix
    pomdp64::GameState global_state;
    pomdp64::Simulator sim;

    std::cout << "[POMDP-64] Diagnostic Initialization Initiated...\n";
    std::cout << "[SYSTEM] Target State Footprint: " << sizeof(global_state) << " Bytes\n";

    if (sizeof(global_state) == 128) {
        std::cout << "[STATUS] COMPLIANCE PASSED: Structural alignment exactly hits 128-byte cache-line target.\n";
    } else {
        std::cout << "[STATUS] WARNING: Memory packing error detected. Target size drifted to: " << sizeof(global_state) << " Bytes\n";
    }

    // Force initialization of layout variables
    sim.reset_board(global_state);

    std::cout << "\n--- DIAGNOSTIC DATA CHANNEL: TOTAL OCCUPANCY BLOCK ---";
    sim.print_bitboard(global_state.total_occ);

    std::cout << "--- DIAGNOSTIC DATA CHANNEL: WHITE PAWNS STRUCT ---";
    sim.print_bitboard(global_state.pieces[pomdp64::WHITE][pomdp64::PAWN]);

    // Manual runtime verification check: query square e1 (Bit 4) inside total occupancy
    uint64_t e1_mask = pomdp64::Simulator::get_square_mask(4, 0); // File e (4), Rank 1 (0)
    if (global_state.total_occ & e1_mask) {
        std::cout << "[SYSTEM] Data query verification successful: Piece confirmed active at coordinate square e1.\n";
    }

    return 0;
}