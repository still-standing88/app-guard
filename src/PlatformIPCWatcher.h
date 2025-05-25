#pragma once

#include "IPCWatcher.h"
#include "utils.h"

#ifdef _WIN32
#include <windows.h>
#include <string>
#include <vector>
#include <memory>
#include <system_error>
#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__) || defined(__DARWIN__)
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string>
#include <vector>
#include <errno.h>
#include <stdexcept>
#include <cstring>
#include <unistd.h>
#include <cstdint>
#endif

#ifdef _WIN32

class WindowsIPCWatcher : public IPCWatcher {
private:
    HANDLE hPipe_ = INVALID_HANDLE_VALUE;
    std::wstring pipeName_;
    bool isPrimary_ = false;

    static const DWORD PIPE_BUFFER_SIZE = sizeof(wchar_t) * 4096;

public:
    WindowsIPCWatcher(const char* app_handle);
    ~WindowsIPCWatcher();
    void SendMsg(IPCMsgData& msg) override;

protected:
    void process_messages() override;
};

#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__) || defined(__DARWIN__)

struct IPCMessageBuffer {
    long msg_type;
    uint32_t data_size;
    char data[1];
};

class UnixIPCWatcher : public IPCWatcher {
private:
    int msg_queue_id_ = -1;
    key_t ipc_key_;
    bool isPrimary_ = false;

    key_t generate_ipc_key(const char* app_handle);
    static const size_t MAX_MSG_SIZE = 8192;
    static const long MSG_TYPE = 1;

public:
    UnixIPCWatcher(const char* app_handle);
    ~UnixIPCWatcher();
    void SendMsg(IPCMsgData& msg) override;

protected:
    void process_messages() override;
};

#endif