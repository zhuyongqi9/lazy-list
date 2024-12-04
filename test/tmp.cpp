#include "file_utils.h"
#include "fmt/chrono.h"
#include <filesystem>
#include <fmt/core.h>
#include <fmt/format.h>
#include <sys/stat.h>
#include <vector>

using namespace std::filesystem;

class FileEntryView {
public:
    enum show_options {
        directory_size = 1,
        file_size = 1 << 2,
        full_path = 1 << 3,
        last_modified_time = 1 << 4,
        created_time = 1 << 5,

        list = file_size,
        search = full_path,
    };   

    void render(const std::vector<std::filesystem::directory_entry> &file_entry, int options) {
        table.clear();
        this->title = "  ";
        std::string file_name_format = fmt::format("{{: <{}.{}}}", 40, 40);
        this->title += fmt::format(file_name_format, "name");
        this->title += fmt::format("{: <10}", "size");
        this->title += "   ";

        for (const auto &item : file_entry) {
            std::string s = "";
            if (options & full_path) {
                s += fmt::format("{: <40}", item.path().string());
            } else {
                s += fmt::format(file_name_format, item.path().filename().string());
            }

            if (item.is_regular_file()) {
                if (options & file_size) {
                    std::string ssize = formatted_file_size(item.path());
                    s += ssize;
                }
            } else if (item.is_directory()) {
                if (options & show_options::directory_size) {
                    std::string ssize = formatted_directory_size(item);
                    s += ssize;
                } else {
                    s += fmt::format("{: >10}", "");
                }
            } else {
                s += fmt::format("{: >10}", "");
            }
            s += "   ";

            if (item.path().filename() != "..") {
                if (options & last_modified_time) {
                    struct stat f_stat;
                    if (stat(item.path().c_str(), &f_stat) == 0) {
                        std::time_t mtime = f_stat.st_mtime;
                        auto t = fmt::localtime(mtime);
                        s += fmt::format("{:%Y/%m/%d %H:%M:%S}", t);
                    }
                } else if (options & created_time) {
                    struct stat f_stat;
                    if (stat(item.path().c_str(), &f_stat) == 0) {
                        #ifdef _DARWIN_FEATURE_64_BIT_INODE
                            std::time_t mtime = f_stat.st_birthtime;
                        #else
                            std::time_t mtime = f_stat.st_ctime;
                        #endif
                        auto t = fmt::localtime(mtime);
                        s += fmt::format("{:%Y/%m/%d %H:%M:%S}", t);
                    }
                }
            }

            table.push_back(s);
        }

    }

    std::vector<std::string> table;
    std::string title = "";
    int selected;
};

void sorted_by_size(std::vector<std::filesystem::directory_entry> &file_entry, bool decrease) {
    std::function<bool(std::filesystem::directory_entry &d1, std::filesystem::directory_entry &d2)> f = [] (std::filesystem::directory_entry &d1, std::filesystem::directory_entry &d2)->bool {
        try {
            if (d1.is_directory() && d2.is_directory()) {
                if (d1.path().filename() == "..") {
                    return true;
                } else if (d2.path().filename() == "..") {
                    return false;
                } else {
                    return d1.path() > d2.path();
                }
            } else if (d1.is_directory()) {
                return true;
            } else if (d2.is_directory()) {
                return false;
            } else {
                try {
                    fmt::println("{} {}", d1.path().string(), d2.path().string());
                    auto s1 = d1.file_size();
                    auto s2 = d2.file_size();
                    return s1 > s2;
                } catch (std::filesystem::filesystem_error &e) {
                    fmt::println("{}", e.what());
                    return false;
                }
            }
        } catch (std::filesystem::filesystem_error &e) {
            return false;
        } catch (std::exception &e) {
            return false;
        }
    };

    for (auto &item : file_entry) {
        if (!(item.path().string() == "" ))fmt::print("s{}\n", item.path().string());
    }
                    auto s = &file_entry;

    if (decrease) {
        std::sort(file_entry.begin(), file_entry.end(), f);

    } else {
        std::sort(file_entry.begin(), file_entry.end(), [&]( std::filesystem::directory_entry &d1,  std::filesystem::directory_entry &d2) -> bool{
            return !f(d1, d2);
        });
    }
}

int main() {
    auto path = std::filesystem::path("/Users/zhuyongqi/Library");
    auto res = list_all(path);
    std::vector<std::string > v;

    FileEntryView view;

    //fmt::println("{}", fmt::join(v, "\n"));

    std::vector<std::filesystem::directory_entry> entry = { std::filesystem::directory_entry("/Users/zhuyongqi/Library"), std::filesystem::directory_entry("/Users/zhuyongqi/Movies")};
    sorted_by_size(res, true); 
    //view.render(res, FileEntryView::list | FileEntryView::last_modified_time | FileEntryView::directory_size);

    //for (auto &item : res) v.push_back(item.path());
    //fmt::println("{}", fmt::join(v, "\n"));
}