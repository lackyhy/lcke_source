#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct TreeNode {
    fs::path path;
    TreeNode* parent;
    std::string name;
    bool is_dir;
    bool expanded;
    std::vector<TreeNode> children;
    TreeNode(const fs::path& p, TreeNode* par = nullptr);
    void expand();
    void collapse();
};

struct VisibleNode {
    TreeNode* node;
    std::vector<bool> prefix_parts;
};

void build_visible_tree(TreeNode& root, std::vector<VisibleNode>& visible, std::vector<bool> prefix_parts = {});
TreeNode* find_node_by_path(TreeNode& root, const fs::path& target_path);
std::string render_tree_line(const std::vector<bool>& prefix_parts, const std::string& name, bool is_dir, bool expanded);
void expand_path_to_node(TreeNode* node); 