#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/event.hpp>
#include <stdexcept>
#include <string>
#include "config_parser.h"
extern Config config;

using namespace ftxui;

static std::string config_page_info;

class ConfigPage {
public:
    ConfigPage() {
        this->input_recycle_bin_path = Input(&tmp.recycle_bin_path, &tmp.recycle_bin_path); 
        this->str_recycle_bin_max_size  = std::to_string(tmp.recycle_bin_max_size);
        this->recycle_bin_max_size = Input(&this->str_recycle_bin_max_size, std::to_string(tmp.recycle_bin_max_size));


        auto option = ButtonOption::Animated();
        this->save_button = Button("Save", [&](){
            this->save();
        });

        this->container = Container::Vertical({
            this->input_recycle_bin_path,
            this->recycle_bin_max_size,
            this->save_button,
        });

        this->component = Renderer(this->container, [&]() {
            return vbox({
                    hbox({
                        text("Recycle bin path") | size(WIDTH, EQUAL, 25) ,
                        separator(),
                        this->input_recycle_bin_path->Render() |  size(WIDTH, ftxui::LESS_THAN, 40)|xflex,
                    }) | xflex,
                    separator(),
                    hbox({
                        text("Recycle bin max_size(GB)") | size(WIDTH, EQUAL,25) ,
                        separator(),
                        this->recycle_bin_max_size->Render() |  size(WIDTH, ftxui::LESS_THAN, 40)|xflex,
                    }) | xflex,
                    separator(),
                    this->save_button->Render() | size(WIDTH, EQUAL, 6) | center,
                    hbox()| yflex,
                    text(fmt::format("info: {}", config_page_info)) | border | size(HEIGHT, EQUAL, 3),
            }) | xflex;

        }) ;    
    }

    void save() {
        try {
            this->tmp.recycle_bin_max_size = std::stoll(str_recycle_bin_max_size);
            config.recycle_bin_path = tmp.recycle_bin_path;
            config.recycle_bin_max_size = this->tmp.recycle_bin_max_size;

            config.store();
            config.refresh();
            config_page_info = "Config saved successfully";
        } catch (std::runtime_error &e) {
            spdlog::error(e.what());
            config_page_info = "Failed to save config, e: " + std::string(e.what());
        }
    }

    Component component;
    Component container;
    Component input_recycle_bin_path;
    std::string str_recycle_bin_max_size = "";
    Component recycle_bin_max_size;
    Component save_button;
    Config tmp;
};