#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
#include <fmt/format.h>
#include <fmt/core.h>
#include <fstream>
#include <filesystem>

class Config {
public:
    Config() {
        std::fstream s("config.txt", std::fstream::app);
        this->parse();
    } 

    std::string value_str(std::string &str) {
        auto pos = str.find('=') + 1;
        if (pos != std::string::npos) {
            return str.substr(pos, str.size() - pos);
        } else {
            return "";
        }
    }

    std::vector<std::string> value_vector(std::string &str) {
        auto pos = str.find('=') + 1;
        std::vector<std::string> res(0);
        if (pos != std::string::npos) {
            auto val = str.substr(pos, str.size() - pos);
            auto s = 0;
            auto e = val.find_first_of(';');
            while (e != std::string::npos) {
                res.push_back(val.substr(s, e - s));
                s = e + 1;
                e = val.find_first_of( ';', s );
            }
        } else {
        }
        return res;
    }

    int parse() {
        std::fstream config("config.txt", std::fstream::in);
        if (config) {
            std::string line;
            if (!(config >> line)) return -1;
            line = value_str(line);
            if (line.size() > 0) recycle_bin_path = line;

            if (!(config >> line)) return -1;
            line = value_str(line);
            if (line.size() > 0) recycle_bin_max_size = std::stoull(line);

            if (!(config >> line)) return -1;
            bookmark = value_vector(line);
        } else {
            fmt::print("failed to open\n");
        }

        return 0;
    }

    void store() {
        std::string temp = "";
        temp += fmt::format("recycle_bin_path={}\n", recycle_bin_path);
        temp += fmt::format("recycle_bin_max_size={}\n", recycle_bin_max_size);
        
        std::string s = "";
        for(auto &item : bookmark) s += item + ";";
        temp += fmt::format("bookmark={}\n", s);
        
        std::fstream out("config.bak", std::fstream::out|std::fstream::trunc);
        if (out) {
            out << temp;
            std::error_code ec;
            std::filesystem::rename("config.bak", "config.txt", ec);
            if (ec.value() != 0) {
                throw std::runtime_error("failed to save config file, error:" + ec.message());
            }
        } else {
            throw std::runtime_error("failed to create config file");
        }
    }

    void refresh() {
        this->parse();        
    }

    void debug() {
        fmt::print("recycle_bin_path: {}\n", recycle_bin_path);
        fmt::print("recycle_bin_max_size: {}\n", recycle_bin_max_size);
        fmt::print("bookmark: {}\n", fmt::join(bookmark, " "));
    }

    std::string recycle_bin_path = std::filesystem::path(std::getenv("HOME")) / "recycle_bin";
    uint64_t recycle_bin_max_size = 1024 * 1024 * 1024 * 10ll;
    std::vector<std::string> bookmark;
};
#endif
