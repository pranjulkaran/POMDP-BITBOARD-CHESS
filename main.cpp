/**
 * @file main.cpp
 * @brief Integrated Diagnostic Suit & Regression Verification Test Bench
 */

#include "pomdp64.hpp"
#include <iostream>

void print_boundary_header(const char* label) {
    std::cout << "==================================================\n";
    std::cout << "DIAGNOSTIC TARGET: " << label << "\n";
    std::cout << "==================================================\n";
}

int main() {
    pomdp64::GameState state;
    pomdp64::Simulator sim;
    sim.reset_board(state);

    std::cout << "[POMDP-64] Master Engine Validation Suite Initialized...\n\n";

    // --- PHASE 1 COMPLIANCE LOGIC CHECK ---
    print_boundary_header("PHASE 1: MEMORY LAYOUT STRUC ALIGNMENT");
    std::cout << "STATE_STRUCT_SIZE_BYTES: " << sizeof(state) << "\n";
    std::cout << "CACHE_LINE_COMPLIANT: " << (sizeof(state) == 128 ? "PASSED" : "FAILED") << "\n\n";

    // --- PHASE 2 COMPLIANCE LOGIC CHECK ---
    print_boundary_header("PHASE 2: LOOPLESS RAY-CAST SLIDING VISION (Rook d4)");
    int d4_square = 27; // File d (3), Rank 4 (3) -> 3 * 8 + 3 = 27
    uint64_t rook_vision_baseline = sim.get_rook_vision(d4_square, state.total_occ);
    sim.print_bitboard(rook_vision_baseline);

    // --- PHASE 3 COMPLIANCE LOGIC CHECK ---
    print_boundary_header("PHASE 3: ATOMIC MUTATION ACCELERATION (e2 -> e4)");
    int e2_pawn = 12; // File e (4), Rank 2 (1) -> 1 * 8 + 4 = 12
    int e4_pawn = 28; // File e (4), Rank 4 (3) -> 3 * 8 + 4 = 28

    std::cout << "[ACTION] Executing state transition: White Pawn e2 -> e4...\n";
    sim.make_move(state, pomdp64::WHITE, pomdp64::PAWN, e2_pawn, e4_pawn);

    std::cout << "\n--- GLOBAL OCCUPANCY STATE POST-MUTATION ---";
    sim.print_bitboard(state.total_occ);

    std::cout << "--- EVALUATING ROOK VISION PATHWAYS AFTER PAWN CLEARANCE ---";
    uint64_t rook_vision_mutated = sim.get_rook_vision(d4_square, state.total_occ);
    sim.print_bitboard(rook_vision_mutated);

    // --- RUNTIME TRANSACTION INTEGRITY CHECKS ---
    std::cout << "--- MATRICES INTEGRITY VERIFICATION SIGNALS ---\n";
    if (!(state.pieces[pomdp64::WHITE][pomdp64::PAWN] & (1ULL << e2_pawn))) {
        std::cout << "[INTEGRITY_PASS] Source bit 12 (e2) cleanly shifted down to 0.\n";
    }
    if (state.pieces[pomdp64::WHITE][pomdp64::PAWN] & (1ULL << e4_pawn)) {
        std::cout << "[INTEGRITY_PASS] Target bit 28 (e4) successfully flashed to 1.\n";
    }
    if (rook_vision_mutated & (1ULL << 4)) {
        std::cout << "[VISION_PASS] Lookless engine successfully tracks light path down to e1 square boundary!\n";
    }

    return 0;
}