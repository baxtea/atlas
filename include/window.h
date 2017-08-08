#ifndef ATLAS_WINDOW_H
#define ATLAS_WINDOW_H

#include <vector>
#include <unordered_set>
#include <string>
#include <memory>
#include <stdint.h>

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#else
#define VK_USE_PLATFORM_XCB_KHR
#endif
#include <vulkan/vulkan.h>

namespace Atlas {
    namespace Backend {
        struct Instance;
        struct Device;
    }

    struct Window {
        //
        // Generic window stuff
        // TODO: expand
        //

        Window(const Backend::Instance& instance, const std::string& name, uint32_t width, uint32_t height);
        Window(const Backend::Instance& instance, uint32_t physical_device_index, const std::string& name, uint32_t width, uint32_t height);
        ~Window();
        bool init();
        void close();

        void set_fullscreen(bool full);
        inline bool is_fullscreen() const {
            return m_fullscreen;
        }

        void handle_events();
        inline bool should_close() const {
            return m_should_close;
        }
        inline void set_should_close(bool request_close) {
            m_should_close = request_close;
        }

        // If the surface type permits it, recreate the swapchain with a different number of backbuffers
        // vsync + double buffered => no tearing, no vsync + double buffered => tearing
        // vsync + triple buffered => no tearing, no vsync + triple buffered => no tearing (when supported. depends on gpu capabilities)
        bool set_triple_buffered(bool enable);

        void set_vsync(bool enable);
        inline bool get_vsync() const {
            return m_vsync;
        }

        //
        // Presentation
        //
        void present(const std::vector<VkSemaphore> wait_semaphores);

        void acquire_next_frame(uint64_t timeout = UINT64_MAX, VkFence signal_when_done = VK_NULL_HANDLE);

        // Wait for this semaphore before rendering
        inline VkSemaphore get_present_complete_semaphore() const {
            return m_present_complete_semaphores[m_frame_index];
        }

        inline VkFramebuffer get_framebuffer() const {
            return m_fbos[m_frame_index];
        }
        inline VkImage get_color_image() const {
            return m_images[m_frame_index];
        }
        inline VkImageView get_color_image_view() const {
            return m_image_views[m_frame_index];
        }
        inline VkImage get_depth_image() const {
            return m_depth;
        }
        inline VkImageView get_depth_image_view() const {
            return m_depth_view;
        }
        inline uint32_t get_frame_index() const {
            return m_frame_index;
        }
    protected:
        friend struct Backend::Device;
#ifdef _WIN32
        bool init_win32();
        void handle_win32_events();
        void shutdown_win32();
#else
        bool init_xcb();
        void handle_xcb_events();
        void shutdown_xcb();
#endif
        bool init_surface();
        bool init_swapchain(Backend::Device* device);
        // TODO: implement
        bool init_framebuffers();

        const std::string m_name;
        // TODO: client (surface) dimensions vs window dimensions
        uint32_t m_width, m_height;


        struct xcb_info;
        struct win32_info;
        union {
            xcb_info* xcb;
            win32_info* win32;
        };

        // Defaults to triple buffered (3 images)
        uint32_t m_n_swapchain_images;
        uint32_t m_frame_index;
        VkSurfaceCapabilitiesKHR m_surface_caps;
        std::vector<VkSurfaceFormatKHR> m_surface_formats;
        std::vector<VkPresentModeKHR> m_present_modes;

        uint32_t m_physical_device_index;
        std::unordered_set<uint32_t> m_present_capable_families;
        const Backend::Instance& m_instance;
        Backend::Device* m_device;
        VkQueue m_present_queue;
        VkSurfaceKHR m_surface;
        VkSwapchainKHR m_swapchain;
        std::vector<VkImage> m_images;
        std::vector<VkImageView> m_image_views;
        VkImage m_depth;
        VkImageView m_depth_view;
        std::vector<VkFramebuffer> m_fbos;
        std::vector<VkSemaphore> m_present_complete_semaphores;


        // Surface functions
        PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR;
        PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR;
        PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
        PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
        PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;
        PFN_vkGetPhysicalDeviceFormatProperties vkGetPhysicalDeviceFormatProperties;
#ifdef _WIN32
        PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;
#else
        PFN_vkCreateXcbSurfaceKHR vkCreateXcbSurfaceKHR;
#endif

        // Swapchain functions
        PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR;
        PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
        PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
        PFN_vkQueuePresentKHR vkQueuePresentKHR;
        PFN_vkCreateSemaphore vkCreateSemaphore;

        bool m_fullscreen;
        bool m_should_close;
        bool m_vsync;
    };
}


#endif // ATLAS_WINDOW_H