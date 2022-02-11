// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include the main libnx system header, for Switch development
#include <switch.h>
#include "logger.hpp"
#include "App.hpp"

// Size of the inner heap (adjust as necessary).
#define INNER_HEAP_SIZE 0x2000000

#ifdef __cplusplus
extern "C" {
#endif

u64 __nx_vi_layer_id;
// Sysmodules should not use applet*.
u32 __nx_applet_type = AppletType_None;

// Sysmodules will normally only want to use one FS session.
u32 __nx_fs_num_sessions = 1;

// Newlib heap configuration function (makes malloc/free work).
void __libnx_initheap(void) {
    static u8 inner_heap[INNER_HEAP_SIZE];
    extern void* fake_heap_start;
    extern void* fake_heap_end;

    // Configure the newlib heap.
    fake_heap_start = inner_heap;
    fake_heap_end = inner_heap + sizeof(inner_heap);
}

static dk::Device device;
static NWindow window;
static ViDisplay display;
static ViLayer layer;
static ImguiService service;
static App* app;

// Service initialization.
void __appInit(void) {
    Result rc;

    // Open a service manager session.
    rc = smInitialize();
    if (R_FAILED(rc)) {
        abortWithResult(0x4200);
        return;
    }

    // Retrieve the current version of Horizon OS.
    rc = setsysInitialize();
    if (R_SUCCEEDED(rc)) {
        SetSysFirmwareVersion fw;
        rc = setsysGetFirmwareVersion(&fw);
        if (R_SUCCEEDED(rc))
            hosversionSet(MAKEHOSVERSION(fw.major, fw.minor, fw.micro));
        setsysExit();
    }

    // Enable this if you want to use HID.
    rc = hidInitialize();
    if (R_FAILED(rc)) {
        abortWithResult(0x4201);
        return;
    }

    static const SocketInitConfig socketInitConfig = {
            .bsdsockets_version = 1,

            .tcp_tx_buf_size = 0x800,
            .tcp_rx_buf_size = 0x800,
            .tcp_tx_buf_max_size = 0x25000,
            .tcp_rx_buf_max_size = 0x25000,

            //We don't use UDP, set all UDP buffers to 0
            .udp_tx_buf_size = 0,
            .udp_rx_buf_size = 0,

            .sb_efficiency = 1,
    };
    rc = socketInitialize(&socketInitConfig);
    if (R_FAILED(rc)) {
        abortWithResult(0x4202);
        return;
    }
    redirectStdoutToLogServer();
    log("Connected early\n");
    R_ASSERT(hidsysInitialize())

    // Enable this if you want to use time.
    /*rc = timeInitialize();
    if (R_FAILED(rc))
        abortWithResult(MAKERESULT(Module_Libnx, LibnxError_InitFail_Time));
    __libnx_init_time();*/

    // Disable this if you don't want to use the filesystem.
    rc = fsInitialize();
    if (R_FAILED(rc)) {
        abortWithResult(0x4203);
        return;
    }

    // Disable this if you don't want to use the SD card filesystem.
    rc = fsdevMountSdmc();
    if (R_FAILED(rc)) {
        abortWithResult(0x4204);
        return;
    }


    log("got to the good shit now\n");

    R_ASSERT_LOG(viInitialize(ViServiceType_Manager), "Initialized vi service")
    R_ASSERT_LOG(viOpenDefaultDisplay(&display), "Opened display")
    R_ASSERT_LOG(viCreateManagedLayer(&display, static_cast<ViLayerFlags>(0), 0, &__nx_vi_layer_id), "Something managed layer")
    R_ASSERT_LOG(viCreateLayer(&display, &layer), "Created layer") // todo make layers whenever i want for external windows?
    R_ASSERT_LOG(viSetLayerScalingMode(&layer, ViScalingMode_FitToLayer), "Set scaling mode")
//    R_ASSERT_LOG(viSetLayerSize(&layer, FB_WIDTH, FB_HEIGHT), "Set layer size")
//    if (s32 layerZ = 0; R_SUCCEEDED(viGetZOrderCountMax(&display, &layerZ)) && layerZ > 0)
//        R_ASSERT_LOG(viSetLayerZ(&layer, layerZ), "Set layer z");

    R_ASSERT_LOG(nwindowCreateFromLayer(&window, &layer), "Created window")

    log("too sexy\n");


    __nx_applet_type = AppletType_Application; // this workaround is the start of my villain arc

    device = dk::DeviceMaker{}
            .setCbDebug([](void* userData, const char* context, DkResult result, const char* message) {
                if (result != DkResult_Success)
                    abortWithLogResult(result, "{yo mama} deko3d error (%d) in %s - %s\n", result, context, message);
                else
                    log("deko3d warning in %s: %s\n", context, message);
            })
            .setFlags(DkDeviceFlags_OriginLowerLeft)
            .create();

    __nx_applet_type = AppletType_None; // evil committed, i'm probably going to jail for life for doing this

    log("created device\n");
    service = ImguiService();
    padConfigureInput(8, HidNpadStyleSet_NpadStandard | HidNpadStyleTag_NpadSystemExt);

    app = new App(service, &window, device);
    log("done init\n");
}

// Service deinitialization.
void __appExit(void) {
    // Close extra services you added to __appInit here.
    delete app;

    nwindowClose(&window);
    viCloseLayer(&layer);
    viCloseDisplay(&display);
    viExit();
    fsdevUnmountAll(); // Disable this if you don't want to use the SD card filesystem.
    fsExit(); // Disable this if you don't want to use the filesystem.
    smExit();
    hidsysExit();
    socketExit();
    hidExit(); // Enable this if you want to use HID.
}

#ifdef __cplusplus
}
#endif

// Main program entrypoint
int main(int argc, char* argv[]) {
    log("hit main\n");

    Event vsyncEvent;
    viGetDisplayVsyncEvent(&display, &vsyncEvent);

    padUpdate(&app->pad);
    while (appletMainLoop()) {
        padUpdate(&app->pad);
        if (!app->update())
            break;
        app->render();

        eventWait(&vsyncEvent, UINT64_MAX);
    }
    log("Exited.");
    exit(0);
}