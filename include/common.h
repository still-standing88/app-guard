/**
 ******************************************************************************
* AppGuard Common Definitions.
* @file common.h
* @brief Common data structures and type definitions for AppGuard library.
* @Copyright (C) 2024 Oussama Ben Gatrane
* @license
* This product is licensed under the MIT License. See the license file for the full license text.
*
* @see https://github.com/still-standing88/app-guard/
* Version: 1.01
*  Author: Oussama Ben Gatrane
* Description: Shared data structures and callback definitions for inter-process communication and application management.
*
  ******************************************************************************
**/

#ifndef APP_GUARD_COMMON_H
#define APP_GUARD_COMMON_H
#pragma once

// Forward declaration
struct IPCMsgData;

/**
 * @brief Callback function type for handling IPC messages.
 * 
 * @param msg_data A pointer to the received IPC message data.
 */
typedef void(*IPCMsgCallback)(const IPCMsgData* msg_data);

/**
 * @brief Callback function type for application quit notifications.
 * 
 */
typedef void(*AppOnQuitCallback)();

/**
 * @brief Structure representing IPC message data.
 * 
 */
struct IPCMsgData {
	/**
	 * @brief Message handle identifier.
	 * 
	 * Handle used for invoking message callbacks.
	 */
	const char* msg_handle;
	
	/**
	 * @brief The actual message content.
	 * 
	 * Message data. String content. 8 KB max.
	 */
	const wchar_t* msg_data;
};

/**
 * @brief Structure representing a registered IPC message handler.
 * 
 */
struct IPCMsg {
	/**
	 * @brief Unique message identifier.
	 * 
	 * Handle used for invoking message callbacks.
	 */
	int msg_id;
	
	/**
	 * @brief Message handle string.
	 * 
	 */
	const char* msg_handle;
	
	/**
	 * @brief Callback function for handling messages.
	 * 
	 */
	IPCMsgCallback callback;
};

#endif // APP_GUARD_COMMON_H