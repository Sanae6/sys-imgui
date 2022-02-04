#pragma once

#include "ImguiService.hpp"
#include <deko3d.hpp>

class App {
public:
    App(const ImguiService& service, NWindow* window) : service(service), window(window) {}
    ~App();
    void run();
private:
    void init();
    void loadShader(dk::Shader&, const char* filename);
    void* readFile(const char* filename);
    const ImguiService service;
    NWindow* window;

    dk::Device device;
    dk::MemBlock framebufferMemBlock;
    dk::Image framebuffers[2];
    dk::Swapchain swapchain;

    uint32_t codeMemOffset = 0;
    dk::MemBlock codeMemBlock;
    dk::Shader vertexShader;
    dk::Shader fragmentShader;

    dk::MemBlock commandBufferMemBlock;
    dk::CmdBuf commandBuffer;
    DkCmdList commandBoundFramebuffers[2];
    DkCmdList commandRender;

    dk::Queue renderQueue;
};

void actualMain(const ImguiService& service, NWindow* window, dk::Device device);