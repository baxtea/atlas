#include "backend.h"

const char* vert_source = 
"#version 430\n"
"layout (location = 0) in vec3 pos;\n"
"layout (location = 1) in vec3 color_in;\n"
"layout (location = 0) out vec3 color_out;\n"
"void main() {\n"
"  gl_Position = vec4(pos, 1.0);\n"
"  color_out = color_in;\n"
"}\n";

const char* frag_source = 
"#version 430\n"
"layout (location = 0) in vec3 color;\n"
"layout (location = 0) out vec4 frag_color;\n"
"void main() {\n"
"  frag_color = vec4(color, 1.0);\n"
"}\n";

const char* app_name = "Breakout";

using namespace Atlas;

int main() {
    Backend::Instance instance(app_name, VK_MAKE_VERSION(0,0,1), VALIDATION_VERBOSE);
    if (!instance.init()) return 1;

    Window window(instance, app_name, 1280, 720);
    //if (!window.set_fullscreen(true)) return 1;
    if (!window.init()) return 1;

    Backend::Device device(window);
    if (!device.init()) return 1;

    while (!window.should_close()) {
        window.handle_events();

        // Logic here

        window.acquire_next_frame();
        if (window.should_recreate_swapchain()) {
            bool success = true;
            success &= window.recreate_swapchain();
            // success &= framebuffer.recreate();
            // success &= rebuild_command_buffers();
            // camera.update_aspect_ratio()
            if (!success) return 1;
        }

        // Render here

        window.present();
    }

    // Window closes automatically on exit
    return 0;
}