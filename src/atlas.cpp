#include <stdio.h>
#include <vector>
#include "atlas.h"
#include <stdint.h>

void say_hello(){ printf("Hello, from atlas!\n"); }

std::shared_ptr<Anvil::Instance> Atlas::g_instance;
std::weak_ptr<Anvil::PhysicalDevice> Atlas::g_physical_device;
std::weak_ptr<Anvil::SGPUDevice> Atlas::g_device;

VkBool32 Atlas::default_debug_callback(VkDebugReportFlagsEXT message_flags, VkDebugReportObjectTypeEXT object_type, const char* layer_prefix, const char* message, void* user_arg) {
    if ((message_flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) != 0) {
        fprintf(stderr, "[!] %s\n", message);
    }

    return false;
}

void Atlas::init_vulkan(const std::string& appName, Anvil::PFNINSTANCEDEBUGCALLBACKPROC debugCallback) {
    // Error handling unnecessary; Anvil will assert on failure
    g_instance = Anvil::Instance::create(appName.c_str(), "Atlas", 
#ifdef ENABLE_VALIDATION
        debugCallback,
#else
        nullptr,
#endif
        nullptr); // callback user args

    constexpr uint32_t preferred_device_index = 0;

    // TODO: if multiple, how to choose which device based on properties?
    g_physical_device = g_instance->get_physical_device(preferred_device_index);

    g_device = Anvil::SGPUDevice::create(g_physical_device,
        Anvil::DeviceExtensionConfiguration(),
        {}, // layers (deprecated)
        false, // transient command buffer allocations only
        false); // support resettable command buffer allocations
}

std::shared_ptr<Anvil::ShaderModuleStageEntryPoint> Atlas::Shader::entry_point() {
    return std::make_shared<Anvil::ShaderModuleStageEntryPoint>();
}

std::shared_ptr<Anvil::ShaderModuleStageEntryPoint> Atlas::Shader::entry_point(std::shared_ptr<Anvil::ShaderModule> module, Anvil::ShaderStage stage, const std::string& func_name) {
    return std::make_shared<Anvil::ShaderModuleStageEntryPoint>(func_name.c_str(), module, stage);
}




void Atlas::shutdown_vulkan() {
    g_device.lock()->destroy();
    g_device.reset();

    g_instance->destroy();
    g_instance.reset();
}