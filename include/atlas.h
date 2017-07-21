#ifndef ATLAS_ATLAS_H
#define ATLAS_ATLAS_H

#include <string>
#include <iostream>
#include <memory>
#include "err.h"
#include "wrappers/instance.h"
#include "wrappers/device.h"

namespace Atlas {
    extern std::shared_ptr<Anvil::Instance> g_instance;
    extern std::weak_ptr<Anvil::PhysicalDevice> g_physical_device;
    extern std::weak_ptr<Anvil::SGPUDevice> g_device;

    VkBool32 default_debug_callback(VkDebugReportFlagsEXT message_flags, VkDebugReportObjectTypeEXT object_type,
        const char* layer_prefix, const char* message, void* user_arg);
    Err init_vulkan(std::string appName, Anvil::PFNINSTANCEDEBUGCALLBACKPROC debugCallback = default_debug_callback);



    void shutdown_vulkan();
}

#endif // ATLAS_ATLAS_H