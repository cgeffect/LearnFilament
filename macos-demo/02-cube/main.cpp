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
#include <filament/LightManager.h>

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
// 1. position: 顶点的位置坐标 (x, y, z)
// 2. color: 顶点的颜色值 (RGBA格式)
struct Vertex {
    filament::math::float3 position;  // 3D位置坐标，使用float3类型 (x, y, z)
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
// 立方体顶点数据详解
// ========================================
// 为了让每个面都是纯色，我们需要为每个面创建独立的顶点
// 立方体有6个面，每个面有4个顶点，总共24个顶点
// 每个面的所有顶点使用相同的颜色
static const Vertex CUBE_VERTICES[24] = {
    // 前面 (红色)
    {{-0.5f, -0.5f,  0.5f}, makeColor(255, 0, 0, 255)},    // 左下前
    {{ 0.5f, -0.5f,  0.5f}, makeColor(255, 0, 0, 255)},    // 右下前
    {{ 0.5f,  0.5f,  0.5f}, makeColor(255, 0, 0, 255)},    // 右上前
    {{-0.5f,  0.5f,  0.5f}, makeColor(255, 0, 0, 255)},    // 左上前
    
    // 后面 (绿色)
    {{-0.5f, -0.5f, -0.5f}, makeColor(0, 255, 0, 255)},    // 左下后
    {{ 0.5f, -0.5f, -0.5f}, makeColor(0, 255, 0, 255)},    // 右下后
    {{ 0.5f,  0.5f, -0.5f}, makeColor(0, 255, 0, 255)},    // 右上后
    {{-0.5f,  0.5f, -0.5f}, makeColor(0, 255, 0, 255)},    // 左上后
    
    // 左面 (蓝色)
    {{-0.5f, -0.5f, -0.5f}, makeColor(0, 0, 255, 255)},    // 左下后
    {{-0.5f, -0.5f,  0.5f}, makeColor(0, 0, 255, 255)},    // 左下前
    {{-0.5f,  0.5f,  0.5f}, makeColor(0, 0, 255, 255)},    // 左上前
    {{-0.5f,  0.5f, -0.5f}, makeColor(0, 0, 255, 255)},    // 左上后
    
    // 右面 (黄色)
    {{ 0.5f, -0.5f, -0.5f}, makeColor(255, 255, 0, 255)},  // 右下后
    {{ 0.5f, -0.5f,  0.5f}, makeColor(255, 255, 0, 255)},  // 右下前
    {{ 0.5f,  0.5f,  0.5f}, makeColor(255, 255, 0, 255)},  // 右上前
    {{ 0.5f,  0.5f, -0.5f}, makeColor(255, 255, 0, 255)},  // 右上后
    
    // 下面 (紫色)
    {{-0.5f, -0.5f, -0.5f}, makeColor(255, 0, 255, 255)},  // 左下后
    {{ 0.5f, -0.5f, -0.5f}, makeColor(255, 0, 255, 255)},  // 右下后
    {{ 0.5f, -0.5f,  0.5f}, makeColor(255, 0, 255, 255)},  // 右下前
    {{-0.5f, -0.5f,  0.5f}, makeColor(255, 0, 255, 255)},  // 左下前
    
    // 上面 (青色)
    {{-0.5f,  0.5f, -0.5f}, makeColor(0, 255, 255, 255)},  // 左上后
    {{ 0.5f,  0.5f, -0.5f}, makeColor(0, 255, 255, 255)},  // 右上后
    {{ 0.5f,  0.5f,  0.5f}, makeColor(0, 255, 255, 255)},  // 右上前
    {{-0.5f,  0.5f,  0.5f}, makeColor(0, 255, 255, 255)},  // 左上前
};

// ========================================
// 立方体索引数据详解
// ========================================
// 立方体有6个面，每个面由2个三角形组成，总共12个三角形
// 每个三角形需要3个索引，总共36个索引
// 索引定义了如何连接顶点形成三角形
// 现在每个面有4个独立顶点，索引需要相应调整
static constexpr uint16_t CUBE_INDICES[36] = {
    // 前面 (红色) - 顶点 0,1,2,3
    0, 1, 2,  0, 2, 3,
    // 后面 (绿色) - 顶点 4,5,6,7
    4, 6, 5,  4, 7, 6,
    // 左面 (蓝色) - 顶点 8,9,10,11
    8, 9, 10,  8, 10, 11,
    // 右面 (黄色) - 顶点 12,13,14,15
    12, 14, 13,  12, 15, 14,
    // 下面 (紫色) - 顶点 16,17,18,19
    16, 17, 18,  16, 18, 19,
    // 上面 (青色) - 顶点 20,21,22,23
    20, 22, 21,  20, 23, 22
};

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
    SDL_Window* window = SDL_CreateWindow("Hello Cube", 
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
    // 顶点缓冲区（VertexBuffer）存储立方体的顶点数据
    // 每个顶点包含位置（POSITION）和颜色（COLOR）信息
    VertexBuffer* vb = VertexBuffer::Builder()
        .vertexCount(24)  // 立方体有24个顶点（6个面，每个面4个顶点）
        .bufferCount(1)  // 使用1个缓冲区
        .attribute(VertexAttribute::POSITION, 0, VertexBuffer::AttributeType::FLOAT3, 0, 16)  // 位置：3个float，偏移0，步长16字节
        .attribute(VertexAttribute::COLOR, 0, VertexBuffer::AttributeType::UBYTE4, 12, 16)   // 颜色：4个字节，偏移12，步长16字节
        .normalized(VertexAttribute::COLOR)  // 颜色值需要归一化（0-255转换为0.0-1.0）
        .build(*engine);
    
    // 将顶点数据上传到GPU
    vb->setBufferAt(*engine, 0, VertexBuffer::BufferDescriptor(CUBE_VERTICES, 384, nullptr));  // 24个顶点 * 16字节 = 384字节

    // 索引缓冲区（IndexBuffer）定义如何连接顶点形成三角形
    // 立方体有12个三角形，每个三角形3个索引，总共36个索引
    IndexBuffer* ib = IndexBuffer::Builder()
        .indexCount(36)  // 36个索引
        .bufferType(IndexBuffer::IndexType::USHORT)  // 使用16位无符号整数
        .build(*engine);
    
    // 将索引数据上传到GPU
    ib->setBuffer(*engine, IndexBuffer::BufferDescriptor(CUBE_INDICES, 72, nullptr));

    // ========================================
    // 第五步：创建材质和可渲染实体
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
        .geometry(0, RenderableManager::PrimitiveType::TRIANGLES, vb, ib, 0, 36)  // 绑定几何体，36个索引
        .culling(false)      // 禁用背面剔除
        .receiveShadows(false)  // 不接收阴影
        .castShadows(false)     // 不投射阴影
        .build(*engine, renderable);

    // 将可渲染实体添加到场景中
    scene->addEntity(renderable);
    // 将场景绑定到视图
    view->setScene(scene);

    // ========================================
    // 第六步：设置渲染参数（视口和相机投影）
    // ========================================
    // 设置视口，定义渲染区域在窗口中的位置和大小
    // Viewport{0, 0, 800, 600} 表示从窗口左上角开始，渲染800x600像素
    view->setViewport(Viewport{0, 0, 800, 600});

    // 设置相机投影，定义如何将3D世界投影到2D屏幕
    // 这里使用透视投影，适合3D渲染
    constexpr double FOV = 45.0;  // 视野角度
    constexpr double NEAR = 0.1;  // 近平面
    constexpr double FAR = 100.0; // 远平面
    constexpr double ASPECT = 800.0 / 600.0;  // 宽高比
    cam->setProjection(FOV, ASPECT, NEAR, FAR);

    // 设置相机位置，让立方体在视野中
    // 稍微倾斜相机，这样可以看到立方体的多个面
    cam->setModelMatrix(filament::math::mat4f::translation(filament::math::float3{0, 0.3, 3}));

    // ========================================
    // 第七步：主渲染循环
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

        // 计算动画时间，用于旋转动画
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);
        float time = duration.count() / 1000.0f;  // 转换为秒

