#pragma once
#ifdef _WIN32
#include <Windows.h>
#elif defined(__linux__) || defined(__unix__)
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#elif defined(__APPLE__) || defined(__DARWIN__)
#include <mach/mach.h>
#include <servers/bootstrap.h>
#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>
#include <string>
#include <iostream>
#include <unistd.h>
#endif // _WIN32


class AppInstance {
protected:
	bool is_first_;

public:
	AppInstance():is_first_(true) {}
	AppInstance(const char* app_handle) {
		this->init(app_handle);
	}

	~AppInstance() {}

	virtual void init(const char* app_handle) = 0;
	virtual void release() = 0;
	virtual bool IsFirstInstance() {return this->is_first_; }
	virtual void FocusWindow(const wchar_t* windowname) = 0;
	virtual int get_process_id() = 0;
};


#ifdef _WIN32

class WinAppInstance : public AppInstance {
private:
	HANDLE mutex_handle_;

public:
	WinAppInstance() : mutex_handle_(nullptr) {}
	~WinAppInstance() { release(); }

	void init(const char* app_handle) override;
	void release() override;
	void FocusWindow(const wchar_t* windowname) override;
	int get_process_id() override;

};

#elif defined(__linux__) || defined(__linux)
class LinuxAppInstance : public AppInstance {
private:
	int lock_fd_;
	const char* app_handle;

public:
	LinuxAppInstance() : lock_fd_(-1) {}
	~LinuxAppInstance() { release(); }

	void init(const char* app_handle) override;
	void release() override;
	void FocusWindow(const wchar_t* windowname) override;
	int get_process_id() override;


	};

#elif defined(__APPLE__) || defined(__DARWIN__)
class MacAppInstance : public AppInstance {
private:
	mach_port_t receive_port_ = MACH_PORT_NULL;
	std::string service_name_;

public:
	MacAppInstance() {}
	~MacAppInstance() { release(); }

	void init(const char* app_handle) override;
	void release() override;
	void FocusWindow(const wchar_t* windowname) override;
	int get_process_id() override;


	};
#endif // _WIN32