#include <editor/Editor.hpp>
#include <iostream>

int main(int argc, const char** argv) {
    if (argc > 2) {
        std::cerr << "Usage: " << argv[0] << " [project folder]" << '\n';
        return 84;
    }
    try {
        if (argc == 2) {
            ampersand::editor::Editor gui(argv[1]);
            gui.run();
        } else {
            ampersand::editor::Editor gui;
            gui.run();
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 84;
    }
    return 0;
}
