#include "ui.h"
#include <algorithm>
#include <locale.h>

void show_message(WINDOW* win, const std::string& message) {
    werase(win);
    mvwprintw(win, 0, 0, "%s", message.c_str());
    wrefresh(win);
    wgetch(win);
}

std::string prompt_user(WINDOW* win, const std::string& prompt) {
    echo();
    nocbreak();
    werase(win);
    mvwprintw(win, 0, 0, "%s", prompt.c_str());
    wrefresh(win);
    char input[256];
    wgetnstr(win, input, 255);
    noecho();
    cbreak();
    return std::string(input);
}

std::filesystem::path show_search_results(WINDOW* parent_win, const std::vector<std::pair<std::string, std::filesystem::path>>& results) {
    int height, width;
    getmaxyx(parent_win, height, width);

    int win_height = std::min((int)results.size() + 2, height - 4);
    int win_width = width - 4;
    WINDOW* win = newwin(win_height, win_width, 2, 2);
    box(win, 0, 0);

    int selected = 0;

    keypad(win, TRUE);
    while (true) {
        werase(win);
        box(win, 0, 0);
        for (int i = 0; i < std::min((int)results.size(), win_height - 2); i++) {
            std::string line = results[i].first + "    " + results[i].second.string();
            if (i == selected) {
                wattron(win, A_REVERSE);
                mvwprintw(win, i + 1, 1, "%.*s", win_width - 2, line.c_str());
                wattroff(win, A_REVERSE);
            } else {
                mvwprintw(win, i + 1, 1, "%.*s", win_width - 2, line.c_str());
            }
        }
        wrefresh(win);

        int ch = wgetch(win);
        if (ch == 'k' && selected > 0) selected--;
        else if (ch == 'j' && selected < (int)results.size() - 1 && selected < win_height - 3) selected++;
        else if (ch == 10 || ch == 13) { // Enter
            delwin(win);
            return results[selected].second;
        } else if (ch == 27) { // ESC
            delwin(win);
            return std::filesystem::path();
        }
    }
}

void show_help(WINDOW* stdscr) {
    clear();
    const char* help_text[] = {
        "Help:",
        "",
        "Navigation:",
        "  Up/Down arrows - move selection",
        "  Enter          - expand/collapse directory or open file menu",
        "  /              - search files by name",
        "  c or C         - run command in selected file's directory",
        "  h              - show this help screen",
        "  q              - quit",
        "",
        "Search menu:",
        "  k              - move up in search results",
        "  j              - move down in search results",
        "  Enter          - select file from search results",
        "  Esc            - cancel search",
        "",
        "Commands:",
        "  open_explorer   - open in gnome-files",
        "  gnome-terminal  - open path in terminal",
        "",
        "Split mode:",
        "  2              - open split menu   ( ls -l )",
        "",
        "Press any key to return..."
    };

    int height, width;
    getmaxyx(stdscr, height, width);

    for (size_t i = 0; i < sizeof(help_text)/sizeof(help_text[0]); ++i) {
        if ((int)i >= height - 1) break;
        mvwprintw(stdscr, i, 0, "%s", help_text[i]);
    }
    wrefresh(stdscr);
    wgetch(stdscr);
    clear();
    refresh();
} 