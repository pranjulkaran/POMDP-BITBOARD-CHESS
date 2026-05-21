/**
 * @file main.cpp
 * @brief Phase 4 Move-Generation Performance Diagnostic Test Bench
 */

#include "pomdp64.hpp"
#include <iostream>

int main() {
    pomdp64::GameState state;
    pomdp64::Simulator sim;
    sim.reset_board(state);

    std::cout << "[POMDP-64] Phase 4 Target Serialization Suite Initialized...\n\n";

    // Allocate an unmanaged flat move stack array (Max buffer allocation ceiling of 256 moves)
    pomdp64::Move local_move_buffer[256];

    // Trigger vectorization compilation sweep
    std::cout << "[SYSTEM] Compiling pseudo-legal action tables for White color spectrum...\n";
    int generated_move_count = sim.generate_pseudo_moves(state, pomdp64::WHITE, local_move_buffer);

    std::cout << "[STATUS] Extraction passed. Total White Actions Registered: " << generated_move_count << "\n\n";

    std::cout << "==================================================\n";
    std::cout << "PARSED ACTION DATA STREAM LISTING\n";
    std::cout << "==================================================\n";

    for (int i = 0; i < generated_move_count; ++i) {
        std::cout << "Action #" << i << " | Piece Type: " << static_cast<int>(local_move_buffer[i].piece_type)
                  << " | From Bit index: " << static_cast<int>(local_move_buffer[i].from)
                  << " -> To Bit index: " << static_cast<int>(local_move_buffer[i].to) << "\n";
    }

    // Direct integrity validation: A white knight on b1 (Index 1) should looplessly find a3 (Index 16) and c3 (Index 18)
    std::cout << "\n[INTEGRITY] Validating Knight path gates extraction...\n";
    bool b1_knight_passed = false;
    for (int i = 0; i < generated_move_count; ++i) {
        if (local_move_buffer[i].from == 1 && local_move_buffer[i].to == 18) {
            b1_knight_passed = true;
            std::cout << "[CHECKPASS] Native Knight move successfully identified: b1 (1) -> c3 (18)\n";
            break;
        }
    }

    if (!b1_knight_passed) {
        std::cout << "[WARNING] Failure flagged. Check step offset calculation matrix values.\n";
    }

    return 0;
}