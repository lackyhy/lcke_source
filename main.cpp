#include "tree.h"
#include "ui.h"
#include "search.h"
#include "commands.h"
#include <ncurses.h>
#include <locale.h>
#include <iostream>
#include <algorithm>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <pty.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <vector>
#include <string.h>
#include <cstdlib>

bool split_mode = false; // глобальный флаг

int main() {
    setlocale(LC_ALL, "");
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    refresh();

    curs_set(0);

    if (!has_colors()) {
        endwin();
        std::cerr << "Terminal does not support colors\n";
        return 1;
    }
    start_color();
    init_pair(1, COLOR_BLUE, COLOR_BLACK);
    init_pair(2, COLOR_CYAN, COLOR_BLACK);
    init_pair(3, COLOR_WHITE, COLOR_BLACK);
    init_pair(4, COLOR_BLACK, COLOR_WHITE);
    init_pair(5, COLOR_GREEN, COLOR_BLACK);

    fs::path root_path = "/";
    TreeNode root(root_path);
    root.expand();

    int selected_index = 0;
    int scroll_offset = 0;

    while (true) {
        int height, width;
        getmaxyx(stdscr, height, width);

        std::vector<VisibleNode> visible_nodes;
        build_visible_tree(root, visible_nodes);
        int n_visible = visible_nodes.size();

        if (selected_index < scroll_offset) scroll_offset = selected_index;
        else if (selected_index >= scroll_offset + height - 1) scroll_offset = selected_index - (height - 2);

        if (!split_mode) {
            clear();
            // Обычный режим — дерево на весь экран
            for (int i = scroll_offset; i < std::min(scroll_offset + height - 1, n_visible); i++) {
                auto& v = visible_nodes[i];
                std::string line = render_tree_line(v.prefix_parts, v.node->name, v.node->is_dir, v.node->expanded);
                if (i == selected_index) {
                    attron(COLOR_PAIR(4));
                    mvprintw(i - scroll_offset, 0, "%s", line.c_str());
                    attroff(COLOR_PAIR(4));
                } else {
                    if (v.node->is_dir) {
                        attron(COLOR_PAIR(v.node->expanded ? 2 : 1));
                        mvprintw(i - scroll_offset, 0, "%s", line.c_str());
                        attroff(COLOR_PAIR(v.node->expanded ? 2 : 1));
                        if (v.node->expanded) {
                            attron(COLOR_PAIR(5)); // зелёный
                            mvprintw(i - scroll_offset, (int)line.size(), " [+]");
                            attroff(COLOR_PAIR(5));
                        }
                    } else {
                        std::string name_lower = v.node->name;
                        std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);

                        bool is_py = name_lower.size() >= 3 && name_lower.compare(name_lower.size() - 3, 3, ".py") == 0;
                        bool is_bat = name_lower.size() >= 4 && name_lower.compare(name_lower.size() - 4, 4, ".bat") == 0;

                        if (is_py || is_bat) {
                            attron(COLOR_PAIR(5));  // зелёный цвет для .py и .bat
                        } else {
                            attron(COLOR_PAIR(3));  // стандартный цвет для файлов
                        }

                        mvprintw(i - scroll_offset, 0, "%s", line.c_str());

                        if (is_py || is_bat) {
                            attroff(COLOR_PAIR(5));
                        } else {
                            attroff(COLOR_PAIR(3));
                        }
                    }
                }
            }

            auto& current_node = visible_nodes[selected_index].node;
            std::string path_str = " Path: " + current_node->path.string() + " ";
            attron(COLOR_PAIR(4));
            mvhline(height - 1, 0, ' ', width);
            mvprintw(height - 1, 0, "%s", path_str.substr(0, width - 1).c_str());
            attroff(COLOR_PAIR(4));
        } else {
            // Два окна: слева дерево, справа терминал
            int tree_width = width * 0.4;
            int term_width = width - tree_width;

            WINDOW* tree_win = newwin(height, tree_width, 0, 0);
            WINDOW* term_win = newwin(height, term_width, 0, tree_width);

            box(tree_win, 0, 0);
            for (int i = scroll_offset; i < std::min(scroll_offset + height - 2, n_visible); i++) {
                auto& v = visible_nodes[i];
                std::string line = render_tree_line(v.prefix_parts, v.node->name, v.node->is_dir, v.node->expanded);
                if (i == selected_index) {
                    wattron(tree_win, COLOR_PAIR(4));
                    mvwprintw(tree_win, i - scroll_offset + 1, 1, "%s", line.c_str());
                    wattroff(tree_win, COLOR_PAIR(4));
                } else {
                    if (v.node->is_dir) {
                        wattron(tree_win, COLOR_PAIR(v.node->expanded ? 2 : 1));
                        mvwprintw(tree_win, i - scroll_offset + 1, 1, "%s", line.c_str());
                        wattroff(tree_win, COLOR_PAIR(v.node->expanded ? 2 : 1));
                        if (v.node->expanded) {
                            wattron(tree_win, COLOR_PAIR(5));
                            mvwprintw(tree_win, i - scroll_offset + 1, (int)line.size() + 1, " [+]");
                            wattroff(tree_win, COLOR_PAIR(5));
                        }
                    } else {
                        std::string name_lower = v.node->name;
                        std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);

                        bool is_py = name_lower.size() >= 3 && name_lower.compare(name_lower.size() - 3, 3, ".py") == 0;
                        bool is_bat = name_lower.size() >= 4 && name_lower.compare(name_lower.size() - 4, 4, ".bat") == 0;

                        if (is_py || is_bat) {
                            wattron(tree_win, COLOR_PAIR(5));
                        } else {
                            wattron(tree_win, COLOR_PAIR(3));
                        }

                        mvwprintw(tree_win, i - scroll_offset + 1, 1, "%s", line.c_str());

                        if (is_py || is_bat) {
                            wattroff(tree_win, COLOR_PAIR(5));
                        } else {
                            wattroff(tree_win, COLOR_PAIR(3));
                        }
                    }
                }
            }
            mvwprintw(tree_win, height - 1, 1, "Tree");

            box(term_win, 0, 0);

            // Выводим ls -l -a для выделенной папки
            auto& current_node = visible_nodes[selected_index].node;
            std::vector<std::string> ls_output = get_command_output_in_path("ls -l -a", current_node->path);
            int max_lines = height - 2; // оставим место для рамки
            for (int i = 0; i < (int)ls_output.size() && i < max_lines; ++i) {
                // Обрезаем перевод строки, если есть
                std::string line = ls_output[i];
                if (!line.empty() && line.back() == '\n') line.pop_back();
                mvwprintw(term_win, i + 1, 1, "%.*s", term_width - 2, line.c_str());
            }
            mvwprintw(term_win, height - 1, 2, "'2' to return");

            wrefresh(tree_win);
            wrefresh(term_win);
            delwin(tree_win);
            delwin(term_win);
        }

        refresh();

        int key = getch();
        if (key == KEY_UP) {
            if (selected_index > 0) selected_index--;
        } else if (key == KEY_DOWN) {
            if (selected_index < n_visible - 1) selected_index++;
        } else if (key == 10 || key == 13) { // Enter
            if (visible_nodes[selected_index].node->is_dir) {
                if (visible_nodes[selected_index].node->expanded)
                    visible_nodes[selected_index].node->collapse();
                else
                    visible_nodes[selected_index].node->expand();
            } else {
                show_message(stdscr, "File actions menu not implemented in this example.");
            }
        } else if (key == '/') {
            WINDOW* input_win = newwin(1, COLS, LINES - 1, 0);
            std::string search_term = prompt_user(input_win, "Search files by name (':exit' to exit): ");
            delwin(input_win);

            if (!search_term.empty()) {
                std::vector<std::pair<std::string, fs::path>> results;
                clear();
                mvprintw(0, 0, "Searching, please wait...");
                refresh();

                safe_recursive_search("/", search_term, results);

                if (results.empty()) {
                    show_message(stdscr, "No files found matching '" + search_term + "'");
                } else {
                    fs::path selected_path = show_search_results(stdscr, results);
                    
                    if (!selected_path.empty()) {
                        // Найти узел в дереве
                        TreeNode* node = find_node_by_path(root, selected_path);
                        if (!node) {
                            // Если узел не найден, раскрываем дерево по пути вручную
                            fs::path p = selected_path;
                            std::vector<std::string> parts;
                            for (const auto& part : p) {
                                parts.push_back(part.string());
                            }
                            TreeNode* current = &root;
                            for (size_t i = 1; i < parts.size(); ++i) { // пропускаем корень "/"
                                if (!current->expanded) current->expand();
                                bool found = false;
                                for (auto& child : current->children) {
                                    if (child.name == parts[i]) {
                                        current = &child;
                                        found = true;
                                        break;
                                    }
                                }
                                if (!found) break;
                            }
                            node = current;
                        }
                        // Раскрыть путь к узлу
                        expand_path_to_node(node);

                        // Обновить видимые узлы
                        std::vector<VisibleNode> visible_nodes;
                        build_visible_tree(root, visible_nodes);

                        // Найти индекс узла в visible_nodes
                        int new_selected_index = 0;
                        for (size_t i = 0; i < visible_nodes.size(); ++i) {
                            if (visible_nodes[i].node->path == node->path) {
                                new_selected_index = i;
                                break;
                            }
                        }

                        // Установить выделение и прокрутку
                        selected_index = new_selected_index;
                        scroll_offset = std::max(0, selected_index - (height / 2));
                    }
                }
            }
        } else if (key == 'c' || key == 'C') {
                auto& current_node = visible_nodes[selected_index].node;
                WINDOW* input_win = newwin(1, COLS, LINES - 1, 0);
                std::string command = prompt_user(input_win, "Enter command to run in " + current_node->path.string() + ": ");
                delwin(input_win);
                  
                if (command == "open_explorer") {
                    std::string cmd = "xdg-open \"" + current_node->path.string() + "\"";
                    run_command_in_path(cmd, current_node->path);
                } else if (!command.empty()) {
                    run_command_in_path(command, current_node->path);
                    getch();
                }
            } 
        else if (key == 'q') {
            break;
        } else if (key == 'h' || key == 'H') {
            show_help(stdscr);
        } else if (key == '2' && !split_mode) {
            split_mode = true;
            continue;
        } else if (key == '2' && split_mode) {
            split_mode = false;
            continue;
        }
    }

    endwin();
    return 0;
} 