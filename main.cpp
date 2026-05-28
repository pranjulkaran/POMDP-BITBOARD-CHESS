/**

* @file main.cpp
* @brief POMDP-64 Full Validation Suite
  */

#include "pomdp64.hpp"

#include <cassert>
#include <cstdlib>
#include <iostream>

using namespace pomdp64;

// ============================================================
// HELPERS
// ============================================================

static void clear_board(GameState& s) {


for (int c = 0; c < 2; ++c)
    for (int p = 0; p < 6; ++p)
        s.pieces[c][p] = 0ULL;

s.white_occ = 0ULL;
s.black_occ = 0ULL;
s.total_occ = 0ULL;
s.metadata = 0ULL;

}

static void require(bool condition, const char* test_name) {

if (!condition) {

    std::cerr
        << "[FAILED] "
        << test_name
        << "\n";

    std::exit(EXIT_FAILURE);
}

std::cout
    << "[PASSED] "
    << test_name
    << "\n";


}

// ============================================================
// MAIN
// ============================================================

int main() {


Simulator sim;
GameState state;

std::cout
    << "====================================\n"
    << "POMDP-64 FULL SYSTEM TEST SUITE\n"
    << "====================================\n\n";

// ========================================================
// TEST 1
// RESET BOARD
// ========================================================

sim.reset_board(state);

require(
    state.pieces[WHITE][PAWN] ==
    0x000000000000FF00ULL,
    "White pawns initialized"
);

require(
    state.pieces[BLACK][KING] ==
    0x1000000000000000ULL,
    "Black king initialized"
);

require(
    state.total_occ != 0ULL,
    "Occupancy initialized"
);

// ========================================================
// TEST 2
// KNIGHT ATTACK TABLE
// ========================================================

{
    uint64_t attacks =
        sim.get_knight_attacks(28);

    require(
        __builtin_popcountll(attacks) == 8,
        "Knight attacks from center"
    );
}

// ========================================================
// TEST 3
// KING ATTACK TABLE
// ========================================================

{
    uint64_t attacks =
        sim.get_king_attacks(28);

    require(
        __builtin_popcountll(attacks) == 8,
        "King attacks from center"
    );
}

// ========================================================
// TEST 4
// ROOK VISION
// ========================================================

{
    clear_board(state);

    state.pieces[WHITE][ROOK] =
        1ULL << 27;

    sim.squash_occupancy(state);

    uint64_t rook =
        sim.get_rook_vision(
            27,
            state.total_occ
        );

    require(
        __builtin_popcountll(rook) == 14,
        "Rook open-board vision"
    );
}

// ========================================================
// TEST 5
// BISHOP VISION
// ========================================================

{
    clear_board(state);

    state.pieces[WHITE][BISHOP] =
        1ULL << 27;

    sim.squash_occupancy(state);

    uint64_t bishop =
        sim.get_bishop_vision(
            27,
            state.total_occ
        );

    require(
        __builtin_popcountll(bishop) == 13,
        "Bishop open-board vision"
    );
}

// ========================================================
// TEST 6
// QUEEN VISION
// ========================================================

{
    clear_board(state);

    state.pieces[WHITE][QUEEN] =
        1ULL << 27;

    sim.squash_occupancy(state);

    uint64_t queen =
        sim.get_queen_vision(
            27,
            state.total_occ
        );

    require(
        __builtin_popcountll(queen) == 27,
        "Queen open-board vision"
    );
}

// ========================================================
// TEST 7
// SIMPLE MOVE
// ========================================================

{
    sim.reset_board(state);

    sim.make_move(
        state,
        WHITE,
        PAWN,
        12,
        28
    );

    require(
        state.pieces[WHITE][PAWN] &
        (1ULL << 28),
        "Pawn moved"
    );

    require(
        !(state.pieces[WHITE][PAWN] &
        (1ULL << 12)),
        "Pawn removed from source"
    );
}

// ========================================================
// TEST 8
// ATTACK DETECTION
// ========================================================

{
    clear_board(state);

    state.pieces[WHITE][KING] =
        1ULL << 4;

    state.pieces[BLACK][ROOK] =
        1ULL << 60;

    sim.squash_occupancy(state);

    require(
        sim.is_square_attacked(
            state,
            4,
            BLACK
        ),
        "Rook attack detected"
    );
}

// ========================================================
// TEST 9
// CHECK DETECTION
// ========================================================

{
    clear_board(state);

    state.pieces[WHITE][KING] =
        1ULL << 4;

    state.pieces[BLACK][QUEEN] =
        1ULL << 60;

    sim.squash_occupancy(state);

    require(
        sim.is_in_check(
            state,
            WHITE
        ),
        "Check detected"
    );
}

// ========================================================
// TEST 10
// PIN DETECTION
// ========================================================

{
    clear_board(state);

    state.pieces[WHITE][KING] =
        1ULL << 4;

    state.pieces[WHITE][ROOK] =
        1ULL << 12;

    state.pieces[BLACK][ROOK] =
        1ULL << 60;

    sim.squash_occupancy(state);

    uint64_t pinned =
        sim.get_pinned_pieces(
            state,
            WHITE
        );

    require(
        pinned & (1ULL << 12),
        "Pinned piece detected"
    );
}

// ========================================================
// TEST 11
// PIN LEGAL MASK
// ========================================================

{
    clear_board(state);

    state.pieces[WHITE][KING] =
        1ULL << 4;

    state.pieces[WHITE][ROOK] =
        1ULL << 12;

    state.pieces[BLACK][ROOK] =
        1ULL << 60;

    sim.squash_occupancy(state);

    uint64_t pinned =
        sim.get_pinned_pieces(
            state,
            WHITE
        );

    uint64_t mask =
        sim.legal_mask_from_pins(
            state,
            WHITE,
            12,
            pinned
        );

    require(
        mask & (1ULL << 20),
        "Pinned movement along file allowed"
    );

    require(
        !(mask & (1ULL << 13)),
        "Pinned sideways movement blocked"
    );
}

// ========================================================
// TEST 12
// ILLEGAL MOVE REJECTION
// ========================================================

{
    clear_board(state);

    state.pieces[WHITE][KING] =
        1ULL << 4;

    state.pieces[WHITE][ROOK] =
        1ULL << 12;

    state.pieces[BLACK][ROOK] =
        1ULL << 60;

    sim.squash_occupancy(state);

    Move move = {
    12,
    13,
    ROOK,
    MOVE_QUIET
    };

    bool ok =
    sim.attempt_move(
        state,
        move
    );

    require(
        !ok,
        "Illegal pinned move rejected"
    );

    require(
        state.pieces[WHITE][ROOK] &
        (1ULL << 12),
        "Rollback successful"
    );
}

// ========================================================
// TEST 13
// LEGAL PIN MOVE
// ========================================================

{
    clear_board(state);

    state.pieces[WHITE][KING] =
        1ULL << 4;

    state.pieces[WHITE][ROOK] =
        1ULL << 12;

    state.pieces[BLACK][ROOK] =
        1ULL << 60;

    sim.squash_occupancy(state);

    Move move = {
    12,
    20,
    ROOK,
    MOVE_QUIET
    };

    bool ok =
    sim.attempt_move(
        state,
        move
    );

    require(
        ok,
        "Pinned vertical move allowed"
    );
}

// ========================================================
// TEST 14
// NEGATIVE FILE PIN
// ========================================================

{
    clear_board(state);

    state.pieces[WHITE][KING] =
        1ULL << 60;

    state.pieces[WHITE][ROOK] =
        1ULL << 52;

    state.pieces[BLACK][ROOK] =
        1ULL << 4;

    sim.squash_occupancy(state);

    uint64_t pinned =
        sim.get_pinned_pieces(
            state,
            WHITE
        );

    require(
        pinned & (1ULL << 52),
        "Negative direction file pin detected"
    );
}

// ========================================================
// TEST 15
// NEGATIVE DIAGONAL PIN
// ========================================================

{
    clear_board(state);

    state.pieces[WHITE][KING] =
        1ULL << 63;

    state.pieces[WHITE][BISHOP] =
        1ULL << 54;

    state.pieces[BLACK][BISHOP] =
        1ULL << 9;

    sim.squash_occupancy(state);

    uint64_t pinned =
        sim.get_pinned_pieces(
            state,
            WHITE
        );

    require(
        pinned & (1ULL << 54),
        "Negative diagonal pin detected"
    );
}

// ========================================================
// TEST 16
// PSEUDO MOVE GENERATION
// ========================================================

{
    sim.reset_board(state);

    Move moves[MAX_MOVES];

    int count =
        sim.generate_pseudo_moves(
            state,
            WHITE,
            moves
        );

    require(
        count > 0,
        "Pseudo moves generated"
    );

    require(
        count < MAX_MOVES,
        "Move buffer respected"
    );
}

// ========================================================
// TEST 17
// VISIBILITY SYSTEM
// ========================================================

{
    sim.reset_board(state);

    uint64_t vis =
        sim.get_visibility_mask(
            state,
            WHITE
        );

    require(
        vis != 0ULL,
        "Visibility generated"
    );
}

// ========================================================
// TEST 18
// CAPTURE EXECUTION
// ========================================================

{
    clear_board(state);

    state.pieces[WHITE][ROOK] =
        1ULL << 0;

    state.pieces[BLACK][KNIGHT] =
        1ULL << 56;

    sim.squash_occupancy(state);

    sim.make_move(
        state,
        WHITE,
        ROOK,
        0,
        56
    );

    require(
        !(state.pieces[BLACK][KNIGHT]),
        "Capture removed enemy piece"
    );

    require(
        state.pieces[WHITE][ROOK] &
        (1ULL << 56),
        "Capture placed moving piece"
    );
}

// ========================================================
// TEST 19
// PROMOTION
// ========================================================

{
    clear_board(state);

    state.pieces[WHITE][PAWN] =
        1ULL << 48;

    sim.squash_occupancy(state);

    Move move = {
    48,
    56,
    PAWN,
    MOVE_PROMOTE_QUEEN
    };

    bool ok =
    sim.attempt_move(
        state,
        move
    );

    require(
        ok,
        "Promotion move accepted"
    );

    require(
        state.pieces[WHITE][QUEEN] &
        (1ULL << 56),
        "Promotion created queen"
    );
}

// ========================================================
// TEST 20
// KING CANNOT MOVE INTO CHECK
// ========================================================

{
    clear_board(state);

    state.pieces[WHITE][KING] =
        1ULL << 4;

    state.pieces[BLACK][ROOK] =
        1ULL << 60;

    sim.squash_occupancy(state);

    Move move = {
    4,
    12,
    KING,
    MOVE_QUIET
    };

    bool ok =
    sim.attempt_move(
        state,
        move
    );

    require(
        !ok,
        "King cannot move into check"
    );
}

// ========================================================
// TEST COMPLETE
// ========================================================

std::cout
    << "\n====================================\n"
    << "ALL TESTS PASSED\n"
    << "SYSTEM VALIDATED\n"
    << "====================================\n";

return 0;

}
