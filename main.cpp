/**
 * @file main.cpp
 * @brief Integrated Blueprint Diagnostics Verification Bench
 */

#include "pomdp64.hpp"
#include <iostream>

int main() {
    pomdp64::GameState state;
    pomdp64::Simulator sim;
    sim.reset_board(state);

    std::cout << "[POMDP-64] Master Blueprint Compilation Check Passed.\n";
    std::cout << "[MEMORY] GameState Layout Byte Size: " << sizeof(state) << " -> (Target: 128)\n";

    pomdp64::Move buffer[256];
    int white_moves = sim.generate_pseudo_moves(state, pomdp64::WHITE, buffer);
    std::cout << "[GENERATION] Action Buffer Extracted " << white_moves << " Initial White Moves.\n";

    return 0;
}