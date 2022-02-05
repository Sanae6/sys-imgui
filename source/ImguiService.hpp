#pragma once

#include <switch.h>

struct ImguiService {
    ImguiService();
    ~ImguiService();

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
    Handle serviceHandle{};
    SmServiceName name{};
    Service service{};
};


