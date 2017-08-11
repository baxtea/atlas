#include "window.h"
#include "backend.h"
#include <algorithm>
#include <array>
#include <assert.h>
#include <stdio.h>
#include <cstring>
#include <chrono>
#include <xcb/xcb_icccm.h>

using namespace Atlas;

//
// XCB implementation
//
#ifndef _WIN32

#include <xcb/xcb.h>
struct Window::xcb_info {
    xcb_connection_t* connection;
    xcb_window_t window;
    xcb_generic_event_t* event;
    xcb_atom_t protocols_atom;
    xcb_atom_t delete_win_atom;
    xcb_atom_t wm_state_atom;
    xcb_atom_t fullscreen_atom;
    xcb_screen_t* screen;
};

bool Window::init_xcb() {
    xcb->connection = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(xcb->connection)) {
        Backend::error("Could not connect to X server");
        return false;
    }

    // TODO: Select screen some other way. For now, always uses the first.
    xcb_screen_iterator_t screen_iter = xcb_setup_roots_iterator(xcb_get_setup(xcb->connection));
    xcb->screen = screen_iter.data;


    const uint32_t value_mask = XCB_CW_EVENT_MASK;
    uint32_t value_list = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY;

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
                        xcb->screen->root_visual,        // Visual
                        value_mask, &value_list);       // Masks

    xcb_generic_error_t* err = xcb_request_check(xcb->connection, err_cookie);
    if (err != NULL)  {
        std::string err_string = "Could not create window. X11 error code " + err->error_code;
        Backend::error(err_string);
        free(err);
        return false;
    }
    free(err);

    xcb_size_hints_t size_hints = {};
    xcb_icccm_size_hints_set_min_size(&size_hints, m_width, m_height);
    xcb_icccm_size_hints_set_max_size(&size_hints, m_width, m_height);
    xcb_icccm_set_wm_size_hints(xcb->connection, xcb->window, XCB_ATOM_WM_NORMAL_HINTS, &size_hints);

    //
    // Grab a bunch of atoms from the window
    //

    xcb_intern_atom_cookie_t delete_cookie = xcb_intern_atom(xcb->connection, 0, strlen("WM_DELETE_WINDOW"), "WM_DELETE_WINDOW");
    xcb_intern_atom_cookie_t protocols_cookie = xcb_intern_atom(xcb->connection, 0, strlen("WM_PROTOCOLS"), "WM_PROTOCOLS");
    xcb_intern_atom_cookie_t wm_state_cookie = xcb_intern_atom(xcb->connection, 0, strlen("_NET_WM_STATE"), "_NET_WM_STATE");
    xcb_intern_atom_cookie_t fullscreen_cookie = xcb_intern_atom(xcb->connection, 0, strlen("_NET_WM_STATE_FULLSCREEN"), "_NET_WM_STATE_FULLSCREEN");


    // Cache the atoms which control if the window is fullscreen
    xcb_intern_atom_reply_t* wm_state_reply = xcb_intern_atom_reply(xcb->connection, wm_state_cookie, nullptr);
    if (!wm_state_reply) {
        Backend::error("Could not retrieve the window's _NET_WM_STATE atom!");
        return false;
    }
    xcb->wm_state_atom = wm_state_reply->atom;
    free(wm_state_reply);
    xcb_intern_atom_reply_t* fullscreen_reply = xcb_intern_atom_reply(xcb->connection, fullscreen_cookie, nullptr);
    if (!fullscreen_reply) {
        Backend::error("Could not retrieve the window's _NET_WM_STATE_FULLSCREEN atom!");
        return false;
    }
    xcb->fullscreen_atom = fullscreen_reply->atom;
    free(fullscreen_reply);

    // Notify the event handler when the close button is pressed
    xcb_intern_atom_reply_t* delete_reply = xcb_intern_atom_reply(
        xcb->connection, delete_cookie, NULL);
    if (!delete_reply) {
        Backend::error("Could not retrieve the window's WM_DELETE_WINDOW atom!");
        return false;
    }
    xcb_intern_atom_reply_t* protocols_reply = xcb_intern_atom_reply(
        xcb->connection, protocols_cookie, NULL);
    if (!delete_reply) {
        Backend::error("Could not retrieve the window's WM_PROTOCOLS atom!");
        return false;
    }
    err_cookie = xcb_change_property_checked(xcb->connection, XCB_PROP_MODE_REPLACE, xcb->window,
        protocols_reply->atom, 4, 32, 1, &delete_reply->atom);
    err = xcb_request_check(xcb->connection, err_cookie);
    if (err != NULL)  {
        std::string err_string = "Could not override window delete handler. X11 error code " + err->error_code;
        Backend::error(err_string);
        free(err);
        return false;
    }
    xcb->delete_win_atom = delete_reply->atom;
    xcb->protocols_atom = protocols_reply->atom;
    free(err);
    free(delete_reply);
    free(protocols_reply);

    // Only returns an error if the window doesn't exist -- and we just checked that it does
    xcb_map_window(xcb->connection, xcb->window);
    set_fullscreen(m_flags & request_fullscreen);
    
    if (xcb_flush(xcb->connection) <= 0) {
        Backend::error("Error while flushing xcb stream!");
        return false;
    }

    return true;
}

