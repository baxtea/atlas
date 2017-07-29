#ifndef ATLAS_WINDOW_H
#define ATLAS_WINDOW_H

#include <stdint.h>
#include <string>
#include <memory>

namespace Anvil {
    class RenderingSurface;
    class Swapchain;
    class Queue;
    class Window;
}

namespace Atlas {
    class Window {
    protected:
        bool init_xcb();
        void handle_xcb_events();
        void shutdown_xcb();
        bool init_win32();
        void handle_win32_events();
        void shutdown_win32();

        bool init_swapchain();

        const std::string m_name;
        uint32_t m_width, m_height;


        struct xcb_info;
        struct win32_info;
        union {
            xcb_info* xcb;
            win32_info* win32;
        };


        const uint32_t m_n_swapchain_images = 3;
        std::shared_ptr<Anvil::RenderingSurface> m_rendering_surface;
        std::shared_ptr<Anvil::Swapchain> m_swapchain;
        std::shared_ptr<Anvil::Window> m_anvil_clone; // Only cares about width, height, platform
        std::shared_ptr<Anvil::Queue> m_present_queue;

        bool m_fullscreen;
        bool m_should_close;
    public:

        Window(const std::string& name, uint32_t width, uint32_t height);
        ~Window();

        void set_fullscreen(bool full);
        bool is_fullscreen() const;

        void handle_events();
        bool should_close() const;
    };
}


#endif // ATLAS_WINDOW_H