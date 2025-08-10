#include <iostream>
#include <cmath>
#include <chrono>

// SDL includes
#include <SDL3/SDL.h>

// Filament includes
#include <filament/Engine.h>
#include <filament/Renderer.h>
#include <filament/Scene.h>
#include <filament/Camera.h>
#include <filament/View.h>
#include <filament/Viewport.h>
#include <filament/VertexBuffer.h>
#include <filament/IndexBuffer.h>
#include <filament/Material.h>
#include <filament/MaterialInstance.h>
#include <filament/RenderableManager.h>
#include <filament/TransformManager.h>
#include <filament/Skybox.h>
#include <utils/EntityManager.h>
#include <backend/DriverEnums.h>

using namespace filament;
using utils::Entity;
using utils::EntityManager;

// Vertex structure for our triangle
struct Vertex {
    filament::math::float2 position;
    uint32_t color;
};

// Triangle vertices (equilateral triangle)
static const Vertex TRIANGLE_VERTICES[3] = {
    {{1, 0}, 0xffff0000u},                    // Red vertex
    {{cos(M_PI * 2 / 3), sin(M_PI * 2 / 3)}, 0xff00ff00u},  // Green vertex
    {{cos(M_PI * 4 / 3), sin(M_PI * 4 / 3)}, 0xff0000ffu},  // Blue vertex
};

static constexpr uint16_t TRIANGLE_INDICES[3] = { 0, 1, 2 };

// Include the pre-compiled material package
static constexpr uint8_t BAKED_COLOR_PACKAGE[] = {
#include "bakedColor.inc"
};

class TriangleRenderer {
private:
    Engine* engine = nullptr;
    Renderer* renderer = nullptr;
    Scene* scene = nullptr;
    View* view = nullptr;
    Camera* camera = nullptr;
    Entity cameraEntity;
    SwapChain* swapChain = nullptr;
    
    VertexBuffer* vertexBuffer = nullptr;
    IndexBuffer* indexBuffer = nullptr;
    Material* material = nullptr;
    MaterialInstance* materialInstance = nullptr;
    Entity renderableEntity;
    Skybox* skybox = nullptr;
    
    SDL_Window* window = nullptr;
    SDL_Renderer* sdlRenderer = nullptr;
    SDL_MetalView metalView = nullptr;
    
    int windowWidth = 800;
    int windowHeight = 600;
    
    std::chrono::high_resolution_clock::time_point startTime;

public:
    bool initialize() {
        std::cout << "Starting initialization..." << std::endl;
        
        // Initialize SDL
        std::cout << "Attempting to initialize SDL..." << std::endl;
        std::cout << "SDL_GetError before init: '" << SDL_GetError() << "'" << std::endl;
        bool result = SDL_Init(SDL_INIT_VIDEO);
        std::cout << "SDL_Init result: " << (result ? "true" : "false") << std::endl;
        if (!result) {
            std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
            return false;
        }
        
        std::cout << "SDL initialized successfully" << std::endl;
        
        // Create window with Metal support
        window = SDL_CreateWindow("Filament Triangle Demo", 
                                 windowWidth, windowHeight, 
                                 SDL_WINDOW_METAL | SDL_WINDOW_RESIZABLE);
        if (!window) {
            std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
            return false;
        }
        
        std::cout << "Window created successfully" << std::endl;
        
        // Create renderer
        sdlRenderer = SDL_CreateRenderer(window, NULL);
        if (!sdlRenderer) {
            std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
            return false;
        }
        
        std::cout << "Renderer created successfully" << std::endl;
        
        // Initialize Filament with Metal backend
        engine = Engine::create(backend::Backend::METAL);
        if (!engine) {
            std::cerr << "Failed to create Filament engine" << std::endl;
            return false;
        }
        
        std::cout << "Filament engine created successfully" << std::endl;
        
        // Create SwapChain from SDL window
        // For SDL3, we need to create a Metal view first
        metalView = SDL_Metal_CreateView(window);
        if (!metalView) {
            std::cerr << "Failed to create Metal view: " << SDL_GetError() << std::endl;
            return false;
        }
        
        void* metalLayer = SDL_Metal_GetLayer(metalView);
        if (!metalLayer) {
            std::cerr << "Failed to get Metal layer: " << SDL_GetError() << std::endl;
            return false;
        }
        
        swapChain = engine->createSwapChain(metalLayer);
        if (!swapChain) {
            std::cerr << "Failed to create SwapChain" << std::endl;
            return false;
        }
        std::cout << "SwapChain created successfully" << std::endl;
        
        renderer = engine->createRenderer();
        if (!renderer) {
            std::cerr << "Failed to create renderer" << std::endl;
            return false;
        }
        
        scene = engine->createScene();
        if (!scene) {
            std::cerr << "Failed to create scene" << std::endl;
            return false;
        }
        
        view = engine->createView();
        if (!view) {
            std::cerr << "Failed to create view" << std::endl;
            return false;
        }
        
        // Create camera
        cameraEntity = EntityManager::get().create();
        camera = engine->createCamera(cameraEntity);
        view->setCamera(camera);
        
        // Create skybox
        skybox = Skybox::Builder().color({0.1f, 0.125f, 0.25f, 1.0f}).build(*engine);
        scene->setSkybox(skybox);
        
        // Disable post-processing for simplicity
        view->setPostProcessingEnabled(false);
        
        // Create vertex buffer
        static_assert(sizeof(Vertex) == 12, "Vertex size should be 12 bytes");
        vertexBuffer = VertexBuffer::Builder()
            .vertexCount(3)
            .bufferCount(1)
            .attribute(VertexAttribute::POSITION, 0, VertexBuffer::AttributeType::FLOAT2, 0, 12)
            .attribute(VertexAttribute::COLOR, 0, VertexBuffer::AttributeType::UBYTE4, 8, 12)
            .normalized(VertexAttribute::COLOR)
            .build(*engine);
        
        vertexBuffer->setBufferAt(*engine, 0,
            VertexBuffer::BufferDescriptor(TRIANGLE_VERTICES, 36, nullptr));
        
        // Create index buffer
        indexBuffer = IndexBuffer::Builder()
            .indexCount(3)
            .bufferType(IndexBuffer::IndexType::USHORT)
            .build(*engine);
        
        indexBuffer->setBuffer(*engine,
            IndexBuffer::BufferDescriptor(TRIANGLE_INDICES, 6, nullptr));
        
        // Create material using the pre-compiled package
        std::cout << "Creating material..." << std::endl;
        
        material = Material::Builder()
            .package((void*) BAKED_COLOR_PACKAGE, sizeof(BAKED_COLOR_PACKAGE))
            .build(*engine);
        
        if (!material) {
            std::cerr << "Failed to create material" << std::endl;
            return false;
        }
        
        materialInstance = material->getDefaultInstance();
        std::cout << "Material created successfully" << std::endl;
        
        // Create renderable
        renderableEntity = EntityManager::get().create();
        RenderableManager::Builder(1)
            .boundingBox({{ -1, -1, -1 }, { 1, 1, 1 }})
            .material(0, materialInstance)
            .geometry(0, RenderableManager::PrimitiveType::TRIANGLES, vertexBuffer, indexBuffer, 0, 3)
            .culling(false)
            .receiveShadows(false)
            .castShadows(false)
            .build(*engine, renderableEntity);
        
        std::cout << "Renderable created successfully" << std::endl;
        
        scene->addEntity(renderableEntity);
        
        // Set up view
        view->setScene(scene);
        
        // Set up viewport
        view->setViewport(Viewport{0, 0, (uint32_t)windowWidth, (uint32_t)windowHeight});
        
        startTime = std::chrono::high_resolution_clock::now();
        
        return true;
    }
    
