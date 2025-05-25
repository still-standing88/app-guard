#include "utils.h"


#include <vector>
#include <stdexcept>
#include <cstring>
#include <random>
#include <algorithm>
#include <locale> 
#include <codecvt>

#ifdef _WIN32
#include <Windows.h>
#endif

std::wstring string_to_wstring(const std::string& str) {
    DISABLE_DEPRECATED_WARNING
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    try {
        return converter.from_bytes(str);
    }
    catch (const std::range_error&) {
        std::wstring result;

        for (char c : str) {
            result += static_cast<wchar_t>(static_cast<unsigned char>(c));
        }
        return result;
    }
    ENABLE_DEPRECATED_WARNING
}


std::string wstring_to_string(const std::wstring& wstr) {
    DISABLE_DEPRECATED_WARNING
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

    try {
        return converter.to_bytes(wstr);
    }
    catch (const std::range_error&) {
        std::string result;

        for (wchar_t wc : wstr) {
            if (wc < 128) result += static_cast<char>(wc);
            else result += '?';
        }

        return result;
    }
    ENABLE_DEPRECATED_WARNING
}

static std::string internal_platform_wchar_to_utf8_string(const wchar_t* wstr) {
    if (!wstr) return std::string();

#ifdef _WIN32
    int wstr_len = static_cast<int>(wcslen(wstr));
    if (wstr_len == 0) return std::string();
    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wstr, wstr_len, nullptr, 0, nullptr, nullptr);
    if (utf8_len == 0) throw std::runtime_error("WideCharToMultiByte: Failed to get UTF-8 size");
    std::string utf8_str(utf8_len, 0);
    if (WideCharToMultiByte(CP_UTF8, 0, wstr, wstr_len, &utf8_str[0], utf8_len, nullptr, nullptr) == 0) {
        throw std::runtime_error("WideCharToMultiByte: Failed to convert to UTF-8");
    }
    return utf8_str;

#else
    DISABLE_DEPRECATED_WARNING
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    ENABLE_DEPRECATED_WARNING
        try {
        return converter.to_bytes(wstr);
    }
    catch (const std::range_error& e) {
        throw std::runtime_error("UTF-32 to UTF-8 conversion failed: " + std::string(e.what()));
    }
#endif
}

static wchar_t* internal_utf8_string_to_platform_wchar(const std::string& utf8_str) {
    if (utf8_str.empty()) {
        wchar_t* empty_wstr = new wchar_t[1];
        empty_wstr[0] = L'\0';
        return empty_wstr;
    }

#ifdef _WIN32
    int utf8_len = static_cast<int>(utf8_str.length());

    int wstr_len = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), utf8_len, nullptr, 0);
    if (wstr_len == 0) throw std::runtime_error("MultiByteToWideChar: Failed to get wchar_t size");
    wchar_t* wstr = new wchar_t[wstr_len + 1];

    if (MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), utf8_len, wstr, wstr_len) == 0) {
        delete[] wstr;
        throw std::runtime_error("MultiByteToWideChar: Failed to convert to wchar_t");
    }
    wstr[wstr_len] = L'\0';
    return wstr;

#else
    DISABLE_DEPRECATED_WARNING

        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    ENABLE_DEPRECATED_WARNING

        try {
        std::wstring temp_wstr = converter.from_bytes(utf8_str);
        wchar_t* wstr = new wchar_t[temp_wstr.length() + 1];
        std::copy(temp_wstr.begin(), temp_wstr.end(), wstr);
        wstr[temp_wstr.length()] = L'\0';
        return wstr;
    }
    catch (const std::range_error& e) {
        throw std::runtime_error("UTF-8 to UTF-32 conversion failed: " + std::string(e.what()));
    }
#endif
}


