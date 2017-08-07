#include "backend.h"
#include "window.h"
#include <chrono>

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
    Backend::Instance instance(app_name, VK_MAKE_VERSION(0,0,1));
    if (!instance.init()) return 1;

    Window window(instance, app_name, 1280, 720);
    if (!window.init()) return 1;

    Backend::Device device(window);
    if (!device.init()) return 1;

    while (!window.should_close()) {
        window.handle_events();
    }

    // Window closes automatically on exit
    return 0;
}

/*
struct App {
    std::chrono::high_resolution_clock::time_point last_frame_time;
    std::chrono::high_resolution_clock::time_point curr_frame_time;
    std::chrono::microseconds time_elapsed;

    std::shared_ptr<Atlas::Window> window;

    std::unique_ptr<Camera> camera;

    std::shared_ptr<Anvil::ShaderModule> vert, frag;

    // Semaphores
};

void init(App& app) {
    app.last_frame_time = std::chrono::high_resolution_clock::now();

    init_vulkan(app_name);
    app.window = std::make_shared<Atlas::Window>(app_name, 1280, 720);


    app.vert = create_shader_module_from_source(vert_source, Anvil::SHADER_STAGE_VERTEX);
    app.frag = create_shader_module_from_source(frag_source, Anvil::SHADER_STAGE_VERTEX);

    app.camera = std::make_unique<Camera>("Camera");
}

void do_frame(App& app) {
    app.window->handle_events();
    app.curr_frame_time = std::chrono::high_resolution_clock::now();
    app.time_elapsed = std::chrono::duration_cast<std::chrono::microseconds>(app.curr_frame_time - app.last_frame_time);
    app.last_frame_time = app.curr_frame_time;
}

void shutdown(App& app) {
    app.frag.reset();
    app.vert.reset();

    app.window.reset();
    shutdown_vulkan();
}

int main() {
    App app;

    init(app);

    while (!app.window->should_close()) {
        do_frame(app);
    }

    shutdown(app);

    return 0;
}*/