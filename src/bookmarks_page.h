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

class BookmarkPage {
public:
    BookmarkPage& operator=(const BookmarkPage &) = delete;
    BookmarkPage() {
        this->compoent = Renderer([](){
            return vbox({
                text("test"),
            });
        });
    }

    Component compoent;
};