    void render() {
        // Calculate time for rotation
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);
        double timeSeconds = duration.count() / 1000.0;
        
        // Update camera projection
        constexpr float ZOOM = 1.5f;
        const float aspect = (float)windowWidth / windowHeight;
        camera->setProjection(Camera::Projection::ORTHO,
            -aspect * ZOOM, aspect * ZOOM,
            -ZOOM, ZOOM, 0, 1);
        
        // Rotate the triangle
        auto& tcm = engine->getTransformManager();
        tcm.setTransform(tcm.getInstance(renderableEntity),
            filament::math::mat4f::rotation(timeSeconds, filament::math::float3{ 0, 0, 1 }));
        
        // Render
        if (renderer->beginFrame(swapChain)) {
            renderer->render(view);
            renderer->endFrame();
        }
    }
    
    bool handleEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    return false;
                case SDL_EVENT_WINDOW_RESIZED:
                    windowWidth = event.window.data1;
                    windowHeight = event.window.data2;
                    view->setViewport(Viewport{0, 0, (uint32_t)windowWidth, (uint32_t)windowHeight});
                    break;
            }
        }
        return true;
    }
    
    void cleanup() {
        if (engine) {
            engine->destroy(skybox);
            engine->destroy(renderableEntity);
            engine->destroy(materialInstance);
            engine->destroy(material);
            engine->destroy(vertexBuffer);
            engine->destroy(indexBuffer);
            engine->destroyCameraComponent(cameraEntity);
            EntityManager::get().destroy(cameraEntity);
            engine->destroy(view);
            engine->destroy(scene);
            engine->destroy(renderer);
            if (swapChain) {
                engine->destroy(swapChain);
            }
            Engine::destroy(&engine);
        }
        
        if (metalView) {
            SDL_Metal_DestroyView(metalView);
        }
        
        if (sdlRenderer) {
            SDL_DestroyRenderer(sdlRenderer);
        }
        
        if (window) {
            SDL_DestroyWindow(window);
        }
        
        SDL_Quit();
    }
    
    void run() {
        while (handleEvents()) {
            render();
            SDL_Delay(16); // ~60 FPS
        }
    }
};

int main() {
    TriangleRenderer renderer;
    
    if (!renderer.initialize()) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return 1;
    }
    
    std::cout << "Triangle demo started. Press Ctrl+C or close window to exit." << std::endl;
    
    renderer.run();
    renderer.cleanup();
    
    return 0;
}