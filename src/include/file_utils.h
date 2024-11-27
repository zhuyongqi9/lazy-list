#ifndef FILE_UTILS_H
#define FILE_UTILS_H
#include <string>
#include <cstdint>
#include <filesystem>
#include <regex>
#include <fmt/core.h>
#include <fmt/format.h>
#include <vector>

static std::string formatted_size(uint64_t size) {
    if (size < 1024) {
    // Byte
        return fmt::format("{:>8} B", size);
    } else if (size < 1024 * 1024) {
    // KB
        double res = size / 1024.0;
        return fmt::format("{:>8.2f}KB", res);
    } else if (size < 1024 * 1024 * 1024) {
    // MB
        double res = size / 1024.0 / 1024;
        return fmt::format("{:>8.2f}MB", res);
    } else {
    //GB
        double res = size / 1024.0 / 1024 / 1024;
        return fmt::format("{:>8.2f}GB", res);
    }
} 

static uint64_t cacl_directory_size(const std::filesystem::path &path) {
    auto it = std::filesystem::recursive_directory_iterator(path);
    uint64_t res = 0;
    for (auto start = it; it != std::filesystem::end(it); it++) {
        if (it->is_regular_file())
            res += (*it).file_size();
    }
    return res;
}

std::string formatted_directory_size(const std::filesystem::path &path) {
    uint64_t res = cacl_directory_size(path);
    return formatted_size(res);
}

std::string formatted_file_size(const std::filesystem::path &path) {
    try {
        auto entry = std::filesystem::directory_entry(path);
        uint64_t size;
        if (entry.is_regular_file()) {
            size = entry.file_size();
        } else {
            size = 0;
        }
        return formatted_size(size);
    } catch (const std::exception &e) {
        //return fmt::format("{:>6}NULL", e.what());
        return fmt::format("{:>6}NULL", "");
    }
}

std::vector<std::string> skipped_file = {
    "Photos Library.photoslibrary",
};

enum search_options {
    regex = 1 << 1,
    caseignored = 1 << 2,
};

bool skipped(std::string &path) {
    for (auto &item : skipped_file) if (item == path) return true;
    return false;
}

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
        for(auto it = file; it != std::filesystem::end(file); it++) {
            const std::filesystem::directory_entry &entry = *it;
            std::string file_name = entry.path().filename().string();
            if (skipped(file_name)) {
                it.disable_recursion_pending();
                continue;
            } else if (file_name.size() > 0 && file_name[0] == '.') {
                it.disable_recursion_pending();
                continue;
            }

            if (option & caseignored) {
                std::for_each(file_name.begin(), file_name.end(), [](char &c) {
                    c = std::toupper(c);
                });
            }

            if (option & regex) {
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
        
    } catch (const std::regex_error &e) {

    }
    return res;
}


std::vector<std::filesystem::directory_entry> list_all(std::filesystem::path path) {
    auto start = std::filesystem::directory_iterator(path, std::filesystem::directory_options::skip_permission_denied);
    std::vector<std::filesystem::directory_entry> res;
    res.push_back(std::filesystem::directory_entry(".."));
    try {
        for (auto it = start; it != std::filesystem::end(start); it++) {
            auto entry = *it;
            res.push_back(entry);
        }
    } catch (std::filesystem::filesystem_error &e) {
    }
    return res;
}

std::vector<std::filesystem::directory_entry> list_all_no_cur(std::filesystem::path path) {
    auto start = std::filesystem::directory_iterator(path, std::filesystem::directory_options::skip_permission_denied);
    std::vector<std::filesystem::directory_entry> res;
    try {
        for (auto it = start; it != std::filesystem::end(start); it++) {
            auto entry = *it;
            res.push_back(entry);
        }
    } catch (std::filesystem::filesystem_error &e) {
    }
    return res;
}



#endif