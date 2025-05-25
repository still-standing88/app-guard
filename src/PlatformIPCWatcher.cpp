#include "PlatformIPCWatcher.h"
#include "utils.h"

#include <iostream>
#include <vector>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <cstring>
#include <cwchar>
#include <cstdint>
#include <cerrno>
#include <algorithm>
#include <codecvt>
#include <locale>


#ifdef _WIN32
const DWORD MAX_IPC_MESSAGE_BYTES_WINDOWS = 10 * 1024 * 1024;

#include <windows.h>

WindowsIPCWatcher::WindowsIPCWatcher(const char* app_handle) : IPCWatcher(app_handle) {
    pipeName_ = L"\\\\.\\pipe\\" + string_to_wstring(std::string(app_handle_));
    // this->start();
}

WindowsIPCWatcher::~WindowsIPCWatcher() {
    stop();
    if (hPipe_ != INVALID_HANDLE_VALUE) {
        CloseHandle(hPipe_);
        hPipe_ = INVALID_HANDLE_VALUE;
    }
}

void WindowsIPCWatcher::SendMsg(IPCMsgData& msg) {
    this->processing = true;
    HANDLE hClientPipe = INVALID_HANDLE_VALUE;
    SerializedIPCBuffer ipc_buffer;
    BOOL success = FALSE;
    DWORD bytesWritten = 0;

    std::wstring targetPipeName = L"\\\\.\\pipe\\" + string_to_wstring(std::string(app_handle_));

    try {
        int retries = 5;

        while (retries-- > 0) {
            hClientPipe = CreateFileW(
                targetPipeName.c_str(),
                GENERIC_WRITE,
                0,
                NULL,
                OPEN_EXISTING,
                0,
                NULL);
            if (hClientPipe != INVALID_HANDLE_VALUE) break;
            DWORD error = GetLastError();
            if (error != ERROR_PIPE_BUSY) {
                throw std::runtime_error("Pipe connection failed with error: " + std::to_string(error));
            }

            if (!WaitNamedPipeW(targetPipeName.c_str(), 1000)) {
                if (GetLastError() != ERROR_SEM_TIMEOUT) {
                    throw std::runtime_error("WaitNamedPipeW failed with error: " + std::to_string(GetLastError()));
                }
            }
        }

        if (hClientPipe == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("Failed to connect to pipe after retries.");
        }

        ipc_buffer = serialize_for_ipc(msg);
        if (!ipc_buffer.data && ipc_buffer.length > 0) {
            throw std::runtime_error("IPC serialization error: null buffer with non-zero length.");
        }

        DWORD bufferTotalSizeBytes = static_cast<DWORD>(ipc_buffer.length);
        success = WriteFile(hClientPipe, &bufferTotalSizeBytes, sizeof(bufferTotalSizeBytes), &bytesWritten, NULL);
        if (!success || bytesWritten != sizeof(bufferTotalSizeBytes)) {
            free_serialized_ipc_buffer(ipc_buffer);
            throw std::runtime_error("Failed to write IPC buffer size to pipe. Error: " + std::to_string(GetLastError()));
        }

        if (bufferTotalSizeBytes > 0) {
            success = WriteFile(hClientPipe, ipc_buffer.data, bufferTotalSizeBytes, &bytesWritten, NULL);
            if (!success || bytesWritten != bufferTotalSizeBytes) {
                free_serialized_ipc_buffer(ipc_buffer);
                throw std::runtime_error("Failed to write IPC buffer data to pipe. Error: " + std::to_string(GetLastError()));
            }
        }
        FlushFileBuffers(hClientPipe);
    }
    catch (const std::exception&) {
        free_serialized_ipc_buffer(ipc_buffer);
        if (hClientPipe != INVALID_HANDLE_VALUE) {
            CloseHandle(hClientPipe);
            hClientPipe = INVALID_HANDLE_VALUE;
        }
        return;
    }

    free_serialized_ipc_buffer(ipc_buffer);
    if (hClientPipe != INVALID_HANDLE_VALUE) {
        CloseHandle(hClientPipe);
    }
    this->processing = false;
}

