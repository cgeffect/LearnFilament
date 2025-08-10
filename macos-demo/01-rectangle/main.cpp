#include <filament/Engine.h>
#include <filament/Renderer.h>
#include <filament/Scene.h>
#include <filament/View.h>
#include <filament/Camera.h>
#include <filament/Material.h>
#include <filament/MaterialInstance.h>
#include <filament/RenderableManager.h>
#include <filament/TransformManager.h>
#include <filament/SwapChain.h>
#include <filament/Viewport.h>
#include <filament/VertexBuffer.h>
#include <filament/IndexBuffer.h>

#include <utils/EntityManager.h>

// 预编译的材质数据
static constexpr uint8_t BAKED_COLOR_PACKAGE[] = {
#include "bakedColor.inc"
};

#include <SDL3/SDL.h>
#include <SDL3/SDL_metal.h>

#include <cmath>
#include <chrono>
#include <iostream>

using namespace filament;
using utils::Entity;

// 颜色打包函数
static uint32_t makeColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return (uint32_t(a) << 24) | (uint32_t(b) << 16) | (uint32_t(g) << 8) | uint32_t(r);
}

int main() {
    // ========================================
    // 第一步：初始化 SDL 和创建窗口
    // ========================================
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Hello Rectangle", 
                                         800, 600, 
                                         SDL_WINDOW_METAL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* sdlRenderer = SDL_CreateRenderer(window, nullptr);
    if (!sdlRenderer) {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_MetalView metalView = SDL_Metal_CreateView(window);
    if (!metalView) {
        std::cerr << "Failed to create Metal view: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(sdlRenderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    void* metalLayer = SDL_Metal_GetLayer(metalView);
    if (!metalLayer) {
        std::cerr << "Failed to get Metal layer: " << SDL_GetError() << std::endl;
        SDL_Metal_DestroyView(metalView);
        SDL_DestroyRenderer(sdlRenderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // ========================================
    // 第二步：初始化 Filament 引擎和核心组件
    // ========================================
    Engine* engine = Engine::create(backend::Backend::METAL);
    if (!engine) {
        std::cerr << "Failed to create Filament engine" << std::endl;
        SDL_Metal_DestroyView(metalView);
        SDL_DestroyRenderer(sdlRenderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SwapChain* swapChain = engine->createSwapChain(metalLayer);
    if (!swapChain) {
        std::cerr << "Failed to create SwapChain" << std::endl;
        engine->destroy(engine);
        SDL_Metal_DestroyView(metalView);
        SDL_DestroyRenderer(sdlRenderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    Renderer* renderer = engine->createRenderer();
    Scene* scene = engine->createScene();
    View* view = engine->createView();

    // ========================================
    // 第三步：创建矩形顶点数据
    // ========================================
    // 矩形顶点：位置 (x, y) + 颜色 (r, g, b, a)
    struct Vertex {
        filament::math::float2 position;  // 2D位置坐标
        uint32_t color;                   // 颜色值（打包的 RGBA）
    };

    // 定义矩形的四个顶点（橙色）
    Vertex vertices[] = {
        // 左下角 - 橙色
        {{-0.5f, -0.3f}, makeColor(255, 165, 0, 255)},
        // 右下角 - 橙色
        {{ 0.5f, -0.3f}, makeColor(255, 165, 0, 255)},
        // 右上角 - 橙色
        {{ 0.5f,  0.3f}, makeColor(255, 165, 0, 255)},
        // 左上角 - 橙色
        {{-0.5f,  0.3f}, makeColor(255, 165, 0, 255)}
    };

    // 定义索引（两个三角形组成一个矩形）
    uint16_t indices[] = {
        0, 1, 2,  // 第一个三角形：左下、右下、右上
        0, 2, 3   // 第二个三角形：左下、右上、左上
    };

    // ========================================
    // 第四步：创建顶点缓冲区和索引缓冲区
    // ========================================
    VertexBuffer* vertexBuffer = VertexBuffer::Builder()
        .vertexCount(4)  // 4个顶点
        .bufferCount(1)  // 1个缓冲区
        .attribute(VertexAttribute::POSITION, 0, VertexBuffer::AttributeType::FLOAT2, 0, 12)  // 位置：2个float，偏移0，步长12字节
        .attribute(VertexAttribute::COLOR, 0, VertexBuffer::AttributeType::UBYTE4, 8, 12)    // 颜色：4个字节，偏移8，步长12字节
        .normalized(VertexAttribute::COLOR)
        .build(*engine);

    IndexBuffer* indexBuffer = IndexBuffer::Builder()
        .indexCount(6)  // 6个索引
        .bufferType(IndexBuffer::IndexType::USHORT)  // 使用16位无符号整数
        .build(*engine);

    // 上传顶点数据
    vertexBuffer->setBufferAt(*engine, 0, VertexBuffer::BufferDescriptor(vertices, sizeof(vertices), nullptr));

    // 上传索引数据
    indexBuffer->setBuffer(*engine, IndexBuffer::BufferDescriptor(indices, sizeof(indices)));

    // ========================================
    // 第五步：创建材质
    // ========================================
    // 使用预编译的材质包
    Material* material = Material::Builder()
        .package(BAKED_COLOR_PACKAGE, sizeof(BAKED_COLOR_PACKAGE))
        .build(*engine);

    MaterialInstance* materialInstance = material->getDefaultInstance();

    // ========================================
    // 第六步：创建渲染实体
    // ========================================
    Entity rectangle = utils::EntityManager::get().create();

    RenderableManager::Builder(1)
        .boundingBox({{ -0.5f, -0.3f, -0.1f }, { 0.5f, 0.3f, 0.1f }})
        .material(0, materialInstance)
        .geometry(0, RenderableManager::PrimitiveType::TRIANGLES, vertexBuffer, indexBuffer)
        .culling(false)
        .receiveShadows(false)
        .castShadows(false)
        .build(*engine, rectangle);

    scene->addEntity(rectangle);

    // ========================================
    // 第七步：设置相机
    // ========================================
    Entity camera = utils::EntityManager::get().create();
    Camera* cam = engine->createCamera(camera);
    view->setCamera(cam);

    // 使用正交投影，适合2D渲染
    constexpr float ZOOM = 1.5f;  // 缩放因子
    constexpr float ASPECT = 800.0f / 600.0f;  // 宽高比
    cam->setProjection(Camera::Projection::ORTHO,
        -ASPECT * ZOOM, ASPECT * ZOOM,  // 左右边界
        -ZOOM, ZOOM,                    // 上下边界
        0, 1);                          // 近远平面

    view->setScene(scene);
    view->setViewport(Viewport{0, 0, 800, 600});

    // ========================================
    // 第八步：主渲染循环
    // ========================================
    bool running = true;
    
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        // 执行渲染
        if (renderer->beginFrame(swapChain)) {
            // 设置清除颜色为深蓝色
            renderer->setClearOptions({
                .clearColor = {0.1f, 0.1f, 0.2f, 1.0f},
                .clear = true
            });
            renderer->render(view);
            renderer->endFrame();
        }
    }

    // ========================================
    // 第九步：清理资源
    // ========================================
    engine->destroy(rectangle);
    utils::EntityManager::get().destroy(rectangle);
    engine->destroy(camera);
    utils::EntityManager::get().destroy(camera);
    engine->destroy(materialInstance);
    engine->destroy(material);
    engine->destroy(vertexBuffer);
    engine->destroy(indexBuffer);
    engine->destroy(view);
    engine->destroy(scene);
    engine->destroy(renderer);
    engine->destroy(swapChain);
    engine->destroy(engine);

    SDL_Metal_DestroyView(metalView);
    SDL_DestroyRenderer(sdlRenderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
} 