#if defined(__linux__) || defined(__unix__)
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#elif defined(__APPLE__) || defined(__DARWIN__)
#include <iostream>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <errno.h>
#endif 

#include <iostream>
#include "utils.h"
#include "IPCWatcher.h"

IPCWatcher::IPCWatcher(const char* app_handle) :
	processing(false), watching(false),
	app_handle_(app_handle) {
	//this->start();
}

IPCWatcher::~IPCWatcher() {
	this->stop();
}

void IPCWatcher::start() {
	if (!this->watching) {
		this->watching = true;
		this->watcher_thread_ = std::thread([this]() {WatchProcess(); });
			// this->watcher_thread_.detach();
	}
	if (!this->processing) {
		this->processing = true;
		this->process_thread_ = std::thread([&]() {process_messages(); });
		// this->process_thread_.detach();
	}
}

void IPCWatcher::stop() {
	this->watching = false;
	this->processing = false;
	this->cv_.notify_all();
	if (this->process_thread_.joinable()) {
		this->process_thread_.join();
	}
	if (this->watcher_thread_.joinable()) {
		this->watcher_thread_.join();
	}
	this->messages_.clear();
	this->msg_requests_ = std::deque<IPCMsgData>();
}

void IPCWatcher::RegisterIPCMsg(IPCMsg& msg) {
	this->mutex_.lock();
	this->messages_.insert({ msg.msg_handle, msg });
	this->mutex_.unlock();
}

void IPCWatcher::UnregisterIPCMsg(int msg_id) {
	this->mutex_.lock();
	for (auto msg : this->messages_) {
		if (msg_id == msg.second.msg_id) {
			this->messages_.erase(msg.first);
			break;
		}
	}
	this->mutex_.unlock();
}

void IPCWatcher::send_request(IPCMsgData& msg_request) {
	this->mutex_.lock();
	this->msg_requests_.push_back(msg_request);
	this->cv_.notify_one();
	this->mutex_.unlock();
}

void IPCWatcher::WatchProcess() {
	while (this->watching) {
		std::unique_lock<std::mutex> lock(this->mutex_);
		this->cv_.wait(lock, [&]() {
			return !msg_requests_.empty() || !watching;
			});
		if (!this->watching) {
			break;
		}
        while (!this->msg_requests_.empty()) {
            auto request = this->msg_requests_.front();
			auto callback_iter = this->messages_.find(std::string(request.msg_handle));
			if (callback_iter != this->messages_.end()) {
				callback_iter->second.callback(&request);
			}
			delete request.msg_handle;
			delete request.msg_data;
            this->msg_requests_.pop_front();
        }
		lock.unlock();
}
}
