#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>

using namespace ftxui;

class RegexButton {
public:
    RegexButton& operator=(const RegexButton&) = delete;

    RegexButton():clicked(false) {
        auto option = ButtonOption::Animated();
        option.transform = [this](const EntryState& s) {
            auto element = text(s.label);
            if (s.focused) {
              element |= bold;
              element |= color(Color::Red);
            }

            if (this->clicked) {
              element |= color(Color::Green);
            }
            //return element | center | borderEmpty | flex;
            return element | center;
        };
        option.animated_colors.background.Set(Color(), Color());

        component = Button(".*", [this] {
            this->clicked = !this->clicked;
        }, option);
    }

    Component render() {
        return component;
    }

    bool clicked;
    Component component; 
};

class CaseIngoreButton {
public:
    CaseIngoreButton& operator=(const CaseIngoreButton&) = delete;
    CaseIngoreButton():clicked(true) {
        auto option = ButtonOption::Animated();
        option.transform = [this](const EntryState& s) {
            auto element = text(s.label);
            if (s.focused) {
              element |= bold;
              element |= color(Color::Red);
            }
            if (this->clicked) 
              element |= color(Color::Green);
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
};


class SearchInput {
public:
    SearchInput& operator=(const SearchInput &) = delete;

    SearchInput() {
        auto option = InputOption::Default();
        option.multiline = false;
        this->component = Input(&text, "", option);
    }

    std::string text = "";
    Component component;
};

class SearchBar {
public:
    SearchBar() {
        container = Container::Horizontal({
            search_input.component,
            button_case_ignored.component, 
            button_regex.component,
        });

        component = Renderer(this->container, [this]() {
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

    SearchInput search_input = SearchInput();
    RegexButton button_regex = RegexButton();
    CaseIngoreButton button_case_ignored = CaseIngoreButton();

    bool regex() { return button_regex.clicked; }
    bool case_ignored() { return button_case_ignored.clicked; }
};