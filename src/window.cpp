#include "window.h"
#include "backend.h"
#include <assert.h>
#include <stdio.h>
#include <cstring>
#include <chrono>

using namespace Atlas;

//
// XCB implementation
//
#ifndef _WIN32

#include <xcb/xcb.h>
struct Window::xcb_info {
    xcb_connection_t* connection;
    xcb_screen_t* screen;
    xcb_window_t window;
    xcb_generic_event_t* event;
    xcb_atom_t protocols_atom;
    xcb_atom_t delete_win_atom;
};

bool Window::init_xcb() {
    xcb->connection = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(xcb->connection)) {
        Backend::error("Could not connect to X server");
        return false;
    }

    // TODO: Select screen manually. For now, always uses the first.
    const xcb_setup_t* setup = xcb_get_setup(xcb->connection);
    xcb_screen_iterator_t screen_iter = xcb_setup_roots_iterator(setup);
    xcb->screen = screen_iter.data;


    const uint32_t value_mask = XCB_CW_EVENT_MASK;
    uint32_t value_list = XCB_EVENT_MASK_EXPOSURE;

    xcb->window = xcb_generate_id(xcb->connection);
    xcb_void_cookie_t err_cookie = xcb_create_window_checked(
                        xcb->connection,                // Connection
                        XCB_COPY_FROM_PARENT,           // Depth
                        xcb->window,                    // Window id
                        xcb->screen->root,              // Parent window
                        0, 0,                           // Location
                        m_width, m_height,              // Dimensions
                        0,                              // Border width
                        XCB_WINDOW_CLASS_INPUT_OUTPUT,  // Class
                        xcb->screen->root_visual,       // Visual
                        value_mask, &value_list);       // Masks

    xcb_generic_error_t* err = xcb_request_check(xcb->connection, err_cookie);
    if (err != NULL)  {
        std::string err_string = "Could not create window. X11 error " + err->error_code;
        Backend::error(err_string);
        return false;
    }

    // We want to be notified when the window manager attempts to destroy the window
    xcb_intern_atom_cookie_t delete_cookie = xcb_intern_atom(
        xcb->connection, 0, strlen("WM_DELETE_WINDOW"), "WM_DELETE_WINDOW");
    xcb_intern_atom_cookie_t protocols_cookie = xcb_intern_atom(
        xcb->connection, 0, strlen("WM_PROTOCOLS"), "WM_PROTOCOLS");

    xcb_intern_atom_reply_t* delete_reply = xcb_intern_atom_reply(
        xcb->connection, delete_cookie, NULL);
    xcb_intern_atom_reply_t* protocols_reply = xcb_intern_atom_reply(
        xcb->connection, protocols_cookie, NULL);

    err_cookie = xcb_change_property_checked(xcb->connection, XCB_PROP_MODE_REPLACE, xcb->window,
        protocols_reply->atom, 4, 32, 1, &delete_reply->atom);
    if (err != NULL)  {
        std::string err_string = "Could not override window delete handler. X11 error " + err->error_code;
        Backend::error(err_string);
        return false;
    }

    xcb->delete_win_atom = delete_reply->atom;
    xcb->protocols_atom = protocols_reply->atom;



    // Only returns an error if the window doesn't exist -- and we just checked that it does
    xcb_map_window(xcb->connection, xcb->window);
    
    if (xcb_flush(xcb->connection) <= 0) {
        Backend::error("Error while flushing xcb stream!");
        return false;
    }

    return true;
}

void Window::handle_xcb_events() {
    xcb_expose_event_t* expose;
    xcb_client_message_event_t* cm;

    while ( (xcb->event = xcb_poll_for_event(xcb->connection)) ) {
        switch (xcb->event->response_type & ~0x80) {
        case XCB_CLIENT_MESSAGE:
            cm = reinterpret_cast<xcb_client_message_event_t*>(xcb->event);
            if (cm->data.data32[0] == xcb->delete_win_atom) {
                m_should_close = true;
            }

            break;
/*
        case XCB_EXPOSE:
            expose = reinterpret_cast<xcb_expose_event_t*>(xcb->event);
            if (expose->window == xcb->window) {
                m_width = expose->width;
                m_height = expose->height;
            }
            break;
*/
        default:
            // Unknown event type, ignore it
            break;
        }

        free (xcb->event);
    }
}

