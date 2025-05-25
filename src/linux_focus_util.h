#pragma once

#ifndef LINUX_FOCUS_UTIL_H
#define LINUX_FOCUS_UTIL_H

#include <string>
#include <vector>
#include <memory>
#include <array>
#include <sstream>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <locale>
#include <codecvt>


#ifndef APP_HAS_X11_SUPPORT_LNX_UTIL 
    #if __has_include(<X11/Xlib.h>) && __has_include(<X11/Xatom.h>) && __has_include(<X11/Xutil.h>)
        #define APP_HAS_X11_SUPPORT_LNX_UTIL 1
        #warning "linux_focus_util.h: APP_HAS_X11_SUPPORT_LNX_UTIL was not pre-defined; detected X11 headers."
    #else
        #define APP_HAS_X11_SUPPORT_LNX_UTIL 0
        #warning "linux_focus_util.h: APP_HAS_X11_SUPPORT_LNX_UTIL was not pre-defined; X11 headers NOT detected."
    #endif
#endif 

#if defined(APP_HAS_X11_SUPPORT_LNX_UTIL) && APP_HAS_X11_SUPPORT_LNX_UTIL == 1
    #include <X11/Xlib.h>
    #include <X11/Xatom.h>
    #include <X11/Xutil.h>
#endif


#if defined(__GNUC__) || defined(__clang__)
#define LFU_DISABLE_DEPRECATED_WARNING \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
#define LFU_ENABLE_DEPRECATED_WARNING \
    _Pragma("GCC diagnostic pop")
#else
#define LFU_DISABLE_DEPRECATED_WARNING
#define LFU_ENABLE_DEPRECATED_WARNING
#endif


namespace LinuxFocusImpl {

    enum DisplayServerType {
        DS_X11 = 1,
        DS_WAYLAND = 2,
        DS_UNKNOWN = 0
    };

    inline std::string linux_wchar_to_utf8_string_for_focus(const wchar_t* wstr) {
        if (!wstr) return std::string();

        LFU_DISABLE_DEPRECATED_WARNING
            std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
        LFU_ENABLE_DEPRECATED_WARNING
            try {
            return converter.to_bytes(wstr);
        }
        catch (const std::range_error&) {
            return std::string();
        }
    }


    inline std::string exec_command(const std::string& cmd) {
        std::array<char, 128> buffer;
        std::string result;
        auto file_closer = [](FILE* fp) { if (fp) pclose(fp); };
        std::unique_ptr<FILE, decltype(file_closer)> pipe(popen(cmd.c_str(), "r"), file_closer);

        if (!pipe) {
            return "";
        }
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        return result;
    }


    inline DisplayServerType detect_display_server() {
        if (std::getenv("WAYLAND_DISPLAY") != nullptr) {
            return DS_WAYLAND;
        }

        if (std::getenv("DISPLAY") != nullptr) {
            return DS_X11;
        }
        return DS_UNKNOWN;
    }


#if defined(APP_HAS_X11_SUPPORT_LNX_UTIL) && APP_HAS_X11_SUPPORT_LNX_UTIL == 1
    inline bool focus_window_x11(const std::string& window_name_utf8) {
        Display* display = XOpenDisplay(nullptr);

        if (!display) {
            return false;
        }


        Window root = DefaultRootWindow(display);
        Atom net_client_list = XInternAtom(display, "_NET_CLIENT_LIST", False);
        Atom net_wm_name = XInternAtom(display, "_NET_WM_NAME", False);
        Atom utf8_string_atom = XInternAtom(display, "UTF8_STRING", False);
        Atom active_window_atom = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);

        Atom actual_type;
        int actual_format;
        unsigned long num_items, bytes_after;
        unsigned char* prop_data = nullptr;


        int status = XGetWindowProperty(display, root, net_client_list, 0, ~0L, False, XA_WINDOW,
            &actual_type, &actual_format, &num_items, &bytes_after, &prop_data);

