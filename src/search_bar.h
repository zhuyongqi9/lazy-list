#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>

using namespace ftxui;

class RegexButton {
public:
    RegexButton():clicked(false) {
        option = ButtonOption::Animated();
        option.transform = [this](const EntryState& s) {
            auto element = text(s.label);
            if (s.focused) {
              element |= bold;
              element |= color(Color::Red);
            }

            if (this->clicked) {
              element |= color(Color::Blue);
            }
            //return element | center | borderEmpty | flex;
            return element | center;
        };
        option.animated_colors.background.Set(Color(), Color());

        component = Button(".*", [this] {
            this->clicked = !this->clicked;
        }, this-> option);
    }

    Component render() {
        return component;
    }

    bool clicked;
    Component component; 
    ButtonOption option;
};

class CaseIngoreButton {
public:
    CaseIngoreButton():clicked(true) {
        this->option = ButtonOption::Animated();
        option.transform = [this](const EntryState& s) {
            auto element = text(s.label);
            if (s.focused) {
              element |= bold;
              element |= color(Color::Red);
            }
            if (this->clicked) 
              element |= color(Color::Grey100);
            //return element | center | borderEmpty | flex;
            return element | center;
        };
        option.animated_colors.background.Set(Color(), Color());
        component = Button("Aa", [this]{
            this->clicked = !this->clicked;
        }, option);
    }

    Component render() {
        return component;
    }

    bool clicked;
    Component component; 
    ButtonOption option;
};


class SearchInput {
public:
    SearchInput() {
        this->component = Input(text, "", InputOption::Default());
    }

    std::string text = "";
    Component component;
};

class SearchBar {
public:
    SearchBar() {
        search_input = SearchInput();
        button_regex = RegexButton();
        button_case_ignored = CaseIngoreButton();

        container = Container::Horizontal({
            search_input.component,
            button_case_ignored.component, 
            button_regex.component,
        });

        component = Renderer(this->container, [&]() {
            return flexbox({ 
                search_input.component->Render(),
                button_case_ignored.component->Render() | size(ftxui::WIDTH, ftxui::EQUAL, 3),
                button_regex.component->Render()| size(ftxui::WIDTH, ftxui::EQUAL, 3),
            });
        });
    }

    std::string& text() { return search_input.text; }
    Component component;
    Component container;

    SearchInput search_input;
    RegexButton button_regex;
    CaseIngoreButton button_case_ignored;

    bool regex() { return button_regex.clicked; }
    bool case_ignored() { return button_case_ignored.clicked; }
};