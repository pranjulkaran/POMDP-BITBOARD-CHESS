/**
 * @file main.cpp
 * @brief POMDP-64 Verification Bench (Rule-Layer Test Suite)
 */

#include "pomdp64.hpp"
#include <iostream>

using namespace pomdp64;

int main() {

    Simulator sim;
    GameState state;

    sim.reset_board(state);

    std::cout << "=====================================\n";
    std::cout << "POMDP-64 :: RULE LAYER VERIFICATION\n";
    std::cout << "=====================================\n\n";

    // =========================================================
    // STRUCTURE CHECK
    // =========================================================

    std::cout << "[STRUCT] GameState size: "
              << sizeof(GameState)
              << " bytes\n\n";

    // =========================================================
    // INITIAL MOVE GENERATION
    // =========================================================

    Move buffer[512];

    int white_moves = sim.generate_pseudo_moves(state, WHITE, buffer);
    int black_moves = sim.generate_pseudo_moves(state, BLACK, buffer);

    std::cout << "[MOVES] White pseudo-legal: " << white_moves << "\n";
    std::cout << "[MOVES] Black pseudo-legal: " << black_moves << "\n\n";

    // =========================================================
    // ATTACK VALIDATION
    // =========================================================

    int white_king_sq = __builtin_ctzll(state.pieces[WHITE][KING]);
    int black_king_sq = __builtin_ctzll(state.pieces[BLACK][KING]);

    bool white_in_check = sim.is_square_attacked(state, white_king_sq, BLACK);
    bool black_in_check = sim.is_square_attacked(state, black_king_sq, WHITE);

    std::cout << "[ATTACK] White king in check: " << white_in_check << "\n";
    std::cout << "[ATTACK] Black king in check: " << black_in_check << "\n\n";

    // =========================================================
    // VISIBILITY TEST
    // =========================================================

    uint64_t white_vis = sim.get_visibility_mask(state, WHITE);

    std::cout << "[VISIBILITY] White mask:\n";
    sim.print_bitboard(white_vis);

    // =========================================================
    // SLIDING ATTACK TEST
    // =========================================================

    uint64_t rook_vis = sim.get_rook_vision(0, state.total_occ);

    std::cout << "[VISION] Rook from a1:\n";
    sim.print_bitboard(rook_vis);

    // =========================================================
    // TRANSACTIONAL MOVE TEST (LEGAL MOVE)
    // =========================================================

    bool move1 = sim.attempt_move(state, 12, 28); // e2 -> e4

    std::cout << "[MOVE] e2 -> e4 result: "
              << (move1 ? "LEGAL" : "ILLEGAL")
              << "\n";

    // =========================================================
    // TRANSACTIONAL MOVE TEST (ILLEGAL SELF-CHECK SCENARIO)
    // =========================================================

    GameState before_illegal = state;

    // try something likely illegal early (king move into pawn shield)
    bool move2 = sim.attempt_move(state, 4, 12);

    std::cout << "[MOVE] e1 -> e2 result: "
              << (move2 ? "LEGAL" : "ILLEGAL (ROLLED BACK)")
              << "\n\n";

    // =========================================================
    // OCCUPANCY CHECK
    // =========================================================

    std::cout << "[OCCUPANCY] Total board state:\n";
    sim.print_bitboard(state.total_occ);

    std::cout << "\n[OCCUPANCY] White occupancy:\n";
    sim.print_bitboard(state.white_occ);

    std::cout << "\n[OCCUPANCY] Black occupancy:\n";
    sim.print_bitboard(state.black_occ);

    // =========================================================
    // CONSISTENCY CHECK
    // =========================================================

    uint64_t recomputed =
        state.white_occ | state.black_occ;

    std::cout << "\n[CONSISTENCY] occupancy match: "
              << (recomputed == state.total_occ)
              << "\n";

    // =========================================================
    // FINAL STATUS
    // =========================================================

    std::cout << "\n[STATUS] Rule-layer verification complete.\n";

    return 0;
}