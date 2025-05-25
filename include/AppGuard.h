/**
 ******************************************************************************
* AppGuard.
* @file AppGuard.h
* @brief Cross-platform C++ library for application instance management and IPC.
* @Copyright (C) 2024 Oussama Ben Gatrane
* @license
* This product is licensed under the MIT License. See the license file for the full license text.
*
* @see https://github.com/still-standing88/app-guard
* Version: 1.01
*  Author: Oussama Ben Gatrane
* Description: Cross-platform C++ library for application instance management and inter-process communication.
*
  ******************************************************************************
**/

#ifndef APP_GUARD_H
#define APP_GUARD_H
#pragma once
#ifdef _WIN32
#ifdef __APPGUARD_EXPORT
#define APPGUARD_API __declspec(dllexport)
#elif defined(__APPGUARD_STATIC)
#define APPGUARD_API
#else
#define APPGUARD_API __declspec(dllimport)
#endif // _WIN32
#else
#ifdef __APPGUARD_EXPORT
#define APPGUARD_API __attribute__((visibility("default"))) // For compilors such as Clang
#else
#define APPGUARD_API
#endif // __APPGUARD_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
#include <stdbool.h>
#include <wchar.h>
#include "common.h"


	/**
	 * @brief Initializes the AppGuard library for application instance management.
	 * 
	 * The ap handle could be a window name or unique. It is used internally by the library.
	 * This function must be called first before any other functions in the library.
	 * 
	 * @param app_handle A const char* representing the unique application identifier.
	 * @param on_quit_callback A callback function to be called when the application quits immediately. 
	 * This callback will only be invoked on secondary instances if the quit_immediate boolean is set to true. It will not be called once the library cleans up.
	 * @param quit_immediate A boolean indicating whether to quit immediately when a secondary instance of the app is detected or not. 
	 * If set to false, the closing of the library/application will be left to the user. 
	 */
	APPGUARD_API void AG_init(const char* app_handle, AppOnQuitCallback on_quit_callback, bool quit_immediate);

	/**
	 * @brief Releases AppGuard resources and performs cleanup.
	 * 
	 * Cleans up the current app instance and ipc related resources.
	 */
	APPGUARD_API void AG_release();

	/**
	 * @brief Checks if the AppGuard library is loaded and initialized.
	 * 
	 * Call this function to check if the library is loaded correctly. 
	 * Returns false if the library is not initialized or was released.
	 * 
	 * @return A bool indicating whether the library is loaded.
	 */
	APPGUARD_API bool AG_is_loaded();

	/**
	 * @brief Determines if this is the primary instance of the application.
	 * 
	 * This function is used internally by the library for surtain checks E.G. if should the app quit if the current process is not a primary instance.
	 * 
	 * @return A bool indicating whether this is the primary application instance.
	 */
	APPGUARD_API bool AG_is_primary_instance();

	/**
	 * @brief Creates an IPC message structure for inter-process communication.
	 * 
	 * Call this function to initialize the ipc message with a handle and a callback.
	 * The handle is used for message lookup. Through it, the library calls the function you provided, passing an IPCMsgData as an argument.
	 * This is used in conjunction with the AG_register_msg and  AG_send_msg_request functions.
	 * 
	 * @param msg A pointer to an IPCMsg structure to be initialized.
	 * @param msg_handle A const char* representing the message identifier.
	 * @param callback A callback function to be invoked using the message handle when an IPC message request is received.
	 */
	APPGUARD_API void AG_create_IPCMsg(IPCMsg* msg, const char* msg_handle, IPCMsgCallback callback);

	/**
	 * @brief Registers an IPC message for receiving communications.
	 * 
	 * Call this function after creating the IPC message.
	 * The function places an id on the message struct which can be used to unregister the message
	 * 
	 * @param msg A pointer to an IPCMsg structure to register.
	 */
	APPGUARD_API void AG_register_msg(IPCMsg* msg);

	/**
	 * @brief Unregisters an IPC message by its ID.
	 * 
	 * @param msg_id An integer representing the message ID to unregister.
	 */
	APPGUARD_API void AG_unregister_msg(int msg_id);

	/**
	 * @brief Sends an IPC message request to another process instance.
	 * 
	 * This function can be used to send an IPC message to a live instance of the program. 
	 * This could typically be used to forward data to the program from other secondary instances before exiting. E.G. Command line arguments.
	 * Only registered messages that are foudn by the same handle provided in the IPCMsgData struct will be called with the IPCMsgData argument passed to them.
	 * 
	 * @param msg_request A pointer to an IPCMsgData structure containing the message to send.
	 */
	APPGUARD_API void AG_send_msg_request(IPCMsgData* msg_request);

	/**
	 * @brief Retrieves the current process ID.
	 * 
	 * @return An integer representing the current process ID.
	 */
	APPGUARD_API int AG_get_process_id();

	/**
	 * @brief Focuses a window by its name.
	 * 
	 * @param window_name A const wchar_t* representing the name of the window to focus.
	 */
	APPGUARD_API void AG_focus_window(const wchar_t* window_name);

#ifdef __cplusplus
} // extern "C"
#endif // cplusplus
#endif // APP_GUARD_H