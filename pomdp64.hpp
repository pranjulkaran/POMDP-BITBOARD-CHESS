#ifndef POMDP64_HPP
#define POMDP64_HPP

#include <cstdint>

namespace pomdp64 {

constexpr int WHITE = 0;
constexpr int BLACK = 1;

constexpr int PAWN   = 0;
constexpr int KNIGHT = 1;
constexpr int BISHOP = 2;
constexpr int ROOK   = 3;
constexpr int QUEEN  = 4;
constexpr int KING   = 5;

struct alignas(64) GameState {
    uint64_t pieces[2][6];
    uint64_t white_occ;
    uint64_t black_occ;
    uint64_t total_occ;
    uint64_t metadata;
};

static_assert(sizeof(GameState) == 128);

struct Move {
    uint8_t from;
    uint8_t to;
    uint8_t piece;
    uint8_t flag;
};

class Simulator {
private:
    uint64_t ray_up[64], ray_down[64], ray_left[64], ray_right[64];
    uint64_t ray_ur[64], ray_ul[64], ray_dr[64], ray_dl[64];

    uint64_t knight_attacks[64];
    uint64_t king_attacks[64];

    void init_ray_masks();
    void init_attack_masks();

public:
    static constexpr uint64_t FILE_A = 0x0101010101010101ULL;
    static constexpr uint64_t FILE_H = 0x8080808080808080ULL;
    static constexpr uint64_t NOT_FILE_A = ~FILE_A;
    static constexpr uint64_t NOT_FILE_H = ~FILE_H;

    static constexpr uint64_t RANK_2 = 0x000000000000FF00ULL;
    static constexpr uint64_t RANK_7 = 0x00FF000000000000ULL;

    Simulator();
    ~Simulator() = default;

    void reset_board(GameState& s);
    void squash_occupancy(GameState& s) const;

    void make_move(GameState& s, int color, int piece, int from, int to);

    uint64_t get_rook_vision(int sq, uint64_t occ) const;
    uint64_t get_bishop_vision(int sq, uint64_t occ) const;
    uint64_t get_queen_vision(int sq, uint64_t occ) const;

    uint64_t get_knight_attacks(int sq) const { return knight_attacks[sq]; }
    uint64_t get_king_attacks(int sq) const { return king_attacks[sq]; }

    bool is_square_attacked(const GameState& s, int sq, int attacker) const;
    bool is_in_check(const GameState& s, int color) const;

    uint64_t get_pinned_pieces(const GameState& s, int color) const;
    uint64_t legal_mask_from_pins(const GameState& s, int color, int from) const;

    int generate_pseudo_moves(const GameState& s, int color, Move* out);

    uint64_t get_visibility_mask(const GameState& s, int color) const;

    void print_bitboard(uint64_t bb) const;
};

} // namespace pomdp64

#endif