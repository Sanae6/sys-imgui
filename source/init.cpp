// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include the main libnx system header, for Switch development
#include <switch.h>

// Size of the inner heap (adjust as necessary).
#define INNER_HEAP_SIZE 0x80000

#ifdef __cplusplus
extern "C" {
#endif

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

// Service initialization.
void __appInit(void) {
    Result rc;

    // Open a service manager session.
    rc = smInitialize();
    if (R_FAILED(rc))
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_InitFail_SM));

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
    if (R_FAILED(rc))
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_InitFail_HID));

    // Enable this if you want to use time.
    /*rc = timeInitialize();
    if (R_FAILED(rc))
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_InitFail_Time));
    __libnx_init_time();*/

    // Disable this if you don't want to use the filesystem.
    rc = fsInitialize();
    if (R_FAILED(rc))
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_InitFail_FS));

    // Disable this if you don't want to use the SD card filesystem.
    rc = fsdevMountSdmc();
    if (R_FAILED(rc))
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_InitFail_FS));

    rc = romfsInit();
    if (R_FAILED(rc))
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_InitFail_FS));


    rc = viInitialize(ViServiceType_Application);
    if (R_FAILED(rc))
        diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_BadGfxInit));

}

// Service deinitialization.
void __appExit(void) {
    // Close extra services you added to __appInit here.
    viExit();
    romfsExit();
    fsdevUnmountAll(); // Disable this if you don't want to use the SD card filesystem.
    fsExit(); // Disable this if you don't want to use the filesystem.
    smExit();
    //timeExit(); // Enable this if you want to use time.
    //hidExit(); // Enable this if you want to use HID.
}

#ifdef __cplusplus
}
#endif

#include "main.hpp"

// Main program entrypoint
int main(int argc, char* argv[]) {
    Result rc;
    ImguiService service = ImguiService();

    NWindow* window = (NWindow*) calloc(1, sizeof(NWindow)); // needs to be on the heap because it's being passed to actualMain
    ViDisplay display = {0}; // can be on the stack since i don't care or need to worry about it
    ViLayer layer = {0}; // same as display
    if (R_SUCCEEDED(rc)) rc = viOpenDisplay("SysImGui-Display", &display);
    if (R_SUCCEEDED(rc)) rc = viCreateLayer(&display, &layer);
    if (R_FAILED(rc)) rc = nwindowCreateFromLayer(window, &layer);

    if (R_SUCCEEDED(rc)) {
        App* app = new App(service, window);

        app->run();
        delete app;
    }

    nwindowClose(window);
    viCloseLayer(&layer);
    smUnregisterService(service.getName());
    svcCloseHandle(service.getHandle());
    return rc != 0;
}