#define VMA_IMPLEMENTATION
#include "backend.h"
#include <algorithm>
#include <stdio.h>
#include <assert.h>

using namespace Atlas;

// TODO: log functions display time

void Backend::log(const std::string& message) {
    Instance::default_dbg_callback(VK_DEBUG_REPORT_DEBUG_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, 0, 0, "App", message.c_str(), nullptr);
}

void Backend::warning(const std::string& message) {
    Instance::default_dbg_callback(VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, 0, 0, "App", message.c_str(), nullptr);
}

void Backend::error(const std::string& message) {
    Instance::default_dbg_callback(VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, 0, 0, "App", message.c_str(), nullptr);
}

bool Atlas::validate(VkResult result) {
    switch (result) {
    case VK_SUCCESS:
    case VK_EVENT_SET:
    case VK_EVENT_RESET:
        return true;
    case VK_NOT_READY:
        printf("[*] Warning: A fence or query not yet completed.\n");
        fflush(stdout);
        return true;
    case VK_TIMEOUT:
        printf("[*] Warning: A wait operation has not completed in the specified time.\n");
        fflush(stdout);
        return true;
    case VK_INCOMPLETE:
        printf("[*] Warning: A return array was too small for the result.\n");
        fflush(stdout);
        return true;
    case VK_SUBOPTIMAL_KHR:
        printf("[*] Warning: A swapchain no longer matches the surface properties exactly, but can still be used to present successfully.\n");
        fflush(stdout);
        return true;
    case VK_ERROR_OUT_OF_HOST_MEMORY:
        fprintf(stderr, "[!] Error: A host memory allocation has failed!\n");
        fflush(stderr);
        return false;
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        fprintf(stderr, "[!] Error: A device memory allocation has failed!\n");
        fflush(stderr);
        return false;
    case VK_ERROR_INITIALIZATION_FAILED:
        fprintf(stderr, "[!] Error: Initialization of an object could not be completed for implementation-specific reasons!\n");
        fflush(stderr);
        return false;
    case VK_ERROR_DEVICE_LOST:
        fprintf(stderr, "[!] Error: The logical or physical device has been lost!\n");
        fflush(stderr);
        return false;
    case VK_ERROR_MEMORY_MAP_FAILED:
        fprintf(stderr, "[!] Error: Mapping of a memory object has failed!\n");
        fflush(stderr);
        return false;
    case VK_ERROR_LAYER_NOT_PRESENT:
        fprintf(stderr, "[!] Error: A requested layer is not present or could not be loaded!\n");
        fflush(stderr);
        return false;
    case VK_ERROR_EXTENSION_NOT_PRESENT:
        fprintf(stderr, "[!] Error: A requested extension is not supported!\n");
        fflush(stderr);
        return false;
    case VK_ERROR_FEATURE_NOT_PRESENT:
        fprintf(stderr, "[!] Error: A requested feature is not supported!\n");
        fflush(stderr);
        return false;
    case VK_ERROR_INCOMPATIBLE_DRIVER:
        fprintf(stderr, "[!] Error: The requested version of Vulkan is not supported by the driver or is otherwise incompatible for implementation-sepcific reasons!\n");
        fflush(stderr);
        return false;
    case VK_ERROR_TOO_MANY_OBJECTS:
        fprintf(stderr, "[!] Error: Too many objects of the type have already been created!\n");
        fflush(stderr);
        return false;
    case VK_ERROR_FORMAT_NOT_SUPPORTED:
        fprintf(stderr, "[!] Error: A requested format is not supported on this device!\n");
        fflush(stderr);
        return false;
    case VK_ERROR_FRAGMENTED_POOL:
        fprintf(stderr, "[!] Error: A requested pool allocation has failed due to fragmentation of the pool's memory!\n");
        fflush(stderr);
        return false;
    case VK_ERROR_SURFACE_LOST_KHR:
        fprintf(stderr, "[!] Error: A surface is no longer available!\n");
        fflush(stderr);
        return false;
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
        fprintf(stderr, "[!] Error: The requested window is already connected to a VkSurfaceKHR, or to some other non-Vulkan API!\n");
        fflush(stderr);
        return false;
    case VK_ERROR_OUT_OF_DATE_KHR:
        fprintf(stderr, "[!] Error: A surface has changed in such a way that it is no longer compatible with the swapchain, and further presentation requests using the swapchain will fail! Applications must query the new surface properties and recreate their swapchain if they wish to continue presenting to the surface.\n");
        fflush(stderr);
        return false;
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
        fprintf(stderr, "[!] Error: The display used by a swapchain does not use the same presentable image layout, or is incompatible in a way that prevents sharing an image!\n");
        fflush(stderr);
        return false;
    }
}


