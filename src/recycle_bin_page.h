
#include "re_file_operation_dialog.h"
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/component.hpp>
#include <filesystem>
#include <ftxui/dom/elements.hpp>
#include <stdexcept>
#include <fmt/format.h>
#include <system_error>
#include <file_utils.h>
#include <file_operation.h>
#include <global_var.h>
#include "config_parser.h"
extern Config config;

using namespace ftxui;

std::string recycle_bin_page_info = "";

class ReFileEntryModel {
public:
    ReFileEntryModel(): file_entry(0) {
    }

    void list(std::filesystem::path &path) {
        try {
            file_entry = list_all(path, 0);
        } catch (std::filesystem::filesystem_error &e) {
            file_entry.clear();
        }
    }

    std::vector<std::filesystem::directory_entry> file_entry;
private:
};

class ReFileEntryView {
public:
    enum show_options {
        directory_size = 1,
        file_size = 1 << 2,
        full_path = 1 << 3,

        list = file_size,
        search = full_path,
    };   

    void render(ReFileEntryModel &model, int options) {
        table.clear();

        for (const auto &item : model.file_entry) {
            std::string s = "";
            if (options & full_path) {
                s += fmt::format("{: <40}", item.path().string());
            } else {
                s += fmt::format("{: <40}", item.path().filename().string());
            }

            table.push_back(s);
        }

    }

    std::vector<std::string> table;
    int selected;
};

class RecycleBin {
public:
    RecycleBin():path(config.recycle_bin_path) {
        std::error_code ec;
        if (!std::filesystem::exists(path, ec)) {
            if (ec.value() != 0) {
                throw std::runtime_error(fmt::format("Failed to show recycle bin, reason: {}", ec.message()));
            }

            auto op = CreateFolderOperation(path);
            op.perform();
        } 

        MenuOption option = MenuOption::Vertical();

        option.entries_option.transform = [&] (EntryState state) {
            Element e = text((state.active ? "> " : "  ") + state.label);  // NOLINT

            try {
                if (state.index == 0) {
                    e |= color(Color::Green);
                } else {
                    if (model.file_entry[state.index].is_directory()) {
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

        };

        this->file_entry = ftxui::Menu(&view.table, &view.selected, option);
        this->container = Container::Stacked({this->file_entry});
        this->component = Renderer(this->container, [&](){
            this->model.list(path);
            this->view.render(this->model, ReFileEntryView::list | ReFileEntryView::directory_size);

            return vbox({
                this->file_entry->Render() | frame | yflex, 
                filler(),
                text(fmt::format("info: {}", recycle_bin_page_info)) | border | size(HEIGHT, EQUAL, 3),
            }) | xflex;
        });

        this->component |= Modal(dialog.component, &dialog.shown);
        this->component |= CatchEvent([&](Event event) {
            if (event == Event::Character('x')) {
                auto dir = this->model.file_entry[view.selected];
                dialog.udpate_path(dir);
                dialog.shown = true;
                return true;
            }

            return false;
        });
    }

    Component component;
    std::filesystem::path path;
    ReFileEntryModel model;
    ReFileEntryView view;
    ReFileOperationDialog dialog = ReFileOperationDialog("");
    Component container;
    Component file_entry;
};