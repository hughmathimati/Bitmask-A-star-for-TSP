#include "header.hpp"
#include "BaseEvaluator.cpp"

constexpr unsigned char max_n = 25, inner_repeats = 1;
class A_star : public BaseEvaluator<A_star, max_n, 10, inner_repeats, true> {
    unsigned char N, start_node;
    uint decode_visited_mask;
    uint min_cost_to_node[max_n];

public:
    A_star(): BaseEvaluator("CSV OUTPUT PATH GOES HERE") {}
    /**
    Each state in the fringe is a 64-bit bitmask.

    The 5 lowest bits are the current node (supports up to N = 32).

    The next N lowest bits are the visited bitmask.

    The remaining 59 - N bits are the bits we are allotted for quantisation of each edge weight.
     - For N = 20, we get 39 bits, which is more than enough.
     - For N = 25, we get 34 bits, meaning if we are to keep the 2e9 quantization factor, we can represent a maximum total function cost of 2^32/(2e9) * 4 = 8.59.
     - For N = 30, we get 29 bits. If we wanted to run our algorithm, we'd have to limit our quantisation to 26 bits,
        which would require lowering our quantisation factor to around 5e7. This would lower our working precision to
        2e-8 (as opposed to 5e-10), but our accumulator now fits in a uint32. This won't work, however, because at N=30,
        the `prev` and `best` arrays alone take up 2^30 * 30 * 4 = 1.288e11 bytes (128.8 GB) of memory EACH. Alas, we
        can only fantasize about the performance we might achieve on N=30 with this algorithm.
     */
    priority_queue<ull, vector<ull>, ranges::greater> fringe;
    /// Used to backtrace the optimal path at the end.
    unsigned char prev[1u << max_n][max_n];
    /// Stores the *actual* cost.
    ull best[1u << max_n][max_n];
    ull mst_cache[1u << max_n];

