#include "ImguiService.hpp"
#include "logger.hpp"

ImguiService::ImguiService() {
    if (R_FAILED(smRegisterService(&serviceHandle, name = smEncodeName("imgui:u"), false, 1)))
        abortWithResult(0x420B);
    serviceCreate(&service, serviceHandle);
}

ImguiService::~ImguiService() {
    smUnregisterService(name);
    svcCloseHandle(serviceHandle);
}
