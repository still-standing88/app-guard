#include <iostream>
#include <thread>
#include "../include/AppGuard.h"
#include "../src/utils.h"

void on_quit() {
    int pid = AG_get_process_id();
    std::cout << "Instance with PID " << pid << " is closing...\n";
}

void handle_ipc_message(const IPCMsgData* msg_data) {
    if (msg_data) {
        std::wcout << L"Received IPC message: " << msg_data->msg_data << L" (from another instance)\n";
    }
}

int main() {
    const char* app_handle = "TestApp";
    bool quit_immediate = false;

    AG_init(app_handle, on_quit, quit_immediate);

    if (AG_is_primary_instance()) {
        std::cout << "Primary instance running. Waiting for other instances...\n";

        IPCMsg ipc_msg;
        AG_create_IPCMsg(&ipc_msg, "InstanceStarted", handle_ipc_message);
        AG_register_msg(&ipc_msg);

        while (AG_is_primary_instance()) {
            //AG_process_messages();  // Ensure messages are processed
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            //std::cout << "test";
        }

        AG_unregister_msg(ipc_msg.msg_id);
    }
    else {
       std::cout << "Secondary instance detected. Sending process ID to primary.\n";

        IPCMsgData msg;
        msg.msg_handle = "InstanceStarted";
        std::wstring pid_str = std::to_wstring(AG_get_process_id());
        msg.msg_data = pid_str.c_str();

        AG_send_msg_request(&msg);
        //while (true) {
            //std::this_thread::sleep_for(std::chrono::milliseconds(50));
        //}
    }

    AG_release();
    return 0;
}
