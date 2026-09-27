#include "header.hpp"
#include "tqdm.hpp"

/**
Children MUST implement `double run(const unsigned char N, const unsigned char start_node)` -> (cost of min cycle).

If inner_repeats == 1, you must implement `void initialise(const unsigned char N)`.

Otherwise, if inner_repeats > 1, your run() function must perform its own clear/initialisation.

7_random_adj_mat_7.txt contains an edge weight of 1.122506918205548709e+00. With a quantization_multiplier of 4e9, this
 gets quantised to the max value representable in a 32-bit integer. Thus, quantization_multiplier is set to 2e9 instead.
 */
template<typename Derived, unsigned char max_n = 50,
    unsigned char repeats = 10, unsigned short inner_repeats = 1,
    bool track_nodes_expanded = false, bool special_initialisation = false,
    double quantization_multiplier = 2e9>
class BaseEvaluator {
protected:
    ull quantised_cost(const uchar* path, const uchar N, const uchar start_node) {
        ull total_cost = 0;
        FOR (i, N - 1)
            total_cost += adj[path[i]][path[i + 1]];
        return total_cost + adj[path[N - 1]][start_node];
    }
    double actual_cost(const uchar* path, const uchar N, const uchar start_node) {
        double total_cost = 0;
        FOR (i, N - 1)
            total_cost += original_weights[path[i]][path[i + 1]];
        return total_cost + original_weights[path[N - 1]][start_node];
    }

private:
    /// If you want to pass your own matrix, fill this in, and change the assignment of `matrices` to call your custom
    /// constexpr lambda:
    static constexpr auto custom_matrix = [] {
        // Put N here. Please make sure it fits in an unsigned char (is at most 255). My code was not written for larger
        // N, as there was no indication that we would need to run on N > 250 (although it would just take changing the
        // `unsigned char`/`uchar`s to `unsigned short`s).
        constexpr unsigned char N = 250;
        array<pair<unsigned char, array<array<char, 256>, 1> >, 1> matrices;
        matrices[0].first = N;
        unsigned char pos = 0;
        // Put the matrix path here.
        for (const char & c: "MATRIX PATH HERE")
            matrices[0].second[0][pos++] = c;
        return matrices;
    };
    /// An example of how you might run the evaluator on multiple matrices at a time.
    static constexpr auto multiple_matrices = [] {
        constexpr unsigned char sizes[] = {5, 6, 7, 8, 9, 10, 15, 20, 25, 30, 40, 50};
        array<pair<unsigned char, array<array<char, 256>, 10> >, 12> matrices;
        FOR(i, 12) {
            const unsigned char size = sizes[i];
            matrices[i].first = size;
            FOR(j, 10) {
                unsigned char pos = 0;
                auto &matrix = matrices[i].second[j];
                for (const char c: project_path)
                    matrix[pos++] = c;
                for (const char c: string_view("Matrices Folder/"))
                    matrix[pos++] = c;

                if (size < 10)
                    matrix[pos++] = '0' + size;
                else {
                    matrix[pos++] = '0' + size / 10;
                    matrix[pos++] = '0' + size % 10;
                }

                for (const char c: string_view("_random_adj_mat_"))
                    matrix[pos++] = c;

                matrix[pos++] = '0' + j;

                for (const char c: string_view(".txt"))
                    matrix[pos++] = c;
                matrix[pos] = '\0';
            }
        }
        return matrices;
    };
    /// Change the lambda function call to switch between matrices.
    static constexpr auto matrices = custom_matrix();
    unsigned char start_nodes[repeats];

protected:
    /// Stores the full-precision float64 edge weights.
    double original_weights[max_n][max_n];
    /// Stores the 32-bit integer-quantised edge weights (multiplied by 4e9, then rounded to the nearest integer).
    uint adj[max_n][max_n];
    /// Used only by A*.
    ull nodes_expanded = 0;
    /// Our constructor will also actually run our evaluator.
    BaseEvaluator(string output_filename = "tsp_benchmark_results.csv") : start_nodes{}, original_weights{}, adj{} {
        ios_base::sync_with_stdio(false);
        cin.tie(nullptr);
        ofstream csv_out(output_filename);
        if constexpr (track_nodes_expanded)
            csv_out << "N,file_idx,cost,cpu_time,real_time,nodes_expanded\n";
        else
            csv_out << "N,file_idx,cost,cpu_time,real_time\n";
        for (const auto &[N, filenames]: matrices) {
            if (N > max_n)
                return;
            FOR (i, repeats)
                start_nodes[i] = i % N;
            for (const auto [file_idx, filename]: tqdm(views::enumerate(filenames), format("N = {}", N))) {
                ifstream fin(filename.data());
                for (int i = 0; i < N; ++i) {
                    for (int j = 0; j < N; ++j) {
                        double weight;
                        fin >> weight;
                        original_weights[i][j] = weight;
                        adj[i][j] = static_cast<uint>(weight * quantization_multiplier + 0.5);
                    }
                }
                if constexpr(special_initialisation) {
                    static_cast<Derived *>(this)->special_initialise();
                }
                double min_cost = numeric_limits<double>::max();
                double min_cpu_time = numeric_limits<double>::max();
                double min_real_time = numeric_limits<double>::max();
                ull min_nodes_expanded = numeric_limits<ull>::max();
                FOR (i, repeats) {
                    if constexpr (inner_repeats == 1)
                        static_cast<Derived *>(this)->initialise(N);
                    this->nodes_expanded = 0;
                    const auto [cost, cpu_time, real_time] = inner_loop(N, start_nodes[i]);
                    ull current_nodes = this->nodes_expanded;
                    setMin(min_cost, cost);
                    setMin(min_cpu_time, cpu_time);
                    setMin(min_real_time, real_time);
                    if constexpr (inner_repeats > 1)
                        current_nodes /= inner_repeats;
                    setMin(min_nodes_expanded, current_nodes);
                }
                csv_out << static_cast<int>(N) << ","
                    << file_idx << ","
                    << setprecision(numeric_limits<double>::max_digits10)
                    << min_cost << ","
                    << min_cpu_time << ","
                    << min_real_time;
                if constexpr (track_nodes_expanded)
                    csv_out << "," << min_nodes_expanded;
                csv_out << "\n";
            }
            // Flush once per N.
            csv_out.flush();
        }
    }
    /// @returns {cost of final run, average cpu time across inner_loop runs, average real time across inner_loop runs}
    tuple<double, double, double> inner_loop(const unsigned char N, const unsigned char start_node) {
        auto real_start = chrono::high_resolution_clock::now();
        const clock_t cpu_start = clock();
        double cost;
        if constexpr (inner_repeats == 1)
            cost = static_cast<Derived *>(this)->run(N, start_node);
        else {
            loop (inner_repeats) {
                cost = static_cast<Derived*>(this)->run(N, start_node);
                // BLIND THE OPTIMIZER:
                // This inline assembly does no actual work (the string "" is empty),
                // but it tells GCC: "I might have modified the variable 'cost',
                // so you are strictly forbidden from deleting this loop."
                asm volatile("" : "+g"(cost) : : "memory");
            }
        }
        const clock_t cpu_end = clock();
        auto real_end = chrono::high_resolution_clock::now();
        if constexpr (inner_repeats == 1) {
            return {
                cost,
                static_cast<double>(cpu_end - cpu_start) / CLOCKS_PER_SEC,
                chrono::duration<double>(real_end - real_start).count()
            };
        } else {
            return {
                cost,
                static_cast<double>(cpu_end - cpu_start) / (inner_repeats * CLOCKS_PER_SEC),
                chrono::duration<double>(real_end - real_start).count() / inner_repeats
            };
        }
    }
};