using namespace Backend;

VkBool32 Instance::default_dbg_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t srcObject, size_t location, int32_t msgCode, const char* pLayerPrefix, const char* pMsg, void* pUserData) {
    // Select prefix depending on flags passed to the callback
    // Note that multiple flags may be set for a single validation message
    std::string desc;

    // Diagnostic info from the Vulkan loader and layers
    // Usually not helpful in terms of API usage, but may help to debug layer and loader problems 
    if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
        desc += " Diagnostics";
    }
    // May indicate sub-optimal usage of the API
    if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
        desc += " Performance Warning";
    }
    // Warnings may hint at unexpected / non-spec API usage
    else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        desc += " Warning";
    }
    // Informal messages that may become handy during debugging
    if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
        desc += " Info";
    }

    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        fprintf(stderr, "[!] Error (%s, code %d): %s\n", pLayerPrefix, msgCode, pMsg);
        //fflush(stderr);
    }
    else {
        printf("[*]%s (%s, code %d): %s\n", desc.c_str(), pLayerPrefix, msgCode, pMsg);
        //fflush(stdout);
    }

    // The return value of this callback controls whether the Vulkan call that caused
    // the validation message will be aborted or not
    // We return VK_FALSE as we DON'T want Vulkan calls that cause a validation message 
    // (and return a VkResult) to abort
    // If you instead want to have calls abort, pass in VK_TRUE and the function will 
    // return VK_ERROR_VALIDATION_FAILED_EXT
    return VK_FALSE;
}

