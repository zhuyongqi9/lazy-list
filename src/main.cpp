#include <filesystem>
#include <system_error>
#include <vector>
#include <string>
#include <fmt/core.h>
#include <iostream>
#include <fmt/color.h>
#include <spdlog/spdlog.h>
#include <regex>

std::error_code FAILED_OPEN_FILE;

std::vector<std::string> skipped_file = {
    "Photos Library.photoslibrary",
};

bool skipped(std::string &path) {
    for (auto &item : skipped_file) if (item == path) return true;
    return false;
}

enum options {
    regex = 1 << 1,
    caseignored = 1 << 2,
};

std::vector<std::filesystem::directory_entry> search_file(std::string &name, std::filesystem::path &path, int option) {
    auto file = std::filesystem::recursive_directory_iterator(path, std::filesystem::directory_options::skip_permission_denied);
    std::vector<std::filesystem::directory_entry> res(0);
    auto target = name;

    if (option & caseignored) {
        std::for_each(target.begin(), target.end(), [](char &c) {
            c = std::toupper(c);
        });
    }

    try {
        fmt::println("{}", option);
        fmt::println("{}", int(regex));
        for(auto it = file; it != std::filesystem::end(file); it++) {
            const std::filesystem::directory_entry &entry = *it;
            std::string file_name = entry.path().filename().string();
            if (skipped(file_name)) {
                it.disable_recursion_pending();
                continue;
            } else if (file_name.size() > 0 && file_name[0] == '.') {
                //fmt::print(fmt::bg(fmt::color::red), file_name);
                //fmt::print("\n");
                it.disable_recursion_pending();
                continue;
            }

            if (option & caseignored) {
                std::for_each(file_name.begin(), file_name.end(), [](char &c) {
                    c = std::toupper(c);
                });
            }


            if (option & regex) {
                fmt::println("regex");
                std::regex pattern(name);
                if (std::regex_match(file_name, pattern)) {
                    res.push_back(entry);
                }
            } else {
                if (file_name.find(target) == 0) {
                    res.push_back(entry);
                }
            }
        }
    } catch (const std::filesystem::filesystem_error &e) {
        
    }
    return res;
}



int main() {
    spdlog::set_level(spdlog::level::debug);
    std::string cur_dir = "/Users/zhuyongqi/Projects/lazy-list"; 

    while(true) {
        std::string file;
        fmt::print("enter file: ");
        std::cin >> file;
        if (file == "quit") return 0;

        std::filesystem::path path = std::filesystem::path(cur_dir);
        std::vector<std::filesystem::directory_entry> res = search_file(file, path, regex);

        if (res.size() > 0) {
            for (auto &item : res) {
                fmt::println("{}", item.path().string());
            }
        }
    }
}