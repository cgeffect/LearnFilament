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
#include "../generated/resources/resources.h"
#include <filament/Viewport.h>
#include <filament/Color.h>

#include <iostream>
#include <fstream>
#include <vector>

using namespace filament;
using namespace filament::math;
using utils::Entity;

// 使用BAKEDTEXTURE材质

// 立方体顶点数据：位置 + UV坐标
struct Vertex {
    float3 position;  // 3D位置坐标
    float2 uv;        // UV纹理坐标
};

// 颜色打包函数
inline uint32_t makeColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    return (a << 24) | (b << 16) | (g << 8) | r;
}

// 立方体的24个顶点（每个面4个顶点，6个面）
static const Vertex CUBE_VERTICES[] = {
    // 前面 (Z = 0.5)
    {{-0.5, -0.5,  0.5}, {0, 0}},  // 0
    {{ 0.5, -0.5,  0.5}, {1, 0}},  // 1
    {{ 0.5,  0.5,  0.5}, {1, 1}},  // 2
    {{-0.5,  0.5,  0.5}, {0, 1}},  // 3
    
    // 后面 (Z = -0.5)
    {{-0.5, -0.5, -0.5}, {0, 0}},  // 4
    {{ 0.5, -0.5, -0.5}, {1, 0}},  // 5
    {{ 0.5,  0.5, -0.5}, {1, 1}},  // 6
    {{-0.5,  0.5, -0.5}, {0, 1}},  // 7
    
    // 右面 (X = 0.5)
    {{ 0.5, -0.5, -0.5}, {0, 0}},  // 8
    {{ 0.5, -0.5,  0.5}, {1, 0}},  // 9
    {{ 0.5,  0.5,  0.5}, {1, 1}},  // 10
    {{ 0.5,  0.5, -0.5}, {0, 1}},  // 11
    
    // 左面 (X = -0.5)
    {{-0.5, -0.5, -0.5}, {0, 0}},  // 12
    {{-0.5, -0.5,  0.5}, {1, 0}},  // 13
    {{-0.5,  0.5,  0.5}, {1, 1}},  // 14
    {{-0.5,  0.5, -0.5}, {0, 1}},  // 15
    
    // 上面 (Y = 0.5)
    {{-0.5,  0.5, -0.5}, {0, 0}},  // 16
    {{ 0.5,  0.5, -0.5}, {1, 0}},  // 17
    {{ 0.5,  0.5,  0.5}, {1, 1}},  // 18
    {{-0.5,  0.5,  0.5}, {0, 1}},  // 19
    
    // 下面 (Y = -0.5)
    {{-0.5, -0.5, -0.5}, {0, 0}},  // 20
    {{ 0.5, -0.5, -0.5}, {1, 0}},  // 21
    {{ 0.5, -0.5,  0.5}, {1, 1}},  // 22
    {{-0.5, -0.5,  0.5}, {0, 1}},  // 23
};

// 立方体的索引数据（6个面，每个面2个三角形）
static const uint16_t CUBE_INDICES[] = {
    // 前面 (红色)
    0, 1, 2,  0, 2, 3,
    // 后面 (绿色)
    4, 5, 6,  4, 6, 7,
    // 右面 (蓝色)
    8, 9, 10,  8, 10, 11,
    // 左面 (黄色)
    12, 13, 14,  12, 14, 15,
    // 上面 (紫色)
    16, 17, 18,  16, 18, 19,
    // 下面 (青色)
    20, 21, 22,  20, 22, 23,
};

