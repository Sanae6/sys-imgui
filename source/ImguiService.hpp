#pragma once

#include <switch/services/sm.h>

struct ImguiService {
    ImguiService() {
        if (R_FAILED(smRegisterService(&serviceHandle, name = smEncodeName("imgui:u"), false, 1)))
            return;
        serviceCreate(&service, serviceHandle);
    }

    ~ImguiService() {
        smUnregisterService(name);
        svcCloseHandle(serviceHandle);
    }

    Handle getHandle() {
        return serviceHandle;
    }

    Service* getService() {
        return &service;
    }

    SmServiceName getName() {
        return name;
    }
private:
    Handle serviceHandle;
    SmServiceName name;
    Service service
};


