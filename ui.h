#pragma once
#include <string>
#include <filesystem>
#include <vector>
#include <ncurses.h>

std::string prompt_user(WINDOW* win, const std::string& prompt);
void show_message(WINDOW* win, const std::string& message);
void show_help(WINDOW* stdscr);
std::filesystem::path show_search_results(WINDOW* parent_win, const std::vector<std::pair<std::string, std::filesystem::path>>& results);
std::vector<std::string> get_command_output_in_path(const std::string& command, const std::filesystem::path& path); 