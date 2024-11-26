#include <chrono>
#include <streambuf>
#include <string>
#include <exception>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/dom/table.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/component/component.hpp>
#include <filesystem>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/chrono.h>
#include <vector>
#include "MESSAGE.h"
#include "file_utils.h" 
#include "search_bar.h"
#include "file_operation_dialog.h"
#include <sys/stat.h>

std::string home_page_info;

class FileEntryModel {
public:
    enum search_options {
        regex = 1 << 1,
        caseignored = 1 << 2,
    };

    FileEntryModel(): file_entry(0) {
    }

    void list(std::filesystem::path &path) {
        try {
            file_entry = list_all(path);
//            sorted_by_size();
        } catch (std::filesystem::filesystem_error &e) {
            file_entry.clear();
        }
    }

    void search(std::string name, std::filesystem::path path,int options) {
        file_entry = search_file(name, path, options);
    }

    void sorted_by_size() {
        std::sort(file_entry.begin(), file_entry.end(), [](const std::filesystem::directory_entry &d1, const std::filesystem::directory_entry &d2)->bool{
            if (d1.is_directory() && d2.is_directory()) {
                if (d2.path().filename() == "..") {
                    return false;
                } else {
                    return true;
                }
            } else if (d1.is_directory()) {
                return true;
            } else if (d2.is_directory()) {
                return false;
            } else {
                try {
                    auto s1 = std::filesystem::file_size(d1);
                    auto s2 = std::filesystem::file_size(d2);
                    return s1 > s2;
                } catch (std::filesystem::filesystem_error &e) {
                    return false;
                }
            }
        });
    }

    std::vector<std::filesystem::directory_entry> file_entry;

private:
};


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

    void render(FileEntryModel &model, int options) {
        table.clear();
        this->title = "  ";
        this->title += fmt::format("{: <40}", "name");
        this->title += fmt::format("{: <10}", "size");
        this->title += "   ";

        if (options & last_modified_time) {
            this->title += fmt::format("{: <17}", "Last Modified Time");
        } else if (options & created_time) {
            this->title += fmt::format("{: <17}", "Created Time");
        }

        for (const auto &item : model.file_entry) {
            std::string s = "";
            if (options & full_path) {
                s += fmt::format("{: <40}", item.path().string());
            } else {
                s += fmt::format("{: <40}", item.path().filename().string());
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


class HomePage {
public:
    HomePage() {
        this->options = FileEntryView::show_options::list | FileEntryView::show_options::created_time;

        file_model.list(current_dir);
        file_view.render(file_model, this->options);

        MenuOption option = MenuOption::Vertical();

        option.entries_option.transform = [&] (EntryState state) {
            Element e = text((state.active ? "> " : "  ") + state.label);  // NOLINT

            try {
                if (state.index == 0) {
                    e |= color(Color::Green);
                } else {
                    if (file_model.file_entry[state.index].is_directory()) {
                        e |= color(Color::Green);
                    }
                }
            } catch (std::filesystem::filesystem_error &e) {

            }

            if (state.focused) {
              e |= inverted;
            }
            if (state.active) {
              e |= bold;
            }
            if (!state.focused && !state.active) {
              //e |= dim;
            } 

            return e;
        };

        option.on_enter = [&]()mutable {
            if (file_view.selected == 0) {
                current_dir = current_dir.parent_path();
            } else {
                auto dir = file_model.file_entry[file_view.selected];
                try {
                    if (dir.is_directory()) {
                        current_dir = dir.path();
                    }
                } catch (std::filesystem::filesystem_error &e) {
                }
            }

            file_model.list(current_dir);
            file_view.render(file_model, this->options);
        };

        this->view = Menu(&file_view.table, &file_view.selected, option);

        this->container = Container::Stacked({
            search_bar.component,
            view,
        });

        this->component = Renderer(container, [&] {
            std::vector<std::filesystem::directory_entry> res;

            try {
                if (search_bar.text().length() > 0) {
                    int options;
                    if (search_bar.regex()) options |= search_options::regex;
                    else if (search_bar.case_ignored())  options |= search_options::caseignored;

                    file_model.search(search_bar.text(), current_dir, options);
                    file_view.render(file_model, FileEntryView::show_options::search);
                } else if (search_bar.text().size() == 0) {
                    file_model.list(current_dir);
                    file_view.render(file_model, this->options);
                }
            } catch (std::exception &ex) {
                home_page_info = ex.what();
            }

            std::string title = search_bar.text().size() > 0 ? "Search Results" : "Files";

            return gridbox({
                {
                    text(fmt::format("Current Dir: {}", current_dir.string())) | bold
                },
                {
                    window(text("Target File"), search_bar.component->Render()) 
                }, {
                    window(text(title), vbox({
                        text(file_view.title),
                        view->Render() | frame | size(HEIGHT, EQUAL, 28),
                    }))
                }, {
                    text(fmt::format("info: {}", home_page_info)) | border
                }        
            });
        });

        this->component |= Modal(dialog.component, &dialog.shown); 
        this->component |= CatchEvent([&](Event event) {
        if (event == Event::Character('x')) {
            try {
                auto dir = file_model.file_entry[file_view.selected];
                home_page_info = dir.path().filename().string() + "test";
                dialog.udpate_path(dir);
                dialog.shown = true;
            } catch (std::exception &e) {
                home_page_info = e.what();
            }
            return true;
        }

        if (event ==  Event::Special("\x1B")) {
            if (dialog.shown) return false;
            else {
                current_dir = current_dir.parent_path();
                return true;
            }
        }
        return false;
    });
    }

    Component component;
    Component container;
    Component view;
    
    std::filesystem::path current_dir =  std::filesystem::current_path();

    SearchBar search_bar = SearchBar();
    FileEntryModel file_model = FileEntryModel();
    FileEntryView file_view = FileEntryView();

    FileOperationDialog dialog = FileOperationDialog(this->current_dir);
    int options;
};