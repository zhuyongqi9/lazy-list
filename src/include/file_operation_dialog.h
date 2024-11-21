#include <filesystem>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <functional>
#include <unordered_map>
#include <fmt/format.h>
#include <file_operation.h>

using namespace ftxui;

//=MESSAGE===================
static const std::string TITLE = "File Operations <Esc>";
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

class FileOperationDialog {
public:
    FileOperationDialog(const std::filesystem::path src):src(src) {
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
            auto f = handle_map[selected].second;
            f(src);
        };

        auto menu = Menu(&view, &selected);
        auto conatiner = Container::Stacked({menu});

        component = Renderer(conatiner, [=] {
            return vbox ({
                window(text(TITLE), menu->Render()),
            });
        });

        component |= CatchEvent([&](Event event) {
            if (event ==  Event::Special("\x1B")) {
                this->shown = false;
                return true;
            }
            return false;
        });
    };

    Element Render() {
        return component->Render();
    }

    Component component;
    int selected;
    bool shown;
    std::vector<std::string> view;
    std::filesystem::path src;
};