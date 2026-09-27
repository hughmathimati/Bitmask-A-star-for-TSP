# Bitmask-A-star-for-TSP
Hyper-optimised bitmask-based A* algorithm for the travelling salesman problem. The algorithm takes as input an adjacency matrix, and assumes the graph is fully-connected with positive weights (the diagonal is zero, as nodes do not have edges to themselves). The algorithm does not assume the edges are bidirectional. Make sure to compile with GCC, and pass `-O3 -ffast-math -unroll-loops -march=native`.

The timings below were obtained on an Apple M5 CPU:
<img width="984" height="584" alt="image" src="https://github.com/user-attachments/assets/658e6799-2835-4805-a5b8-7b929f3187d3" />

# How it works
Edge weights are quantised from 64-bit floats into 32-bit integers; the actual floating-point cost is computed at the very end from the optimal path found. As such, this program works best on graphs where edge weights are around the same order of magnitude. The `quantization_multiplier` inside `BaseEvaluator.cpp` will need to be modified so that, when multiplied by the maximum edge cost in your graph, the resulting value fits inside an unsigned 32-bit integer (at its current value of `2e9`, the maximum edge weight supported is just above 2). Of course, decreasing this value also decreases the working precision, which is 1 divided by the `quantization_multiplier`.

States are represented with 64-bit bitmasks, strategically partitioned into three parts:
 - The 5 lowest bits are the current node (supports up to $N = 32$).
 - The next $N$ lowest bits are the visited bitmask.
 - The remaining $59 - N$ bits are the bits we are allotted for quantisation of each edge weight. Since these comprise the highest bits of the bitmask, the priority queue automatically sorts states by this value.
     - For $N = 20$, we get 39 (32 + 7) bits, meaning if we are to keep the 2e9 quantization factor, we can represent a maximum total function cost of $\frac{2^{32}}{2\times10^9} \times 2^7 \approx 274.88$.
     - For $N = 25$, we get 34 (32 + 2) bits, meaning if we are to keep the 2e9 quantization factor, we can represent a maximum total function cost of $\frac{2^{32}}{2\times10^9} \times 2^2 \approx 8.59$.
     - For $N = 30$, we get 29 bits. If we wanted to run our algorithm, we'd have to limit our quantisation to $29 - x$ bits, where $\frac{2^{29-x}}{\text{quantisation factor}} \times 2^x$ is our desired maximum total function cost. This would also require lowering our quantisation factor.
         - Let's say we lower our quantisation to 27 bits ($x = 2$). If we targeted a maximum total function cost of $8.59$ again, we would have to lower our quantisation factor to around 6e7. This would lower our working precision to
        $\approx 1.67\times10^{-8}$ (as opposed to $5\times10^{-10}$), but our accumulator now fits in a uint32.

        This won't work, however, because at $N = 30$, our `prev` and `best` arrays alone take up $2^{30} \times 30 \times 4 = 1.288\times10^{11}$ bytes (128.8 GB) of memory EACH. Alas, we can only fantasize about the performance we might achieve on N=30 with this algorithm.

The A* heuristic function is the cost of the minimum spanning tree containing the candidate next node, all other remaining nodes, and the start node (since we must return to the start at the end). Since this MST is actually the same for all of a given node's neighbours, we calculate this heuristic cost once when expanding the corresponding state and simply add it onto all its children's real costs. MST costs are cached by a bitmask of the nodes they contain, so that we only compute a given MST once.

The $N - 2$ case (where there are only two remaining cities in a route) is handled and hard-coded as a special case for further optimisation. Extensive use of bitmasks is employed throughout the program to maximise performance.
