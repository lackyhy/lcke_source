#include "search.h"
#include <algorithm>

void safe_recursive_search(const std::filesystem::path& dir, const std::string& search_term, std::vector<std::pair<std::string, std::filesystem::path>>& results) {
    const size_t MAX_RESULTS = 150;
    std::error_code ec;
    if (!std::filesystem::exists(dir, ec) || !std::filesystem::is_directory(dir, ec)) return;

    for (auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        if (ec) {
            ec.clear();
            continue;
        }

        auto path = entry.path();
        std::string name = path.filename().string();
        std::string name_lower = name;
        std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);

        std::string search_lower = search_term;
        std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(), ::tolower);

        if (name_lower.find(search_lower) != std::string::npos) {
            results.emplace_back(name, path);
            if (results.size() >= MAX_RESULTS) return;
        }

        if (entry.is_directory(ec) && !ec) {
            safe_recursive_search(path, search_term, results);
            if (results.size() >= MAX_RESULTS) return;
        }
    }
} 