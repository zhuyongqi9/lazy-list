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

std::vector<std::string> skipped_file = {
    "Photos Library.photoslibrary",
};

bool skipped(std::string &path) {
    for (auto &item : skipped_file) if (item == path) return true;
    return false;
}

std::vector<std::filesystem::directory_entry> search_file(std::string &name, std::filesystem::path &path) {
    auto file = std::filesystem::recursive_directory_iterator(path, std::filesystem::directory_options::skip_permission_denied);
    std::vector<std::filesystem::directory_entry> res(0);

    try {
        for(auto it = file; it != std::filesystem::end(file); it++) {
            const std::filesystem::directory_entry &entry = *it;
            std::string file_name = entry.path().filename().string();
            if (skipped(file_name)) {
                it.disable_recursion_pending();
                continue;
            }

            if (file_name.size() > 0 && file_name[0] == '.') {
                //fmt::print(fmt::bg(fmt::color::red), file_name);
                //fmt::print("\n");
                it.disable_recursion_pending();
                continue;
            }
            //fmt::println(entry.path().string());
            if (file_name.find(name) == 0) {
                res.push_back(entry);
            }
        }
    } catch (const std::filesystem::filesystem_error &e) {
        
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

    string_view.push_back("..");
    for (auto &file : files) {
        string_view.push_back(file.path().filename().string());
    }

    MenuOption option = MenuOption::Vertical();
    option.entries_option.transform = [&] (EntryState state) {
        Element e = text((state.active ? "> " : "  ") + state.label);  // NOLINT
        if (state.focused) {
          e |= inverted;
        }
        if (state.active) {
          e |= bold;
        }
        if (!state.focused && !state.active) {
          //e |= dim;
        } 

        try {
            if (std::filesystem::directory_entry(current / state.label).is_directory()) {
                e |= color(Color::Green);
            }
        } catch (std::filesystem::filesystem_error &e) {

        }
        return e;
    };

    option.on_enter = [&current, &files, &string_view, selected]()mutable {
        if (*selected == 0) {
            current = current.parent_path();
            files = std::vector<std::filesystem::directory_entry>(list_all(current));
            string_view.clear();
            string_view.push_back("..");
            for (auto &file : files) {
                string_view.push_back(file.path().filename().string());
            }
        } else {
            std::filesystem::directory_entry selected_entry = (files)[*selected-1];
            if (selected_entry.is_directory()) {
                current = selected_entry.path();
                files = std::vector<std::filesystem::directory_entry>(list_all(current));
                string_view.clear();
                string_view.push_back("..");
                for (auto &file : files) {
                    string_view.push_back(file.path().filename().string());
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


    int selected = 0;
    auto view = list_view(current, files, string_view);
    int a = 0;

    std::string target_file;
    Component input_target_file = Input(&target_file, "", InputOption::Default());
    input_target_file |= CatchEvent([&](Event event) {
        if (!input_target_file->Focused()) return true;
        return false;
    });


    Component container = Container::Stacked({
        input_target_file,
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
            res = search_file(target_file, current);
            files.clear();
            string_view.clear();
            for (auto &item : res)  {
                files.push_back(item);
                string_view.push_back(item.path().string());
            }
        } else if (target_file.size() == 0) {
            res = list_all(current); 
            files.clear();
            string_view.clear();
            string_view.push_back("..");
            for (auto &item : res)  {
                files.push_back(item);
                string_view.push_back(item.path().filename().string());
            }
        }


        return gridbox({
            {
                text(fmt::format("Current Dir: {}", current.string())) | bold
            },
            {
                window(text("Target File"), input_target_file->Render()) 
            }, {
                view->Render() | frame | size(HEIGHT, LESS_THAN, 20) 
            }, {
                text(fmt::format("a: {}", a)) | border
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
        return false;
    });


    screen.Loop(component);
}
