#include "fmt/format.h"
#include "fmt/os.h"
#include <exception>
#include <filesystem>
#include <fmt/core.h>
#include <fmt/ostream.h>
#include <stdexcept>
#include <system_error>
#include <fstream>

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
        auto copy = CopyFileOperation(src, dst);
        copy.perform();
        auto del = DeleteFileOperation(src);
        del.perform();
    }
private:
    std::filesystem::path src;
    std::filesystem::path dst;
};