    void initialise(const unsigned char N) {
        while (!fringe.empty())
            fringe.pop();

        FOR (i, 1u << N)
            fill_n(best[i], N, ULLONG_MAX);

        fill_n(mst_cache, 1u << N, 0);
    }
    ull create_mask(const ull current, const ull visited_mask, const ull cost) const {
        return current | visited_mask << 5 | cost << (N + 5);
    }
    tuple<unsigned char, uint, ull> decode_mask(const ull mask) const {
        // We don't need a `cost_mask` because we can just right-shift to discard all lower bits.
        return {
            // `current` is the lowest five bits.
            mask & (1u << 5) - 1,
            // `visited` is the next N bits.
            mask >> 5 & decode_visited_mask,
            // `cost` is all remaining bits.
            mask >> (N + 5)
        };
    }
    /// Calculates the MST via Prim's Algorithm. Uses the *quantised* edge weights.
    ull heuristic (const uint original_mask) {
        if (mst_cache[original_mask])
            return mst_cache[original_mask];
        uint mask = original_mask;

        // The start_node is always included in the MST because we need to finish back at the start.
        // So why not root the MST at the start node?
        for (const unsigned char next : iterate_bits(mask))
            min_cost_to_node[next] = adj[start_node][next];
        ull total_cost = 0;
        while (mask) {
            unsigned char best_node;
            uint min_cost = UINT_MAX;
            for (const unsigned char next : iterate_bits(mask)) {
                const uint candidate_cost = min_cost_to_node[next];
                if (candidate_cost < min_cost) {
                    best_node = next;
                    min_cost = candidate_cost;
                }
            }
            total_cost += min_cost;
            mask ^= 1u << best_node;
            for (const unsigned char next : iterate_bits(mask))
                setMin(min_cost_to_node[next], adj[best_node][next]);
        }
        return mst_cache[original_mask] = total_cost;
    }
    double run(const unsigned char N, const unsigned char start_node) {
        if constexpr (inner_repeats > 1)
            initialise(N);
        this->N = N;
        this->start_node = start_node;
        // Precompute this since we'll ostensibly use it many times inside decode_mask()
        decode_visited_mask = (1u << N) - 1;
        // We can get away with 1u instead of 1ull because we don't plan on running A* for N > 25 anyway.
        fringe.push(create_mask(start_node, 1u << start_node, 0));
        // If we're guaranteed a Hamiltonian cycle exists, we can use `while (true)` instead of `while(!pq.empty())` to
        // avoid the cost of a useless check every iteration.
        while (true) {
            const auto [current, visited_mask, total_cost] = decode_mask(fringe.top());
            const unsigned char visited_popcount = popcount(visited_mask);
            // Exit case. Break out of the loop upon seeing the first one, because it must be the optimal route.
            if (visited_popcount == N) break;
            // Don't pop if we're exiting since we don't want to lose the winning state. Otherwise, we can pop.
            fringe.pop();
            // total cost - heuristic = actual cost (to get to this state).
            // `visited_mask` is parent's `visited_mask` with the current node's bit set, so parent's remaining MST mask
            // is the inverse of this visited mask but with the current node's bit set.
            // For the first iteration of this loop, total_cost is zero. However, mst_cache is also initialised to all
            // zeroes. Thus, actual_cost evaluates to zero, and we don't have any problems.
            const ull actual_cost = total_cost - mst_cache[(1u << N) - 1 ^ visited_mask | 1u << current];
            // It's > and not >= because we're setting the best (min) cost at the time of enqueueing a state. This is so we don't enqueue unnecessary states
            // Of course, there's a very slim chance we could enqueue two different ways of arriving at the same state which have the same cost.
            // However, since we set the best (min) cost at the time of enqueueing a state, this repeated optimal state won't enqueue any nodes.
            if (actual_cost > best[visited_mask][current])
                continue;

            this->nodes_expanded++;

            // Don't pop until after the exit case so that we keep the winning state.
            // We can hardcode the near-end case
            if (visited_popcount == N - 2) {
                const unsigned char first = countr_one(visited_mask);
                const uint missing_second = visited_mask | 1u << first;
                const unsigned char second = countr_one(visited_mask | 1u << first);
                const uint missing_first = visited_mask | 1u << second;
                const uint full_mask = (1u << N) - 1;

                const ull first_cost = actual_cost + adj[current][first] + adj[first][second];
                if (first_cost < best[full_mask][second]) {
                    best[full_mask][second] = first_cost;
                    prev[full_mask][second] = first;
                    prev[missing_second][first] = current;
                    fringe.push(create_mask(second, full_mask, first_cost + adj[second][start_node]));
                }
                const ull second_cost = actual_cost + adj[current][second] + adj[second][first];
                if (second_cost < best[full_mask][first]) {
                    best[full_mask][first] = second_cost;
                    prev[full_mask][first] = second;
                    prev[missing_first][second] = current;
                    fringe.push(create_mask(first, full_mask, second_cost + adj[first][start_node]));
                }
            } else {
                uint remaining = (1u << N) - 1 ^ visited_mask; // 1 for unvisited nodes.
                // The heuristic cost is going to be the same for all nodes explored here.
                const ull heuristic_cost = heuristic(remaining);
                for (const unsigned char next : iterate_bits(remaining)) {
                    const uint next_visited_mask = visited_mask | 1u << next;
                    const ull next_actual_cost = actual_cost + adj[current][next];
                    if (next_actual_cost < best[next_visited_mask][next]) {
                        best[next_visited_mask][next] = next_actual_cost;
                        prev[next_visited_mask][next] = current;
                        fringe.push(create_mask(next, next_visited_mask, next_actual_cost + heuristic_cost));
                    }
                }
            }
        }
        // At this point, fringe.top() has a state where `current` is the last node, `visited_mask` is completely full,
        // and cost is the total quantised cost of the cycle. All we need to do now is to compute the original cost of
        // this cycle.
        double total_cost = 0;
        auto [last_node, visited_mask, _] = decode_mask(fringe.top());
        unsigned char node = last_node;
        while (node != start_node) {
            const unsigned char prev_node = prev[visited_mask][node];
            total_cost += original_weights[prev_node][node];
            visited_mask ^= 1u << node;
            node = prev_node;
        }
        // Now total_cost contains the sum of the cost of all edges from the start node to the second-to-last node.
        // The two edges left are from second-to-last to last and from last to start.
        return total_cost + original_weights[last_node][start_node];
    }
};

int main() {
    // Allocate the arrays on the stack, not the heap.
    auto* solver = new A_star();
}