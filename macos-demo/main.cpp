#include <iostream>
#include <filament/Engine.h>
#include <filament/Renderer.h>
#include <filament/Scene.h>
#include <filament/Camera.h>
#include <filament/View.h>
#include <filament/Texture.h>
#include <filament/Material.h>

int main() {
    filament::Engine* engine = filament::Engine::create();
    if (!engine) {
        std::cerr << "Failed to create engine" << std::endl;
        return 1;
    }
    filament::Renderer* renderer = engine->createRenderer();
    if (!renderer) {
        std::cerr << "Failed to create renderer" << std::endl;
        return 1;
    }
    filament::Scene* scene = engine->createScene();
    if (!scene) {
        std::cerr << "Failed to create scene" << std::endl;
        return 1;
    }
    
    std::cout << "Hello, World!" << std::endl;
    return 0;
}