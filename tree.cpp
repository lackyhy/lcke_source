#include "tree.h"
#include <algorithm>
#include <locale.h>

TreeNode::TreeNode(const fs::path& p, TreeNode* par)
    : path(p), parent(par), name(p.filename().string()) {
    is_dir = fs::is_directory(p);
    if (parent == nullptr) name = p.string(); // root
    expanded = false;
}

void TreeNode::expand() {
    if (!is_dir || expanded) return;
    children.clear();
    try {
        std::vector<fs::directory_entry> entries;
        for (auto& entry : fs::directory_iterator(path)) {
            entries.push_back(entry);
        }
        std::sort(entries.begin(), entries.end(), [](auto& a, auto& b) {
            return a.path().filename() < b.path().filename();
        });
        for (auto& entry : entries) {
            children.emplace_back(entry.path(), this);
        }
        expanded = true;
    } catch (...) {
        children.clear();
        expanded = true;
    }
}

void TreeNode::collapse() {
    children.clear();
    expanded = false;
}

void build_visible_tree(TreeNode& root, std::vector<VisibleNode>& visible, std::vector<bool> prefix_parts) {
    visible.push_back({&root, prefix_parts});
    if (root.expanded) {
        int count = root.children.size();
        for (int i = 0; i < count; i++) {
            bool is_branch = i < count - 1;
            auto new_prefix = prefix_parts;
            new_prefix.push_back(is_branch);
            build_visible_tree(root.children[i], visible, new_prefix);
        }
    }
}

TreeNode* find_node_by_path(TreeNode& root, const fs::path& target_path) {
    if (root.path == target_path) return &root;
    if (root.expanded) {
        for (auto& child : root.children) {
            TreeNode* res = find_node_by_path(child, target_path);
            if (res) return res;
        }
    }
    return nullptr;
}

std::string render_tree_line(const std::vector<bool>& prefix_parts, const std::string& name, bool is_dir, bool expanded) {
    setlocale(LC_ALL, "");
    std::string line;
    for (size_t i = 0; i + 1 < prefix_parts.size(); i++) {
        line += prefix_parts[i] ? "|   " : "    ";
    }
    if (!prefix_parts.empty()) {
        line += prefix_parts.back() ? "|-- " : "\\--";
    }
    line += name;
    return line;
}

void expand_path_to_node(TreeNode* node) {
    if (!node) return;
    TreeNode* current = node->parent;
    while (current) {
        if (!current->expanded) current->expand();
        current = current->parent;
    }
} 