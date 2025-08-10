#include <filament/Engine.h>
#include <filament/Renderer.h>
#include <filament/Scene.h>
#include <filament/View.h>
#include <filament/Camera.h>
#include <filament/Material.h>
#include <filament/MaterialInstance.h>
#include <filament/RenderableManager.h>
#include <filament/TransformManager.h>
#include <filament/Skybox.h>
#include <filament/SwapChain.h>
#include <filament/Viewport.h>
#include <filament/LightManager.h>

#include <utils/EntityManager.h>

#include <filameshio/MeshReader.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_metal.h>

#include <cmath>
#include <chrono>
#include <iostream>

// 包含原始的资源文件
#include "../generated/resources/resources.h"
#include "../generated/resources/monkey.h"

using namespace filament;
using namespace filamesh;
using namespace filament::math;
using utils::Entity;

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
    SDL_Window* window = SDL_CreateWindow("Hello PBR", 
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
    // 第四步：加载猴头模型
    // ========================================
    // 使用原始的猴头模型数据，这是 Filament 示例中使用的标准模型
    MeshReader::Mesh mesh = MeshReader::loadMeshFromBuffer(engine, MONKEY_SUZANNE_DATA, nullptr, nullptr, nullptr);

    // ========================================
    // 第五步：创建PBR材质
    // ========================================
    // 使用原始的PBR材质，这是 Filament 示例中使用的标准材质
    Material* material = Material::Builder()
        .package(RESOURCES_AIDEFAULTMAT_DATA, RESOURCES_AIDEFAULTMAT_SIZE)
        .build(*engine);

    // 创建材质实例并设置PBR参数
    MaterialInstance* materialInstance = material->createInstance();
    // 设置基础颜色（线性RGB空间）
    materialInstance->setParameter("baseColor", RgbType::LINEAR, float3{0.8f});
    // 设置金属度（0.0 = 非金属，1.0 = 金属）
    materialInstance->setParameter("metallic", 1.0f);
    // 设置粗糙度（0.0 = 完全光滑，1.0 = 完全粗糙）
    materialInstance->setParameter("roughness", 0.4f);
    // 设置反射率（控制菲涅尔反射的强度）
    materialInstance->setParameter("reflectance", 0.5f);

    // ========================================
    // 第六步：设置猴头模型的材质和变换
    // ========================================
    // 将材质应用到猴头模型
    auto& rcm = engine->getRenderableManager();
    auto& tcm = engine->getTransformManager();
    
    // 获取猴头模型的渲染实例并设置材质
    auto ri = rcm.getInstance(mesh.renderable);
    rcm.setMaterialInstanceAt(ri, 0, materialInstance);
    
    // 设置猴头模型不投射阴影
    rcm.setCastShadows(ri, false);
    
    // 设置猴头模型的初始变换（位置和缩放）
    auto ti = tcm.getInstance(mesh.renderable);
    mat4f transform = mat4f{ mat3f(1), float3(0, 0, -4) } * tcm.getWorldTransform(ti);
    tcm.setTransform(ti, transform);
    
    // 将猴头模型添加到场景中
    scene->addEntity(mesh.renderable);

    // ========================================
    // 第七步：添加光源
    // ========================================
    // 创建太阳光源
    Entity light = utils::EntityManager::get().create();
    LightManager::Builder(LightManager::Type::SUN)
        .color(filament::Color::toLinear<filament::ACCURATE>(filament::sRGBColor(0.98f, 0.92f, 0.89f)))
        .intensity(110000.0f)
        .direction({ 0.7f, -1.0f, -0.8f })
        .sunAngularRadius(1.9f)
        .castShadows(false)
        .build(*engine, light);
    scene->addEntity(light);

    // 将场景绑定到视图
    view->setScene(scene);

    // ========================================
    // 第八步：设置渲染参数（视口和相机投影）
    // ========================================
    // 设置视口，定义渲染区域在窗口中的位置和大小
    // Viewport{0, 0, 800, 600} 表示从窗口左上角开始，渲染800x600像素
    view->setViewport(Viewport{0, 0, 800, 600});

    // 设置相机投影，定义如何将3D世界投影到2D屏幕
    // 这里使用透视投影（PERSPECTIVE），适合3D渲染
    constexpr double FOV = 45.0;  // 视野角度
    constexpr double NEAR = 0.1;  // 近平面
    constexpr double FAR = 100.0; // 远平面
    constexpr double ASPECT = 800.0 / 600.0;  // 宽高比
    cam->setProjection(FOV, ASPECT, NEAR, FAR);

    // 设置相机位置，让球体在视野中
    cam->setModelMatrix(filament::math::mat4f::translation(filament::math::float3{0, 0, 3}));

    // ========================================
    // 第九步：主渲染循环
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

        // 应用旋转变换，让猴头模型绕Y轴旋转
        auto& tcm = engine->getTransformManager();  // 获取变换管理器
        auto ti = tcm.getInstance(mesh.renderable);  // 获取猴头模型的变换实例
        tcm.setTransform(ti, transform * mat4f::rotation(time, float3{ 0, 1, 0 }));  // 绕Y轴旋转

        // 执行渲染
        if (renderer->beginFrame(swapChain)) {  // 开始渲染帧
            renderer->render(view);              // 渲染视图
            renderer->endFrame();                // 结束渲染帧
        }
    }

    // ========================================
    // 第十步：清理资源（非常重要！）
    // ========================================
    // 按照创建的反序销毁 Filament 对象，避免内存泄漏
    engine->destroy(mesh.renderable);  // 销毁猴头模型
    utils::EntityManager::get().destroy(mesh.renderable);  // 销毁猴头模型实体ID
    engine->destroy(light);       // 销毁光源
    utils::EntityManager::get().destroy(light);  // 销毁光源实体ID
    engine->destroy(materialInstance);  // 销毁材质实例
    engine->destroy(material);    // 销毁材质
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