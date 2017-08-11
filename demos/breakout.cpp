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


    Backend::RenderPass renderpass(device);
    uint32_t color_index = renderpass.add_attachment(window.get_color_format(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
    uint32_t depth_index = renderpass.add_attachment(window.get_depth_format(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE);
    
    VkAttachmentReference color_ref = { color_index, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkAttachmentReference depth_ref = { depth_index, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_ref;
    subpass.pDepthStencilAttachment = &depth_ref;
    uint32_t subpass_index = renderpass.add_subpass(subpass);

    VkSubpassDependency load_deps = {};
    load_deps.srcSubpass = VK_SUBPASS_EXTERNAL;
    load_deps.dstSubpass = subpass_index;
    load_deps.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;			
    load_deps.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;	
    load_deps.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    load_deps.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    load_deps.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    renderpass.add_dependency(load_deps);
    VkSubpassDependency store_deps = {};
    store_deps.srcSubpass = subpass_index;
    store_deps.dstSubpass = VK_SUBPASS_EXTERNAL;
    store_deps.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;	
    store_deps.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    store_deps.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    store_deps.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    store_deps.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    renderpass.add_dependency(store_deps);

    if (!renderpass.init()) return 1;
    // TODO: framebuffers don't work correctly in fullscreen (at least with xcb)
    if (!window.init_framebuffers(renderpass)) return 1;

    while (!window.should_close()) {
        window.handle_events();

        // Logic here

        window.acquire_next_frame();
        if (window.should_rebuild()) {
            bool success = true;
            success &= window.rebuild(renderpass);
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