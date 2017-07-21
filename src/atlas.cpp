#include <iostream>
#include <vector>
#include "atlas.h"
#include "mesh.h"

void say_hello(){ std::cout << "Hello, from atlas!\n"; }

std::shared_ptr<Anvil::Instance> Atlas::g_instance;
std::weak_ptr<Anvil::PhysicalDevice> Atlas::g_physical_device;
std::weak_ptr<Anvil::SGPUDevice> Atlas::g_device;

using namespace Atlas;

VkBool32 default_debug_callback(VkDebugReportFlagsEXT message_flags, VkDebugReportObjectTypeEXT object_type, const char* layer_prefix, const char* message, void* user_arg) {
    if ((message_flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) != 0) {
        std::cerr << "[!] " << message << '\n';
    }

    return false;
}

Err init_vulkan(const std::string& appName, Anvil::PFNINSTANCEDEBUGCALLBACKPROC debugCallback) {
    g_instance = Anvil::Instance::create(appName.c_str(), "Atlas", 
#ifdef ENABLE_VALIDATION
        debugCallback,
#else
        nullptr,
#endif
        nullptr); // callback user args

    // TODO: if multiple, how to choose which device based on properties?
    g_physical_device = g_instance->get_physical_device(0);

    g_device = Anvil::SGPUDevice::create(g_physical_device,
        Anvil::DeviceExtensionConfiguration(),
        {}, // layers (deprecated)
        false, // transient command buffer allocations only
        false); // support resettable command buffer allocations
}



void shutdown_vulkan() {
    g_device.lock()->destroy();
    g_device.reset();

    g_instance->destroy();
    g_instance.reset();
}