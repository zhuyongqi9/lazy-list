#ifndef RE_FILE_OPERATION_DIALOG_H
#define RE_FILE_OPERATION_DIALOG_H

#include "fmt/core.h"
#include <filesystem>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <functional>
#include <stdexcept>
#include <string>
#include <fmt/format.h>
#include <file_operation.h>
#include <vector>
#include "file_operation_dialog.h"
#include "config_parser.h"
extern Config config;

using namespace ftxui;

extern std::string recycle_bin_page_info;


class ReFileOperationDialog {
public:
    ReFileOperationDialog& operator=(const ReFileOperationDialog&) = delete;
    ReFileOperationDialog(const std::filesystem::path src):src(src), view(0) {
        for(auto &pair : handle_map) view.push_back(pair.first);

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
            auto f = handle_map[this->selected].second;
            f();
        };

        menu = Menu(&view, &selected, option);
        container = Container::Stacked({
            dst.component,
            menu,
        });

        component = Renderer(container, [&]() {
            return vbox ({
                window(text(fmt::format(TITLE, this->src.filename().string())+ std::to_string(this->selected))  | color(Color::Green), menu->Render()),
            }) | center;
        });

        component |= CatchEvent([&](Event event) {
            if (event ==  Event::Special("\x1B")) {
                this->shown = false;
                return true;
            }
            return false;
        });

        dst.shown = false;
        component |= Modal(dst.component, &dst.shown);
    };

    Element Render() {
        return component->Render();
    }

    void udpate_path(const std::filesystem::path &path) {
        this->src = path;
    }

    Component component;
    Component menu;
    Component container;
    int selected;
    bool shown;
    DstDialog dst = DstDialog();
    std::vector<std::string> view;

private:
    std::filesystem::path src;

    std::vector<std::pair<std::string, std::function<void()>>> handle_map = {
        {fmt::format("{: <8}Restore File", "C"), [&]()->void{
            dst.shown = true;
            auto str = this->src.filename().string();
            std::for_each(str.begin(), str.end(), [](char &c){
                if (c == '\\') {
                    c = '/';
                }
            });

            dst.message = str;
            dst.f = [&](std::string &dst) {
                try {
                    auto op = MoveFileOperation(this->src, dst);
                    op.perform();
                    recycle_bin_page_info = fmt::format("File copied successfully.");
                } catch (std::runtime_error &e){
                    recycle_bin_page_info = e.what();
                }
                this->shown = false;
            };
        }},
        {fmt::format("{: <8}Permanently deleted the file.", "c"), [&]()->void{
            auto op = DeleteFileOperation(this->src);
            try {
                op.perform();
                recycle_bin_page_info = "File Deleted successfully";
            } catch (std::runtime_error &e) {
                recycle_bin_page_info = e.what();
            }
            this->shown = false;
        }},
        {fmt::format("{: <8}Empty Recycle Bin", "c"), [&]()->void{
            try {
                auto path = this->src.parent_path();
                auto op = DeleteFileOperation(path);
                op.perform();
                auto op2 = CreateFolderOperation(path);
                op2.perform();
                recycle_bin_page_info = "Recycle Bin emptied successfully.";
            } catch (std::runtime_error &e) {
                recycle_bin_page_info = "Failed to empty the Recycle Bin.";
                recycle_bin_page_info = e.what();
            }
            this->shown = false;
        }},

    };
};
#endif