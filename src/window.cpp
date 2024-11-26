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
#include <fmt/core.h>
#include <fmt/format.h>
#include <memory>
#include <string>
#include <vector>
#include "MESSAGE.h"
#include "search_bar.h"
#include "bookmarks_page.h"
#include "recycle_bin_page.h"
#include "file_operation_dialog.h"
#include <spdlog/spdlog.h>
#include "spdlog/sinks/basic_file_sink.h"
#include "config_parser.h"
#include "home_page.h"
#include "config_page.h"

using namespace ftxui;

Config config;

HomePage homepage;
RecycleBin recycle_bin_page;
BookmarkPage bookmark_page;
ConfigPage config_page;

int tab_selected = 0;

void go_to_homepage(const std::string &path) {
    homepage.current_dir = path;
    tab_selected = 0;
}

void go_to_homepage() {
    tab_selected = 0;
}

int main() {
    spdlog::set_level(spdlog::level::debug);
    std::shared_ptr<spdlog::logger> logger;
    try {
        logger = spdlog::basic_logger_mt("logger", "logs/debug_log.txt");
    } catch (const spdlog::spdlog_ex &ex) {
        fmt::println("Log init failed: {}", ex.what());
    }

    try {
        auto screen = ScreenInteractive::Fullscreen(); 

        std::vector<std::string> tab_values {
            "HomePage",
            "Bookmark",
            "Recycle Bin",
            "Config",
        };
        auto tab_menu = Menu(&tab_values, &tab_selected);

        auto tab_container = Container::Tab({
            homepage.component,
            bookmark_page.component,
            recycle_bin_page.component,
            config_page.component,
        }, &tab_selected);

        auto total_container = Container::Horizontal({
            tab_menu,
            tab_container,
        });

        auto component = Renderer(total_container, [&] {
            return hbox({
                tab_menu->Render() | size(ftxui::WIDTH, ftxui::GREATER_THAN, 20),
                separator(),
                tab_container->Render(),
            }) | border;
        });

        component |= CatchEvent([&](Event event) {
            if (event == Event::Character('q')) {
                screen.ExitLoopClosure()();
                return true;
            }
            return false;
        });

        screen.Loop(component);
    } catch (std::exception &e) {
        logger->debug("{}", e.what());
    }

    return 0;
}
