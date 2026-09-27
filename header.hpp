/**
SETS THE NAMESPACE TO std!!!

This header is adapted from my competitive programming template.
 */
#pragma once
#include <bits/stdc++.h>
#define PARENS ()
#define EXPAND(...) EXPAND2(EXPAND2(EXPAND2(EXPAND2(__VA_ARGS__))))
#define EXPAND2(...) EXPAND1(EXPAND1(EXPAND1(EXPAND1(__VA_ARGS__))))
#define EXPAND1(...) __VA_ARGS__
#define FOR_EACH(macro, ...) __VA_OPT__(EXPAND(FOR_EACH_HELPER(macro, __VA_ARGS__)))
#define FOR_EACH_HELPER(macro, a1, ...) macro(a1) __VA_OPT__(FOR_EACH_AGAIN PARENS (macro, __VA_ARGS__))
#define FOR_EACH_AGAIN() FOR_EACH_HELPER
// #ifdef ONLINE_JUDGE
//     #pragma GCC optimize("O3,fast-math,unroll-loops")
//     #pragma GCC target("avx2,popcnt,lzcnt,abm,bmi,bmi2,fma,tune=native")
//     #define assume_helper(cond) ;[[assume(cond)]]
//     #define assume(cond, ...) [[assume(cond)]] __VA_OPT__(FOR_EACH(assume_helper, __VA_ARGS__))
// #else
//     #pragma GCC optimize("O0")
//     #include <cassert>
//     #define assume_helper(cond) ;assert(cond)
//     #define assume(cond, ...) assert(cond) __VA_OPT__(FOR_EACH(assume_helper, __VA_ARGS__))
// #endif
using namespace std;
typedef unsigned char uchar;
typedef unsigned int uint;
typedef long long ll;
typedef unsigned long long ull;
typedef pair<int, int> pii;
typedef pair<uint, uint> puii;
typedef pair<ll, ll> pll;
typedef pair<ull, ull> pull;
#define hashset unordered_set
#define hashmap unordered_map
// `all` macro conflicts with std::views::all(), used in tqdm.hpp.
// #define all(array) begin(array), end(array)
#define loop(num) for (uint _ = 0; _ < num; ++_)
#define setMin(x, y) x = (y < x? y : x)
#define setMax(x, y) x = (y > x? y : x)
#define FOR(index, limit) for (uint index = 0; index < limit; ++index)
#define yes cout << "YES\n"
#define no cout << "NO\n"
#define cint_helper(x) ;int x; cin >> x
#define cint(x, ...) int x; cin >> x __VA_OPT__(FOR_EACH(cint_helper, __VA_ARGS__))
#define cull_helper(x) ;ull x; cin >> x
#define cull(x, ...) ull x; cin >> x __VA_OPT__(FOR_EACH(cull_helper, __VA_ARGS__))
#define p_op(op,T) T operator op(const T& a, const T& b) {return {a.first op b.first, a.second op b.second};}
#define p_ops(T) p_op(+, T) p_op(-, T) p_op (*, T) p_op(/, T)
FOR_EACH(p_ops, pii, puii, pll, pull)
void print(auto t, const auto... args) {
    cout << t;
    ([](const auto i) { cout << " " << i; }(args), ...);
    cout << "\n";
}
// contiguous_iterator requires both the [] operator and that the elements lie contiguous in memory (for prefetching).
template<contiguous_iterator it, typename T, predicate<T, T> C = ranges::less>
it fast_lower_bound(it f, it l, const T &v, C comp = {}) {
    size_t len = distance(f, l);
    while (len > 1) {
        size_t half = len / 2;
        len -= half;
        __builtin_prefetch(&f[len / 2 - 1]);
        __builtin_prefetch(&f[half + len / 2 - 1]);
        f = comp(f[half - 1], v)? f + half : f;
    }
    f = len > 0? f + comp(*f, v) : f;
    return f;
}
template<size_t N>
/// @returns The number of 1-bits read in.
int read_to_bitset(bitset<N> &bs) {
    string s;
    cin >> s;
    int count = 0;
    for (int i = 0; i < s.length(); ++i) {
        bs[i] = s[i] - '0';
        count += bs[i];
    }
    return count;
}
void skip_lines(const uint nlines, const bool from_middle = true) {
    // If we start calling ignore() at the beginning of a line, everything's fine. However, if we just read in a value,
    // our input stream actually lies right before the '\n' at the end of that line. Thus, we actually need to call
    // ignore() one extra time.
    loop(nlines + from_middle)
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
}
constexpr int fast_gcd (int a, int b) {
   if (a == 0) return b;
   if (b == 0) return a;
   int az = __builtin_ctz(a);
   const int bz = __builtin_ctz(b);
   const int shift = min(az, bz);
   b >>= bz;
   while (a != 0) {
       a >>= az;
       const int diff = b - a;
       az = __builtin_ctz(diff);
       b = min(a, b);
       a = abs(diff);
   }
   return b << shift;
}
constexpr ull mod = 1e9 + 7;
constexpr ull binpow(ull base, ull power) {
    ull r = 1;
    while (power) {
        if (power & 1)
            r = r * base % mod;
        base = base * base % mod;
        power >>= 1;
    }
    return r;
}
constexpr ull mod_inverse(ull a) {
    ull r = 1;
#pragma GCC unroll(30)
    for (int l = 0; l < 30; ++l) {
        if (mod - 2 & 1 << l)
            r = r * a % mod;
        a = a * a % mod;
    }
    return r;
}
template<size_t N> // Must be constexpr.
struct DSU {
    int nodes[N]{};
    explicit DSU () { fill(all(nodes), -1); }
    int CC (const int node) { return nodes[node] < 0 ? node : nodes[node] = CC(nodes[node]); }
    int size (const int node) { return -nodes[CC(node)]; }
    void unite (int a, int b) {
        a = CC(a), b = CC(b);
        if (a == b) return;
        if (-nodes[a] > -nodes[b]) swap(a, b);
        nodes[b] += nodes[a];
        nodes[a] = b;
    }
};
#define ret(...) {print(__VA_ARGS__); return;}
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————\\
// The code below is written by Gemini.

