#include <SDL3/SDL.h>
#include <SDL3/SDL_metal.h>
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
#include <filament/Texture.h>
#include <filament/TextureSampler.h>

#include <utils/EntityManager.h>
#include <filameshio/MeshReader.h>
#include "../generated/resources/resources.h"
#include <filament/Viewport.h>
#include <filament/Color.h>

#include <iostream>
#include <fstream>
#include <vector>

using namespace filament;
using namespace filament::math;
using namespace filamesh;
using utils::Entity;

int main() {
    // ========================================
    // 第一步：初始化SDL
    // ========================================
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    // 创建窗口
    SDL_Window* window = SDL_CreateWindow(
        "Filament Cube OBJ Demo",
        800, 600,
        SDL_WINDOW_METAL | SDL_WINDOW_RESIZABLE
    );
    if (!window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // 创建SDL渲染器
    SDL_Renderer* sdlRenderer = SDL_CreateRenderer(window, nullptr);
    if (!sdlRenderer) {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // 创建Metal视图
    SDL_MetalView metalView = SDL_Metal_CreateView(window);
    if (!metalView) {
        std::cerr << "Failed to create Metal view: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(sdlRenderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // 获取Metal层
    void* metalLayer = SDL_Metal_GetLayer(metalView);
    if (!metalLayer) {
        std::cerr << "Failed to get Metal layer" << std::endl;
        SDL_Metal_DestroyView(metalView);
        SDL_DestroyRenderer(sdlRenderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // ========================================
    // 第二步：创建Filament引擎
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

    // 创建渲染器、场景、视图和交换链
    Renderer* renderer = engine->createRenderer();
    Scene* scene = engine->createScene();
    View* view = engine->createView();
    SwapChain* swapChain = engine->createSwapChain(metalLayer);

    // ========================================
    // 第三步：加载cube.filamesh模型
    // ========================================
    std::string filameshPath = "/tmp/cube.filamesh";
    
    // 读取filamesh文件
    std::ifstream filameshFile(filameshPath, std::ios::binary);
    if (!filameshFile.is_open()) {
        std::cerr << "Failed to open filamesh file: " << filameshPath << std::endl;
        engine->destroy(engine);
        SDL_Metal_DestroyView(metalView);
        SDL_DestroyRenderer(sdlRenderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // 读取文件内容
    std::string filameshContent((std::istreambuf_iterator<char>(filameshFile)),
                                std::istreambuf_iterator<char>());
    filameshFile.close();

    // 使用MeshReader加载模型
    MeshReader::Mesh mesh = MeshReader::loadMeshFromBuffer(engine, 
        filameshContent.c_str(), nullptr, nullptr, nullptr);

    if (!mesh.renderable) {
        std::cerr << "Failed to load mesh from filamesh file" << std::endl;
        engine->destroy(engine);
        SDL_Metal_DestroyView(metalView);
        SDL_DestroyRenderer(sdlRenderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    std::cout << "Successfully loaded cube.filamesh model" << std::endl;

    // ========================================
    // 第四步：创建材质
    // ========================================
    Material* material = Material::Builder()
        .package(RESOURCES_BAKEDTEXTURE_DATA, RESOURCES_BAKEDTEXTURE_SIZE)
        .build(*engine);

    MaterialInstance* materialInstance = material->getDefaultInstance();

    // ========================================
    // 第五步：加载纹理
    // ========================================
    Texture* texture = nullptr;
    std::string texturePath = "/Users/jason/Jason/opengl/LearnFilament/macos-demo/rgba8_200x200.rgba";
    
    std::ifstream textureFile(texturePath, std::ios::binary);
    if (textureFile.is_open()) {
        std::vector<uint8_t> textureData(200 * 200 * 4);
        textureFile.read(reinterpret_cast<char*>(textureData.data()), textureData.size());
        textureFile.close();

        texture = Texture::Builder()
            .width(200)
            .height(200)
            .levels(1)
            .format(Texture::InternalFormat::RGBA8)
            .build(*engine);

        Texture::PixelBufferDescriptor buffer(
            textureData.data(),
            textureData.size(),
            Texture::Format::RGBA,
            Texture::Type::UBYTE
        );
        texture->setImage(*engine, 0, std::move(buffer));

        // 设置纹理到材质
        TextureSampler sampler(TextureSampler::MinFilter::LINEAR, TextureSampler::MagFilter::LINEAR);
        sampler.setWrapModeS(TextureSampler::WrapMode::CLAMP_TO_EDGE);
        sampler.setWrapModeT(TextureSampler::WrapMode::CLAMP_TO_EDGE);
        materialInstance->setParameter("albedo", texture, sampler);

        std::cout << "Texture loaded successfully" << std::endl;
    } else {
        std::cerr << "Failed to load texture, using default material" << std::endl;
    }

    // ========================================
    // 第六步：设置模型材质
    // ========================================
    auto& rcm = engine->getRenderableManager();
    rcm.setMaterialInstanceAt(rcm.getInstance(mesh.renderable), 0, materialInstance);

    // 将模型添加到场景
    scene->addEntity(mesh.renderable);

    // ========================================
    // 第七步：设置相机
    // ========================================
    Entity camera = utils::EntityManager::get().create();
    Camera* cam = engine->createCamera(camera);
    view->setCamera(cam);

    // 使用透视投影
    constexpr double FOV = 45.0;
    constexpr double NEAR = 0.1;
    constexpr double FAR = 100.0;
    constexpr double ASPECT = 800.0 / 600.0;
    cam->setProjection(FOV, ASPECT, NEAR, FAR);

    // 设置相机位置
    cam->setModelMatrix(mat4f::translation(float3{0, 0, 3}));

    // ========================================
    // 第八步：设置场景
    // ========================================
    // 设置天空盒
    Skybox* skybox = Skybox::Builder().color({0.1, 0.1, 0.2, 1.0}).build(*engine);
    scene->setSkybox(skybox);

    view->setScene(scene);
    view->setViewport(Viewport{0, 0, 800, 600});

    // ========================================
    // 第九步：主渲染循环
    // ========================================
    bool running = true;
    auto startTime = std::chrono::high_resolution_clock::now();
    
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        // 计算动画时间
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);
        float time = duration.count() / 1000.0f;

        // 应用旋转变换
        auto& tcm = engine->getTransformManager();
        
        // 分阶段旋转：前8秒横向转，后8秒纵向转
        float rotationTime = fmod(time, 16.0f);
        float horizontalRotation = 0.0f;
        float verticalRotation = 0.0f;
        
        if (rotationTime < 8.0f) {
            horizontalRotation = (rotationTime / 8.0f) * 2.0f * M_PI;
        } else {
            horizontalRotation = 2.0f * M_PI;
            verticalRotation = ((rotationTime - 8.0f) / 8.0f) * 2.0f * M_PI;
        }
        
        tcm.setTransform(tcm.getInstance(mesh.renderable), 
            mat4f::rotation(horizontalRotation, float3{ 0, 1, 0 }) * 
            mat4f::rotation(verticalRotation, float3{ 1, 0, 0 }));

        // 执行渲染
        if (renderer->beginFrame(swapChain)) {
            renderer->setClearOptions({
                .clearColor = {0.1f, 0.1f, 0.2f, 1.0f},
                .clear = true
            });
            renderer->render(view);
            renderer->endFrame();
        }
    }

    // ========================================
    // 第十步：清理资源
    // ========================================
    // 先从场景中移除实体
    scene->remove(mesh.renderable);
    
    // 销毁渲染实体（这会自动清理所有相关的材质实例）
    engine->destroy(mesh.renderable);
    
    if (texture) {
        engine->destroy(texture);
    }
    engine->destroy(materialInstance);
    engine->destroy(material);
    engine->destroy(mesh.vertexBuffer);
    engine->destroy(mesh.indexBuffer);
    engine->destroy(skybox);
    engine->destroy(swapChain);
    engine->destroy(view);
    engine->destroy(scene);
    engine->destroy(renderer);
    engine->destroyCameraComponent(camera);
    utils::EntityManager::get().destroy(camera);

    engine->destroy(engine);

    SDL_Metal_DestroyView(metalView);
    SDL_DestroyRenderer(sdlRenderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
} 