void WindowsIPCWatcher::process_messages() {
    hPipe_ = CreateNamedPipeW(
        pipeName_.c_str(),
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,
        WindowsIPCWatcher::PIPE_BUFFER_SIZE,
        WindowsIPCWatcher::PIPE_BUFFER_SIZE,
        0,
        NULL);

    if (hPipe_ == INVALID_HANDLE_VALUE) {
        if (GetLastError() == ERROR_ACCESS_DENIED || GetLastError() == ERROR_PIPE_BUSY) {
            isPrimary_ = false;
        }
        return;
    }
    isPrimary_ = true;

    OVERLAPPED overlapped = { 0 };
    overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (overlapped.hEvent == NULL) {
        CloseHandle(hPipe_);
        hPipe_ = INVALID_HANDLE_VALUE;
        isPrimary_ = false;
        return;
    }

    while (processing && isPrimary_) {
        BOOL connectPending = FALSE;
        BOOL connected = ConnectNamedPipe(hPipe_, &overlapped);

        if (!connected) {
            DWORD error = GetLastError();
            if (error == ERROR_IO_PENDING) {
                connectPending = TRUE;
            }
            else if (error == ERROR_PIPE_CONNECTED) {
                SetEvent(overlapped.hEvent);
                connectPending = TRUE;
            }
            else {
                ResetEvent(overlapped.hEvent);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
        }
        else {
            SetEvent(overlapped.hEvent);
            connectPending = TRUE;
        }

        if (connectPending) {
            DWORD waitResult = WaitForSingleObject(overlapped.hEvent, 250);
            if (waitResult == WAIT_TIMEOUT) {
                if (!processing) break;
                continue;
            }
            else if (waitResult == WAIT_OBJECT_0) {
                DWORD bytesTransferred;
                if (!GetOverlappedResult(hPipe_, &overlapped, &bytesTransferred, FALSE)) {
                    DWORD error = GetLastError();
                    if (error != ERROR_PIPE_CONNECTED && error != ERROR_SUCCESS && error != ERROR_IO_INCOMPLETE) {
                        DisconnectNamedPipe(hPipe_);
                        ResetEvent(overlapped.hEvent);
                        continue;
                    }
                }
            }
            else {
                DisconnectNamedPipe(hPipe_);
                ResetEvent(overlapped.hEvent);
                continue;
            }
        }
        ResetEvent(overlapped.hEvent);

        DWORD ipcMessageTotalSize = 0;
        DWORD bytesReadFromPipe = 0;
        BOOL readSuccess = FALSE;

        OVERLAPPED readOvSize = { 0 };
        readOvSize.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (readOvSize.hEvent == NULL) { DisconnectNamedPipe(hPipe_); ResetEvent(overlapped.hEvent); continue; }

        readSuccess = ReadFile(hPipe_, &ipcMessageTotalSize, sizeof(ipcMessageTotalSize), NULL, &readOvSize);
        if (!readSuccess && GetLastError() == ERROR_IO_PENDING) {
            if (WaitForSingleObject(readOvSize.hEvent, INFINITE) == WAIT_OBJECT_0) {
                DWORD GORBytesReadSize;
                if (GetOverlappedResult(hPipe_, &readOvSize, &GORBytesReadSize, FALSE) && GORBytesReadSize == sizeof(ipcMessageTotalSize)) {
                    bytesReadFromPipe = GORBytesReadSize;
                    readSuccess = TRUE;
                }
            }
        }
        else if (readSuccess) {
            DWORD GORBytesReadSize;
            if (GetOverlappedResult(hPipe_, &readOvSize, &GORBytesReadSize, TRUE) && GORBytesReadSize == sizeof(ipcMessageTotalSize)) {
                bytesReadFromPipe = GORBytesReadSize;
                readSuccess = TRUE;
            }
        }
        if (readOvSize.hEvent != NULL) CloseHandle(readOvSize.hEvent);


        if (!readSuccess) {
            if (GetLastError() == ERROR_BROKEN_PIPE) isPrimary_ = false;
            DisconnectNamedPipe(hPipe_); ResetEvent(overlapped.hEvent); continue;
        }

        if (ipcMessageTotalSize == 0) {
            IPCMsgData received_data = deserialize_from_ipc(nullptr, 0);
            if (received_data.msg_handle != nullptr) {
                send_request(received_data);
            }
            else {
                free_ipc_msg_data(received_data);
            }
            DisconnectNamedPipe(hPipe_); ResetEvent(overlapped.hEvent); continue;
        }

        if (ipcMessageTotalSize > MAX_IPC_MESSAGE_BYTES_WINDOWS) {
            DisconnectNamedPipe(hPipe_); ResetEvent(overlapped.hEvent); continue;
        }

        std::vector<char> ipc_buffer_vector(ipcMessageTotalSize);
        DWORD totalBytesReadForData = 0;

        OVERLAPPED readOvData = { 0 };
        readOvData.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (readOvData.hEvent == NULL) { DisconnectNamedPipe(hPipe_); ResetEvent(overlapped.hEvent); continue; }

        readSuccess = ReadFile(hPipe_, ipc_buffer_vector.data(), ipcMessageTotalSize, NULL, &readOvData);
        if (!readSuccess && GetLastError() == ERROR_IO_PENDING) {
            if (WaitForSingleObject(readOvData.hEvent, INFINITE) == WAIT_OBJECT_0) {
                DWORD GORBytesReadData;
                if (GetOverlappedResult(hPipe_, &readOvData, &GORBytesReadData, FALSE) && GORBytesReadData == ipcMessageTotalSize) {
                    totalBytesReadForData = GORBytesReadData;
                    readSuccess = TRUE;
                }
            }
        }
        else if (readSuccess) {
            DWORD GORBytesReadData;
            if (GetOverlappedResult(hPipe_, &readOvData, &GORBytesReadData, TRUE) && GORBytesReadData == ipcMessageTotalSize) {
                totalBytesReadForData = GORBytesReadData;
                readSuccess = TRUE;
            }
        }
        if (readOvData.hEvent != NULL) CloseHandle(readOvData.hEvent);

        if (readSuccess) {
            IPCMsgData received_data = deserialize_from_ipc(ipc_buffer_vector.data(), ipcMessageTotalSize);
            if (received_data.msg_handle != nullptr) {
                send_request(received_data);
            }
            else {
                free_ipc_msg_data(received_data);
            }
        }
        else {
            if (GetLastError() == ERROR_BROKEN_PIPE) isPrimary_ = false;
        }
        DisconnectNamedPipe(hPipe_);
    }

    if (overlapped.hEvent != NULL) {
        CloseHandle(overlapped.hEvent);
    }
    if (hPipe_ != INVALID_HANDLE_VALUE) {
        if (isPrimary_) {
            DisconnectNamedPipe(hPipe_);
        }
        CloseHandle(hPipe_);
        hPipe_ = INVALID_HANDLE_VALUE;
    }
    isPrimary_ = false;
}

#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__) || defined(__DARWIN__)

const uint32_t MAX_IPC_MESSAGE_BYTES_UNIX = 7 * 1024;

key_t UnixIPCWatcher::generate_ipc_key(const char* app_handle) {
    std::string key_str = std::string(app_handle);
    uint32_t hash = 0;

    for (char c : key_str) {
        hash = hash * 31 + c;
    }

    return (key_t)(0x12340000 + (hash & 0xFFFF));
}


UnixIPCWatcher::UnixIPCWatcher(const char* app_handle) : IPCWatcher(app_handle) {
    try {
        ipc_key_ = generate_ipc_key(app_handle_);
    } catch (const std::exception&) {
        ipc_key_ = -1;
    }
}

UnixIPCWatcher::~UnixIPCWatcher() {
    stop();
    if (msg_queue_id_ != -1) {
        if (isPrimary_) {
            msgctl(msg_queue_id_, IPC_RMID, NULL);
        }
        msg_queue_id_ = -1;
    }
}


void UnixIPCWatcher::SendMsg(IPCMsgData& msg) {
    this->processing = true;
    SerializedIPCBuffer ipc_buffer;
    
    if (ipc_key_ == -1) {
        this->processing = false;
        return;
    }

    try {
        int target_queue = msgget(ipc_key_, 0);
        if (target_queue == -1) {
            throw std::runtime_error("Target message queue not found");
        }

        ipc_buffer = serialize_for_ipc(msg);
        if (!ipc_buffer.data && ipc_buffer.length > 0) {
            throw std::runtime_error("IPC serialization failed");
        }

        if (ipc_buffer.length > MAX_IPC_MESSAGE_BYTES_UNIX) {
            free_serialized_ipc_buffer(ipc_buffer);
            //throw std::runtime_error("Message too large for System V message queue");
            return ;
        }


        size_t msg_buffer_size = sizeof(IPCMessageBuffer) + ipc_buffer.length;
        IPCMessageBuffer* msg_buffer = (IPCMessageBuffer*)malloc(msg_buffer_size);
        if (!msg_buffer) {
            free_serialized_ipc_buffer(ipc_buffer);
            throw std::runtime_error("Failed to allocate message buffer");
        }

        msg_buffer->msg_type = MSG_TYPE;
        msg_buffer->data_size = static_cast<uint32_t>(ipc_buffer.length);
        
        if (ipc_buffer.length > 0) {
            memcpy(msg_buffer->data, ipc_buffer.data, ipc_buffer.length);
        }

        int retries = 3;

        while (retries-- > 0) {
            if (msgsnd(target_queue, msg_buffer, sizeof(uint32_t) + ipc_buffer.length, IPC_NOWAIT) == 0) {
                break;
            }
            if (errno == EAGAIN) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }
            //throw std::runtime_error("Failed to send message: " + std::string(strerror(errno)));
            return ;
        }

        free_serialized_ipc_buffer(ipc_buffer);
        free(msg_buffer);

    } catch (const std::exception&) {
        if (ipc_buffer.data) {
            free_serialized_ipc_buffer(ipc_buffer);
        }
    }
    
    this->processing = false;
}