void Window::shutdown_xcb() {
    if (xcb->window)
        xcb_destroy_window(xcb->connection, xcb->window);
    if (xcb->connection)
        xcb_disconnect(xcb->connection);
}



#endif // !_WIN32
#ifdef _WIN32
#include <Windows.h>

struct Window::win32_info {
    HINSTANCE inst;
    HWND hwnd;
    MSG msg;
};


LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    LONG_PTR lpUserData;
    Atlas::Window* window;

    switch (msg) {
    case WM_CLOSE:
        lpUserData = GetWindowLongPtr(hWnd, GWLP_USERDATA);
        window = reinterpret_cast<Atlas::Window*>(lpUserData);
        if (window)
            window->set_should_close(true);

        break;
    case WM_DESTROY:
        DestroyWindow(hWnd);
        PostQuitMessage(0);
        break;
    case WM_PAINT:
        ValidateRect(hWnd, NULL);
        break;
    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

bool Window::init_win32() {
    win32->inst = GetModuleHandle(NULL);


    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(WNDCLASSEX));

    wc.cbSize           = sizeof(WNDCLASSEX);
    wc.style            = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc      = (WNDPROC)WndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = win32->inst;
    wc.hIcon            = (HICON)LoadIcon(NULL, MAKEINTRESOURCE(IDI_APPLICATION));
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName     = m_name.c_str();
    wc.lpszClassName    = m_name.c_str();
    wc.hIconSm          = (HICON)LoadIcon(NULL, MAKEINTRESOURCE(IDI_APPLICATION));

    if (!RegisterClassEx(&wc)) {
        std::string err_string = "Failed to register WNDCLASSEX! Error code " + GetLastError();
        Backend::error(err_string);
        return false;
    }


    win32->hwnd = CreateWindowEx(
                WS_EX_APPWINDOW,        // Extended style
                m_name.c_str(),         // Class name
                m_name.c_str(),         // Window title
                WS_CLIPSIBLINGS |       // Window style
                WS_CLIPCHILDREN |       // Window style (cont)
                WS_OVERLAPPEDWINDOW,    // Window style (cont)
                0, 0,                   // Position
                m_width, m_height,      // Dimensions
                NULL,                   // No parent window
                NULL,                   // No menu
                win32->inst,            // Instance
                NULL);                  // Don't pass anything to WM_CREATE
    
    if (!win32->hwnd) {
        Backend::error("Failed to create window!\n");
        return false;
    }

    SetWindowLongPtr(win32->hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    ShowWindow(win32->hwnd, SW_SHOW);
    return true;
}

void Window::handle_win32_events() {
    while (PeekMessage(&win32->msg, NULL, 0, 0, PM_REMOVE)) {
        if (win32->msg.message == WM_QUIT)
            m_should_close = true;
        
        TranslateMessage(&win32->msg);
        DispatchMessage(&win32->msg);
    }
}

void Window::shutdown_win32() {
    if (win32->hwnd)
        DestroyWindow(win32->hwnd);

    UnregisterClass(m_name.c_str(), win32->inst);
}

#endif // _WIN32

Window::Window(const Backend::Instance& instance, const std::string& name, uint32_t width, uint32_t height)
    : Window(instance, instance.get_preferred_device_index(), name, width, height)
{ }


Window::Window(const Backend::Instance& instance, uint32_t physical_device_index, const std::string& name, uint32_t width, uint32_t height)
    : m_name(name), m_width(width), m_height(height), m_fullscreen(false)
    , m_should_close(false), m_image_is_old(false), m_n_swapchain_images(3)
    , m_instance(instance), m_surface(VK_NULL_HANDLE), m_swapchain(VK_NULL_HANDLE)
    , m_physical_device_index(physical_device_index), vkCreateSwapchainKHR(VK_NULL_HANDLE), vkDestroySwapchainKHR(VK_NULL_HANDLE)
    , vkGetSwapchainImagesKHR(VK_NULL_HANDLE), vkAcquireNextImageKHR(VK_NULL_HANDLE), vkQueuePresentKHR(VK_NULL_HANDLE)
    , vkGetPhysicalDeviceFormatProperties(VK_NULL_HANDLE), m_device(VK_NULL_HANDLE), m_vsync(true)
{
    // Surface functions
    vkDestroySurfaceKHR = reinterpret_cast<PFN_vkDestroySurfaceKHR>( vkGetInstanceProcAddr(instance.vk(), "vkDestroySurfaceKHR") );
    vkGetPhysicalDeviceSurfaceSupportKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>( vkGetInstanceProcAddr(instance.vk(), "vkGetPhysicalDeviceSurfaceSupportKHR") );
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>( vkGetInstanceProcAddr(instance.vk(), "vkGetPhysicalDeviceSurfaceCapabilitiesKHR") );
    vkGetPhysicalDeviceSurfaceFormatsKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>( vkGetInstanceProcAddr(instance.vk(), "vkGetPhysicalDeviceSurfaceFormatsKHR") );
    vkGetPhysicalDeviceSurfacePresentModesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>( vkGetInstanceProcAddr(instance.vk(), "vkGetPhysicalDeviceSurfacePresentModesKHR") );
    vkGetPhysicalDeviceFormatProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceFormatProperties>( vkGetInstanceProcAddr(instance.vk(), "vkGetPhysicalDeviceFormatProperties") );
#ifdef _WIN32
    vkCreateWin32SurfaceKHR = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>( vkGetInstanceProcAddr(instance.vk(), "vkCreateWin32SurfaceKHR") );
#else
    vkCreateXcbSurfaceKHR = reinterpret_cast<PFN_vkCreateXcbSurfaceKHR>( vkGetInstanceProcAddr(instance.vk(), "vkCreateXcbSurfaceKHR") );
#endif
    // Swapchain functions are loaded when the device is initialized
}

Window::~Window() {
    // Swapchain and image views are destroyed with the device destructor

    if (m_surface)
        vkDestroySurfaceKHR(m_instance.vk(), m_surface, nullptr);

#   ifdef _WIN32
        shutdown_win32();
        free(win32);
#   else
        shutdown_xcb();
        free(xcb);
#   endif
}

bool Window::init_surface() {
#ifdef _WIN32
    VkWin32SurfaceCreateInfoKHR surface_info = {
        VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,    // sType
        nullptr,                                            // pNext
        0,                                                  // flags
        win32->inst,                                        // hinstance
        win32->hwnd                                         // hwnd
    };
    vkCreateWin32SurfaceKHR(m_instance.vk(), &surface_info, nullptr, &m_surface);

#else
    VkXcbSurfaceCreateInfoKHR surface_info = {
        VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,  // sType
        nullptr,                                        // pNext
        0,                                              // flags
        xcb->connection,                                // connection
        xcb->window                                     // window
    };
    vkCreateXcbSurfaceKHR(m_instance.vk(), &surface_info, nullptr, &m_surface);
#endif

    VkPhysicalDevice phys = m_instance.get_physical_device(m_physical_device_index).device;

    // Query physical device surface capabilities
    VkResult res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys, m_surface, &m_surface_caps);
    if (!validate(res)) return false;

    uint32_t n_surface_formats;
    res = vkGetPhysicalDeviceSurfaceFormatsKHR(phys, m_surface, &n_surface_formats, NULL);
    if (!validate(res)) return false;
    m_surface_formats.resize(n_surface_formats);
    res = vkGetPhysicalDeviceSurfaceFormatsKHR(phys, m_surface, &n_surface_formats, m_surface_formats.data());
    if (!validate(res)) return false;
    
    uint32_t n_present_modes;
    res = vkGetPhysicalDeviceSurfacePresentModesKHR(phys, m_surface, &n_present_modes, NULL);
    if (!validate(res)) return false;
    m_present_modes.resize(n_present_modes);
    res = vkGetPhysicalDeviceSurfacePresentModesKHR(phys, m_surface, &n_present_modes, m_present_modes.data());
    if (!validate(res)) return false;

    // Search for a present-capable queue
    uint32_t n_physical_device_queues;
    vkGetPhysicalDeviceQueueFamilyProperties(phys, &n_physical_device_queues, NULL);
    bool no_present_queue = true;
    VkBool32 supports_present;
    for (uint32_t queue_family = 0; queue_family < n_physical_device_queues; ++queue_family) {
        supports_present = VK_FALSE;
        res = vkGetPhysicalDeviceSurfaceSupportKHR(phys, queue_family, m_surface, &supports_present);
        if (validate(res) && supports_present) {
            m_present_capable_families.insert(queue_family);
            no_present_queue= false;
        }
    }

    if (no_present_queue) {
        Backend::error("Couldn't find a present-capable queue family!");
        return false;
    }

    return true;
}

