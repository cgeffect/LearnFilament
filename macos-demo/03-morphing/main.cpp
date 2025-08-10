#include <filament/Engine.h>
#include <filament/Renderer.h>
#include <filament/Scene.h>
#include <filament/View.h>
#include <filament/Camera.h>
#include <filament/VertexBuffer.h>
#include <filament/IndexBuffer.h>
#include <filament/Material.h>
#include <filament/MaterialInstance.h>
#include <filament/RenderableManager.h>
#include <filament/TransformManager.h>
#include <filament/Skybox.h>
#include <filament/SwapChain.h>
#include <filament/Viewport.h>
#include <filament/SkinningBuffer.h>

#include <utils/EntityManager.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_metal.h>

#include <cmath>
#include <chrono>
#include <iostream>

using namespace filament;
using utils::Entity;

// 预编译的材质数据
static constexpr uint8_t BAKED_COLOR_PACKAGE[] = {
#include "../01-triangle/bakedColor.inc"
};

// ========================================
// 顶点数据结构定义
// ========================================
// 每个顶点包含两个信息：
// 1. position: 顶点的位置坐标 (x, y)
// 2. color: 顶点的颜色值 (RGBA格式)
struct Vertex {
    filament::math::float2 position;  // 2D位置坐标，使用float2类型 (x, y)
    uint32_t color;                   // 颜色值，使用32位无符号整数存储RGBA
};

// 辅助函数：将RGBA分量组合成32位颜色值
// 虽然我们想用 {255, 0, 0, 255} 这样的格式来定义颜色
// 但最终必须打包成一个 uint32_t 传给 GPU
// 格式：0xAARRGGBB (A=透明度, R=红, G=绿, B=蓝)
inline uint32_t makeColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    return (a << 24) | (r << 16) | (g << 8) | b;
}

// ========================================
// 三角形顶点数据详解
// ========================================
// 我们定义一个三角形，三个顶点分别位于：
// 1. 顶点0: (1, 0) - 正右方，红色
// 2. 顶点1: (-0.5, 0.866) - 左上方，绿色  
// 3. 顶点2: (-0.5, -0.866) - 左下方，蓝色
static const Vertex TRIANGLE_VERTICES[3] = {
    {{1.0f, 0.0f}, makeColor(255, 0, 0, 255)},      // 顶点0: 正右方，红色
    {{-0.5f, 0.866f}, makeColor(0, 255, 0, 255)},   // 顶点1: 左上方，绿色
    {{-0.5f, -0.866f}, makeColor(0, 0, 255, 255)},  // 顶点2: 左下方，蓝色
};

// ========================================
// 变形目标数据详解
// ========================================
// 变形目标定义了顶点在不同状态下的位置
// 这里定义了两个变形目标，让三角形可以变形

// 第一个变形目标：将三角形变形为不同的形状
static const filament::math::float3 MORPH_TARGET_1[3] = {
    {-2.0f, 0.0f, 0.0f},   // 顶点0移动到左方
    {0.0f, 2.0f, 0.0f},    // 顶点1移动到上方
    {1.0f, 0.0f, 0.0f},    // 顶点2移动到右方
};

// 第二个变形目标：另一种变形形状
static const filament::math::float3 MORPH_TARGET_2[3] = {
    {0.0f, 2.0f, 0.0f},    // 顶点0移动到上方
    {-2.0f, 0.0f, 0.0f},   // 顶点1移动到左方
    {1.0f, 0.0f, 0.0f},    // 顶点2移动到右方
};

// 切线数据（这里不使用，但需要提供）
static const filament::math::short4 MORPH_TANGENTS[3] = {
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0}
};

// ========================================
// 三角形索引数据详解
// ========================================
// 索引定义了如何连接顶点形成三角形：
// 索引 [0, 1, 2] 表示：
// - 从顶点0连接到顶点1
// - 从顶点1连接到顶点2  
// - 从顶点2连接回顶点0
// 这样就形成了一个完整的三角形
static constexpr uint16_t TRIANGLE_INDICES[3] = { 0, 1, 2 };