/// Custom struct to iterate over the set bits of a bitmask.
template<typename mask_t, typename item_t = uchar>
struct iterate_bits {
    mask_t mask;

    // This constructor is the key. It forces the compiler to deduce mask_t
    // directly from the argument (e.g., passing a `ull` makes mask_t a `ull`).
    constexpr iterate_bits(mask_t m) : mask(m) {}

    // A dummy type used purely to tell the compiler when to stop.
    // This costs 0 bytes of memory and 0 registers.
    struct sentinel {};

    struct iterator {
        mask_t m;

        // Extracts the lowest set bit index
        [[nodiscard]] item_t operator*() const {
            return countr_zero(m);
        }

        // Advances the iterator
        iterator& operator++() {
            // OPTIMIZATION: m &= m - 1
            // This is mathematically identical to `m ^= 1ULL << next`, but faster.
            // On x86, it compiles down to a single instruction (`blsr`), bypassing
            // the need to calculate the bit-shift entirely.
            m &= m - 1;
            return *this;
        }

        // The loop continues as long as this returns true
        [[nodiscard]] bool operator!=(sentinel) const {
            return m != 0;
        }
    };

    [[nodiscard]] iterator begin() const { return iterator{mask}; }
    [[nodiscard]] sentinel end() const { return sentinel{}; }
};

struct FastRNG {
    uint64_t state;

    // The seed MUST NOT be zero!
    FastRNG(const uint64_t seed) : state(seed) {}

    // The core generator: 3 hardware instructions
    uint64_t next() {
        state ^= state << 13;
        state ^= state >> 7;
        state ^= state << 17;
        return state;
    }

    // --- For RRNN: Fast Bounded Integer ---
    uint32_t next_uint(uint32_t bound) {
        // OPTIMIZATION: Lemire's Multiplication Method
        // Using modulo (next() % bound) triggers a hardware division instruction
        // which stalls the CPU pipeline for 30-40 cycles.
        // This trick uses a single 3-cycle multiplication instead.
        uint64_t random32 = next() & 0xFFFFFFFF;
        return (random32 * bound) >> 32;
    }

    // --- For Simulated Annealing: Fast Float [0.0, 1.0) ---
    double next_double() {
        // OPTIMIZATION: Bit-masking to double
        // Standard (rand() / RAND_MAX) uses a slow floating-point division.
        // This shifts 53 bits of randomness directly into the mantissa of a
        // double-precision float and multiplies by a hex-float constant.
        return (next() >> 11) * 0x1.0p-53;
    }
};
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————\\
// ...and now this is me again.

void random_path(uchar* order, uchar N, uchar start_node, FastRNG& rng) {
    // Populate the array sequentially, skipping the start_node
    order[0] = start_node;
    unsigned char idx = 1;
    for (unsigned char i = 0; i < N; ++i)
        if (i != start_node)
            order[idx++] = i;
    // Keep the start_node constant; shuffle the rest
    for (unsigned char i = N - 1; i > 1; --i) {
        // rng.next_uint(i) returns [0, i-1]. Adding 1 shifts the range to [1, i].
        unsigned char j = 1 + rng.next_uint(i);
        swap(order[i], order[j]);
    }
}