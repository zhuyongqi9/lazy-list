#ifndef GLOBAL_VAR_H
#define GLOBAL_VAR_H
#include <string>
#include <filesystem>

const std::string home = std::getenv("HOME");
const std::filesystem::path p_home = std::filesystem::path(home);

const std::string CONFIG_PATH = p_home / ".lazylist" / "config.txt";
const std::string CONFIG_PATH_TMP = p_home / ".lazylist" / "config.bak"; 
const std::string RECYCLE_BIN_PATH = p_home / "recycle_bin";

const int KB = 1024;

const int MB = 1024 * KB;

const int FILENAME_LENGTH_MAX = 40;

const int GB = MB * 1024;
#endif