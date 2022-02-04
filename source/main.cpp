#include <switch.h>
#include <deko3d.hpp>
#include "ImguiService.hpp"

#define FB_WIDTH  320
#define FB_HEIGHT 180
#define CODEMEMSIZE (64*1024)

// Define the size of the memory block that will hold code
#define CODEMEMSIZE (64*1024)

// Define the size of the memory block that will hold command lists
#define CMDMEMSIZE (16*1024)

void loadShader() {

}

void actualMain(ImguiService service, NWindow* window, dk::Device device) {
    dk::ImageLayout framebufferLayout;
    dk::ImageLayoutMaker{device}
            .setDimensions(FB_WIDTH, FB_HEIGHT)
            .setFlags(DkImageFlags_UsageRender | DkImageFlags_UsagePresent | DkImageFlags_HwCompression)
            .setFormat(DkImageFormat_RGBA8_Unorm)
            .initialize(framebufferLayout);

}