SerializedIPCBuffer serialize_for_ipc(const IPCMsgData& platform_msg_data) {
    std::string handle_utf8;

    if (platform_msg_data.msg_handle) {
        handle_utf8 = platform_msg_data.msg_handle;
    }

    std::string data_utf8;
    if (platform_msg_data.msg_data) {
        data_utf8 = internal_platform_wchar_to_utf8_string(platform_msg_data.msg_data);
    }

    uint32_t handle_len = static_cast<uint32_t>(handle_utf8.length());
    uint32_t data_len = static_cast<uint32_t>(data_utf8.length());

    size_t total_buffer_size = sizeof(uint32_t) + handle_len + sizeof(uint32_t) + data_len;
    char* buffer = new char[total_buffer_size];
    char* current_pos = buffer;

    memcpy(current_pos, &handle_len, sizeof(uint32_t));
    current_pos += sizeof(uint32_t);
    if (handle_len > 0) {
        memcpy(current_pos, handle_utf8.data(), handle_len);
        current_pos += handle_len;
    }

    memcpy(current_pos, &data_len, sizeof(uint32_t));
    current_pos += sizeof(uint32_t);
    if (data_len > 0) {
        memcpy(current_pos, data_utf8.data(), data_len);
    }

    SerializedIPCBuffer result;
    result.data = buffer;
    result.length = total_buffer_size;
    return result;
}


IPCMsgData deserialize_from_ipc(const char* ipc_buffer, size_t buffer_length) {
    IPCMsgData result = { nullptr, nullptr };
    if (!ipc_buffer || buffer_length == 0) return result;

    const char* current_pos = ipc_buffer;
    const char* const buffer_end = ipc_buffer + buffer_length;

    if (static_cast<size_t>(buffer_end - current_pos) < sizeof(uint32_t)) return result;
    uint32_t handle_len;
    memcpy(&handle_len, current_pos, sizeof(uint32_t));
    current_pos += sizeof(uint32_t);

    if (static_cast<size_t>(buffer_end - current_pos) < handle_len) return result;
    if (handle_len > 0) {
        char* msg_handle_alloc = new char[handle_len + 1];
        memcpy(msg_handle_alloc, current_pos, handle_len);
        msg_handle_alloc[handle_len] = '\0';
        result.msg_handle = msg_handle_alloc;
    }

    else {
        char* empty_handle = new char[1];
        empty_handle[0] = '\0';
        result.msg_handle = empty_handle;
    }
    current_pos += handle_len;

    if (static_cast<size_t>(buffer_end - current_pos) < sizeof(uint32_t)) {
        free_ipc_msg_data(result); result = { nullptr, nullptr }; return result;
    }
    uint32_t data_len;
    memcpy(&data_len, current_pos, sizeof(uint32_t));
    current_pos += sizeof(uint32_t);

    if (static_cast<size_t>(buffer_end - current_pos) < data_len) {
        free_ipc_msg_data(result); result = { nullptr, nullptr }; return result;
    }
    if (data_len > 0) {
        std::string data_utf8(current_pos, data_len);
        result.msg_data = internal_utf8_string_to_platform_wchar(data_utf8);
    }

    else {
        result.msg_data = internal_utf8_string_to_platform_wchar("");
    }

    return result;
}


void free_serialized_ipc_buffer(SerializedIPCBuffer& buffer) {
    delete[] buffer.data;
    buffer.data = nullptr;
    buffer.length = 0;
}

void free_ipc_msg_data(IPCMsgData& data) {
    if (data.msg_handle) {
        delete[] data.msg_handle;
        data.msg_handle = nullptr;
    }
    if (data.msg_data) {
        delete[] data.msg_data;
        data.msg_data = nullptr;
    }
}


int random_number(int min, int max) {
    static std::random_device r_device;
    static std::mt19937 r_gen(r_device());
    std::uniform_int_distribution<> r_dist(min, max);
    return r_dist(r_gen);
}

std::string public_platform_wchar_to_utf8_string(const wchar_t* wstr) {
    return internal_platform_wchar_to_utf8_string(wstr);
}
