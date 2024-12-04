#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/event.hpp>
#include <file_utils.h>
#include <file_operation.h>
#include <global_var.h>
#include <stdexcept>
#include "config_parser.h"
#include <spdlog/spdlog.h>

extern Config config;

static std::string bookmark_page_info;

using namespace ftxui;

class BookmarkDialog{
public:
    BookmarkDialog(std::vector<std::pair<std::string, std::function<void()>>> *handle_map): handle_map(handle_map) {
        for(auto &pair : *handle_map) view.push_back(pair.first);

        MenuOption option = MenuOption::Vertical();
        option.entries_option.transform = [&] (EntryState state) {
            Element e = text((state.active ? "> " : "  ") + state.label);  // NOLINT

            if (state.focused) {
              e |= inverted;
            }
            if (state.active) {
                e |= bold;
                e |= color(Color::Green);
            }

            if (!state.focused && !state.active) {
              //e |= dim;
            } 

            return e;
        };

        option.on_enter = [&]()mutable {
            auto f = (*(this->handle_map))[this->selected].second;
            f();
            this->shown = false;
        };

        this->menu = Menu(&view, &selected, option);

        this->container = Container::Stacked({
            menu,
        });

        this->component = Renderer(container, [&]() {
            return vbox ({
                window(text(this->src + std::to_string(this->selected))  | color(Color::Green), menu->Render()),
            }) | center;
        });

        component |= CatchEvent([&](Event event) {
            if (event ==  Event::Special("\x1B")) {
                this->shown = false;
                return true;
            }
            return false;
        });

    }

    Component component;
    Component menu;
    Component container;
    int selected;
    bool shown;
    std::string src;
    std::vector<std::string> view;
    std::vector<std::pair<std::string, std::function<void()>>> *handle_map;

private:
};

class BookmarkEntryModel {
public:
    BookmarkEntryModel(): table(0) {
    }

    void list() {
        config.refresh();
        table = config.bookmark;
    }

    std::string get_selected() {
        return this->table[this->selected]; 
    }

    std::vector<std::string> table;
    int selected;

private:
};

void go_to_homepage(const std::string &path);

class BookmarkPage {
public:
    BookmarkPage& operator=(const BookmarkPage &) = delete;
    BookmarkPage() {
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

            return e;
        };

        option.on_enter = [&]()mutable {
            go_to_homepage(model.get_selected()); 
        };

        this->file_entry = ftxui::Menu(&model.table, &model.selected, option);
        this->container = Container::Stacked({
            this->file_entry,
        });
        this->component = Renderer(this->container, [&](){
            this->model.list();

            return vbox({
                this->file_entry->Render() | frame | yflex, 
                filler(),
                text(fmt::format("info: {}", bookmark_page_info)) | border | size(HEIGHT, EQUAL, 3) , 
            }) | xflex; 
        });

        this->component |= CatchEvent([&](Event event) {
            if (event == Event::Character('x')) {
                dialog.src = model.get_selected();
                dialog.shown = true;
                return true;
            }

            return false;
        });

        this->dialog.shown = false;
        this->component |= Modal(dialog.component, &dialog.shown);
    }

    BookmarkEntryModel model;
    Component file_entry;
    Component container;
    Component component;

    std::vector<std::pair<std::string, std::function<void()>>> handle_map = {
        {fmt::format("{: <8}Remove", "c"), [&]()->void {
            config.bookmark.erase(config.bookmark.begin() + this->model.selected);
            try {
                config.store();
                bookmark_page_info = "Folder removed successfully";
            } catch (std::runtime_error &e) {
                bookmark_page_info = "failed to remove, e: " + std::string(e.what());
            }
        }}
    };

    BookmarkDialog dialog = BookmarkDialog(&(this->handle_map));
};