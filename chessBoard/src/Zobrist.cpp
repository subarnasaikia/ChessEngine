#include "Zobrist.hpp"

namespace chess {
namespace zobrist {

uint64_t psq[PIECE_NB][SQUARE_NB];
uint64_t side;
uint64_t castling[16];
uint64_t epFile[FILE_NB];

namespace {

// Deterministic xorshift PRNG (same generator as the magic search in
// Attacks.cpp), so keys are reproducible from run to run.
struct PRNG {
    uint64_t s;
    explicit PRNG(uint64_t seed) : s(seed) {}
    uint64_t next() {
        s ^= s >> 12;
        s ^= s << 25;
        s ^= s >> 27;
        return s * 2685821657736338717ULL;
    }
};

} // namespace

void init() {
    static bool done = false;
    if (done) return;

    PRNG rng(0x9E3779B97F4A7C15ULL); // fixed seed -> reproducible keys

    for (int p = 0; p < PIECE_NB; ++p)
        for (int s = 0; s < SQUARE_NB; ++s)
            psq[p][s] = rng.next();

    side = rng.next();

    for (int c = 0; c < 16; ++c)
        castling[c] = rng.next();

    for (int f = 0; f < FILE_NB; ++f)
        epFile[f] = rng.next();

    done = true;
}

} // namespace zobrist
} // namespace chess
