#include <editor/Editor.hpp>
#include <iostream>

int main(int argc, const char** argv) {
    if (argc > 2) {
        std::cerr << "Usage: " << argv[0] << " [project folder]" << '\n';
        return 84;
    }
    try {
        ampersand::editor::Editor gui = argc == 2
                                            ? ampersand::editor::Editor(argv[1])
                                            : ampersand::editor::Editor();

        gui.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 84;
    }
    return 0;
}