int main() {
    // ========================================
    // 第一步：初始化 SDL 和创建窗口
    // ========================================
    // SDL 是一个跨平台的窗口和输入库，用于创建窗口和处理用户输入
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    // 创建 SDL 窗口，指定大小和标志
    // SDL_WINDOW_METAL: 启用 Metal 支持（macOS 的图形 API）
    // SDL_WINDOW_RESIZABLE: 允许用户调整窗口大小
    SDL_Window* window = SDL_CreateWindow("Hello Morphing", 
                                         800, 600, 
                                         SDL_WINDOW_METAL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // 创建 SDL 渲染器（虽然 Filament 会处理实际渲染，但 SDL 需要这个）
    SDL_Renderer* sdlRenderer = SDL_CreateRenderer(window, nullptr);
    if (!sdlRenderer) {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // 创建 Metal 视图，这是 Filament 与 Metal 图形 API 交互的桥梁
    SDL_MetalView metalView = SDL_Metal_CreateView(window);
    if (!metalView) {
        std::cerr << "Failed to create Metal view: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(sdlRenderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // 获取 Metal 层，Filament 需要这个来创建渲染目标
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
    // 创建 Filament 引擎，这是所有 Filament 功能的核心
    // backend::Backend::METAL 指定使用 Metal 图形 API（macOS 专用）
    Engine* engine = Engine::create(backend::Backend::METAL);
    if (!engine) {
        std::cerr << "Failed to create Filament engine" << std::endl;
        SDL_Metal_DestroyView(metalView);
        SDL_DestroyRenderer(sdlRenderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // 创建交换链（SwapChain），用于在 CPU 和 GPU 之间交换渲染结果
    // 它连接 Filament 和窗口系统，确保渲染内容能显示在屏幕上
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

    // 创建 Filament 的三个核心组件：
    // 1. Renderer: 负责实际的渲染工作
    // 2. Scene: 包含所有要渲染的对象（几何体、光源等）
    // 3. View: 定义如何观察场景（相机、视口等）
    Renderer* renderer = engine->createRenderer();
    Scene* scene = engine->createScene();
    View* view = engine->createView();

    // ========================================
    // 第三步：设置场景环境（天空盒和相机）
    // ========================================
    // 创建天空盒，为场景提供背景色
    // 这里设置为深蓝色 {0.1, 0.125, 0.25, 1.0}，最后一个参数是透明度
    Skybox* skybox = Skybox::Builder().color({0.1, 0.125, 0.25, 1.0}).build(*engine);
    scene->setSkybox(skybox);

    // 创建相机，用于观察场景
    // Entity 是 Filament 中所有对象的标识符
    Entity camera = utils::EntityManager::get().create();
    Camera* cam = engine->createCamera(camera);
    view->setCamera(cam);  // 将相机绑定到视图

    // ========================================
    // 第四步：创建几何体数据（顶点和索引缓冲区）
    // ========================================
    // 顶点缓冲区（VertexBuffer）存储三角形的顶点数据
    // 每个顶点包含位置（POSITION）和颜色（COLOR）信息
    VertexBuffer* vb = VertexBuffer::Builder()
        .vertexCount(3)  // 三角形有3个顶点
        .bufferCount(1)  // 使用1个缓冲区
        .attribute(VertexAttribute::POSITION, 0, VertexBuffer::AttributeType::FLOAT2, 0, 12)  // 位置：2个float，偏移0，步长12字节
        .attribute(VertexAttribute::COLOR, 0, VertexBuffer::AttributeType::UBYTE4, 8, 12)    // 颜色：4个字节，偏移8，步长12字节
        .normalized(VertexAttribute::COLOR)  // 颜色值需要归一化（0-255转换为0.0-1.0）
        .build(*engine);
    
    // 将顶点数据上传到GPU
    vb->setBufferAt(*engine, 0, VertexBuffer::BufferDescriptor(TRIANGLE_VERTICES, 36, nullptr));

    // 索引缓冲区（IndexBuffer）定义如何连接顶点形成三角形
    // 虽然这里三角形很简单（0,1,2），但索引缓冲区在复杂几何体中很有用
    IndexBuffer* ib = IndexBuffer::Builder()
        .indexCount(3)  // 3个索引
        .bufferType(IndexBuffer::IndexType::USHORT)  // 使用16位无符号整数
        .build(*engine);
    
    // 将索引数据上传到GPU
    ib->setBuffer(*engine, IndexBuffer::BufferDescriptor(TRIANGLE_INDICES, 6, nullptr));

    // ========================================
    // 第五步：创建变形目标缓冲区
    // ========================================
    // 变形目标缓冲区定义了顶点在不同状态下的位置
    // 这里创建两个变形目标，让三角形可以在不同形状之间变形
    MorphTargetBuffer* morphTargetBuffer = MorphTargetBuffer::Builder()
        .vertexCount(3)  // 3个顶点
        .count(2)        // 2个变形目标
        .build(*engine);

    // 设置第一个变形目标的位置数据
    morphTargetBuffer->setPositionsAt(*engine, 0, MORPH_TARGET_1, 3, 0);
    morphTargetBuffer->setTangentsAt(*engine, 0, MORPH_TANGENTS, 3, 0);

    // 设置第二个变形目标的位置数据
    morphTargetBuffer->setPositionsAt(*engine, 1, MORPH_TARGET_2, 3, 0);
    morphTargetBuffer->setTangentsAt(*engine, 1, MORPH_TANGENTS, 3, 0);

    // ========================================
    // 第六步：创建材质和可渲染实体
    // ========================================
    // 材质（Material）定义物体的外观，包括颜色、纹理、光照等
    // 这里使用预编译的材质包，包含着色器代码和材质参数
    Material* material = Material::Builder()
        .package((void*)BAKED_COLOR_PACKAGE, sizeof(BAKED_COLOR_PACKAGE))
        .build(*engine);

    // 创建可渲染实体（Renderable），这是 Filament 渲染的核心概念
    // 它将几何体（顶点+索引）和材质组合成一个可渲染的对象
    Entity renderable = utils::EntityManager::get().create();
    RenderableManager::Builder(1)  // 1表示有1个子网格
        .boundingBox({{ -1, -1, -1 }, { 1, 1, 1 }})  // 包围盒，用于视锥剔除
        .material(0, material->getDefaultInstance())  // 绑定材质到第0个子网格
        .geometry(0, RenderableManager::PrimitiveType::TRIANGLES, vb, ib, 0, 3)  // 绑定几何体
        .culling(false)      // 禁用背面剔除
        .receiveShadows(false)  // 不接收阴影
        .castShadows(false)     // 不投射阴影
        .morphing(morphTargetBuffer)  // 绑定变形目标缓冲区
        .build(*engine, renderable);

    // 将可渲染实体添加到场景中
    scene->addEntity(renderable);
    // 将场景绑定到视图
    view->setScene(scene);

    // ========================================
    // 第七步：设置渲染参数（视口和相机投影）
    // ========================================
    // 设置视口，定义渲染区域在窗口中的位置和大小
    // Viewport{0, 0, 800, 600} 表示从窗口左上角开始，渲染800x600像素
    view->setViewport(Viewport{0, 0, 800, 600});

    // 设置相机投影，定义如何将3D世界投影到2D屏幕
    // 这里使用正交投影（ORTHO），适合2D渲染
    constexpr float ZOOM = 1.5f;
    constexpr float ASPECT = 800.0f / 600.0f;
    cam->setProjection(Camera::Projection::ORTHO,
        -ASPECT * ZOOM, ASPECT * ZOOM,
        -ZOOM, ZOOM, 0, 1);

    // ========================================
    // 第八步：主渲染循环
    // ========================================
    bool running = true;
    auto startTime = std::chrono::high_resolution_clock::now();  // 记录开始时间，用于动画
    
    while (running) {
        // 处理用户输入事件（如关闭窗口）
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        // 计算动画时间，用于变形动画
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);
        float time = duration.count() / 1000.0f;  // 转换为秒

        // 计算变形权重，让三角形在两个变形目标之间平滑过渡
        // 使用正弦函数创建循环动画
        float morphWeight = (float)(sin(time) / 2.0f + 0.5f);  // 0.0 到 1.0 之间
        float weights[] = {1.0f - morphWeight, morphWeight};  // 两个变形目标的权重

        // 设置变形权重
        auto& rm = engine->getRenderableManager();
        rm.setMorphWeights(rm.getInstance(renderable), weights, 2, 0);

        // 执行渲染
        if (renderer->beginFrame(swapChain)) {  // 开始渲染帧
            renderer->render(view);              // 渲染视图
            renderer->endFrame();                // 结束渲染帧
        }
    }

    // ========================================
    // 第九步：清理资源（非常重要！）
    // ========================================
    // 按照创建的反序销毁 Filament 对象，避免内存泄漏
    engine->destroy(renderable);  // 销毁可渲染实体
    utils::EntityManager::get().destroy(renderable);  // 销毁实体ID
    engine->destroy(material);    // 销毁材质
    engine->destroy(vb);          // 销毁顶点缓冲区
    engine->destroy(ib);          // 销毁索引缓冲区
    engine->destroy(morphTargetBuffer);  // 销毁变形目标缓冲区
    engine->destroy(skybox);      // 销毁天空盒
    engine->destroyCameraComponent(camera);  // 销毁相机组件
    utils::EntityManager::get().destroy(camera);  // 销毁相机实体ID
    engine->destroy(view);        // 销毁视图
    engine->destroy(scene);       // 销毁场景
    engine->destroy(renderer);    // 销毁渲染器
    engine->destroy(swapChain);   // 销毁交换链
    engine->destroy(engine);      // 销毁引擎

    // 清理 SDL 资源
    SDL_Metal_DestroyView(metalView);
    SDL_DestroyRenderer(sdlRenderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
} 