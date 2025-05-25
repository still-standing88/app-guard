#include <pybind11/pybind11.h>
#include <pybind11/functional.h> 
#include <pybind11/stl.h>       

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <cstring> 
#include <iostream> 

#if defined(__linux__) || defined(__APPLE__) || defined(__DARWIN__) || defined(__MACH__)
#include <locale>
#include <codecvt>
#endif

#include "AppGuard.h" 

namespace py = pybind11;

static py::function g_on_quit_callback_py;
static std::map<std::string, py::function> g_ipc_msg_callbacks_py;
static std::map<int, std::string> g_msg_id_to_handle_map;
static std::map<int, py::object> g_active_ipc_msg_objects;
static std::mutex g_callback_mutex;

#pragma pack(push, 1)
#pragma pack(pop)

void app_on_quit_trampoline_c() {
    std::lock_guard<std::mutex> lock(g_callback_mutex);
    if (g_on_quit_callback_py && !g_on_quit_callback_py.is_none()) {
        try {
            py::gil_scoped_acquire acquire_gil; 
            g_on_quit_callback_py();
        } catch (const py::error_already_set &e) {
            py::print("[AppGuard Python] Error in on_quit_callback:");
            py::print(e.what());
        }
    }
}

void ipc_msg_trampoline_c(const IPCMsgData* msg_data_c) {
    if (!msg_data_c || !msg_data_c->msg_handle) {
        return; 
    }

    std::string msg_handle_str(msg_data_c->msg_handle);
    py::function python_callback;

    {
        std::lock_guard<std::mutex> lock(g_callback_mutex);
        auto it = g_ipc_msg_callbacks_py.find(msg_handle_str);
        if (it != g_ipc_msg_callbacks_py.end()) {
            python_callback = it->second;
        }
    }

    if (python_callback && !python_callback.is_none()) {
        try {
            py::gil_scoped_acquire acquire_gil; 

            py::dict py_msg_data_dict;
            py_msg_data_dict["msg_handle"] = py::str(msg_data_c->msg_handle); 
            
            if (msg_data_c->msg_data) {
#if defined(__linux__) || defined(__APPLE__) || defined(__DARWIN__) || defined(__MACH__)
                std::string utf8_data = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(
                    reinterpret_cast<const wchar_t*>(msg_data_c->msg_data)
                );
                py_msg_data_dict["msg_data"] = py::str(utf8_data);
#else
                std::wstring wstr_data(msg_data_c->msg_data);
                py_msg_data_dict["msg_data"] = py::cast(wstr_data);
#endif
            } else {
                py_msg_data_dict["msg_data"] = py::none();
            }
            
            python_callback(py_msg_data_dict);

        } catch (const py::error_already_set &e) {
            py::print("[AppGuard Python] Error in IPC callback for handle '", msg_handle_str, "':");
            py::print(e.what());
        }
    }
}

class PyIPCMsg {
public:
    IPCMsg c_msg_struct;          
    std::string msg_handle_copy;  

    PyIPCMsg() {
        std::memset(&c_msg_struct, 0, sizeof(IPCMsg));
    }

    void setup_internal(const std::string& handle_str, py::function callback_fn_py) {
        this->msg_handle_copy = handle_str;

        c_msg_struct.msg_handle = this->msg_handle_copy.c_str();
        c_msg_struct.callback = ipc_msg_trampoline_c; 

        {
            std::lock_guard<std::mutex> lock(g_callback_mutex);
            g_ipc_msg_callbacks_py[this->msg_handle_copy] = callback_fn_py;
        }

        AG_create_IPCMsg(&c_msg_struct, this->msg_handle_copy.c_str(), ipc_msg_trampoline_c);

        if (c_msg_struct.msg_id != 0) { 
             std::lock_guard<std::mutex> lock(g_callback_mutex);
             g_msg_id_to_handle_map[c_msg_struct.msg_id] = this->msg_handle_copy;
        }
    }

    int get_msg_id() const { return c_msg_struct.msg_id; }
    const char* get_msg_handle_c_str() const { return c_msg_struct.msg_handle; } 
};


