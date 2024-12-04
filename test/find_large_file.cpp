#include "fmt/format.h"
#include <__filesystem/recursive_directory_iterator.h>
#include <fmt/core.h>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <global_var.h>
#include <sys/_types/_uid_t.h>
#include <sys/stat.h>
#include <unistd.h>
#include "file_utils.h"

bool isOwnedByCurrentUser(const std::filesystem::path &path) {
    struct stat s;
    if (stat(path.c_str(), &s) == 0) {
        uid_t uid = geteuid();
        return uid == s.st_uid;
    } else {
        throw std::runtime_error(fmt::format("failed to stat {}", path.string()));
    }
}

std::vector<std::filesystem::directory_entry> find_large_file(const std::filesystem::path &p, int n) {
    std::vector<std::filesystem::directory_entry> res;
    try {
        auto start = std::filesystem::recursive_directory_iterator(p, std::filesystem::directory_options::skip_permission_denied | std::filesystem::directory_options::follow_directory_symlink);
        for (auto it = start; it != std::filesystem::end(start); it++) {
            auto item = *it;
            if (skipped(item.path())) {
                fmt::println("ss");
                it.disable_recursion_pending();
                continue;
            } 
            

            if (item.is_regular_file()) {
                if (item.file_size() > n * GB) {
                    res.push_back(item);
                } 
            }
        }
    } catch (std::filesystem::filesystem_error &e) {
        throw std::runtime_error("failed to find larget file" + std::string(e.what()));
    }
    return res;
}

int main() {
    std::uint64_t res = cacl_directory_size("/opt/compiler-explorer");
    fmt::println("{}", res / 1024.0 / 1024 / 1024);
}