#pragma once


#if defined(__GNUC__) || defined(__clang__)
#define DISABLE_DEPRECATED_WARNING \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
#define ENABLE_DEPRECATED_WARNING \
    _Pragma("GCC diagnostic pop")
#elif defined(_MSC_VER)
#define DISABLE_DEPRECATED_WARNING \
    __pragma(warning(push)) \
    __pragma(warning(disable: 4996))
#define ENABLE_DEPRECATED_WARNING \
    __pragma(warning(pop))
#else
#define DISABLE_DEPRECATED_WARNING
#define ENABLE_DEPRECATED_WARNING
#endif

#include <string>
#include <vector>
#include "../include/common.h"

std::wstring string_to_wstring(const std::string& str);
std::string wstring_to_string(const std::wstring& wstr);
std::string public_platform_wchar_to_utf8_string(const wchar_t* wstr);


struct SerializedIPCBuffer {
    char* data;
    size_t length;
    SerializedIPCBuffer() : data(nullptr), length(0) {}
};

SerializedIPCBuffer serialize_for_ipc(const IPCMsgData& platform_msg_data);
IPCMsgData deserialize_from_ipc(const char* ipc_buffer, size_t buffer_length);

void free_serialized_ipc_buffer(SerializedIPCBuffer& buffer);
void free_ipc_msg_data(IPCMsgData& data);
int random_number(int min, int max);