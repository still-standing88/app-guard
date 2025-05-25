#include "utils.h"
#include "AppInstance.h"


#include <string>
#include <cwchar>


#ifdef _WIN32
void WinAppInstance::init(const char* app_handle) {
    mutex_handle_ = CreateMutexA(nullptr, TRUE, app_handle);
    this->is_first_ = (GetLastError() != ERROR_ALREADY_EXISTS);
}

void WinAppInstance::release() {
    if (mutex_handle_) {
        CloseHandle(mutex_handle_);
        mutex_handle_ = nullptr;
        }
    }

void WinAppInstance::FocusWindow(const wchar_t* windowname) {
    HWND hwnd = FindWindowW(nullptr, windowname);
    if (hwnd) {
        SetForegroundWindow(hwnd);
}
    }

int WinAppInstance::get_process_id() {
    return GetCurrentProcessId();
}

#elif defined(__linux__) || defined(__linux)
#include "linux_focus_util.h"

void LinuxAppInstance::init(const char* app_handle) {
    this->app_handle = app_handle;
    lock_fd_ = open(("/tmp/" + std::string(app_handle) + ".lock").c_str(), O_CREAT | O_RDWR, 0666);
    if (lock_fd_ == -1) return;

    if (::flock(lock_fd_, LOCK_EX | LOCK_NB) == -1) {
        is_first_ = false;
    }
    else {
        is_first_ = true;
    }
    }

void LinuxAppInstance::release() {
    if (lock_fd_ != -1) {
        close(lock_fd_);
        if (is_first_) {
            unlink(("/tmp/" + std::string(app_handle) + ".lock").c_str());
        }
        lock_fd_ = -1;
    }
}

void LinuxAppInstance::FocusWindow(const wchar_t* windowname_wchar) {
    if (!windowname_wchar || std::wcslen(windowname_wchar) == 0) {
        return;
    }

    std::string window_name_utf8;
    try {
        window_name_utf8 = LinuxFocusImpl::linux_wchar_to_utf8_string_for_focus(windowname_wchar);
    }
    catch (const std::exception&) {
        return;
    }

    if (window_name_utf8.empty()) {
        return;
    }

    bool focused = false;
    LinuxFocusImpl::DisplayServerType display_server = LinuxFocusImpl::detect_display_server();

    switch (display_server) {
    case LinuxFocusImpl::DS_X11:
#if APP_HAS_X11_SUPPORT_LNX_UTIL
        focused = LinuxFocusImpl::focus_window_x11(window_name_utf8);
#endif
        if (!focused) {
            LinuxFocusImpl::focus_window_generic_tools(window_name_utf8);
        }
        break;

    case LinuxFocusImpl::DS_WAYLAND:
        focused = LinuxFocusImpl::focus_window_wayland(window_name_utf8);
        if (!focused) {
            LinuxFocusImpl::focus_window_generic_tools(window_name_utf8);
        }
        break;

    default:
        LinuxFocusImpl::focus_window_generic_tools(window_name_utf8);
        break;
    }
}

int LinuxAppInstance::get_process_id() {
    return getpid();
}
#elif defined(__APPLE__) || defined(__DARWIN__)

void MacAppInstance::init(const char* app_handle) {
    service_name_ = "com.example." + std::string(app_handle);

    mach_port_t existing_port;
    kern_return_t kr = bootstrap_look_up(bootstrap_port, service_name_.c_str(), &existing_port);

    if (kr == KERN_SUCCESS) {
        // Another instance exists
        is_first_ = false;
        mach_port_deallocate(mach_task_self(), existing_port);
    }
    else {
        // First instance
        is_first_ = true;
        kr = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &receive_port_);
        if (kr != KERN_SUCCESS) {
            throw std::runtime_error("Failed to create Mach port.");
        }

        kr = bootstrap_register(bootstrap_port, const_cast<char*>(service_name_.c_str()), receive_port_);
        if (kr != KERN_SUCCESS) {
            mach_port_deallocate(mach_task_self(), receive_port_);
            throw std::runtime_error("Failed to register Mach port.");
        }
    }
}

void MacAppInstance::release() {
    if (is_first_ && receive_port_ != MACH_PORT_NULL) {
        bootstrap_register(bootstrap_port, const_cast<char*>(service_name_.c_str()), MACH_PORT_NULL);
        mach_port_deallocate(mach_task_self(), receive_port_);
        receive_port_ = MACH_PORT_NULL;
    }
}

void MacAppInstance::FocusWindow(const wchar_t* windowname) {
    if (!windowname || std::wcslen(windowname) == 0) {
        return;
    }

    std::string target_window_name_utf8;
    try {
        target_window_name_utf8 = public_platform_wchar_to_utf8_string(windowname);
    }
    catch (const std::exception&) {
        return;
    }

    if (target_window_name_utf8.empty() && std::wcslen(windowname) > 0) {
        return;
    }

    if (target_window_name_utf8.empty()) {
        return;
    }

    // Escape special characters for AppleScript
    std::string escapedName = "";
    for (char c : target_window_name_utf8) {
        if (c == '"' || c == '\\') {
            escapedName += '\\';
        }
        escapedName += c;
    }

    std::string script = "osascript -e 'tell application \"System Events\" to tell (first process whose name contains \""
        + escapedName + "\") to set frontmost to true' > /dev/null 2>&1";
    system(script.c_str());
}

int MacAppInstance::get_process_id() {
    return getpid();
}
#endif // _WIN32