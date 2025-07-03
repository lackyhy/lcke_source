#pragma once
#include <string>
#include <filesystem>
#include <vector>

void safe_recursive_search(const std::filesystem::path& dir, const std::string& search_term, std::vector<std::pair<std::string, std::filesystem::path>>& results); 