#ifndef FILE_OPERATION_H
#define FILE_OPERATION_H
#include "fmt/format.h"
#include "fmt/os.h"
#include <algorithm>
#include <exception>
#include <filesystem>
#include <fmt/core.h>
#include <fmt/ostream.h>
#include <stdexcept>
#include <system_error>
#include <fstream>
#include <thread>
#include "global_var.h"
#include "config_parser.h"

extern Config config;

class FileOperation {
public:
    virtual void perform() {
    }
};


class CreateFileOperation : public FileOperation {
public:
    CreateFileOperation(const std::filesystem::path &path):path(path) {
    }

    virtual void perform() {
        std::error_code ec;
        if (std::filesystem::exists(path, ec)) {
            throw std::runtime_error(fmt::format("File:{:} alreay exists", path.string()));
        } else {
            try {
                std::ofstream file(path.string());
            } catch (std::exception &e) {
                throw std::runtime_error(fmt::format("Failed to Create file"));
            }
        }
    }

private:
    std::filesystem::path path;
};

class CreateFolderOperation : public FileOperation {
public:
    CreateFolderOperation(const std::filesystem::path &path):path(path) {
    }

    virtual void perform() {
        std::error_code ec;
        if (std::filesystem::exists(path, ec)) {
            throw std::runtime_error(fmt::format("Diretory: {:} alreay exists", path.string()));
        } else {
            try {
                std::filesystem::create_directory(path);
            } catch (std::filesystem::filesystem_error &e) {
                throw std::runtime_error(fmt::format("Failed to Create Directory\n Reason: {}", e.what()));
            } 
        }

    }
private:
    std::filesystem::path path;
};

class SoftDeleteFileOperation : public FileOperation {
public:
    SoftDeleteFileOperation(const std::filesystem::path &path): path(path) {

    }

    virtual void perform() {
        std::error_code ec;
        if (!std::filesystem::exists(path, ec)) {
            // do nothing
        } else {
            try {
                //auto dir = std::filesystem::directory_entry(path);
                //if (dir.is_directory()) {
                //    std::filesystem::remove_all(path);
                //} else if (dir.is_regular_file()) {
                //    std::filesystem::remove(path);
                //} else {
                //    throw std::runtime_error(fmt::format("Failed to Delete {}\n Reason: not regular file", path.string()));
                //}
                std::string full_path = path.string();
                std::for_each(full_path.begin(), full_path.end(), [](char &c) {
                    if (c == '/')
                        c = '\\';
                });
                auto dst = std::filesystem::path(RECYCLE_BIN_PATH) / full_path;
                std::filesystem::rename(path, dst);
            } catch (std::filesystem::filesystem_error &e) {
                throw std::runtime_error(fmt::format("Failed to Delete File\n Reason: {}", e.what()));
            } 
        }
    }
private:
    std::filesystem::path path;
};

class DeleteFileOperation : public FileOperation {
public:
    DeleteFileOperation(const std::filesystem::path &path): path(path) {

    }

    virtual void perform() {
        std::error_code ec;
        if (!std::filesystem::exists(path, ec)) {
            // do nothing
        } else {
            try {
                auto dir = std::filesystem::directory_entry(path);
                if (dir.is_directory()) {
                    std::filesystem::remove_all(path);
                } else if (dir.is_regular_file()) {
                    std::filesystem::remove(path);
                } else {
                    throw std::runtime_error(fmt::format("Failed to Delete {}\n Reason: not regular file", path.string()));
                }
            } catch (std::filesystem::filesystem_error &e) {
                throw std::runtime_error(fmt::format("Failed to Delete File\n Reason: {}", e.what()));
            } catch (std::runtime_error &e) {
                throw e;
            }
        }
    }
private:
    std::filesystem::path path;
};

class CopyFileOperation : public FileOperation {
public:
    CopyFileOperation(const std::filesystem::path &src, const std::filesystem::path &dst): src(src), dst(dst) {
    }

    virtual void perform() {
        using namespace std::filesystem;
        std::error_code ec;
        if (exists(src, ec)) {
            if (exists(dst, ec)) {
                throw std::runtime_error(fmt::format("File {} already exists", dst.string())); 
            } else {
                std::filesystem::copy(src, dst, ec);
                if (ec.value() != 0) {
                    throw std::runtime_error(fmt::format("Failed to copy File to {}\n Reason: ", dst.string(), ec.message()));
                }
            }
        } else {
            if (ec.value() != 0) {
                throw std::runtime_error(fmt::format("Failed to Open: {}\n Reasion: {}", src.string(), ec.message()));
            } else {
                throw std::runtime_error(fmt::format("Source File: {}\n doesn't exist", src.string()));
            }
        }
    }
private:
    std::filesystem::path src;
    std::filesystem::path dst;
};

class MoveFileOperation : public FileOperation {
public:
    MoveFileOperation(const std::filesystem::path &src, const std::filesystem::path &dst):src(src), dst(dst) { 

    }
    virtual void perform() {
        std::error_code ec;
        std::filesystem::rename(src, dst);
        if (ec.value() != 0) {
            throw std::runtime_error("Failed to move file, reasion: " + ec.message());
        }
    }
private:
    std::filesystem::path src;
    std::filesystem::path dst;
};

class CompressFileOperation : public FileOperation {
public:
    CompressFileOperation(const std::filesystem::path &src, const std::filesystem::path &dst):src(src),dst(dst) {}

    virtual void perform() {
        auto cmd = fmt::format("zip -j -r {} {}", dst.string(), src.string());
        int ret = 0;
        auto f = [&] () {
            ret = std::system(cmd.c_str());
        };
        std::thread t(f);
        t.join();

        if (ret != 0) {
            throw std::runtime_error(fmt::format("Failed to Compress File err_code: {}", ret));
        }
    }
private:
    std::filesystem::path src;
    std::filesystem::path dst;
};

class ExtractFileOperation {
public:
    ExtractFileOperation(const std::filesystem::path &src, const std::filesystem::path &dst):src(src),dst(dst) {}

    virtual void perform() {
        using namespace std::filesystem;
        std::error_code ec;
        if (exists(src, ec)) {
            if (!exists(dst, ec)) {
                auto cmd = fmt::format("unzip {} -d {}", src.string(), dst.string());
                int ret = 0;
                auto f = [&] () {
                    ret = std::system(cmd.c_str());
                };
                std::thread t(f);
                t.join();

                if (ret != 0) {
                    throw std::runtime_error(fmt::format("Failed to Compress File err_code: {}", ret));
                }
            } else {
                throw std::runtime_error(fmt::format("Target File {} already exits, please delete first", dst.string()));
            }
        } else {
            throw std::runtime_error(fmt::format("File {} not exits", src.string()));
        }
    }

private:
    std::filesystem::path src;
    std::filesystem::path dst;
};

std::string filename_without_ext(const std::filesystem::path &path) {
    auto name = path.filename().string();
    std::string res;
    std::string::size_type pos = 0;
    if ((pos = name.find_last_of(".")) != std::string::npos) {
        return name.substr(0, pos);
    } else {
        return name;
    }
};

#endif
