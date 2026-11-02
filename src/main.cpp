#include <iostream>

#include "application.hpp"
#include "utils.hpp"

std::string scene = "../asset/scene/sponza.json";

int main(int argc, char** argv) {
    try {
        tinyglrenderer::Application app(1500, 900, "TinyGLRenderer");
        app.load(scene);
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}