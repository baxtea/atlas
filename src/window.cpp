#include "window.h"
#include <assert.h>
#include <stdio.h>
#include <cstring>

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
        fprintf(stderr, "[!] Could not connect to X server\n");
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
        fprintf(stderr, "[!] Could not create window. X11 error %d\n", err->error_code);
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
        fprintf(stderr, "[!] Could not override window destroy handler. X11 error %d\n", err->error_code);
        return false;
    }

    xcb->delete_win_atom = delete_reply->atom;
    xcb->protocols_atom = protocols_reply->atom;



    // Only returns an error if the window doesn't exist -- and we just checked that it does
    xcb_map_window(xcb->connection, xcb->window);
    
    if (xcb_flush(xcb->connection) <= 0) {
        fprintf(stderr, "[!] Error while flushing xcb stream!\n");
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
            if (cm->data.data32[0] = xcb->delete_win_atom)
                m_should_close = true;

            break;

        case XCB_EXPOSE:
            expose = reinterpret_cast<xcb_expose_event_t*>(xcb->event);
            if (expose->window == xcb->window) {
                m_width = expose->width;
                m_height = expose->height;
            }
            break;

        default:
            // Unknown event type, ignore it
            break;
        }

        free (xcb->event);
    }
}

void Window::shutdown_xcb() {
    xcb_unmap_window(xcb->connection, xcb->window);
    xcb_destroy_window(xcb->connection, xcb->window);
    xcb_disconnect(xcb->connection);
}



#endif // !_WIN32
#ifdef _WIN32
#include <Windows.h>

struct Window::win32_info {
    HINSTANCE inst;
    HWND window;
    MSG msg;
};


LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CLOSE:
        LONG_PTR lpUserData = GetWindowLongPtr(hWnd, GWLP_USERDATA);
        Atlas::Window* window = reinterpret_cast<Atlas::Window*>(lpUserData);
        if (window)
            window->m_should_close = true;

        break;
    case WM_DESTROY:
        DestroyWindow(hWnd);
        PostQuitMessage(0);
        break;
    case WM_PAINT:
        ValidateRect(hWnd, NULL);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
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
    wc.hIcon            = LoadIcon(NULL, MAKEINTRESOURCE(IDI_APPLICATION));
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName     = m_name.c_str();
    wc.hIconSm          = LoadIcon(NULL, MAKEINTRESOURCE(IDI_APPLICATION));

    if (!RegisterClassEx(&wc)) {
        fprintf(stderr, "[!] Failed to register WNDCLASSEX!\n");
        return false;
    }


    win32->window = CreateWindowEx(
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
    
    if (!win32->window) {
        fprintf(stderr, "[!] Failed to create window!\n");
        return false;
    }

    SetWindowLongPtr(win32->window, GWLP_USERDATA, this);
    return true;
}

void Window::handle_win32_events() {
    while (PeekMessage(&win32->msg, NULL, 0, 0, PM_REMOVE)) {
        if (win32->msg == WM_QUIT)
            m_should_close = true;
        
        TranslateMessage(&win32->msg);
        DispatchMessage(&win32->msg);
    }
}

void Window::shutdown_win32() {
    DestroyWindow(win32->window);
    UnregisterClassEx(m_name.c_str(), win32->inst);
}

#endif // _WIN32


Window::Window(const std::string& name, uint32_t width, uint32_t height)
    : m_name(name), m_width(width), m_height(height), m_should_close(false) {

#   ifdef _WIN32
        win32 = static_cast<win32_info*>(malloc(sizeof(win32_info)));
        init_win32();
#   else
        xcb = static_cast<xcb_info*>(malloc(sizeof(xcb_info)));
        init_xcb();
#   endif

    init_swapchain();
    init_framebuffers();
}

Window::~Window() {
#   ifdef _WIN32
        shutdown_win32();
        free(win32);
#   else
        shutdown_xcb();
        free(xcb);
#   endif
}

/*
#include "misc/window.h"
void do_nothing(void*) {};
struct AnvilClone : public Anvil::Window {
    AnvilClone(const std::string& title, uint32_t w, uint32_t h)
    : Anvil::Window(title, w, h, &do_nothing, nullptr) {
#   ifdef _WIN32
#       if defined(ANVIL_INCLUDE_WIN3264_WINDOW_SYSTEM_SUPPORT)
            m_platform = Anvil::WINDOW_PLATFORM_SYSTEM;
#       else
            fprintf(stderr, "[!] Found no Anvil equivalent to Atlas Win32 platform!\n");
            m_platform = Anvil::WINDOW_PLATFORM_DUMMY;
#       endif

#   else // _WIN32
#       if defined(ANVIL_INCLUDE_XCB_WINDOW_SYSTEM_SUPPORT)
            m_platform = Anvil::WINDOW_PLATFORM_XCB;
#       else
            fprintf(stderr, "[!] Found no Anvil equivalent to Atlas xcb platform!\n");
            m_platform = Anvil::WINDOW_PLATFORM_DUMMY;
#       endif
#   endif
    }
    ~AnvilClone() {};

    void run() {};
    Anvil::WindowPlatform m_platform;
    Anvil::WindowPlatform get_platform() const {
        return m_platform;
    }
    WindowHandle set_handle(WindowHandle handle) {
        m_window = handle;
    }
};

void Window::init_swapchain() {
    AnvilClone* clone = new AnvilClone(m_name, m_width, m_height);
#ifdef _WIN32
    clone->set_handle(win32->window);
#else
    clone->set_handle(xcb->window);
#endif

    m_anvil_clone.reset(clone);

        std::shared_ptr<Anvil::SGPUDevice> device_locked_ptr(g_device);

    m_rendering_surface = Anvil::RenderingSurface::create(g_instance,
                                                              g_device,
                                                              m_anvil_clone);

    m_rendering_surface->set_name((m_name + " rendering surface").c_str());


    m_swapchain = device_locked_ptr->create_swapchain(m_rendering_surface,
                                                          m_anvil_clone,
                                                          VK_FORMAT_B8G8R8A8_UNORM,
                                                          VK_PRESENT_MODE_FIFO_KHR,
                                                          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                                          m_n_swapchain_images);

    m_swapchain->set_name((m_name + " swapchain").c_str());

    // Cache the queue we are going to use for presentation
    const std::vector<uint32_t>* present_queue_fams_ptr = nullptr;

    if (!m_rendering_surface->get_queue_families_with_present_support(device_locked_ptr->get_physical_device(),
                                                                         &present_queue_fams_ptr) )
    {
        anvil_assert_fail();
    }

    m_present_queue = device_locked_ptr->get_queue(present_queue_fams_ptr->at(0),
                                                       0); // in_n_queue 
}

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