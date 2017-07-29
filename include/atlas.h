#ifndef ATLAS_ATLAS_H
#define ATLAS_ATLAS_H

#include <string>
#include <memory>
#include <utility>
#include "wrappers/instance.h"
#include "wrappers/device.h"
#include "wrappers/shader_module.h"
#include "misc/glsl_to_spirv.h"

#include "window.h"
#include "camera.h"

namespace Atlas {
    extern std::shared_ptr<Anvil::Instance> g_instance;
    extern std::weak_ptr<Anvil::PhysicalDevice> g_physical_device;
    extern std::weak_ptr<Anvil::SGPUDevice> g_device;

    VkBool32 default_debug_callback(VkDebugReportFlagsEXT message_flags, VkDebugReportObjectTypeEXT object_type,
        const char* layer_prefix, const char* message, void* user_arg);
    void init_vulkan(const std::string& appName, Anvil::PFNINSTANCEDEBUGCALLBACKPROC debugCallback = default_debug_callback);

    //
    // Shaders
    //

    template<typename ... Types> inline void dummy_func(Types&& ...) {}

    template<typename ... Types> std::shared_ptr<Anvil::ShaderModule>
    create_shader_module_from_source(const std::string& glsl, Anvil::ShaderStage stage, std::pair<std::string, Types> ... definitions) {
        
        std::shared_ptr<Anvil::GLSLShaderToSPIRVGenerator> generator;

        generator = Anvil::GLSLShaderToSPIRVGenerator::create(g_device,
            Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE,
            glsl,
            stage);

        // Iterate over definitions
        dummy_func(generator->add_definition_value_pair(definitions.first, definitions.second)...);
        
        return Anvil::ShaderModule::create_from_spirv_generator(g_device, generator);
    }
    
    template<typename ... Types> std::shared_ptr<Anvil::ShaderModule>
    create_shader_module_from_file(const std::string& filename, Anvil::ShaderStage stage, std::pair<std::string, Types> ... definitions) {
        
        std::shared_ptr<Anvil::GLSLShaderToSPIRVGenerator> generator;

        generator = Anvil::GLSLShaderToSPIRVGenerator::create(g_device,
            Anvil::GLSLShaderToSPIRVGenerator::MODE_LOAD_SOURCE_FROM_FILE,
            filename,
            stage);

        // Iterate over definitions
        dummy_func(generator->add_definition_value_pair(definitions.first, definitions.second)...);
        
        
        return Anvil::ShaderModule::create_from_spirv_generator(g_device, generator);
    }
    
    std::shared_ptr<Anvil::ShaderModuleStageEntryPoint> shader_entry_point_none();
    std::shared_ptr<Anvil::ShaderModuleStageEntryPoint> shader_entry_point(std::shared_ptr<Anvil::ShaderModule> module, Anvil::ShaderStage stage, const std::string& func_name = "main");

    void shutdown_vulkan();
}

#endif // ATLAS_ATLAS_H