#include "atlas.h"
#include "camera.h"
#include <chrono>

using namespace Atlas;

struct Frame {
    std::chrono::microseconds time_elapsed;
    std::unique_ptr<Camera> camera;
};

bool draw_frame(const Frame& info) {

}

int main() {
    std::chrono::high_resolution_clock::time_point last_frame_time = std::chrono::high_resolution_clock::now();
    std::chrono::high_resolution_clock::time_point curr_frame_time;
    Frame frame_info;
    bool good;

    frame_info.camera = std::make_unique<Camera>("Camera");


    init_vulkan("Breakout");
    auto vertex_shader = Shader::create_module_from_source("", Anvil::SHADER_STAGE_VERTEX);
    //if (load_shader_from_glsl_source())
    /*
    create_window();
    */


    do {
        curr_frame_time = std::chrono::high_resolution_clock::now();
        frame_info.time_elapsed = std::chrono::duration_cast<std::chrono::microseconds>(curr_frame_time - last_frame_time);
        last_frame_time = curr_frame_time;

        good &= draw_frame(frame_info);
    } while (good);
    return 0;
}