void Window::handle_xcb_events() {
    while ( (xcb->event = xcb_poll_for_event(xcb->connection)) ) {
        switch (xcb->event->response_type & ~0x80) {
        case XCB_CLIENT_MESSAGE: {
            xcb_client_message_event_t* client_event = reinterpret_cast<xcb_client_message_event_t*>(xcb->event);
            if (client_event->window == xcb->window && client_event->type == xcb->protocols_atom && client_event->data.data32[0] == xcb->delete_win_atom) {
                m_flags |= request_close;
            }

            break;
        }
        case XCB_CONFIGURE_NOTIFY: {
            Backend::log("Resize requested...");
            const xcb_configure_notify_event_t* cfg_event = reinterpret_cast<const xcb_configure_notify_event_t*>(xcb->event);
            if (  ((m_desired_width == m_width) && (m_desired_height == m_height))
            && ((cfg_event->width != m_width) || (cfg_event->height != m_height))  ) {
                m_desired_width = cfg_event->width;
                m_desired_height = cfg_event->height;
                m_flags |= surface_changed;
            }
        }
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
    switch (msg) {
    case WM_CLOSE: {
        LONG_PTR lpUserData = GetWindowLongPtr(hWnd, GWLP_USERDATA);
        Atlas::Window* window = reinterpret_cast<Atlas::Window*>(lpUserData);
        if (window)
            window->set_should_close(true);

        break;
    }
    case WM_DESTROY:
        DestroyWindow(hWnd);
        PostQuitMessage(0);
        break;
    case WM_PAINT:
        ValidateRect(hWnd, NULL);
        break;
    case WM_SIZE:
        if (  ((m_desired_width == m_width) && (m_desired_height == m_height) && (wParam != SIZE_MINIMIZED) {
            if ( (m_flags & resizing) || ((wParam == SIZE_MAXIMIZED) || (wParam == SIZE_RESTORED)) ) {
                m_desired_width = LOWORD(lParam);
                m_desired_height = HIWORD(lParam);
                m_flags |= surface_changed;
            }
        }
            && ((cfg_event->width != m_width) || (cfg_event->height != m_height))  ) {
                m_desired_width = cfg_event->width;
                m_desired_height = cfg_event->height;
                m_flags |= surface_changed;
            }
        break;
    case WM_ENTERSIZEMOVE:
        m_flags |= resizing;
        break;
    case WM_EXITSIZEMOVE:
        m_flags &= (~resizing);
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

    long dwStyle = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    win32->hwnd = CreateWindowEx(
                WS_EX_APPWINDOW,        // Extended style
                m_name.c_str(),         // Class name
                m_name.c_str(),         // Window title
                dwStyle,                // Window style
                CW_USEDEFAULT,          // Width
                CW_USEDEFAULT,          // Height
                m_width, m_height,      // Dimensions
                NULL,                   // No parent window
                NULL,                   // No menu
                win32->inst,            // Instance
                NULL);                  // Don't pass anything to WM_CREATE
    
    if (!win32->hwnd) {
        Backend::error("Failed to create window!\n");
        return false;
    }

    RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = m_width;
    rect.bottom = m_height;
    SetWindowLongPtr(hWnd, GWL_STYLE, dwStyle | WS_VISIBLE);
    AdjustWindowRect(&rect, dwStyle, FALSE); // Make the client area (not the window) equal to the desired surface size
    SetWindowLongPtr(win32->hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));


    set_fullscreen(m_flags & request_fullscreen);
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
    : m_name(name), m_width(width), m_height(height), m_flags(0), m_n_swapchain_images(3), m_frame_index(0), m_desired_width(width), m_desired_height(height)
    , m_color_format(VK_FORMAT_UNDEFINED), m_depth_format(VK_FORMAT_UNDEFINED), m_color_space(VK_COLOR_SPACE_MAX_ENUM_KHR), m_instance(instance), m_device(nullptr)
    , m_physical_device_index(physical_device_index), m_surface(VK_NULL_HANDLE), m_swapchain(VK_NULL_HANDLE), m_depth(VK_NULL_HANDLE), m_depth_view(VK_NULL_HANDLE)
    , vkCreateSwapchainKHR(VK_NULL_HANDLE), vkGetSwapchainImagesKHR(VK_NULL_HANDLE), vkCreateFramebuffer(VK_NULL_HANDLE), vkDestroyFramebuffer(VK_NULL_HANDLE)
    , vkAcquireNextImageKHR(VK_NULL_HANDLE), vkQueuePresentKHR(VK_NULL_HANDLE), vkGetPhysicalDeviceFormatProperties(VK_NULL_HANDLE)
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

#   ifdef _WIN32
        win32 = static_cast<win32_info*>(malloc(sizeof(win32_info)));
        memset(win32, 0, sizeof(win32_info));
#   else
        xcb = static_cast<xcb_info*>(malloc(sizeof(xcb_info)));
        memset(xcb, 0, sizeof(xcb_info));
#   endif
}

Window::~Window() {
    // Semaphores, depth view, depth image, color image views, and swapchain are destroyed (in that order) with the device destructor
    // Color images are released automatically when the swapchain is destroyed

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

    // Get preferred color format
    if ( (m_surface_formats.size() == 1) && (m_surface_formats[0].format == VK_FORMAT_UNDEFINED) ) {
        // There is no preferred format, so assume VK_FORMAT_B8G8R8A8_UNORM
        m_color_format = VK_FORMAT_B8G8R8A8_UNORM;
        m_color_space = m_surface_formats[0].colorSpace;
    }
    else {
        // Iterate over the list of available surface formats and look for VK_FORMAT_B8G8R8A8_UNORM
        bool found_B8G8R8A8_UNORM = false;
        for (const auto& format : m_surface_formats) {
            if (format.format == VK_FORMAT_B8G8R8A8_UNORM) {
                m_color_format = format.format;
                m_color_space = format.colorSpace;
                found_B8G8R8A8_UNORM = true;
                break;
            }
        }
        // If it is not available, select the first available color format
        if (!found_B8G8R8A8_UNORM) {
            m_color_format = m_surface_formats[0].format;
            m_color_space = m_surface_formats[0].colorSpace;
        }
    }

    return true;
}

bool Window::init_swapchain(Backend::Device* device) {
    if (!device) {
        Backend::error("init_swapchain called with null device handle!");
        return false;
    }

    m_device = device;
    // If we don't make the new swapchain a derivative of the old one, all the resources have to be reloaded
    // (which is obviously too much for changing vsync)
    VkSwapchainKHR old_swapchain = m_swapchain;

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
    if (!(m_flags & vsync)) {
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
    // Make sure the requested image count is within the min and max for this surface
    uint32_t desired_n_images = m_n_swapchain_images;
    if (desired_n_images < m_surface_caps.minImageCount) {
        desired_n_images = m_surface_caps.minImageCount;
    }
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
    VkCompositeAlphaFlagBitsKHR composite_alpha = VK_COMPOSITE_ALPHA_FLAG_BITS_MAX_ENUM_KHR;
    const std::array<VkCompositeAlphaFlagBitsKHR, 4> preferred_alphas = {
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
    if (composite_alpha == VK_COMPOSITE_ALPHA_FLAG_BITS_MAX_ENUM_KHR) {
        Backend::error("Could not find a supported composite alpha for the presentation surface!");
        return false;
    }

    // Set an additional usage flag for blitting with the swapchain images if supported
    VkImageUsageFlags image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    VkFormatProperties format_props;
    vkGetPhysicalDeviceFormatProperties(m_instance.get_physical_device(m_physical_device_index).device, m_color_format, &format_props);
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
        m_color_format,                                   // imageFormat
        m_color_space,                                    // imageColorSpace
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
    
    VkResult res = vkCreateSwapchainKHR(device->vk(), &swapchain_info, nullptr, &m_swapchain);
    if (!validate(res)) return false;

    // If this was a re-creation of an existing swapchain, destroy the old one
    // (also cleans up all the presentable images and semaphores)
    if (old_swapchain != VK_NULL_HANDLE) {
        for (auto& semaphore : m_image_available_semaphores) {
            vkDestroySemaphore(device->vk(), semaphore, nullptr);
        }
        vkDestroyImageView(device->vk(), m_depth_view, nullptr);
        for (auto& view : m_image_views) {
            vkDestroyImageView(device->vk(), view, nullptr);
        }
        vkDestroySwapchainKHR(device->vk(), old_swapchain, nullptr);
    }

    // Get the swapchain images
    res = vkGetSwapchainImagesKHR(device->vk(), m_swapchain, &m_n_swapchain_images, nullptr);
    if (!validate(res)) return false;
    m_images.resize(m_n_swapchain_images);
    m_image_views.resize(m_n_swapchain_images);
    m_image_available_semaphores.resize(m_n_swapchain_images);
    res = vkGetSwapchainImagesKHR(device->vk(), m_swapchain, &m_n_swapchain_images, m_images.data());
    if (!validate(res)) return false;


    // Create image views
    VkImageSubresourceRange subresource_range = {
        VK_IMAGE_ASPECT_COLOR_BIT,  // aspectMask
        0,                          // baseMipLevel
        1,                          // levelCount
        0,                          // baseArrayLayer
        1                           // layerCount
    };
    VkImageViewCreateInfo color_view_info = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,   // sType
        nullptr,                                    // pNext
        0,                                          // flags
        VK_NULL_HANDLE,                             // image
        VK_IMAGE_VIEW_TYPE_2D,                      // viewType
        m_color_format,                               // format
        {   VK_COMPONENT_SWIZZLE_R,
            VK_COMPONENT_SWIZZLE_G,
            VK_COMPONENT_SWIZZLE_B,
            VK_COMPONENT_SWIZZLE_A
        },                                          // components
        {   VK_IMAGE_ASPECT_COLOR_BIT,              // aspectMask
            0,                                      // baseMipLevel
            1,                                      // levelCount
            0,                                      // baseArrayLayer
            1                                       // layerCount
        }                                           // subresourceRange
    };
    for (uint32_t i = 0; i < m_n_swapchain_images; ++i) {
        color_view_info.image = m_images[i];
        res = vkCreateImageView(device->vk(), &color_view_info, nullptr, &m_image_views[i]);
        if (!validate(res)) return false;
    }

    //
    // Create a depth/stencil buffer
    //

    // Find the best depth format
    // TODO: change function signature to be able to require stencil
    std::array<VkFormat, 5> depth_formats = {
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM
    };

    VkImageTiling depth_tiling;
    VkFormatProperties format_properties;
    // Ideally, use a format which supports a depth/stencil attachment with optimal tiling
    for (const auto& format : depth_formats) {
        vkGetPhysicalDeviceFormatProperties(m_device->get_physical_device().device, format, &format_properties);
        if (format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            m_depth_format = format;
            depth_tiling = VK_IMAGE_TILING_OPTIMAL;
            break;
        }
    }
    // Resort to linear tiling if it's the only one that supports a depth/stencil attachment
    if (m_depth_format == VK_FORMAT_UNDEFINED) {
        for (const auto& format : depth_formats) {
            vkGetPhysicalDeviceFormatProperties(m_device->get_physical_device().device, format, &format_properties);
            if (format_properties.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                m_depth_format = format;
                depth_tiling = VK_IMAGE_TILING_OPTIMAL;
                break;
            }
        }
    }
    if (m_depth_format == VK_FORMAT_UNDEFINED) {
        Backend::error("No tiling formats support a depth/stencil attachment!");
        return false;
    }

    // Create the image
    VkImageCreateInfo depth_info = {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,            // sType
        nullptr,                                        // pNext
        0,                                              // flags
        VK_IMAGE_TYPE_2D,                               // imageType
        m_depth_format,                                   // format
        {   extent.width,                               // extent
            extent.height,
            1
        },
        1,                                              // mipLevels
        1,                                              // arrayLayers
        VK_SAMPLE_COUNT_1_BIT,                          // samples
        depth_tiling,                                   // tiling
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,    // usage
        VK_SHARING_MODE_EXCLUSIVE,                      // sharingMode
        0,                                              // queueFamilyIndexCount
        0,                                              // pQueueFamilyIndices
        VK_IMAGE_LAYOUT_UNDEFINED                       // initialLayout
    };
    VmaMemoryRequirements depth_reqs = {
        VK_TRUE,                    // ownMemory
        VMA_MEMORY_USAGE_GPU_ONLY   // usage
        // Fill rest with 0s
    };
    res = vmaCreateImage(m_device->get_allocator(), &depth_info, &depth_reqs, &m_depth, nullptr, nullptr);
    if (!validate(res)) return false;

    // Add a stencil attachment if the depth image supports it
    VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;

    const std::array<VkFormat, 4> stencil_formats = {
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_S8_UINT
    };
    if (std::find(stencil_formats.begin(), stencil_formats.end(), m_depth_format) != stencil_formats.end())
        aspect_mask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    

    VkImageViewCreateInfo depth_view_info = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,   // sType
        nullptr,                                    // pNext
        0,                                          // flags
        m_depth,                                    // image
        VK_IMAGE_VIEW_TYPE_2D,                      // viewType
        m_depth_format,                               // format
        {   VK_COMPONENT_SWIZZLE_R,
            VK_COMPONENT_SWIZZLE_G,
            VK_COMPONENT_SWIZZLE_B,
            VK_COMPONENT_SWIZZLE_A
        },                                          // components
        {   aspect_mask,                            // aspectMask
            0,                                      // baseMipLevel
            1,                                      // levelCount
            0,                                      // baseArrayLayer
            1,                                      // layerCount
        }                                           // subresourceRange
    };

    res = vkCreateImageView(m_device->vk(), &depth_view_info, nullptr, &m_depth_view);
    if (!validate(res)) return false;


    // Create semaphores to signal when presentation is complete
    VkSemaphoreCreateInfo semaphore_info = {
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,    // sType
        nullptr,                                    // pNext
        0                                           // flags
    };
    for (uint32_t i = 0; i < m_n_swapchain_images; ++i) {
        res = vkCreateSemaphore(device->vk(), &semaphore_info, nullptr, &m_image_available_semaphores[i]);
        if (!validate(res)) return false;
    }

    return true;
}

bool Window::init_framebuffers(const Backend::RenderPass& renderpass, const std::vector<VkImageView>& attachments_in) {
    std::vector<VkImageView> attachments(attachments_in.size() + 2);
    attachments[1] = m_depth_view;
    std::vector<VkImageView>::iterator copy_start = attachments.begin();
    std::advance(copy_start, 2);
    std::copy(attachments_in.begin(), attachments_in.end(), copy_start);

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderpass.vk();
    framebufferInfo.attachmentCount = 2;
    framebufferInfo.width = m_width;
    framebufferInfo.height = m_height;
    framebufferInfo.layers = 1;

    m_framebuffers.resize(m_n_swapchain_images);

    for (size_t i = 0; i < m_n_swapchain_images; i++) {
        attachments[0] = m_image_views[i];
        framebufferInfo.pAttachments = attachments.data();

        if (!validate(vkCreateFramebuffer(m_device->vk(), &framebufferInfo, nullptr, &m_framebuffers[i])))
            return false;
    }
    return true;
}

bool Window::init() {
#   ifdef _WIN32
        return init_win32() && init_surface();
#   else
        return init_xcb() && init_surface();
#   endif
}

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

bool Window::acquire_next_frame(uint64_t timeout, VkFence fence) {
    VkResult res = vkAcquireNextImageKHR(m_device->vk(), m_swapchain, timeout, m_image_available_semaphores[m_frame_index], fence, &m_frame_index);
    if ((res == VK_SUBOPTIMAL_KHR) || (res == VK_ERROR_OUT_OF_DATE_KHR))
        m_flags |= surface_changed;
    return validate(res);
}

bool Window::present(const std::vector<VkSemaphore> wait_semaphores) {
    VkPresentInfoKHR present_info = {
        VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, // sType
        nullptr,                            // pNext
        static_cast<uint32_t>(wait_semaphores.size()), // waitSemaphoreCount
        wait_semaphores.data(),             // pWaitSemaphores
        1,                                  // swapchainCount
        &m_swapchain,                       // pSwapchains
        &m_frame_index,                     // pImageIndices
        nullptr                             // pResults
    };
    return validate( vkQueuePresentKHR(m_present_queue, &present_info) );
}

bool Window::set_fullscreen(bool full) {
#ifdef _WIN32
    if (full) {
        SetWindowLongPtr(win32->hwnd, GWL_STYLE, WS_SYSMENU | WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE);
        MoveWindow(win32->hwnd, 0, 0, m_width, m_height, TRUE);

        DEVMODE dm;
        dm.dmSize = sizeof(DEVMODE);
        dm.dmPelsWidth = m_width;
        dm.dmPelsHeight = m_height;
        dm.dmBitsPerPel = 32; // If this isn't supported, you shouldn't be running vulkan
        dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
        return (ChangeDisplaySettings(&dm, 0) == DISP_CHANGE_SUCCESSFUL);
    } else {
        RECT rect;
        rect.left = 0;
        rect.top = 0;
        rect.right = m_width;
        rect.bottom = m_height;
        SetWindowLongPtr(win32->hwnd, GWL_STYLE, WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE);
        AdjustWindowRect(&rect, WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE);
        MoveWindow(win32->hwnd, 0, 0, rect.right-rect.left, rect.bottom-rect.top, TRUE);
        return (ChangeDisplaySettings(0, 0) == DISP_CHANGE_SUCCESSFUL);
    }
#else
    xcb_client_message_event_t ev = {};
    ev.response_type = XCB_CLIENT_MESSAGE;
    ev.type = xcb->wm_state_atom;
    ev.format = 32;
    ev.window = xcb->window;
    memset(ev.data.data32, 0, 5 * sizeof(uint32_t));
    ev.data.data32[0] = _NET_WM_STATE_TOGGLE;
    if (full) {
        m_flags |= request_fullscreen;
        ev.data.data32[0] = _NET_WM_STATE_ADD;
    }
    else {
        m_flags &= (~request_fullscreen);
        ev.data.data32[0] = _NET_WM_STATE_REMOVE;
    }
    ev.data.data32[1] = xcb->fullscreen_atom;

    if (xcb->window && xcb->connection) {
        if ( full != (m_flags & current_fullscreen) ) { // If fullscreen requested, and currently windowed; or if windowed requested and currently fullscreen
            xcb_void_cookie_t err_cookie = xcb_send_event_checked (xcb->connection, 1, xcb->window, XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY, reinterpret_cast<const char*>(&ev));
            xcb_generic_error_t* err = xcb_request_check(xcb->connection, err_cookie);
            if (err != NULL)  {
                std::string err_string = "Could not toggle fullscreen! X11 error code " + err->error_code;
                Backend::error(err_string);
                free(err);
                return false;
            }
            free(err);
            if (xcb_flush(xcb->connection) <= 0) {
                Backend::error("Error while flushing xcb stream!");
                return false;
            }

            if (full) m_flags |= current_fullscreen;
            else m_flags &= (~current_fullscreen);
        }
    }

    return true;
#endif
}

bool Window::rebuild(const Backend::RenderPass& renderpass, const std::vector<VkImageView>& attachments) {
    if (m_device) {
        m_width = m_desired_width;
        m_height = m_desired_height;
        m_flags &= (~surface_changed);

        vkDeviceWaitIdle(m_device->vk());

        for (auto iter = m_framebuffers.rbegin(); iter != m_framebuffers.rend(); ++iter) {
            if (*iter)
                vkDestroyFramebuffer(m_device->vk(), *iter, nullptr);
        }

        return init_swapchain(m_device) && init_framebuffers(renderpass, attachments);
    }
    else return false;
}