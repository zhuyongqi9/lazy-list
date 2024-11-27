#ifndef GLOBAL_VAR_H
#define GLOBAL_VAR_H
#include <string>
#include <filesystem>

static std::string home = std::getenv("HOME");
const std::string RECYCLE_BIN_PATH = std::filesystem::path(home) / "recycle_bin";

const int MB = 1024 * 1024;

#endif