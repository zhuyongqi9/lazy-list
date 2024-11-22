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
#include <memory>
#include <string>
#include <vector>
#include "MESSAGE.h"
#include "file_utils.h" 
#include "search_bar.h"
#include "recycle_bin_page.h"
#include "file_operation_dialog.h"
#include <spdlog/spdlog.h>
#include "spdlog/sinks/basic_file_sink.h"

using namespace ftxui;

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
        } catch (std::filesystem::filesystem_error &e) {
            file_entry.clear();
        }
    }

    void search(std::string name, std::filesystem::path path,int options) {
        file_entry = search_file(name, path, options);
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

        list = file_size,
        search = full_path,
    };   

    void render(FileEntryModel &model, int options) {
        table.clear();

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
                }
            }
            table.push_back(s);
        }

    }

    std::vector<std::string> table;
    int selected;
};

FileEntryModel file_model;
FileEntryView file_view;
std::filesystem::path current_dir =  std::filesystem::current_path();

Component list_view() {
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
        file_view.render(file_model, FileEntryView::show_options::list);
    };

    auto entry = Menu(&file_view.table, &file_view.selected, option);
    return entry;
}

void init() {
    file_model.list(current_dir);
    file_view.render(file_model, FileEntryView::show_options::list);
}

std::string info;
int main() {
    spdlog::set_level(spdlog::level::debug);
    std::shared_ptr<spdlog::logger> logger;
    try {
        logger = spdlog::basic_logger_mt("logger", "logs/debug_log.txt");
    } catch (const spdlog::spdlog_ex &ex) {
        fmt::println("Log init failed: {}", ex.what());
    }

    init();
    try {
    auto screen = ScreenInteractive::Fullscreen(); 
    auto view = list_view();

    FileOperationDialog dialog(current_dir);

    std::string e;

    SearchBar search_bar;
    Component container = Container::Stacked({
        search_bar.component,
        view,
    });


    auto homepage_component = Renderer(container, [&] {
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
                file_view.render(file_model, FileEntryView::show_options::list);
            }
        } catch (std::exception &ex) {
            e = ex.what();
        }
        std::string title = search_bar.text().size() > 0 ? "Search Results" : "Files";

        return gridbox({
            {
                text(fmt::format("Current Dir: {}", current_dir.string())) | bold
            },
            {
                window(text("Target File"), search_bar.component->Render()) 
            }, {
                window(text(title), view->Render() | frame | size(HEIGHT, EQUAL, 28))
            }, {
                text(fmt::format("info: {}", info)) | border
            }        
        });
    });
    homepage_component |= Modal(dialog.component, &dialog.shown); 

    homepage_component |= CatchEvent([&](Event event) {
        if (event == Event::Character('x')) {
            try {
                auto dir = file_model.file_entry[file_view.selected];
                info = dir.path().filename().string() + "test";
                dialog.udpate_path(dir);
                dialog.shown = true;
            } catch (std::exception &e) {
                info = e.what();
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

    auto recycle_bin_page = RecycleBin();   


    std::vector<std::string> tab_values {
        "HomePage",
        "Recycle Bin",
    };
    int tab_selected = 0;
    auto tab_menu = Menu(&tab_values, &tab_selected);

    auto tab_container = Container::Tab({
        homepage_component,
        recycle_bin_page.componet,
    }, &tab_selected);

    auto total_container = Container::Horizontal({
        tab_menu,
        tab_container,
    });

    auto component = Renderer(total_container, [&] {
        return hbox({
            tab_menu->Render() | size(ftxui::WIDTH, ftxui::GREATER_THAN, 20),
            separator(),
            tab_container->Render(),
        }) | border;
    });

    component |= CatchEvent([&](Event event) {
        if (event == Event::Character('q')) {
            screen.ExitLoopClosure()();
            return true;
        }
        return false;
    });

    screen.Loop(component);
    } catch (std::exception &e) {
        logger->debug("{}", e.what());
    }

    return 0;
}
