#include <Core/Window.hpp>

#include "Minecraft.hpp"

int main(void) {
    Game::Init();
    while (!Window::ShouldClose()) {
        Game::Update();
        Game::Render();
    }
    Game::Destroy();
}
