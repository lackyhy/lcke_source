#include "commands.h"
#include <unistd.h>
#include <sys/wait.h>
#include <iostream>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <vector>
#include <string>
#include <cstdio>

void run_command_in_path(const std::string& command, const std::filesystem::path& path) {
    pid_t pid = fork();
    if (pid == 0) {
        std::string dir = path.string();
        if (!std::filesystem::is_directory(path)) {
            dir = path.parent_path().string();
        }
        if (chdir(dir.c_str()) != 0) {
            std::cerr << "chdir failed: " << strerror(errno) << std::endl;
            _exit(1);
        }
        execl("/bin/sh", "sh", "-c", command.c_str(), (char*) nullptr);
        std::cerr << "execl failed: " << strerror(errno) << std::endl;
        _exit(1);
    } else if (pid > 0) {
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            std::cerr << "waitpid failed: " << strerror(errno) << std::endl;
        } else {
            if (WIFEXITED(status)) {
                int exit_code = WEXITSTATUS(status);
                if (exit_code != 0) {
                    std::cerr << "Child process exited with code " << exit_code << std::endl;
                }
            } else if (WIFSIGNALED(status)) {
                std::cerr << "Child process killed by signal " << WTERMSIG(status) << std::endl;
            }
        }
    } else {
        std::cerr << "fork failed: " << strerror(errno) << std::endl;
    }
}

std::vector<std::string> get_command_output_in_path(const std::string& command, const std::filesystem::path& path) {
    std::vector<std::string> output;
    std::string dir = path.string();
    if (!std::filesystem::is_directory(path)) {
        dir = path.parent_path().string();
    }
    std::string full_command = "cd '" + dir + "' && " + command + " 2>&1";
    FILE* pipe = popen(full_command.c_str(), "r");
    if (!pipe) return output;
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        output.emplace_back(buffer);
    }
    pclose(pipe);
    return output;
} 