PYBIND11_MODULE(AppGuard, m) { 
    m.doc() = "Python bindings for the AppGuard C library";

    py::class_<PyIPCMsg>(m, "IPCMsg")
        .def(py::init<>()) 
        .def_property_readonly("msg_id", &PyIPCMsg::get_msg_id)
        .def_property_readonly("msg_handle", [](const PyIPCMsg& self) -> py::object { 
            if (self.get_msg_handle_c_str()) {
                return py::str(self.get_msg_handle_c_str()); 
            }
            return py::none(); 
        });

    m.def("AG_init", [](const std::string& app_handle, py::function on_quit_cb_py, bool quit_immediate) {
        AppOnQuitCallback c_on_quit_trampoline = nullptr;
        if (on_quit_cb_py && !on_quit_cb_py.is_none()) {
            std::lock_guard<std::mutex> lock(g_callback_mutex);
            g_on_quit_callback_py = on_quit_cb_py; 
            c_on_quit_trampoline = app_on_quit_trampoline_c;
        } else {
            std::lock_guard<std::mutex> lock(g_callback_mutex);
            g_on_quit_callback_py = py::function(); 
        }
        AG_init(app_handle.c_str(), c_on_quit_trampoline, quit_immediate);
    }, py::arg("app_handle"), py::arg("on_quit_callback").none(true), py::arg("quit_immediate"));

    m.def("AG_release", []() {
        AG_release();
        std::lock_guard<std::mutex> lock(g_callback_mutex);
        g_on_quit_callback_py = py::function(); 
        g_ipc_msg_callbacks_py.clear();
        g_msg_id_to_handle_map.clear();
        g_active_ipc_msg_objects.clear(); 
    });

    m.def("AG_is_loaded", &AG_is_loaded);
    m.def("AG_is_primary_instance", &AG_is_primary_instance);

    m.def("AG_create_IPCMsg", [](PyIPCMsg &msg_obj_py, const std::string& msg_handle, py::function callback_py) {
        if (!callback_py || callback_py.is_none()) {
            throw py::type_error("AG_create_IPCMsg: callback cannot be None.");
        }
        msg_obj_py.setup_internal(msg_handle, callback_py);
    }, py::arg("msg_obj"), py::arg("msg_handle"), py::arg("callback"));

    m.def("AG_register_msg", [](py::object msg_obj_py_generic) { 
        if (!py::isinstance<PyIPCMsg>(msg_obj_py_generic)) {
            throw py::type_error("AG_register_msg: msg_obj must be an instance of AppGuard.IPCMsg.");
        }
        PyIPCMsg& msg_obj_py = msg_obj_py_generic.cast<PyIPCMsg&>(); 
        
        AG_register_msg(&(msg_obj_py.c_msg_struct));

        if (msg_obj_py.c_msg_struct.msg_id != 0) {
            std::lock_guard<std::mutex> lock(g_callback_mutex);
            g_active_ipc_msg_objects[msg_obj_py.c_msg_struct.msg_id] = msg_obj_py_generic;
        }
    }, py::arg("msg_obj"));

    m.def("AG_unregister_msg", [](int msg_id) {
        std::string handle_to_remove;
        bool msg_was_active = false;
        {
            std::lock_guard<std::mutex> lock(g_callback_mutex);
            auto it_handle = g_msg_id_to_handle_map.find(msg_id);
            if (it_handle != g_msg_id_to_handle_map.end()) {
                handle_to_remove = it_handle->second;
                g_msg_id_to_handle_map.erase(it_handle);
            }
            if (g_active_ipc_msg_objects.count(msg_id)) {
                msg_was_active = true;
            }
        }

        AG_unregister_msg(msg_id); 

        {
            std::lock_guard<std::mutex> lock(g_callback_mutex);
            if (!handle_to_remove.empty()) {
                g_ipc_msg_callbacks_py.erase(handle_to_remove);
            }
            if (msg_was_active) {
                 g_active_ipc_msg_objects.erase(msg_id); 
            }
        }
    }, py::arg("msg_id"));

    m.def("AG_send_msg_request", [](const std::string& msg_handle, const py::object& msg_data_py) {
        IPCMsgData c_msg_data_to_send;
        std::memset(&c_msg_data_to_send, 0, sizeof(IPCMsgData)); 
        std::wstring msg_data_wstr_holder; 

        c_msg_data_to_send.msg_handle = msg_handle.c_str();

        if (msg_data_py.is_none()) {
            c_msg_data_to_send.msg_data = nullptr;
        } else {
            if (!py::isinstance<py::str>(msg_data_py)) { 
                 throw py::type_error("AG_send_msg_request: msg_data must be a Python string or None.");
            }
#if defined(__linux__) || defined(__APPLE__) || defined(__DARWIN__) || defined(__MACH__)
            std::string utf8_str = msg_data_py.cast<std::string>();
            msg_data_wstr_holder = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(utf8_str);
#else
            msg_data_wstr_holder = msg_data_py.cast<std::wstring>();
#endif
            c_msg_data_to_send.msg_data = msg_data_wstr_holder.c_str();
        }
        
        AG_send_msg_request(&c_msg_data_to_send);
    }, py::arg("msg_handle"), py::arg("msg_data").none(true));

    m.def("AG_get_process_id", &AG_get_process_id);

    m.def("AG_focus_window", [](const py::str& window_name_py_str) {
#if defined(__linux__) || defined(__APPLE__) || defined(__DARWIN__) || defined(__MACH__)
        std::string utf8_str = window_name_py_str.cast<std::string>();
        std::wstring window_name_wstr = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(utf8_str);
#else
        std::wstring window_name_wstr = window_name_py_str.cast<std::wstring>();
#endif
        AG_focus_window(window_name_wstr.c_str());
    }, py::arg("window_name"));

#ifdef VERSION_INFO
    m.attr("__version__") = PYBIND11_TOSTRING(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif
}