// 加载RGBA纹理数据
Texture* loadRGBATexture(Engine* engine, const std::string& filename, int width, int height) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open texture file: " << filename << std::endl;
        return nullptr;
    }
    
    // 读取RGBA数据
    std::vector<uint8_t> data(width * height * 4);
    file.read(reinterpret_cast<char*>(data.data()), data.size());
    file.close();
    
    // 检查文件大小
    size_t expectedSize = width * height * 4;
    if (data.size() != expectedSize) {
        std::cerr << "Texture file size mismatch. Expected: " << expectedSize 
                  << ", Got: " << data.size() << std::endl;
        return nullptr;
    }
    
    // 创建纹理
    Texture* texture = Texture::Builder()
        .width(width)
        .height(height)
        .levels(1)
        .format(Texture::InternalFormat::RGBA8)
        .build(*engine);
    
    // 上传纹理数据
    Texture::PixelBufferDescriptor buffer(
        data.data(),
        data.size(),
        Texture::Format::RGBA,
        Texture::Type::UBYTE
    );
    texture->setImage(*engine, 0, std::move(buffer));
    
    std::cout << "Texture loaded successfully: " << width << "x" << height 
              << ", size: " << data.size() << " bytes" << std::endl;
    
    return texture;
}

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
        "Filament Cube Map Demo",
        800, 600,
        SDL_WINDOW_METAL | SDL_WINDOW_RESIZABLE
    );
    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
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
    // 第二步：初始化Filament
    // ========================================
    Engine* engine = Engine::create(backend::Backend::METAL);
    if (!engine) {
        std::cerr << "Failed to create Filament engine" << std::endl;
        SDL_Metal_DestroyView(metalView);
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
    // 第三步：创建顶点缓冲区和索引缓冲区
    // ========================================
    VertexBuffer* vertexBuffer = VertexBuffer::Builder()
        .vertexCount(24)  // 24个顶点
        .bufferCount(1)  // 1个缓冲区
        .attribute(VertexAttribute::POSITION, 0, VertexBuffer::AttributeType::FLOAT3, 0, 20)  // 位置：3个float，偏移0，步长20字节
        .attribute(VertexAttribute::UV0, 0, VertexBuffer::AttributeType::FLOAT2, 12, 20)     // UV：2个float，偏移12，步长20字节
        .build(*engine);

    IndexBuffer* indexBuffer = IndexBuffer::Builder()
        .indexCount(36)  // 36个索引（6个面 × 6个索引）
        .bufferType(IndexBuffer::IndexType::USHORT)
        .build(*engine);

    // 上传顶点数据
    vertexBuffer->setBufferAt(*engine, 0, VertexBuffer::BufferDescriptor(CUBE_VERTICES, sizeof(CUBE_VERTICES), nullptr));

    // 上传索引数据
    indexBuffer->setBuffer(*engine, IndexBuffer::BufferDescriptor(CUBE_INDICES, sizeof(CUBE_INDICES)));

    // ========================================
    // 第四步：加载纹理
    // ========================================
    Texture* texture = loadRGBATexture(engine, 
        "/Users/jason/Jason/opengl/LearnFilament/macos-demo/rgba8_200x200.rgba", 
        200, 200);
    if (!texture) {
        std::cerr << "Failed to load texture" << std::endl;
        engine->destroy(engine);
        SDL_Metal_DestroyView(metalView);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // ========================================
    // 第五步：创建材质
    // ========================================
    Material* material = Material::Builder()
        .package(RESOURCES_BAKEDTEXTURE_DATA, RESOURCES_BAKEDTEXTURE_SIZE)
        .build(*engine);

    MaterialInstance* materialInstance = material->getDefaultInstance();
    
    // 设置纹理到材质
    TextureSampler sampler(TextureSampler::MinFilter::LINEAR, TextureSampler::MagFilter::LINEAR);
    sampler.setWrapModeS(TextureSampler::WrapMode::CLAMP_TO_EDGE);
    sampler.setWrapModeT(TextureSampler::WrapMode::CLAMP_TO_EDGE);
    materialInstance->setParameter("albedo", texture, sampler);

    // ========================================
    // 第六步：创建渲染实体
    // ========================================
    Entity cube = utils::EntityManager::get().create();

    RenderableManager::Builder(1)
        .boundingBox({{ -0.5, -0.5, -0.5 }, { 0.5, 0.5, 0.5 }})
        .material(0, materialInstance)
        .geometry(0, RenderableManager::PrimitiveType::TRIANGLES, vertexBuffer, indexBuffer)
        .culling(false)
        .receiveShadows(false)
        .castShadows(false)
        .build(*engine, cube);

    scene->addEntity(cube);

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

    // 设置相机位置，稍微偏移以便更好地观察立方体
    cam->setModelMatrix(mat4f::translation(float3{0, 0, 3}));

    view->setScene(scene);
    view->setViewport(Viewport{0, 0, 800, 600});

    // ========================================
    // 第八步：主渲染循环
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

        // 应用旋转变换：先横向转一圈，然后纵向转一圈
        auto& tcm = engine->getTransformManager();
        
        // 分阶段旋转：前8秒横向转，后8秒纵向转
        float rotationTime = fmod(time, 16.0f);  // 16秒一个完整周期
        float horizontalRotation = 0.0f;
        float verticalRotation = 0.0f;
        
        if (rotationTime < 8.0f) {
            // 前8秒：横向旋转一圈
            horizontalRotation = (rotationTime / 8.0f) * 2.0f * M_PI;
        } else {
            // 后8秒：纵向旋转一圈
            horizontalRotation = 2.0f * M_PI;  // 保持横向旋转完成
            verticalRotation = ((rotationTime - 8.0f) / 8.0f) * 2.0f * M_PI;
        }
        
        tcm.setTransform(tcm.getInstance(cube), 
            mat4f::rotation(horizontalRotation, float3{ 0, 1, 0 }) * 
            mat4f::rotation(verticalRotation, float3{ 1, 0, 0 }));

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
    engine->destroy(cube);
    utils::EntityManager::get().destroy(cube);
    engine->destroy(camera);
    utils::EntityManager::get().destroy(camera);
    engine->destroy(materialInstance);
    engine->destroy(material);
    engine->destroy(texture);
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