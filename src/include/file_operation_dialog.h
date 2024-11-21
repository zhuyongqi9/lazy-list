#include "fmt/core.h"
#include <filesystem>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <functional>
#include <unordered_map>
#include <fmt/format.h>
#include <file_operation.h>
#include <vector>

using namespace ftxui;

//=MESSAGE===================
static const std::string TITLE = "Selected file: \"{}\" <Esc>";
//=====================================

static std::vector<std::pair<std::string, std::function<void(const std::filesystem::path &)>>> handle_map = {
    {fmt::format("{: <8}Create File", "c"), [](const std::filesystem::path &path)->void{}},
    {fmt::format("{: <8}Create Folder", "C"), [](const std::filesystem::path &path)->void{}},
    {fmt::format("{: <8}Rename File", "r"), [](const std::filesystem::path &path)->void{}},
    {fmt::format("{: <8}Copy File To", "p"), [](const std::filesystem::path &path)->void{}},
    {fmt::format("{: <8}Move File To", "m"), [](const std::filesystem::path &path)->void{}},
    {fmt::format("{: <8}Delete File", "D"), [](const std::filesystem::path &path)->void{
        auto op = DeleteFileOperation(path);
        op.perform();
    }},
    {fmt::format("{: <8}Compress File", "z"), [](const std::filesystem::path &path)->void{}},
    {fmt::format("{: <8}DeCompress File", "d"), [](const std::filesystem::path &path)->void{}},
};

class DstDialog {
public:
    DstDialog():shown(false) {
        auto option = InputOption::Default();
        option.on_enter = [&]() {
            this->shown = false;
        };

        this->input = Input(&(this->file), "", option);

        this->container = Container::Stacked({this->input});

        this->component = Renderer(this->container, [&]() {
            return vbox ({
                window(text("Target File Path" + file) , this->input->Render() | size(WIDTH, GREATER_THAN, 30)),
            });
        });
    }
    bool shown;
    Component component;
    Component container;
    Component input;
    std::string file="";

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
            dst.shown = true;
            auto f = handle_map[this->selected].second;
            f(src);
        };

        menu = Menu(&view, &selected, option);
        container = Container::Stacked({menu});

        component = Renderer(container, [&]() {
            return vbox ({
                window(text(fmt::format(TITLE, this->src.filename().string())) | color(Color::Green), menu->Render()),
            });
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
};