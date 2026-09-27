/// This was written by Gemini.
#pragma once

#include <iostream>
#include <chrono>
#include <string>
#include <iomanip>
#include <ranges>

template <std::ranges::view View>
class tqdm {
    View view_;
    std::string desc_;
    size_t total_ = 0;
    std::chrono::time_point<std::chrono::steady_clock> start_time_;
    std::chrono::time_point<std::chrono::steady_clock> last_print_;

    static void print_time(double seconds) {
        const int m = static_cast<int>(seconds) / 60;
        const int s = static_cast<int>(seconds) % 60;
        std::cerr << std::setfill('0') << std::setw(2) << m << ":"
                  << std::setfill('0') << std::setw(2) << s;
    }

public:
    // Constructor applies std::views::all to perfectly forward containers or views
    template <std::ranges::range R>
    explicit tqdm(R&& r, std::string desc = "")
        : view_(std::views::all(std::forward<R>(r))), desc_(std::move(desc)) {
        if constexpr (std::ranges::sized_range<R>) {
            total_ = std::ranges::size(r);
        }
    }

    void update(size_t count) {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = now - start_time_;

        // Throttle updates to 10 FPS to prevent terminal bottlenecking
        if (count > 0 && count < total_ && (now - last_print_) < std::chrono::milliseconds(100)) {
            return;
        }
        last_print_ = now;

        double t = elapsed.count();
        double speed = (t > 0) ? count / t : 0;

        // \r returns to start of line, \033[K clears the line to prevent trailing artifacts
        std::cerr << "\r\033[K";
        if (!desc_.empty()) std::cerr << desc_ << ": ";

        if (total_ > 0) {
            int pct = (count * 100) / total_;
            std::cerr << std::setfill(' ') << std::setw(3) << pct << "%|";

            constexpr int bar_width = 30;
            const int filled = (count * bar_width) / total_;
            for (int i = 0; i < bar_width; ++i) {
                // \xe2\x96\x88 is the UTF-8 encoding for the solid block █
                if (i < filled) std::cerr << "\xe2\x96\x88";
                else std::cerr << " ";
            }
            std::cerr << "| " << count << "/" << total_;
        } else {
            std::cerr << count << "it"; // Fallback if size is unknown
        }

        std::cerr << " [";
        print_time(t);
        if (total_ > 0) {
            std::cerr << "<";
            if (speed > 0 && count < total_) {
                print_time((total_ - count) / speed);
            } else {
                std::cerr << "00:00";
            }
        }

        std::cerr << ", ";
        if (speed > 0) {
            if (speed >= 1.0) {
                std::cerr << std::fixed << std::setprecision(2) << speed << "it/s";
            } else {
                std::cerr << std::fixed << std::setprecision(2) << (1.0 / speed) << "s/it";
            }
        } else {
            std::cerr << "?it/s";
        }
        std::cerr << "]" << std::flush;

        // Print a clean newline exactly once when finished
        if (count == total_ && total_ > 0) std::cerr << "\n";
    }

    // The Custom Iterator Wrapper
    struct iterator {
        std::ranges::iterator_t<View> it_;
        tqdm* parent_;
        size_t count_;

        decltype(auto) operator*() { return *it_; }

        iterator& operator++() {
            ++it_;
            ++count_;
            parent_->update(count_);
            return *this;
        }

        bool operator!=(const auto& other) const {
            return it_ != other;
        }
    };

    auto begin() {
        start_time_ = std::chrono::steady_clock::now();
        last_print_ = start_time_;
        update(0); // Paint the 0% bar immediately
        return iterator{std::ranges::begin(view_), this, 0};
    }

    auto end() {
        return std::ranges::end(view_);
    }
};

// C++17 Class Template Argument Deduction (CTAD) Guide
template <std::ranges::range R>
tqdm(R&&, std::string = "") -> tqdm<std::views::all_t<R>>;

// trange helper function (perfect for simple loops)
inline auto trange(size_t start, size_t end, std::string desc = "") {
    return tqdm(std::views::iota(start, end), std::move(desc));
}
inline auto trange(size_t end, std::string desc = "") {
    return tqdm(std::views::iota(static_cast<size_t>(0), end), std::move(desc));
}