bool Window::init_swapchain(VkDevice device) {
    m_device = device;
    // If we don't make the new swapchain a derivative of the old one, all the resources have to be reloaded
    // (which is obviously too much for changing vsync)
    VkSwapchainKHR old_swapchain = m_swapchain;

    // Get preferred formats
    VkFormat color_format;
    VkColorSpaceKHR color_space;
    if ( (m_surface_formats.size() == 1) && (m_surface_formats[0].format == VK_FORMAT_UNDEFINED) ) {
        // There is no preferred format, so assume VK_FORMAT_B8G8R8A8_UNORM
        color_format = VK_FORMAT_B8G8R8A8_UNORM;
        color_space = m_surface_formats[0].colorSpace;
    }
    else {
        // Iterate over the list of available surface formats and look for VK_FORMAT_B8G8R8A8_UNORM
        bool found_B8G8R8A8_UNORM = false;
        for (const auto& format : m_surface_formats) {
            if (format.format == VK_FORMAT_B8G8R8A8_UNORM) {
                color_format = format.format;
                color_space = format.colorSpace;
                found_B8G8R8A8_UNORM = true;
                break;
            }
        }
        // If it is not available, select the first available color format
        if (!found_B8G8R8A8_UNORM) {
            color_format = m_surface_formats[0].format;
            color_space = m_surface_formats[0].colorSpace;
        }
    }

    // Image size
    VkExtent2D extent;
    if (m_surface_caps.currentExtent.width == (uint32_t)-1) {
        // 0xFFFFFFFF means that the dimensions can be requested
        extent.width = m_width;
        extent.height = m_height;
    }
    else {
        // If the surface size is defined, the swap chain size must match
        extent = m_surface_caps.currentExtent;
    }

    // Swapchain present mode
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    if (!m_vsync) {
        // TODO: Look into VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR
        for (const auto& mode : m_present_modes) {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                // Lowest-latency non-tearing present mode
                present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
            else if ( (present_mode != VK_PRESENT_MODE_MAILBOX_KHR) && (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) ) {
                // May (and probably will) tear, but no latency
                present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            }
        }
    }

    // Swapchain image count
    uint32_t desired_n_images = m_surface_caps.minImageCount + 1;
    if ( (m_surface_caps.maxImageCount > 0) && (desired_n_images > m_surface_caps.maxImageArrayLayers) ) {
        // Be safe in the case where min = max, and min + 1 is no good
        desired_n_images = m_surface_caps.maxImageCount;
    }

    // Surface transformation
    VkSurfaceTransformFlagsKHR pre_transform = m_surface_caps.currentTransform;
    if (m_surface_caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        // Set to a non-rotated transform, it available
        pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }

    // Find a supported composite alpha format
    VkCompositeAlphaFlagBitsKHR composite_alpha;
    std::vector<VkCompositeAlphaFlagBitsKHR> preferred_alphas = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
    };
    for (const auto& alpha : preferred_alphas) {
        if (m_surface_caps.supportedCompositeAlpha & alpha) {
            composite_alpha = alpha;
            break;
        }
    }

    // Set an additional usage flag for blitting with the swapchain images if supported
    VkImageUsageFlags image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    VkFormatProperties format_props;
    vkGetPhysicalDeviceFormatProperties(m_instance.get_physical_device(m_physical_device_index).device, color_format, &format_props);
    if (format_props.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) {
        image_usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    if (format_props.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT) {
        image_usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    // Create the swapchain
    VkSwapchainCreateInfoKHR swapchain_info = {
        VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,    // sType
        nullptr,                                        // pNext
        0,                                              // flags
        m_surface,                                      // surface
        desired_n_images,                               // minImageCount
        color_format,                                   // imageFormat
        color_space,                                    // imageColorSpace
        extent,                                         // imageExtent
        1,                                              // imageArrayLayers
        image_usage,                                    // imageUsage
        VK_SHARING_MODE_EXCLUSIVE,                      // imageSharingMode
        0,                                              // queueFamilyIndexCount (ignored when imageSharingMode is exlusive)
        nullptr,                                        // pQueueFamilyIndices (also ignored)
        (VkSurfaceTransformFlagBitsKHR)pre_transform,   // preTransform
        composite_alpha,                                // compositeAlpha
        present_mode,                                   // presentMode
        VK_TRUE,                                        // clipped
        old_swapchain                                   // oldSwapchain
    };
    
    VkResult res = vkCreateSwapchainKHR(device, &swapchain_info, nullptr, &m_swapchain);
    if (!validate(res)) return false;

    // If this was a re-creation of an existing swapchain, destroy the old one
    // (also cleans up all the presentable images)
    if (old_swapchain != VK_NULL_HANDLE) {
        for (auto& view : m_image_views) {
            vkDestroyImageView(device, view, nullptr);
        }
        vkDestroySwapchainKHR(device, old_swapchain, nullptr);
    }

    // Get the swapchain images
    uint32_t n_images;
    res = vkGetSwapchainImagesKHR(device, m_swapchain, &n_images, nullptr);
    if (!validate(res)) return false;
    m_images.resize(n_images);
    m_image_views.resize(n_images);
    res = vkGetSwapchainImagesKHR(device, m_swapchain, &n_images, m_images.data());
    if (!validate(res)) return false;


    // Create image views
    VkImageSubresourceRange subresource_range = {
        VK_IMAGE_ASPECT_COLOR_BIT,  // aspectMask
        0,                          // baseMipLevel
        1,                          // levelCount
        0,                          // baseArrayLayer
        1                           // layerCount
    };
    VkImageViewCreateInfo attachment_view = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,   // sType
        nullptr,                                    // pNext
        0,                                          // flags
        VK_NULL_HANDLE,                             // image
        VK_IMAGE_VIEW_TYPE_2D,                      // viewType
        color_format,                               // format
        {   VK_COMPONENT_SWIZZLE_R,
            VK_COMPONENT_SWIZZLE_G,
            VK_COMPONENT_SWIZZLE_B,
            VK_COMPONENT_SWIZZLE_A
        },                                          // components
        subresource_range                           // subresourceRange
    };
    for (uint32_t i = 0; i < n_images; ++i) {
        attachment_view.image = m_images[i];
        res = vkCreateImageView(device, &attachment_view, nullptr, &m_image_views[i]);
        if (!validate(res)) return false;
    }

    return true;
}

