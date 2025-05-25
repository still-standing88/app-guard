#define __APPGUARD_EXPORT

#include "AppInstance.h"
#include "IPCWatcher.h"
#include "PlatformIPCWatcher.h"
#include "utils.h"
#include "../include/AppGuard.h"

IPCWatcher* ipc_watcher = nullptr;
AppInstance* app_instance = nullptr;
extern bool is_initialized = false;


extern "C" APPGUARD_API void AG_init(const char* app_handle, AppOnQuitCallback on_quit_callback, bool quit_immediate) {
	if (!is_initialized || ipc_watcher == nullptr || app_instance == nullptr) {
		if (ipc_watcher == nullptr) {
#ifdef _WIN32
			ipc_watcher = new WindowsIPCWatcher(app_handle);
#elif defined(__linux__) || defined(__unix__)
			ipc_watcher = new UnixIPCWatcher(app_handle);
#elif defined(__APPLE__) || defined(__DARWIN__)
			ipc_watcher = new UnixIPCWatcher(app_handle);
#endif // _WIN32
				ipc_watcher->start();
		}

		if (app_instance == nullptr) {
#ifdef _WIN32
			app_instance = new WinAppInstance();
#elif defined(__linux__) || defined(__unix__)
			app_instance = new LinuxAppInstance();
#elif defined(__APPLE__) || defined(__DARWIN__)
			app_instance = new MacAppInstance();
#endif // _WIN32
			if (app_instance != nullptr) {
				app_instance->init(app_handle);
			}
		}
		if (!AG_is_primary_instance()) {
			ipc_watcher->stop();
			if (quit_immediate) {
				if (on_quit_callback != nullptr) {
					on_quit_callback();
				}
				AG_release();
				exit(0);
			}
		}
		is_initialized = true;
	}
}

extern "C" APPGUARD_API void AG_release() {
	if (app_instance != nullptr) {
		app_instance->release();
		delete app_instance;
		app_instance = nullptr;
	}
	if (ipc_watcher != nullptr) {
		ipc_watcher->stop();
		delete ipc_watcher;
		ipc_watcher = nullptr;
	}
	is_initialized = false;
}

extern "C" APPGUARD_API bool AG_is_loaded() {
return (is_initialized && ipc_watcher != nullptr && app_instance != nullptr) ? true: false;
}

extern "C" APPGUARD_API bool AG_is_primary_instance() {
	if (app_instance != nullptr) {
		return app_instance->IsFirstInstance();
	}
	return false;
}

extern "C" APPGUARD_API void AG_create_IPCMsg(IPCMsg* msg, const char* msg_handle, IPCMsgCallback callback) {
	if (msg == nullptr ) { return; }
	if (msg_handle == nullptr) { return; }
	if (callback == nullptr ) { return; }
	msg->msg_id = random_number(1000, 9999);
	msg->msg_handle = msg_handle;
	msg->callback = callback;
}

extern "C" APPGUARD_API void AG_register_msg(IPCMsg* msg) {
	if (ipc_watcher != nullptr && msg != nullptr ) {
		ipc_watcher->RegisterIPCMsg(*msg);
	}
}

extern "C" APPGUARD_API void AG_unregister_msg(int msg_id) {
	if (ipc_watcher != nullptr) {
		ipc_watcher->UnregisterIPCMsg(msg_id);
	}
}

extern "C" APPGUARD_API void AG_send_msg_request(IPCMsgData* msg_request) {
	if (!AG_is_primary_instance() && ipc_watcher != nullptr && msg_request != NULL) {
		ipc_watcher->SendMsg(*msg_request);
	}
}

extern "C" APPGUARD_API int AG_get_process_id() {
	if (app_instance != nullptr) {
		return app_instance->get_process_id();
	}
	return -1;
}

extern "C" APPGUARD_API void AG_focus_window(const wchar_t* window_name) {
	if (app_instance != nullptr && window_name != nullptr) {
		app_instance->FocusWindow(window_name);
	}
}
