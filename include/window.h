#ifndef ATLAS_WINDOW_H
#define ATLAS_WINDOW_H

#include <stdint.h>
#include <string>
#include <memory>
#include "backend.h"

namespace Atlas {
    class Device;

    namespace Backend {
        struct Swapchain {
            Swapchain(const Device& device);
        };
    }

    struct Window {
        //
        // Generic window stuff
        // TODO: expand
        //

        Window(const Backend::Device& device, const std::string& name, uint32_t width, uint32_t height);
        ~Window();
        bool init();

        void set_fullscreen(bool full);
        inline bool is_fullscreen() const {
            return m_fullscreen;
        }

        void handle_events();
        inline bool should_close() const {
            return m_should_close;
        }

        //
        // Presentation
        //
        // inline std::shared_ptr<Anvil::Swapchain> get_swapchain() {
        //     return m_swapchain;
        // }
        void present();

        // If the surface type permits it, switch to a swapchain with 
        // If init() has already been called, this call may fail
        bool set_triple_buffered(bool enable);
        VkImage get_current_image();

    protected:
#ifdef _WIN32
        friend LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
        bool init_win32();
        void handle_win32_events();
        void shutdown_win32();
#else
        bool init_xcb();
        void handle_xcb_events();
        void shutdown_xcb();
#endif
        bool init_surface();
        bool init_swapchain();
        bool init_framebuffers();

        const std::string m_name;
        uint32_t m_width, m_height;


        struct xcb_info;
        struct win32_info;
        union {
            xcb_info* xcb;
            win32_info* win32;
        };

        // Defaults to triple buffered (3 images)
        uint32_t n_swapchain_images;
        VkSurfaceCapabilitiesKHR m_surface_caps;
        std::vector<VkSurfaceFormatKHR> m_surface_formats;
        std::vector<VkPresentModeKHR> m_present_modes;
        const Backend::Device& m_device;
        VkSurfaceKHR m_present_surface;
        VkSwapchainKHR m_swapchain;
        VkImage m_current_image;
        std::vector<VkFramebuffer> m_fbos;


        PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR;
        PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR;
        PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
        PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
        PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;
#ifdef _WIN32
        PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;
#else
        PFN_vkCreateXcbSurfaceKHR vkCreateXcbSurfaceKHR;
#endif

        
        /*
        std::shared_ptr<Anvil::Window> m_anvil_clone; // Only cares about width, height, platform
        std::shared_ptr<Anvil::RenderingSurface> m_rendering_surface;
        std::shared_ptr<Anvil::Swapchain> m_swapchain;
        std::shared_ptr<Anvil::Framebuffer> m_framebuffers[m_n_swapchain_images];
        std::shared_ptr<Anvil::Queue> m_present_queue;*/

        bool m_fullscreen;
        bool m_should_close;
        bool m_image_is_old;
    };
}


#endif // ATLAS_WINDOW_H