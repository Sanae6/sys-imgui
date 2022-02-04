#include <cstdlib>
#include <cstdio>
#include <switch.h>
#include <deko3d.hpp>
#include "main.hpp"
#include "ImguiService.hpp"

#define FB_NUM    2
#define FB_WIDTH  320
#define FB_HEIGHT 180
#define CODEMEMSIZE (64*1024)

// Define the size of the memory block that will hold code
#define CODEMEMSIZE (64*1024)

// Define the size of the memory block that will hold command lists
#define CMDMEMSIZE (16*1024)

void* App::readFile(const char* filename) {
    FILE* f = fopen(filename, "rb");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    rewind(f);

    char* code = (char*) malloc(fsize + 1);
    fread(code, 1, fsize, f);
    fclose(f);

    code[fsize] = 0;
}

void App::loadShader(dk::Shader& shader, const char* filename) {

    // Open the file, and retrieve its size
    FILE* f = fopen(filename, "rb");
    fseek(f, 0, SEEK_END);
    uint32_t size = ftell(f);
    rewind(f);

    // Look for a spot in the code memory block for loading this shader. Note that
    // we are just using a simple incremental offset; this isn't a general purpose
    // allocation algorithm.
    uint32_t codeOffset = codeMemOffset;
    codeMemOffset += (size + DK_SHADER_CODE_ALIGNMENT - 1) & ~(DK_SHADER_CODE_ALIGNMENT - 1);

    // Read the file into memory, and close the file
    fread((uint8_t*) codeMemBlock.getCpuAddr() + codeOffset, size, 1, f);
    fclose(f);

    dk::ShaderMaker{codeMemBlock, codeOffset}
            .initialize(shader);
}

void App::init() {
    dk::Device device = dk::DeviceMaker{}
            .setFlags(DkDeviceFlags_OriginLowerLeft)
            .create();

    dk::ImageLayout framebufferLayout;
    dk::ImageLayoutMaker{device}
            .setDimensions(FB_WIDTH, FB_HEIGHT)
            .setFlags(DkImageFlags_UsageRender | DkImageFlags_UsagePresent | DkImageFlags_HwCompression)
            .setFormat(DkImageFormat_RGBA8_Unorm)
            .initialize(framebufferLayout);

    u32 framebufferSize = (framebufferLayout.getSize() + framebufferLayout.getAlignment() - 1) &
                          ~(framebufferLayout.getAlignment() - 1);

    framebufferMemBlock = dk::MemBlockMaker{device, framebufferSize}
            .setFlags(DkMemBlockFlags_GpuCached | DkMemBlockFlags_Image)
            .create();

    const dk::Image* swapchainImages[FB_NUM];
    for (unsigned i = 0; i < FB_NUM; i++) {
        swapchainImages[i] = &framebuffers[i];
        framebuffers[i]
                .initialize(framebufferLayout, framebufferMemBlock, i * framebufferSize);
    }

    swapchain = dk::SwapchainMaker{device, (void*) window,
                                   reinterpret_cast<const DkImage* const*>(swapchainImages), FB_NUM}
            .create();

    codeMemBlock = dk::MemBlockMaker{device, CODEMEMSIZE}
            .setFlags(DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached | DkMemBlockFlags_Code)
            .create();

    loadShader(vertexShader, "romfs:/shaders/vert.dksh");
    loadShader(fragmentShader, "romfs:/shaders/frag.dksh");

    commandBufferMemBlock = dk::MemBlockMaker{device, CMDMEMSIZE}
            .setFlags(DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached)
            .create();

    commandBuffer = dk::CmdBufMaker{device}
            .create();

    commandBuffer.addMemory(commandBufferMemBlock, 0, CMDMEMSIZE);

    for (unsigned i = 0; i < FB_NUM; i++) {
        const dk::ImageView view = dk::ImageView{framebuffers[i]};
        commandBuffer.bindRenderTargets({&view}, nullptr);
        commandBoundFramebuffers[i] = commandBuffer.finishList();
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

    renderQueue = dk::QueueMaker{device}
            .setFlags(DkQueueFlags_Graphics)
            .create();
}

void App::run() {
    int slot = renderQueue.acquireImage(swapchain);
    renderQueue.submitCommands(commandBoundFramebuffers[slot]);
    renderQueue.submitCommands(commandRender);
    renderQueue.presentImage(swapchain, slot)
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