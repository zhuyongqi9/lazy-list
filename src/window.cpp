#include <algorithm>
#include <cstdint>
#include <regex>
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
        file_entry = list_all(path);
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
                s += fmt::format("{:40}", item.path().string());
            } else {
                s += item.path().filename().string();
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
            current_dir = file_model.file_entry[file_view.selected];
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

int main() {
    init();

    auto screen = ScreenInteractive::Fullscreen(); 
    auto view = list_view();
    std::string e;

    SearchBar search_bar;
    Component container = Container::Stacked({
        search_bar.component,
        view
    });


    auto component = Renderer(container, [&] {
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
        }
        catch (std::exception &ex) {
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
                window(text(title), view->Render() | frame | size(HEIGHT, LESS_THAN, 20))
            }, {
                text(fmt::format("e: {}", e)) | border
            },{
                text(fmt::format("target: {}", search_bar.search_input.text)) | border
            }, {
                text(fmt::format("target: {}", search_bar.regex())) | border
            }
        });
    });

    component |= CatchEvent([&](Event event) {
        if (event == Event::Character('q')) {
          screen.ExitLoopClosure()();
          return true;
        }

        if (event ==  Event::Special("\x1B")) {
            current_dir = current_dir.parent_path();
            return true;
        }
        return false;
    });


    screen.Loop(component);
    return 0;
}
