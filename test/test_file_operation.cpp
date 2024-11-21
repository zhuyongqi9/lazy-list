#include <exception>
#include <file_operation.h>
#include <filesystem>
#include <stdexcept>
#include <system_error>
#include "fmt/core.h"
#include <memory>
#include "fmt/color.h"


int main() {
    std::error_code ec;
    std::filesystem::path cur_dir = std::filesystem::current_path();

    std::unique_ptr<FileOperation> operation;
    

    try {
        cur_dir = cur_dir / "test_dir";
        if (std::filesystem::exists(cur_dir)) {
            operation = std::make_unique<DeleteFileOperation>(cur_dir);
            operation->perform();
            if (std::filesystem::exists(cur_dir)) {
                throw std::runtime_error("expected delete " + cur_dir.string());
            }
        }

        operation = std::make_unique<CreateFolderOperation>(cur_dir);
        operation->perform();
        if (!std::filesystem::exists(cur_dir)) {
            throw std::runtime_error("expcet test_dir folder created");
        }

        auto file = cur_dir / "test.txt";
        operation = std::make_unique<CreateFileOperation>(file);
        operation->perform();

        if (!std::filesystem::exists(file)) {
            throw std::runtime_error("expect test.txt created");
        }

        auto file2 = cur_dir / "test2.txt";
        operation = std::make_unique<CopyFileOperation>(file, file2);
        operation->perform();
        if (!std::filesystem::exists(file2)) {
            throw std::runtime_error("expect test2.txt created");
        }

        auto file3 = cur_dir /"test3.txt";
        operation = std::make_unique<MoveFileOperation>(file2, file3);
        operation->perform();
        if (std::filesystem::exists(file3)) {
            if (std::filesystem::exists(file2)) {
                throw std::runtime_error("expect test2.txt created");
            }
        } else {
            throw std::runtime_error("expect test3.txt created");
        }


        fmt::print(fmt::fg(fmt::color::green), "All Test Passed!\n");

    } catch (std::exception &e) {
        auto s = fmt::format(fmt::emphasis::bold| bg(fmt::color::red) | fg(fmt::color::white), e.what());        
        fmt::println("Test Failed: {}", s);
    }
}