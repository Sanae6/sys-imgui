#include <cstdlib>
#include <cstdio>
#include <switch.h>
#include <deko3d.hpp>
#include "App.hpp"
#include "ImguiService.hpp"
#include "logger.hpp"

#define FB_NUM    2
#define CODEMEMSIZE (64*1024)

// Define the size of the memory block that will hold code
#define CODEMEMSIZE (64*1024)

// Define the size of the memory block that will hold command lists
#define CMDMEMSIZE (16*1024)

App::App(const ImguiService &service, NWindow* window, dk::Device device) : service(service), window(window), device(device) {
    init();
}

void* App::readFile(const char* filename) {
    FILE* f = fopen(filename, "rb");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    rewind(f);

    char* code = (char*) malloc(fsize + 1);
    fread(code, 1, fsize, f);
    fclose(f);

    code[fsize] = 0;
    return code;
}

void App::loadShader(dk::Shader &shader, const char* filename) {
    log("loading shader at %s\n", filename);
    // Open the file, and retrieve its size
    FILE* f = fopen(filename, "rb");
    if (!f) {
        abortWithLogResult(0, "Come on...\n");
        return;
    }
    fseek(f, 0, SEEK_END);
    uint32_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    log("size = %d\n", size);

    // Look for a spot in the code memory block for loading this shader. Note that
    // we are just using a simple incremental offset; this isn't a general purpose
    // allocation algorithm.
    uint32_t codeOffset = codeMemOffset;
    codeMemOffset += (size + DK_SHADER_CODE_ALIGNMENT - 1) & ~(DK_SHADER_CODE_ALIGNMENT - 1);
    log("offset = %d\n");

    // Read the file into memory, and close the file
    fread((uint8_t*) codeMemBlock.getCpuAddr() + codeOffset, size, 1, f);
    fclose(f);
    log("closed file\n");

    dk::ShaderMaker{codeMemBlock, codeOffset}
            .initialize(shader);
}

void App::init() {
    log("App::init\n");
    padInitializeAny(&pad);
    log("pad initialized\n");

    dk::ImageLayout framebufferLayout;
    dk::ImageLayoutMaker{device}
            .setDimensions(FB_WIDTH, FB_HEIGHT)
            .setFlags(DkImageFlags_UsageRender | DkImageFlags_UsagePresent | DkImageFlags_HwCompression)
            .setFormat(DkImageFormat_RGBA8_Unorm)
            .initialize(framebufferLayout);
    log("created image layout\n");

    u32 framebufferSize = (framebufferLayout.getSize() + framebufferLayout.getAlignment() - 1) &
                          ~(framebufferLayout.getAlignment() - 1);

    framebufferMemBlock = dk::MemBlockMaker{device, FB_NUM * framebufferSize}
            .setFlags(DkMemBlockFlags_GpuCached | DkMemBlockFlags_Image)
            .create();
    log("created framebufferMemBlock\n");

    const dk::Image* swapchainImages[FB_NUM];
    for (unsigned i = 0; i < FB_NUM; i++) {
        swapchainImages[i] = &framebuffers[i];
        framebuffers[i]
                .initialize(framebufferLayout, framebufferMemBlock, i * framebufferSize);
        log("created framebuffer image i=%d mbsize=%d fbsize=%d offset %d\n", i, framebufferMemBlock.getSize(), framebufferSize, i * framebufferSize);
    }

    swapchain = dk::SwapchainMaker{device, (void*) window,
                                   reinterpret_cast<const DkImage* const*>(swapchainImages), FB_NUM}
            .create();
    log("created swapchain\n");

    codeMemBlock = dk::MemBlockMaker{device, CODEMEMSIZE}
            .setFlags(DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached | DkMemBlockFlags_Code)
            .create();
    log("created codeMemBlock\n");

    loadShader(vertexShader, "sdmc:/switch/sys-imgui/shaders/vert.dksh");
    log("loaded vertex shader\n");
    loadShader(fragmentShader, "sdmc:/switch/sys-imgui/shaders/frag.dksh");
    log("loaded fragment shader\n");

    commandBufferMemBlock = dk::MemBlockMaker{device, CMDMEMSIZE}
            .setFlags(DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached)
            .create();
    log("created commandBufferMemBlock\n");

    commandBuffer = dk::CmdBufMaker{device}
            .create();
    log("created command buffer\n");

    commandBuffer.addMemory(commandBufferMemBlock, 0, CMDMEMSIZE);
    log("added memblock to command buffer\n");

    for (unsigned i = 0; i < FB_NUM; i++) {
        const dk::ImageView view = dk::ImageView{framebuffers[i]};
        commandBuffer.bindRenderTargets({&view}, nullptr);
        commandBoundFramebuffers[i] = commandBuffer.finishList();
        log("bound render target %d\n", i);
    }

    DkViewport viewport = {0.0f, 0.0f, (float) FB_WIDTH, (float) FB_HEIGHT, 0.0f, 1.0f};
    DkScissor scissor = {0, 0, FB_WIDTH, FB_HEIGHT};
    dk::RasterizerState rasterizerState = dk::RasterizerState();
    dk::ColorState colorState = dk::ColorState();
    dk::ColorWriteState colorWriteState = dk::ColorWriteState();

    commandBuffer.setViewports(0, viewport);
    commandBuffer.setScissors(0, scissor);
    commandBuffer.clearColor(0, DkColorMask_RGBA, 0.125f, 0.294f, 0.478f, 1.0f);
    commandBuffer.bindShaders(DkStageFlag_GraphicsMask, {&vertexShader, &fragmentShader});
    commandBuffer.bindRasterizerState(rasterizerState);
    commandBuffer.bindColorState(colorState);
    commandBuffer.bindColorWriteState(colorWriteState);
    commandBuffer.draw(DkPrimitive_Triangles, 3, 1, 0, 0);
    commandRender = commandBuffer.finishList();
    log("finished render command list\n");

    renderQueue = dk::QueueMaker{device}
            .setFlags(DkQueueFlags_Graphics)
            .create();
    log("created render queue, done init.\n");

    startNs = armTicksToNs(svcGetSystemTick());
}

bool App::update() {
    log("btn = %ull\n", padGetButtons(&pad));
    if (padGetButtons(&pad) & HidNpadButton_Minus) return false;

    return true;
}

void App::render() {
    int slot = renderQueue.acquireImage(swapchain);
    renderQueue.submitCommands(commandBoundFramebuffers[slot]);
    renderQueue.submitCommands(commandRender);
    renderQueue.presentImage(swapchain, slot);
}

App::~App() {
    // Make sure the rendering queue is idle before destroying anything
    renderQueue.waitIdle();

    // Destroy all the resources we've created
    renderQueue.destroy();
    commandBuffer.destroy();
    commandBufferMemBlock.destroy();
    codeMemBlock.destroy();
    swapchain.destroy();
    framebufferMemBlock.destroy();
    device.destroy();
}
