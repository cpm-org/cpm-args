#include "suggest.hpp"

#include <algorithm>

namespace cpm::args::detail {

std::size_t edit_distance(std::string_view a, std::string_view b) {
    // d[i][j]: the distance between a's first i characters and b's first j.
    std::vector<std::vector<std::size_t>> d(a.size() + 1, std::vector<std::size_t>(b.size() + 1));
    for (std::size_t i = 0; i <= a.size(); ++i) d[i][0] = i;
    for (std::size_t j = 0; j <= b.size(); ++j) d[0][j] = j;
    for (std::size_t i = 1; i <= a.size(); ++i) {
        for (std::size_t j = 1; j <= b.size(); ++j) {
            const std::size_t cost = a[i - 1] == b[j - 1] ? 0 : 1;
            d[i][j] = std::min({d[i - 1][j] + 1, d[i][j - 1] + 1, d[i - 1][j - 1] + cost});
            if (i > 1 && j > 1 && a[i - 1] == b[j - 2] && a[i - 2] == b[j - 1]) d[i][j] = std::min(d[i][j], d[i - 2][j - 2] + 1);
        }
    }
    return d[a.size()][b.size()];
}

std::optional<std::string> closest(std::string_view word, const std::vector<std::string>& candidates) {
    std::optional<std::string> best;
    std::size_t best_distance = 3;
    bool tied = false;
    for (const std::string& candidate : candidates) {
        const std::size_t distance = edit_distance(word, candidate);
        if (distance < best_distance) {
            best = candidate;
            best_distance = distance;
            tied = false;
        } else if (distance == best_distance && best && *best != candidate) {
            tied = true;
        }
    }
    if (tied) return std::nullopt;
    return best;
}

}  // namespace cpm::args::detail
