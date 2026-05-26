#include "pomdp64.hpp"
#include <iostream>
#include <cassert>

using namespace pomdp64;

// ============================================================
// SIMPLE TEST UTILITIES
// ============================================================

static void print_fail(const char* test) {
    std::cout << "[FAIL] " << test << "\n";
    std::exit(1);
}

static void expect(bool cond, const char* test) {
    if (!cond) print_fail(test);
}

static int count_bits(uint64_t bb) {
    int c = 0;
    while (bb) {
        bb &= bb - 1;
        c++;
    }
    return c;
}

// ============================================================
// TEST 1: BOARD INITIALIZATION
// ============================================================

static void test_reset_board() {
    Simulator sim;
    GameState s{};

    sim.reset_board(s);

    uint64_t total =
        s.white_occ | s.black_occ;

    expect(total != 0ULL, "reset_board: pieces exist");

    expect((s.pieces[WHITE][KING] & (1ULL << 4)) != 0,
           "reset_board: white king correct square");

    expect((s.pieces[BLACK][KING] & (1ULL << 60)) != 0,
           "reset_board: black king correct square");
}

// ============================================================
// TEST 2: OCCUPANCY CONSISTENCY
// ============================================================

static void test_occupancy() {
    Simulator sim;
    GameState s{};

    sim.reset_board(s);

    uint64_t manual = 0;

    for (int c = 0; c < 2; c++)
        for (int p = 0; p < 6; p++)
            manual |= s.pieces[c][p];

    expect(manual == s.total_occ,
           "occupancy: total_occ matches bitboards");
}

// ============================================================
// TEST 3: KNIGHT ATTACKS
// ============================================================

static void test_knight_attacks() {
    Simulator sim;
    GameState s{};

    sim.reset_board(s);

    // clear board for deterministic test
    for (int c = 0; c < 2; c++)
        for (int p = 0; p < 6; p++)
            s.pieces[c][p] = 0;

    s.pieces[WHITE][KNIGHT] = 1ULL << 36; // d5
    sim.squash_occupancy(s);

    uint64_t attacks = sim.get_knight_attacks(36);

    expect(attacks != 0, "knight attacks non-zero");
    expect((attacks & (1ULL << 21)) != 0, "knight attack valid square exists");
}

// ============================================================
// TEST 4: ROOK RAYS
// ============================================================

static void test_rook_vision() {
    Simulator sim;
    GameState s{};

    for (int i = 0; i < 64; i++)
        s.pieces[WHITE][PAWN] = 0;

    s.pieces[WHITE][ROOK] = 1ULL << 27; // d4
    sim.squash_occupancy(s);

    uint64_t vision = sim.get_rook_vision(27, s.total_occ);

    expect(vision != 0, "rook vision exists");
}

// ============================================================
// TEST 5: CHECK DETECTION (KING ATTACK)
// ============================================================

static void test_check_detection() {
    Simulator sim;
    GameState s{};

    sim.reset_board(s);

    // remove everything
    for (int c = 0; c < 2; c++)
        for (int p = 0; p < 6; p++)
            s.pieces[c][p] = 0;

    // white king
    s.pieces[WHITE][KING] = 1ULL << 4;

    // black rook giving check
    s.pieces[BLACK][ROOK] = 1ULL << 60;

    sim.squash_occupancy(s);

    bool check = sim.is_in_check(s, WHITE);

    expect(check == true, "check detection works");
}

// ============================================================
// TEST 6: PIN DETECTION BASIC LINE
// ============================================================

static void test_pin_detection() {
    Simulator sim;
    GameState s{};

    sim.reset_board(s);

    // clear board
    for (int c = 0; c < 2; c++)
        for (int p = 0; p < 6; p++)
            s.pieces[c][p] = 0;

    // white king on e1
    s.pieces[WHITE][KING] = 1ULL << 4;

    // white rook on e2 (potential pinned line)
    s.pieces[WHITE][ROOK] = 1ULL << 12;

    // black rook on e8 (pin source)
    s.pieces[BLACK][ROOK] = 1ULL << 60;

    sim.squash_occupancy(s);

    uint64_t pinned = sim.get_pinned_pieces(s, WHITE);

    expect(pinned != 0, "pin detection finds pinned piece");
}

// ============================================================
// TEST 7: MOVE GENERATION BASIC
// ============================================================

static void test_move_generation() {
    Simulator sim;
    GameState s{};

    sim.reset_board(s);

    Move moves[256];

    int n = sim.generate_pseudo_moves(s, WHITE, moves);

    expect(n > 0, "move generation produces moves");
    expect(n < 256, "move generation within buffer");
}

// ============================================================
// TEST 8: VISIBILITY MASK
// ============================================================

static void test_visibility() {
    Simulator sim;
    GameState s{};

    sim.reset_board(s);

    uint64_t vis = sim.get_visibility_mask(s, WHITE);

    expect(vis != 0, "visibility not empty");
}

// ============================================================
// MAIN TEST RUNNER
// ============================================================

int main() {

    std::cout << "POMDP-64 FULL SYSTEM TEST START\n";

    test_reset_board();
    test_occupancy();
    test_knight_attacks();
    test_rook_vision();
    test_check_detection();
    test_pin_detection();
    test_move_generation();
    test_visibility();

    std::cout << "ALL TESTS PASSED\n";
    return 0;
}