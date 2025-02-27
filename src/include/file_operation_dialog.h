#ifndef FILE_OPERATION_DIALOG_H
#define FILE_OPERATION_DIALOG_H
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <stdexcept>
#include <fmt/format.h>
#include <file_operation.h>
#include <vector>
#include "config_parser.h"
extern Config config;

using namespace ftxui;

//=MESSAGE===================
static const std::string TITLE = "Selected file: \"{}\" [Esc]";
//=====================================

extern std::string home_page_info;

class DstDialog {
public:
    DstDialog():shown(false) {
        auto option = InputOption::Default();
        option.multiline = false;
        option.on_enter = [&]() {
            auto begin = this->message.find_first_not_of(" ");
            auto end = this->message.find_last_not_of(" ");
            auto str = message.substr(begin, end + 1 - begin);
            f(str);
            this->shown = false;
            this->message = "";
        };

        this->input = Input(&(this->message), &(this->message), option);

        this->container = Container::Stacked({this->input});

        this->component = Renderer(this->container, [&]() {
            return vbox ({
                window(text("Target File Path" + message) , this->input->Render() | size(WIDTH, GREATER_THAN, 30)),
            });
        });
    }
    bool shown;
    Component component;
    Component container;
    Component input;

    std::string message="";
    std::function<void(std::string &)> f;
private:
};

class FileOperationDialog {
public:
    FileOperationDialog& operator=(const FileOperationDialog&) = delete;
    FileOperationDialog(const std::filesystem::path src):src(src), view(0) {
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
                window(text(fmt::format(TITLE, this->src.filename().string()))  | color(Color::Green), menu->Render()),
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
        {fmt::format("{: <8}Create File", ""), [&]()->void{
            dst.shown = true;
            dst.f = [&](std::string &dst) {
                try {
                    auto op = CreateFileOperation(dst);
                    op.perform();
                    home_page_info = fmt::format("File {} created successfully.!", dst);
                } catch (std::runtime_error &e){
                    home_page_info = e.what();
                }
                this->shown = false;
            };
        }},

        {fmt::format("{: <8}Create Folder", ""), [&]()->void{
            dst.shown = true;
            dst.f = [&](std::string &dst) {
                try {
                    auto op = CreateFolderOperation(dst);
                    op.perform();
                    home_page_info = fmt::format("Folder {} created successfully.!", dst);
                } catch (std::runtime_error &e){
                    home_page_info = e.what();
                }
                this->shown = false;
            };
        }},

        {fmt::format("{: <8}Rename File", ""), [&]()->void{
            dst.shown = true;
            dst.f = [&](std::string &dst) {
                try {
                    std::filesystem::path target = this->src.parent_path() / dst;
                    auto op = MoveFileOperation(this->src, target);
                    op.perform();
                    home_page_info = fmt::format("File renamed successfully.");
                } catch (std::runtime_error &e){
                    home_page_info = e.what();
                }
                this->shown = false;
            };
        }},

        {fmt::format("{: <8}Copy File To", ""), [&]()->void{
            dst.shown = true;
            dst.message = this->src.string();
            dst.f = [&](std::string &dst) {
                try {
                    auto op = CopyFileOperation(this->src, dst);
                    op.perform();
                    home_page_info = fmt::format("File copied successfully.");
                } catch (std::runtime_error &e){
                    home_page_info = e.what();
                }
                this->shown = false;
            };
        }},

        {fmt::format("{: <8}Move File To", ""), [&]()->void{
            dst.shown = true;
            dst.message = this->src.string();
            dst.f = [&](std::string &dst) {
                try {
                    auto op = MoveFileOperation(this->src, dst);
                    op.perform();
                    home_page_info = fmt::format("File copied successfully.");
                } catch (std::runtime_error &e){
                    home_page_info = e.what();
                }
                this->shown = false;
            };
        }},

        {fmt::format("{: <8}Delete File", ""), [&]()->void{
            auto op = SoftDeleteFileOperation(this->src);
            try {
                op.perform();
                home_page_info = "File Deleted successfully";
            } catch (std::runtime_error &e) {
                home_page_info = e.what();
            }
            this->shown = false;
        }},

        {fmt::format("{: <8}Compress File", ""), [&]()->void{
            dst.shown = true;
            auto path = this->src.parent_path() / fmt::format("{}.zip", filename_without_ext(this->src));
            dst.message = path.string();
            dst.f = [&](std::string &dst) {
                try {
                    auto op = CompressFileOperation(this->src, dst);
                    op.perform();
                    home_page_info = fmt::format("File compressed successfully");
                } catch (std::runtime_error &e){
                    home_page_info = e.what();
                }
                this->shown = false;
            };
        }},
        {fmt::format("{: <8}Extract File", ""), [&]()->void{
            dst.shown = true;
            auto path = this->src.parent_path() / fmt::format("{}", filename_without_ext(this->src));
            dst.message = path.string();

            dst.f = [&](std::string &dst) {
                try {
                    auto op = ExtractFileOperation(this->src, dst);
                    op.perform();
                    home_page_info = fmt::format("File Extracted successfully");
                } catch (std::runtime_error &e){
                    home_page_info = e.what();
                }
                this->shown = false;
            };
        }},

        {fmt::format("{: <8}Add to bookmark", ""), [&]()->void{
            if (std::filesystem::directory_entry(src).is_directory()) {
                try {
                    config.bookmark.push_back(src);
                    config.store();
                    home_page_info = "Successfully added to Bookmarks";
                } catch (std::runtime_error &e) {
                    spdlog::error(e.what());
                    home_page_info = fmt::format("failed to add to bookmark, e:{}" , e.what());
                }
                this->shown = false;
            } else {
                home_page_info = "Can only add folder to bookmark";
                this->shown = false;
            }
        }},
    };
};

#endif