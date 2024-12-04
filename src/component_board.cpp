#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include "file_operation_dialog.h"
#include <filesystem>

using namespace ftxui;

std::filesystem::path cur_dir = std::filesystem::current_path();

int main() {
    auto screen = ScreenInteractive::Fullscreen(); 
    auto dialog = FileOperationDialog(cur_dir);

    auto component = Renderer([&] {
        return gridbox({
            {text(cur_dir.string())}
        });
    });

    component |= Modal(dialog.component, &dialog.shown);
    dialog.shown = true;

    component |= CatchEvent([&](Event event) {
        if (event == Event::Character('q')) {
          screen.ExitLoopClosure()();
          return true;
        }

        return false;
    });

    screen.Loop(component);
    return 0;
}