bool Window::init() {
#   ifdef _WIN32
        win32 = static_cast<win32_info*>(malloc(sizeof(win32_info)));
        memset(&win32, sizeof(win32_info), 0);
        return init_win32() && init_surface();
#   else
        xcb = static_cast<xcb_info*>(malloc(sizeof(xcb_info)));
        memset(&xcb, sizeof(xcb_info), 0);
        return init_xcb() && init_surface();
#   endif
}

/*
void Window::init_framebuffers() {
    bool result;

    for (uint32_t n_swapchain_image = 0;
                  n_swapchain_image < m_n_swapchain_images;
                ++n_swapchain_image)
    {
        m_framebuffers[n_swapchain_image] = Anvil::Framebuffer::create(
            g_device,
            m_width, m_height,
            1); // n_layers

        m_framebuffers[n_swapchain_image]->set_name_formatted("Framebuffer used to render to swapchain image [%d]",
            n_swapchain_image);

        result = m_framebuffers[n_swapchain_image]->add_attachment(m_swapchain->get_image_view(n_swapchain_image),
            nullptr); // out_opt_attachment_id_ptrs
        anvil_assert(result);
    }
}*/

void Window::handle_events() {
#if _WIN32
    handle_win32_events();
#else
    handle_xcb_events();
#endif
}

void Window::close() {
#if _WIN32
    DestroyWindow(win32->hwnd);
    win32->hwnd = 0;
#else
    xcb_destroy_window(xcb->connection, xcb->window);
    xcb->window = 0;
#endif
}