        // 应用旋转变换，让立方体绕Y轴旋转，并添加初始倾斜
        auto& tcm = engine->getTransformManager();  // 获取变换管理器
        
        // 创建一个复合变换：先倾斜，再旋转
        filament::math::mat4f initialTilt = filament::math::mat4f::rotation(0.3f, filament::math::float3{ 1, 0, 0 });  // 绕X轴倾斜
        filament::math::mat4f rotation = filament::math::mat4f::rotation(time, filament::math::float3{ 0, 1, 0 });     // 绕Y轴旋转
        filament::math::mat4f transform = rotation * initialTilt;  // 先倾斜，再旋转
        
        tcm.setTransform(tcm.getInstance(renderable), transform);  // 设置实体的变换

        // 执行渲染
        if (renderer->beginFrame(swapChain)) {  // 开始渲染帧
            renderer->render(view);              // 渲染视图
            renderer->endFrame();                // 结束渲染帧
        }
    }

    // ========================================
    // 第八步：清理资源（非常重要！）
    // ========================================
    // 按照创建的反序销毁 Filament 对象，避免内存泄漏
    engine->destroy(renderable);  // 销毁可渲染实体
    utils::EntityManager::get().destroy(renderable);  // 销毁实体ID
    engine->destroy(material);    // 销毁材质
    engine->destroy(vb);          // 销毁顶点缓冲区
    engine->destroy(ib);          // 销毁索引缓冲区
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