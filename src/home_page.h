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
#include <vector>
#include "MESSAGE.h"
#include "file_utils.h" 
#include "search_bar.h"
#include "file_operation_dialog.h"

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



class HomePage {
public:
    HomePage() {
        file_model.list(current_dir);
        file_view.render(file_model, FileEntryView::show_options::list);

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
                    file_view.render(file_model, FileEntryView::show_options::list);
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
                    window(text(title), view->Render() | frame | size(HEIGHT, EQUAL, 28))
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
};