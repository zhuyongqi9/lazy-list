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

    std::string target_file;
    Component input_target_file = Input(&target_file, "", InputOption::Default());
    input_target_file |= CatchEvent([&](Event event) {
//        if (!input_target_file->Focused()) return true;
        return false;
    });

    bool button_case_ignored_clicked = true;
    ButtonOption option1 = ButtonOption::Animated();
    option1.transform = [&](const EntryState& s) {
        auto element = text(s.label);
        if (s.focused) {
          element |= bold;
          element |= color(Color::Red);
        }

        if (button_case_ignored_clicked) {
          element |= color(Color::Green);
        }
        //return element | center | borderEmpty | flex;
        return element | center;
    };
    option1.animated_colors.background.Set(Color(), Color());
    Component button_case_ignored = Button ("Aa", [&] {
        button_case_ignored_clicked = !button_case_ignored_clicked;
    }, option1);

    bool button_regex_clicked = false;
    ButtonOption option2 = ButtonOption::Animated();
    option2.transform = [&](const EntryState& s) {
        auto element = text(s.label);
        if (s.focused) {
          element |= bold;
          element |= color(Color::Red);
        }
        if (button_regex_clicked) 
          element |= color(Color::Green);
        //return element | center | borderEmpty | flex;
        return element | center;
    };
    option2.animated_colors.background.Set(Color(), Color());
    Component button_regex = Button(".*", [&]{
        button_regex_clicked = !button_regex_clicked;
    }, option2);


    int serach_button_selected;

    Component search_button_container = Container::Horizontal({
        input_target_file,
        button_case_ignored,
        button_regex,
    });

    Component input_search = Renderer(search_button_container, [&](){
        auto renderer_button_case_ignored = button_case_ignored->Render() | size(ftxui::WIDTH, ftxui::EQUAL, 3);
        auto renderer_button_regex = button_regex->Render() | size(ftxui::WIDTH, ftxui::EQUAL, 3);

        return flexbox({ 
            input_target_file->Render(),
            renderer_button_case_ignored,
            renderer_button_regex,
        });
    });


    Component container = Container::Vertical({
        input_search,
        view
    });


    auto component = Renderer(container, [&] {
        std::vector<std::filesystem::directory_entry> res;

        try {
        if (target_file.length() > 0) {
            int options;
            if (button_regex_clicked) options |= search_options::regex;
            else if (button_case_ignored_clicked)  options |= search_options::caseignored;

            file_model.search(target_file, current_dir, options);
            file_view.render(file_model, FileEntryView::show_options::search);
        } else if (target_file.size() == 0) {
            file_model.list(current_dir);
            file_view.render(file_model, FileEntryView::show_options::list);
        }
        }
        catch (std::exception &ex) {
            e = ex.what();
        }
        std::string title = target_file.size() > 0 ? "Search Results" : "Files";

        return gridbox({
            {
                text(fmt::format("Current Dir: {}", current_dir.string())) | bold
            },
            {
                window(text("Target File"), input_search->Render()) 
            }, {
                window(text(title), view->Render() | frame | size(HEIGHT, LESS_THAN, 20))
            }, {
                text(fmt::format("e: {}", e)) | border
            },{
                text(fmt::format("target: {}", target_file)) | border
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