        Window target_window = 0;
        if (status == Success && prop_data) {

            Window* windows = reinterpret_cast<Window*>(prop_data);
            for (unsigned long i = 0; i < num_items; ++i) {
                unsigned char* name_data = nullptr;
                Atom name_actual_type;
                int name_actual_format;
                unsigned long name_num_items, name_bytes_after;


                status = XGetWindowProperty(display, windows[i], net_wm_name, 0, ~0L, False, utf8_string_atom,
                    &name_actual_type, &name_actual_format, &name_num_items, &name_bytes_after, &name_data);

                if (status != Success || !name_data) {

                    if (name_data) XFree(name_data);
                    name_data = nullptr;
                    status = XGetWindowProperty(display, windows[i], XA_WM_NAME, 0, ~0L, False, XA_STRING,
                        &name_actual_type, &name_actual_format, &name_num_items, &name_bytes_after, &name_data);
                }

                if (status == Success && name_data) {
                    std::string current_name;
                    if (name_actual_type == utf8_string_atom || name_actual_type == XA_STRING) {

                         current_name = std::string(reinterpret_cast<char*>(name_data), name_num_items);
                    }
                    XFree(name_data);

                    if (!current_name.empty() && current_name == window_name_utf8) {
                        target_window = windows[i];
                        break; 
                    }
                }
            }
            XFree(prop_data);
        }


        bool success_focus = false;

        if (target_window != 0) {
            XEvent event;
            std::memset(&event, 0, sizeof(event));
            event.xclient.type = ClientMessage;
            event.xclient.window = target_window;
            event.xclient.message_type = active_window_atom;
            event.xclient.format = 32;
            event.xclient.data.l[0] = 1;
            event.xclient.data.l[1] = CurrentTime;
            event.xclient.data.l[2] = 0;
            event.xclient.data.l[3] = 0;
            event.xclient.data.l[4] = 0;

            XSendEvent(display, root, False, SubstructureRedirectMask | SubstructureNotifyMask, &event);
            XFlush(display);
            success_focus = true;
        }


        XCloseDisplay(display);
        return success_focus;
    }

#else
    inline bool focus_window_x11(const std::string& /*window_name_utf8*/) {
        return false; 
    }
#endif // APP_HAS_X11_SUPPORT_LNX_UTIL

    inline bool focus_window_wayland(const std::string& window_name_utf8) {
        std::string command;
        int sys_result = -1;

        std::string escaped_name = "'";
        for (char c : window_name_utf8) {
            if (c == '\'') {
                escaped_name += "'\\''"; 

            } else {
                escaped_name += c;
            }
        }

        escaped_name += "'";

        // sway
        command = "swaymsg '[title=" + escaped_name + "] focus' > /dev/null 2>&1";
        sys_result = system(command.c_str());
        if (sys_result != -1 && WIFEXITED(sys_result) && WEXITSTATUS(sys_result) == 0) return true;

        // Hyprland
        command = "hyprctl dispatch focuswindow \"title:" + escaped_name + "\" > /dev/null 2>&1";

        std::string hypr_escaped_name = window_name_utf8;
        std::string temp_hypr_escaped;
        for(char c : window_name_utf8) {

            if (c == '"' || c == '$' || c == '`' || c == '\\') temp_hypr_escaped += '\\';
            temp_hypr_escaped += c;
        }
        command = "hyprctl dispatch focuswindow \"title:" + temp_hypr_escaped + "\" > /dev/null 2>&1";

        sys_result = system(command.c_str());
        if (sys_result != -1 && WIFEXITED(sys_result) && WEXITSTATUS(sys_result) == 0) return true;
        
        return false;
    }

    inline bool focus_window_generic_tools(const std::string& window_name_utf8) {
        std::string command;
        int sys_result = -1;
        
        std::string escaped_name = "'"; // Start with single quote
        for (char c : window_name_utf8) {
            if (c == '\'') {
                escaped_name += "'\\''"; 
            } else {
                escaped_name += c;
            }
        }

        escaped_name += "'";

        // wmctrl
        command = "wmctrl -a " + escaped_name + " > /dev/null 2>&1";

        sys_result = system(command.c_str());
        if (sys_result != -1 && WIFEXITED(sys_result) && WEXITSTATUS(sys_result) == 0) return true;

        // xdotool 

        if (std::getenv("DISPLAY") != nullptr) {
            std::string xdotool_escaped_name = window_name_utf8;

            std::string temp_xdotool_escaped;
            for(char c : window_name_utf8) {
                if (strchr("^$.[]*+?()|\\", c)) temp_xdotool_escaped += '\\';
                temp_xdotool_escaped += c;
            }

            command = "xdotool search --onlyvisible --name \"" + temp_xdotool_escaped + "\" windowactivate --sync > /dev/null 2>&1";

            sys_result = system(command.c_str());

            if (sys_result != -1 && WIFEXITED(sys_result) && WEXITSTATUS(sys_result) == 0) return true;
        }
        return false;
    }

} // namespace LinuxFocusImpl

#endif // LINUX_FOCUS_UTIL_H