Instance::Instance(const std::string& app_name, uint32_t app_version)
    : m_dbg_callback(VK_NULL_HANDLE), m_instance(VK_NULL_HANDLE), instance_flags(0)
{
    // Store application info
    app_info = {
        VK_STRUCTURE_TYPE_APPLICATION_INFO, // sType
        nullptr,                            // pNext
        app_name.c_str(),                   // pApplicationName
        app_version,                        // applicationVersion
        "Atlas",                            // pEngineName
        VK_MAKE_VERSION(                    // engineVersion
            ATLAS_VERSION_MAJOR,
            ATLAS_VERSION_MINOR,
            ATLAS_VERSION_PATCH),
        VK_MAKE_VERSION(1,0,57)              // apiVersion
    };

    // Enumerate layers
    uint32_t n_supported_layers;
    VkResult res = vkEnumerateInstanceLayerProperties(&n_supported_layers, NULL);
    validate(res);

    m_supported_layers.resize(n_supported_layers);
    res = vkEnumerateInstanceLayerProperties(&n_supported_layers, m_supported_layers.data());
    validate(res);

    // Enumerate extensions
    uint32_t n_layer_extensions;
    std::vector<VkExtensionProperties> layer_extensions;

    // (1) Start by enumerating the global extensions
    res = vkEnumerateInstanceExtensionProperties(NULL, &n_layer_extensions, NULL);
    if (validate(res)) {
        layer_extensions.resize(n_layer_extensions);
        res = vkEnumerateInstanceExtensionProperties(NULL, &n_layer_extensions, layer_extensions.data());
        for (const auto& ext : layer_extensions)
            m_extension_providers[ext.extensionName] = {NULL, ext.specVersion};
        
        // (2) Then enumerate the extensions provided by layers
        for (const auto& layer : m_supported_layers) {
            res = vkEnumerateInstanceExtensionProperties(layer.layerName, &n_layer_extensions, NULL);
            if (validate(res)) {
                layer_extensions.resize(n_layer_extensions);
                res = vkEnumerateInstanceExtensionProperties(layer.layerName, &n_layer_extensions, layer_extensions.data());
                for (const auto& ext : layer_extensions) {
                    m_extension_providers.insert(std::make_pair<const std::string, ExtensionProvider>(ext.extensionName, {layer.layerName, ext.specVersion}));
                }
            }
        }
    }

    // (3) Provide default extensions
    enabled_extensions = {
#if ENABLE_VALIDATION
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
#endif
#if _WIN32
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#else
        VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#endif
        VK_KHR_SURFACE_EXTENSION_NAME
    };

    if (is_extension_supported(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
        enabled_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

    // Ensure that the debug report extension uses debug layers to intercept calls
    m_extension_providers[VK_EXT_DEBUG_REPORT_EXTENSION_NAME].layer_name = "VK_LAYER_LUNARG_standard_validation";
}

Instance::~Instance() {
    if (m_dbg_callback)
        vkDestroyDebugReportCallbackEXT(m_instance, m_dbg_callback, NULL);
    if (m_instance)
        vkDestroyInstance(m_instance, NULL);
}

bool Instance::init() {
    // Make sure we have the required layers for each extension
    for (const auto& pair : m_extension_providers) {
        if (pair.second.layer_name != NULL) {
            // Loader eliminates duplicates, so don't bother to check
            m_enabled_layers.push_back(pair.second.layer_name);
            printf("Enabled layer %s for extension %s\n", pair.second.layer_name, pair.first.c_str());
        }
    }

    // Create instance
    VkInstanceCreateInfo instance_info = {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, // sType
        nullptr,                                // pNext
        instance_flags,                         // flags
        &app_info,                              // pApplicationInfo
        static_cast<uint32_t>(m_enabled_layers.size()),    // enabledLayerCount
        m_enabled_layers.data(),                // ppEnabledLayerNames
        static_cast<uint32_t>(enabled_extensions.size()),  // enabledExtensionCount
        enabled_extensions.data()               // ppEnabledExtensionNames
    };
    // TODO: custom memory allocator
    VkResult res = vkCreateInstance(&instance_info, NULL, &m_instance);
    if (!validate(res)) return false;


    load_func_pointers();

#ifdef ENABLE_VALIDATION
    VkDebugReportCallbackCreateInfoEXT callback_info = {
        VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,    // sType
        nullptr,                                                    // pNext
#if ENABLE_VALIDATION == 2
        VK_DEBUG_REPORT_DEBUG_BIT_EXT |
        VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
#endif
        VK_DEBUG_REPORT_ERROR_BIT_EXT |
        VK_DEBUG_REPORT_WARNING_BIT_EXT |
        VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,                // flags
        default_dbg_callback,                                       // pfnCallback
        this                                                        // pUserData
    };

    res = vkCreateDebugReportCallbackEXT(m_instance, &callback_info, NULL, &m_dbg_callback);
    if (!validate(res)) return false;
#endif

    // Enumerate physical devices
    uint32_t n_physical_devices = 0;
    vkEnumeratePhysicalDevices(m_instance, &n_physical_devices, NULL);
    std::vector<VkPhysicalDevice> phys(n_physical_devices);
    m_physical_devices.reserve(n_physical_devices);
    vkEnumeratePhysicalDevices(m_instance, &n_physical_devices, phys.data());

    PhysicalDevice details;
    uint32_t n_queues = 0, n_extensions = 0;
    std::vector<VkQueueFamilyProperties> queues;
    std::vector<VkExtensionProperties> extensions;
    for (auto device : phys) {
        details.device = device;
        vkGetPhysicalDeviceProperties(details.device, &details.props);
        vkGetPhysicalDeviceFeatures(details.device, &details.features);

        vkGetPhysicalDeviceQueueFamilyProperties(details.device, &n_queues, NULL);
        queues.resize(n_queues);
        vkGetPhysicalDeviceQueueFamilyProperties(details.device, &n_queues, queues.data());

        details.queue_families.compute_count = 0;
        details.queue_families.universal_count = 0;
        details.queue_families.transfer_count = 0;

        // Vulkan 1.0 spec demands that at least one queue support graphics and compute
        // and every queue supporting graphics or compute also supports transfer
        // Prefer to have dedicated compute or transfer queues if they are available
        for (uint32_t index = 0; index < queues.size(); ++index) {
            if ( (queues[index].queueFlags & VK_QUEUE_COMPUTE_BIT) && !(queues[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) ) {
                details.queue_families.compute = index;
                details.queue_families.compute_count = queues[index].queueCount;
                continue;
            }
            if ( (queues[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) && (index != details.queue_families.compute) ) {
                details.queue_families.universal = index;
                details.queue_families.universal_count = queues[index].queueCount;
                continue;
            }
            if ( (queues[index].queueFlags & VK_QUEUE_TRANSFER_BIT)
            && (index != details.queue_families.compute)
            && (index != details.queue_families.universal) ) {
                details.queue_families.transfer = index;
                details.queue_families.transfer_count = queues[index].queueCount;
                continue;
            }
        }
        if (details.queue_families.compute_count == 0) {
            details.queue_families.compute = details.queue_families.universal;
        }
        if (details.queue_families.transfer_count == 0) {
            details.queue_families.transfer = details.queue_families.universal;
        }

        // Enumerate global extensions
        res = vkEnumerateDeviceExtensionProperties(details.device, NULL, &n_extensions, NULL);
        if (!validate(res)) continue;
        extensions.resize(n_extensions);
        res = vkEnumerateDeviceExtensionProperties(details.device, NULL, &n_extensions, extensions.data());
        if (!validate(res)) continue;
        for (const auto& ext : extensions)
            details.supported_extensions.insert(ext.extensionName);
        // Enumerate layer extensions
        for (const char* layer_name : m_enabled_layers) {
            res = vkEnumerateDeviceExtensionProperties(details.device, layer_name, &n_extensions, NULL);
            if (!validate(res)) continue;
            extensions.resize(n_extensions);
            res = vkEnumerateDeviceExtensionProperties(details.device, layer_name, &n_extensions, extensions.data());
            if (!validate(res)) continue;
            for (const auto& ext : extensions)
                details.supported_extensions.insert(ext.extensionName);
        }

        m_physical_devices.push_back(details);
    }
    // Successful if at least one physical device could be queried
    return (m_physical_devices.size() != 0);
}

void Instance::load_func_pointers() {
    // Debug procs
    vkCreateDebugReportCallbackEXT = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>( vkGetInstanceProcAddr(m_instance, "vkCreateDebugReportCallbackEXT") );
    vkDebugReportMessageEXT = reinterpret_cast<PFN_vkDebugReportMessageEXT>( vkGetInstanceProcAddr(m_instance, "vkDebugReportMessageEXT") );
    vkDestroyDebugReportCallbackEXT = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>( vkGetInstanceProcAddr(m_instance, "vkDestroyDebugReportCallbackEXT") );

    //
}

uint32_t Instance::get_preferred_device_index() const {
    // TODO: determine based on specs
    return 0;
}

bool Instance::is_layer_supported(const std::string& name) const {
    return (std::find_if(m_supported_layers.begin(), m_supported_layers.end(), 
    [&](const VkLayerProperties& layer) -> bool {
        return (name == layer.layerName);
    }) != m_supported_layers.end());
}

bool Instance::is_extension_supported(const std::string& name) const {
    return (m_extension_providers.find(name) != m_extension_providers.end());
}

//////////////
//          //
//  DEVICE  //
//          //
//////////////


Device::Device(const Instance& source) : Device(source, source.get_preferred_device_index()) {
}

Device::Device(const Instance& source, uint32_t physical_device_index)
    : m_instance(source.vk()), m_enabled_layers(source.m_enabled_layers)
    , m_physical_device(source.get_physical_device(physical_device_index))
    , m_device(VK_NULL_HANDLE), m_universal_queue(VK_NULL_HANDLE)
    , m_compute_queue(VK_NULL_HANDLE), m_transfer_queue(VK_NULL_HANDLE)
    , m_queue_flags(0), m_allocator(VK_NULL_HANDLE)
    , command_pool_flags(0), n_threads(1)
{
    uint32_t n_supported_extensions;
    const char* layer_name = NULL;
    VkResult res = vkEnumerateDeviceExtensionProperties(m_physical_device.device, NULL, &n_supported_extensions, NULL);
    if (!validate(res)) return;
    std::vector<VkExtensionProperties> supported_extensions(n_supported_extensions);
    res = vkEnumerateDeviceExtensionProperties(m_physical_device.device, NULL, &n_supported_extensions, supported_extensions.data());
    if (!validate(res)) return;
    for (const auto& ext : supported_extensions) {
        m_supported_extensions.insert(ext.extensionName);
    }

    for (const auto& layer_name : m_enabled_layers) {
        res = vkEnumerateDeviceExtensionProperties(m_physical_device.device, layer_name, &n_supported_extensions, NULL);
        if (!validate(res)) return;
        std::vector<VkExtensionProperties> supported_extensions(n_supported_extensions);
        VkResult res = vkEnumerateDeviceExtensionProperties(m_physical_device.device, layer_name, &n_supported_extensions, supported_extensions.data());
        if (!validate(res)) return;
        for (const auto& ext : supported_extensions) {
            m_supported_extensions.insert(ext.extensionName);
        }
    }

    enabled_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
}

bool Device::init() {
    std::vector<VkDeviceQueueCreateInfo> queue_info;
    const float default_queue_priority = 0.0f;
    VkDeviceQueueCreateInfo queue = {
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, // sType
        nullptr,                                    // pNext
        0,                                          // flags (reserved)
        m_physical_device.queue_families.universal, // queueFamilyIndex
        1,                                          // queueCount
        &default_queue_priority                     // pQueuePriorities
    };

    constexpr uint32_t universal_index = 0;
    uint32_t compute_index = 0;
    uint32_t transfer_index = 0;

    if (m_physical_device.props.vendorID == VK_VENDOR_ID_AMD) {
        // Works best with 1 universal, 1 compute, and 1 transfer queue

        if (m_physical_device.queue_families.transfer_count > 0) {
            // Use a dedicated transfer queue if it exists
            m_queue_flags |= transfer_owns_self;
            m_queue_flags |= transfer_is_dedicated;
        }
        else if (m_physical_device.queue_families.universal_count > queue.queueCount) {
            m_queue_flags |= transfer_owns_self;
            transfer_index = 1;
            queue.queueCount++;
        }

        if (m_physical_device.queue_families.compute_count > 0) {
            // Use a dedicated compute queue if it exists
            m_queue_flags |= compute_owns_self;
            m_queue_flags |= compute_is_dedicated;
        }
        else if (m_physical_device.queue_families.universal_count > queue.queueCount) {
            m_queue_flags |= compute_owns_self;
            compute_index = transfer_index + 1;
            queue.queueCount++;
        }

        // Request universal queue(s)
        queue_info.push_back(queue);
        // Request compute queue
        if (m_physical_device.queue_families.compute_count > 0) {
            queue.queueCount = 1;
            queue.queueFamilyIndex = m_physical_device.queue_families.compute;
            queue_info.push_back(queue);
        }
        // Request transfer queue
        if (m_physical_device.queue_families.transfer_count > 0) {
            queue.queueCount = 1;
            queue.queueFamilyIndex = m_physical_device.queue_families.transfer;
            queue_info.push_back(queue);
        }
    }
    else if (m_physical_device.props.vendorID == VK_VENDOR_ID_NVIDIA) {
        // Works best with 1 universal and 1 transfer queue

        if (m_physical_device.queue_families.transfer_count > 0) {
            // Use a dedicated transfer queue if it exists
            m_queue_flags |= transfer_owns_self;
            m_queue_flags |= transfer_is_dedicated;

            queue_info.push_back(queue);
            queue.queueFamilyIndex = m_physical_device.queue_families.transfer;
            queue_info.push_back(queue);
        }
        else if (m_physical_device.queue_families.universal_count >= 2) {
            // Create a second universal queue and use it as transfer, if possible
            m_queue_flags |= transfer_owns_self;
            transfer_index = 1;

            queue.queueCount++;
            queue_info.push_back(queue);
        } else {
            // Reuse the same queue for transfer if it's the only one available
            queue_info.push_back(queue);
        }
    }
    else {
        // Default to 1 universal queue
        queue_info.push_back(queue);
    }

    VkDeviceCreateInfo device_info = {
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,   // sType
        nullptr,                                // pNext
        0,                                      // flags (reserved)
        static_cast<uint32_t>(queue_info.size()),           // queueCreateInfoCount
        queue_info.data(),                      // pQueueCreateInfos
        static_cast<uint32_t>(m_enabled_layers.size()),     // enabledLayerCount (deprecated, but provide for backwards compatibility)
        m_enabled_layers.data(),                // ppEnabledLayerNames (deprecated, but provide for backwards compatibility)
        static_cast<uint32_t>(enabled_extensions.size()),   // enabledExtensionCount
        enabled_extensions.data(),              // ppEnabledExtensionNames

    };
    VkResult res = vkCreateDevice(m_physical_device.device, &device_info, NULL, &m_device);
    if (!validate(res)) return false;

    // Store queues
    vkGetDeviceQueue(m_device, m_physical_device.queue_families.universal, universal_index, &m_universal_queue);
    
    if (m_queue_flags & compute_owns_self)
        vkGetDeviceQueue(m_device, m_physical_device.queue_families.compute, compute_index, &m_compute_queue);
    else
        m_compute_queue = m_universal_queue;

    if (m_queue_flags & transfer_owns_self)
        vkGetDeviceQueue(m_device, m_physical_device.queue_families.transfer, transfer_index, &m_transfer_queue);
    else
        m_transfer_queue = m_universal_queue;


    // Create memory allocator
    VmaAllocatorCreateInfo allocator_info = {
        m_physical_device.device,
        m_device
    };
    res = vmaCreateAllocator(&allocator_info, &m_allocator);
    if (!validate(res)) return false;


    // Create command pools
    VkCommandPoolCreateInfo pool_info = {
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, // sType
        nullptr,                                    // pNext
        command_pool_flags,                         // flags
        m_physical_device.queue_families.universal  // queueFamilyIndex
    };

    m_command_pools.resize(QUEUE_FAMILY_COUNT);
    for (uint32_t i = QUEUE_FAMILY_FIRST; i < QUEUE_FAMILY_COUNT; ++i) {
        for (uint32_t j = 0; j < n_threads; ++j) {
            pool_info.queueFamilyIndex = m_physical_device.queue_families.indices[i];
            res = vkCreateCommandPool(m_device, &pool_info, NULL, &m_command_pools[j * QUEUE_FAMILY_COUNT + i]);
            if (!validate(res)) return false;
        }
    }
    
    return true;
}

Device::~Device() {
    for (auto& pool : m_command_pools) {
        if (pool)
            vkDestroyCommandPool(m_device, pool, NULL);
    }
    if (m_allocator)
        vmaDestroyAllocator(m_allocator);

    // Queues are released automatically with the destruction of the device

    if (m_device)
        vkDestroyDevice(m_device, NULL);
}

bool Device::is_extension_supported(const std::string& name) const {
    return (m_supported_extensions.find(name) != m_supported_extensions.end());
}