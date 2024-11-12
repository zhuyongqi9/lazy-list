#include <__filesystem/recursive_directory_iterator.h>
#include <filesystem>
#include <system_error>
#include <vector>
#include <string>
#include <fmt/core.h>
#include <iostream>
#include <fmt/color.h>

std::error_code FAILED_OPEN_FILE;

std::vector<std::string> skipped_file = {
    "Photos Library.photoslibrary",
};

bool skipped(std::string &path) {
    for (auto &item : skipped_file) if (item == path) return true;
    return false;
}

std::vector<std::filesystem::directory_entry> search_file(std::string &name, std::filesystem::path &path) {
    auto file = std::filesystem::recursive_directory_iterator(path, std::filesystem::directory_options::skip_permission_denied);
    std::vector<std::filesystem::directory_entry> res(0);


    for(auto it = file; it != std::filesystem::end(file); it++) {
        const std::filesystem::directory_entry &entry = *it;
        std::string file_name = entry.path().filename().string();
        if (skipped(file_name)) {
            it.disable_recursion_pending();
            continue;
        }

        if (file_name.size() > 0 && file_name[0] == '.') {
            //fmt::print(fmt::bg(fmt::color::red), file_name);
            //fmt::print("\n");
            it.disable_recursion_pending();
            continue;
        }
        //fmt::println(entry.path().string());
        if (file_name.find(name) == 0) {
            res.push_back(entry);
        }
    }
    return res;
}

int main() {
    std::string cur_dir = "/Users/zhuyongqi/Projects"; 

    while(true) {
        std::string file;
        fmt::print("enter file: ");
        std::cin >> file;
        if (file == "quit") return 0;

        std::filesystem::path path = std::filesystem::path(cur_dir);
        std::vector<std::filesystem::directory_entry> res = search_file(file, path);

        if (res.size() > 0) {
            for (auto &item : res) {
                fmt::println("{}", item.path().string());
            }
        }
    }
}