void UnixIPCWatcher::process_messages() {
    if (ipc_key_ == -1) {
        return;
    }

    msg_queue_id_ = msgget(ipc_key_, IPC_CREAT | IPC_EXCL | 0600);
    if (msg_queue_id_ == -1) {
        if (errno == EEXIST) {
            msg_queue_id_ = msgget(ipc_key_, 0);
            if (msg_queue_id_ == -1) {
                return;
            }

            isPrimary_ = false;
        } else {
            return;
        }

    } else {
        isPrimary_ = true;
    }

    if (!isPrimary_) {
        return;
    }

    size_t max_msg_buffer_size = sizeof(IPCMessageBuffer) + MAX_IPC_MESSAGE_BYTES_UNIX;
    IPCMessageBuffer* msg_buffer = (IPCMessageBuffer*)malloc(max_msg_buffer_size);

    if (!msg_buffer) {
        return;
    }

    while (processing && isPrimary_) {
        try {
            ssize_t msg_size = msgrcv(msg_queue_id_, msg_buffer, MAX_IPC_MESSAGE_BYTES_UNIX + sizeof(uint32_t), 
                                    MSG_TYPE, IPC_NOWAIT);
            
            if (msg_size == -1) {
                if (errno == ENOMSG) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }
                if (errno == EINTR) {
                    continue;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(250));
                continue;
            }

            if (msg_size < sizeof(uint32_t)) {
                continue;
            }

            uint32_t data_size = msg_buffer->data_size;
            
            if (data_size == 0) {
                IPCMsgData received_data = deserialize_from_ipc(nullptr, 0);
                if (received_data.msg_handle != nullptr) {
                    send_request(received_data);
                } else {
                    free_ipc_msg_data(received_data);
                }
                continue;
            }

            if (data_size > MAX_IPC_MESSAGE_BYTES_UNIX) {
                continue;
            }

            if (msg_size != sizeof(uint32_t) + data_size) {
                continue;
            }

            IPCMsgData received_data = deserialize_from_ipc(msg_buffer->data, data_size);
            if (received_data.msg_handle != nullptr) {
                send_request(received_data);
            } else {
                free_ipc_msg_data(received_data);
            }

        } catch (const std::exception&) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    free(msg_buffer);
    
    if (msg_queue_id_ != -1) {
        if (isPrimary_) {
            msgctl(msg_queue_id_, IPC_RMID, NULL);
        }
        msg_queue_id_ = -1;
    }
    isPrimary_ = false;
}

#endif // Platform check