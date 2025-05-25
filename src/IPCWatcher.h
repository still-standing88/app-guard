#pragma once

#ifdef _WIN32
#include <Windows.h>
#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__) || defined(__DARWIN__)
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>
#endif // _WIN32

#include <string>
#include <functional>
#include <unordered_map>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "../include/common.h"



class IPCWatcher {
private:
	std::thread watcher_thread_;
	std::thread process_thread_;
	std::condition_variable cv_;


	void WatchProcess();

public:
	IPCWatcher(const char* app_handle);
	~IPCWatcher();

	void start();
	void stop();
	void RegisterIPCMsg(IPCMsg& msg);
	void UnregisterIPCMsg(int msg_id);

	virtual void SendMsg(IPCMsgData& msg) = 0;

protected:
    void send_request(IPCMsgData& msg_request);
	virtual void process_messages() = 0;

	std::mutex mutex_;
	std::unordered_map<std::string, IPCMsg> messages_;
	std::deque<IPCMsgData> msg_requests_;
	bool processing;
	bool watching;
	const char* app_handle_;
};
