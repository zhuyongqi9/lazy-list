#include <functional>
#include <stdexcept>
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
#include "file_utils.h" 
#include "global_var.h"
#include "search_bar.h"
#include "file_operation_dialog.h"
#include <sys/stat.h>
#include <spdlog/spdlog.h>

std::string home_page_info;

class FileEntryModel {
public:
    FileEntryModel(): file_entry(0) {
    }

    void list(std::filesystem::path &path) {
        try {
            this->file_entry = list_all(path, list_options::parent_path);
        } catch (std::runtime_error &e) {
            this->file_entry = std::vector<std::filesystem::directory_entry>(0);
            home_page_info = std::string("Failed to list files ") + e.what();
        }
    }

    void search(std::string name, std::filesystem::path path,int options) {
        this->file_entry = search_file(name, path, options);
    }

    void sorted_by_size(bool decrease) {
        std::function<bool(const std::filesystem::directory_entry &d1, const std::filesystem::directory_entry &d2)> f = [] (const std::filesystem::directory_entry &d1, const std::filesystem::directory_entry &d2)->bool {
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
                        auto s1 = d1.file_size();
                        auto s2 = d2.file_size();
                        return s1 > s2;
                    } catch (std::filesystem::filesystem_error &e) {
                        return false;
                    }
                }
            } catch (std::filesystem::filesystem_error &e) {
                return false;
            } catch (std::exception &e) {
                return false;
            }
        };


        if (decrease) {
            std::sort(file_entry.begin(), file_entry.end(), f);
        } else {
            std::sort(file_entry.begin(), file_entry.end(), [&](const std::filesystem::directory_entry &d1, const std::filesystem::directory_entry &d2) -> bool{
                return !f(d1, d2);
            });
        }
    }

    void filter_file_size(std::uint64_t min, std::uint64_t max) {
        std::vector<std::filesystem::directory_entry> res;

        for (auto &item : this->file_entry) {
            std::uint64_t size;

            if (item.is_directory()) { 
                try {
                    size = cacl_directory_size(item);
                } catch (std::filesystem::filesystem_error &e) {
                    size = 0;
                }
            } else size = item.file_size();

            if (size < min) {
                continue;
            }

            if (max != 0) {
                if (size > max) {
                    continue;
                }
            }
            res.push_back(item);
        }

        this->file_entry = res;
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
        std::string file_name_format = fmt::format("{{: <{}.{}}}", FILENAME_LENGTH_MAX, FILENAME_LENGTH_MAX);
        this->title = "  ";
        this->title += fmt::format(file_name_format, "name");
        this->title += fmt::format("{: <10}", "size");
        this->title += "   ";

        for (const auto &item : model.file_entry) {
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

class FilterBar {
public:
    FilterBar () {
        this->input_min_size = Input(&this->str_min_size, &this->str_min_size, InputOption::Default());
        this->container = Container::Horizontal({
            this->input_min_size,
        });

        this->compoent = Renderer(this->container, [&]() {
            if (this->str_min_size.size() > 0) {
                try {
                    this->min_size = std::stoll(this->str_min_size);
                } catch (std::invalid_argument &e) {
                    this->min_size = 0;
                } catch (std::out_of_range &e) {
                    this->min_size = 0;
                }
            } else {
                this->min_size = 0;
            }

            return hbox({
                text(" File size") | color(Color::Red),
                text(" >= ")| color(Color::Red),
                this->input_min_size->Render() | size(WIDTH, EQUAL, 4),
                text(" MB")| color(Color::Red),
            });
        });
    }

    Component compoent;
    Component container;
    Component input_min_size;
    std::string str_min_size;
    uint64_t min_size = 0;
};

class HomePage {
public:
    HomePage() {
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
        };


        this->view = Menu(&file_view.table, &file_view.selected, option);

        {
            auto option = CheckboxOption::Simple();
            option.transform = [&](const EntryState& s) {
                this->refresh_flag = true;
                auto suffix = std::string(s.state ? "↓" : "↑");  // NOLINT
                auto str = fmt::format("{: >5}Size{}","", suffix);
                auto t = text(str);
                return t;
            };
            this->file_size_checkbox = Checkbox("size", &this->file_size_decreased, option);
        }

        {
            auto option = CheckboxOption::Simple();
            option.transform = [](const EntryState& s) {
                auto t = text(s.label);
                return t;
            };
            this->file_time_drop = Dropdown({
                .radiobox = {&this->file_time_type, &this->file_time_selected},
                .checkbox = option,
                .transform =
                    [](bool open, Element checkbox, Element radiobox) {
                    if (open) {
                      return vbox({
                          checkbox | inverted,
                          radiobox | vscroll_indicator | frame |
                              size(HEIGHT, LESS_THAN, 10),
                          filler(),
                      });
                    } else {
                        return vbox({
                            checkbox,
                            filler(),
                        });
                    }
                },
            });
        }

        this->button_filter_bar = Button("Filter", [&]() {
            this->show_filter_bar = !this->show_filter_bar;
            this->refresh_flag = true;
        });

        this->container = Container::Stacked({
            search_bar.component,
            this->button_filter_bar,
            this->filter_bar.compoent,
            this->file_size_checkbox,
            this->file_time_drop,
            view,
        });


        this->component = Renderer(container, [&] {
            if (cur_folder_entry_change() 
            || refresh_flag_change() 
            || search_input_change()
            || min_size_change()
            ) 
                refresh_file_view();
            std::string title = search_bar.text().size() > 0 ? "Search Results" : "Files";

            std::vector<Element> elements(0);
            elements.push_back(text(fmt::format("Current Dir: {}", current_dir.string())) | bold);

            elements.push_back(
                hbox({
                window(text("Target File"), search_bar.component->Render()) ,
                    this->button_filter_bar->Render() | align_right,
                }) | size(HEIGHT, EQUAL, 3)
            );

            if (this->show_filter_bar) {
                elements.push_back(this->filter_bar.compoent->Render());
            }

            elements.push_back(
                window(text(title), vbox({
                    hbox({
                        text(fmt::format("  {: <40}", "Name")),
                        this->file_size_checkbox->Render(),
                        text("   "),
                        this->file_time_drop->Render(),
                    }),
                    view->Render() | frame ,
                })) | yflex 
            );

            elements.push_back(
                    text(fmt::format("info: {}", home_page_info)) | border | size(HEIGHT, EQUAL, 3)
            );
            elements.push_back(
                    text("[x]open menu; [q]quit; [Enter]enter into selected dir; [Esc]enter into parent path") |  size(HEIGHT, EQUAL, 1) | color(Color::Blue)
            );
            return vbox(elements);
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
            } else if (event ==  Event::Special("\x1B")) {
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
    Component file_time_drop;
    std::vector<std::string> file_time_type = {"Last Modified Time", "Created Time"};
    int file_time_selected;

    Component file_size_checkbox;
    bool file_size_decreased = true;

    FilterBar filter_bar;
    Component button_filter_bar;
    bool show_filter_bar;
    
    bool refresh_flag = false;

    std::filesystem::path current_dir =  std::filesystem::current_path();

    SearchBar search_bar = SearchBar();
    FileEntryModel file_model = FileEntryModel();
    FileEntryView file_view = FileEntryView();

    FileOperationDialog dialog = FileOperationDialog(this->current_dir);
    int show_options;

    std::time_t cur_dir_m_time = 0;

private:
    std::string last_search_input = "";
    uint64_t last_min_size = 0;
    bool min_size_change() {
        if (this->filter_bar.min_size != last_min_size) {
            last_min_size = this->filter_bar.min_size;
            return true;
        } else {
            return false;
        }
    }

    bool search_input_change() {
        if (last_search_input != this->search_bar.text()) {
            last_search_input = this->search_bar.text();
            return true;
        } else {
            return false;
        }
    }

    bool refresh_flag_change() {
        if (this->refresh_flag) {
            this->refresh_flag = false;
            return true;
        } else {
            return false;
        }
    }

    void update_file_time_show_options() {
        switch (file_time_selected) {
            case 0:
                this->show_options |= FileEntryView::show_options::last_modified_time;
                break;
            case 1:
                this->show_options |= FileEntryView::show_options::created_time;
                break;
            default:
                this->show_options |= FileEntryView::show_options::last_modified_time;
        }
    }

    bool cur_folder_entry_change() {
        struct stat s;
        if (stat(current_dir.string().c_str(), &s) == 0) {
            std::time_t time = s.st_mtime;
            if (time != cur_dir_m_time) {
                this->cur_dir_m_time = time;
                return true;
            } else {
                return false;
            }
        } else {
            return false;
        }
    }

    void refresh_file_view() {
        spdlog::debug("refresh  file view");
        this->show_options = 0;
        this->show_options |= FileEntryView::show_options::file_size;
        this->update_file_time_show_options();

        try {
            if (search_bar.text().length() > 0) {
                int search_options;
                if (search_bar.regex()) search_options |= search_options::regex;
                else if (search_bar.case_ignored())  search_options |= search_options::caseignored;

                file_model.search(search_bar.text(), current_dir, search_options);

                this->show_options |= FileEntryView::show_options::full_path;
                if (this->show_filter_bar) {
                    this->file_model.filter_file_size(this->filter_bar.min_size * MB, 0);
                    this->show_options |= FileEntryView::show_options::directory_size;
                }

                file_view.render(file_model, FileEntryView::show_options::search);

            } else if (search_bar.text().size() == 0) {
                file_model.list(current_dir);

                if (this->file_size_decreased) {
                    file_model.sorted_by_size(true);
                } else {
                    file_model.sorted_by_size(false);
                }

                if (this->show_filter_bar) {
                    this->file_model.filter_file_size(this->filter_bar.min_size * MB, 0);
                    this->show_options |= FileEntryView::show_options::directory_size;
                } 

                file_view.render(file_model, this->show_options);
            }

        } catch (std::exception &ex) {
            home_page_info = ex.what();
        }
        
    }
};