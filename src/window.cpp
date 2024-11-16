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
#include <string_view>
#include <vector>
#include "MESSAGE.h"

uint64_t directory_size(std::filesystem::path path) {
    auto it = std::filesystem::recursive_directory_iterator(path);
    uint64_t res = 0;
    for (auto start = it; it != std::filesystem::end(it); it++) {
        if (it->is_regular_file())
            res += (*it).file_size();
    }
    return res;
}

std::string formatted_file_size(std::filesystem::path path) {
    try {
        auto entry = std::filesystem::directory_entry(path);
        uint64_t size;
        if (entry.is_regular_file()) {
            size = entry.file_size();
        } else if (entry.is_directory()) {
            size = directory_size(path);
        } else {
            size = 0;
        }
        if (size < 1024) {
        // Byte
            return fmt::format("{:>8} B", size);
        } else if (size < 1024 * 1024) {
        // KB
            double res = size / 1024.0;
            return fmt::format("{:>8.2f}KB", res);
        } else if (size < 1024 * 1024 * 1024) {
        // MB
            double res = size / 1024.0 / 1024;
            return fmt::format("{:>8.2f}MB", res);
        } else {
        //GB
            double res = size / 1024.0 / 1024 / 1024;
            return fmt::format("{:>8.2f}GB", res);
        }
    } catch (const std::exception &e) {
        //return fmt::format("{:>6}NULL", e.what());
        return fmt::format("{:>6}NULL", "");
    }
}

std::string file_view_string(std::filesystem::path path, bool full_path = false) {
    std::string size = formatted_file_size(path);
    if (full_path) {
        return fmt::format("{:<32} {}", path.string(), size);
    } else {
        return fmt::format("{:<32} {}", path.filename().string(), size);
    }
}



std::vector<std::string> skipped_file = {
    "Photos Library.photoslibrary",
};

bool skipped(std::string &path) {
    for (auto &item : skipped_file) if (item == path) return true;
    return false;
}



enum options {
    regex = 1 << 1,
    caseignored = 1 << 2,
};

std::vector<std::filesystem::directory_entry> search_file(std::string &name, std::filesystem::path &path, int option) {
    auto file = std::filesystem::recursive_directory_iterator(path, std::filesystem::directory_options::skip_permission_denied);
    std::vector<std::filesystem::directory_entry> res(0);
    auto target = name;

    if (option & caseignored) {
        std::for_each(target.begin(), target.end(), [](char &c) {
            c = std::toupper(c);
        });
    }

    try {
        for(auto it = file; it != std::filesystem::end(file); it++) {
            const std::filesystem::directory_entry &entry = *it;
            std::string file_name = entry.path().filename().string();
            if (skipped(file_name)) {
                it.disable_recursion_pending();
                continue;
            } else if (file_name.size() > 0 && file_name[0] == '.') {
                it.disable_recursion_pending();
                continue;
            }

            if (option & caseignored) {
                std::for_each(file_name.begin(), file_name.end(), [](char &c) {
                    c = std::toupper(c);
                });
            }

            if (option & regex) {
                std::regex pattern(name);
                if (std::regex_match(file_name, pattern)) {
                    res.push_back(entry);
                }
            } else {
                if (file_name.find(target) == 0) {
                    res.push_back(entry);
                }
            }
        }
    } catch (const std::filesystem::filesystem_error &e) {
        
    } catch (const std::regex_error &e) {

    }
    return res;
}


std::vector<std::filesystem::directory_entry> list_all(std::filesystem::path path) {
    auto start = std::filesystem::directory_iterator(path, std::filesystem::directory_options::skip_permission_denied);
    std::vector<std::filesystem::directory_entry> res;
    try {
        for (auto it = start; it != std::filesystem::end(start); it++) {
            auto entry = *it;
            res.push_back(entry);
        }
    } catch (std::filesystem::filesystem_error &e) {
    }
    return res;
}

using namespace ftxui;

Component list_view(std::filesystem::path &current, std::vector<std::filesystem::directory_entry> &files, std::vector<std::string> &string_view) {
    std::shared_ptr<int> selected = std::make_shared<int>(0);

    MenuOption option = MenuOption::Vertical();
    option.entries_option.transform = [&] (EntryState state) {
        Element e = text((state.active ? "> " : "  ") + state.label);  // NOLINT

        try {
            if (state.index == 0) {
                e |= color(Color::Green);
            } else {
                if (files[state.index-1].is_directory()) {
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

    option.on_enter = [&current, &files, &string_view, selected]()mutable {
        if (*selected == 0) {
            current = current.parent_path();
            files = std::vector<std::filesystem::directory_entry>(list_all(current));
            string_view.clear();
            string_view.push_back(M_PARENT_PATH);
            for (auto &file : files) {
                string_view.push_back(file_view_string(file.path()));
            }
        } else {
            std::filesystem::directory_entry selected_entry = (files)[*selected-1];
            if (selected_entry.is_directory()) {
                current = selected_entry.path();
                files = std::vector<std::filesystem::directory_entry>(list_all(current));
                string_view.clear();
                string_view.push_back(M_PARENT_PATH);
                for (auto &file : files) {
                    string_view.push_back(file_view_string(file.path()));
                }
            }
        }
    };

    auto entry = Menu(&string_view, selected.get(), option);

    return entry;
}



int main() {
    auto screen = ScreenInteractive::Fullscreen(); 


    std::filesystem::path current =  std::filesystem::current_path();
    std::vector<std::filesystem::directory_entry> files = list_all(current);
    std::vector<std::string> string_view;
    string_view.push_back(M_PARENT_PATH);
    for (auto &item : files) string_view.push_back(file_view_string(item.path()));


    int selected = 0;
    auto view = list_view(current, files, string_view);
    int a = 0;

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

        //if (target_file.size() > 0) {
        //    view->Detach();
        //    view = list_view(current);
        //    container->Add(view);
        //}

        std::vector<std::filesystem::directory_entry> res;
        

        if (target_file.size() > 0) {
            int options;
            if (button_regex_clicked) options |= options::regex;
            else if (button_case_ignored_clicked)  options |= options::caseignored;
            res = search_file(target_file, current,  options);
            files.clear();
            string_view.clear();
            for (auto &item : res)  {
                files.push_back(item);
                string_view.push_back(file_view_string(item.path(), true));
            }
        } else if (target_file.size() == 0) {
            res = list_all(current); 
            files.clear();
            string_view.clear();
            string_view.push_back(M_PARENT_PATH);
            for (auto &item : res)  {
                files.push_back(item);
                string_view.push_back(file_view_string(item.path()));
            }
        }

        std::string title = target_file.size() > 0 ? "Search Results" : "Files";

        return gridbox({
            {
                text(fmt::format("Current Dir: {}", current.string())) | bold
            },
            {
                window(text("Target File"), input_search->Render()) 
            }, {
                window(text(title), view->Render() | frame | size(HEIGHT, LESS_THAN, 20))
            }, {
                text(fmt::format("a: {}", button_regex_clicked)) | border
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
            current = current.parent_path();
            return true;
        }
        return false;
    });


    screen.Loop(component);
    return 0;
}
