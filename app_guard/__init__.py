from functools import wraps
from typing import Optional, Callable

from .AppGuard import (
    AG_init,
    AG_release,
    AG_is_loaded,
    AG_is_primary_instance,
    AG_create_IPCMsg,
    AG_register_msg,
    AG_unregister_msg,
    AG_send_msg_request,
    AG_get_process_id,
    AG_focus_window,
    IPCMsg
)


class NotLoadedError(Exception):
    """Exception raised when AppGuard is not loaded/initialized."""
    pass

class AppGuardError(Exception):
    """General AppGuard exception."""
    pass


def CheckInit(func):
    """Decorator to check if AppGuard is initialized before calling methods."""
    @wraps(func)
    def wrapper(self, *args, **kw):
        if self.is_loaded():
            res = func(self, *args, **kw)
            return res
        else:
            raise NotLoadedError("AppGuard is not loaded. Initialize AppGuard before calling any method.")
    return wrapper

class AppGuard:
    """
    Cross-platform Python wrapper for application instance management and IPC.
    
    This class provides a high-level interface to the AppGuard C++ library,
    allowing for single-instance application management and inter-process communication.
    """

    @classmethod
    def init(cls, app_handle: str, on_quit_callback: Callable, quit_immediate: bool = True) -> None:
        """
        Initialize the AppGuard library for application instance management.
        
        The app_handle could be a window name or unique identifier. It is used internally by the library.
        This function must be called first before any other functions in the library.
        
        Args:
            app_handle (str): A unique application identifier.
            on_quit_callback (Callable): A callback function to be called when the application quits immediately.
                This callback will only be invoked on secondary instances if the quit_immediate boolean is set to true.
            quit_immediate (bool, optional): Whether to quit immediately when a secondary instance of the app is detected.
                If set to False, the closing of the library/application will be left to the user. Defaults to True.
                
        Raises:
            AppGuardError: If initialization fails.
        """
        try:
            AG_init(app_handle, on_quit_callback, quit_immediate)
        except Exception as e:
            raise AppGuardError(f"Error initializing AppGuard {str(e)}")

    @classmethod
    def release(cls) -> None:
        """
        Release AppGuard resources and perform cleanup.
        
        Cleans up the current app instance and IPC related resources.
        """
        if cls.is_loaded():
            AG_release()

    @classmethod
    def is_loaded(cls) -> bool:
        """
        Check if the AppGuard library is loaded and initialized.
        
        Call this function to check if the library is loaded correctly. 
        Returns False if the library is not initialized or was released.
        
        Returns:
            bool: True if the library is loaded, False otherwise.
        """
        return AG_is_loaded()

    @classmethod
    @CheckInit
    def is_primary_instance(cls) -> bool:
        """
        Determine if this is the primary instance of the application.
        
        This function is used internally by the library for certain checks, e.g., if the app should quit 
        if the current process is not a primary instance.
        
        Returns:
            bool: True if this is the primary application instance, False otherwise.
        """
        return AG_is_primary_instance()

    def __enter__(self):
        """Context manager entry. Note: init() must be called separately with required parameters."""
        if not AppGuard.is_loaded():
            raise NotLoadedError("AppGuard must be initialized with init() before using as context manager")
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        """Context manager exit. Releases AppGuard resources."""
        AppGuard.release()
        return False

    def __init__(self) -> None:
        """Initialize AppGuard instance."""
        pass

    @CheckInit
    def create_ipc_msg(self, msg_handle: str, callback: Callable) -> IPCMsg:
        """
        Create an IPC message structure for inter-process communication.
        
        Call this function to initialize the IPC message with a handle and a callback.
        The handle is used for message lookup. Through it, the library calls the function you provided, 
        passing an IPCMsgData as an argument. This is used in conjunction with the register_msg and 
        send_msg_request methods.
        
        Args:
            msg_handle (str): A message identifier.
            callback (Callable): A callback function to be invoked using the message handle when an IPC message request is received.
            
        Returns:
            IPCMsg: The created IPC message structure.
        """
        ipc_msg: IPCMsg = IPCMsg()
        AG_create_IPCMsg(ipc_msg, msg_handle, callback)
        return ipc_msg

    @CheckInit
    def register_msg(self, msg: IPCMsg) -> None:
        """
        Register an IPC message for receiving communications.
        
        Call this function after creating the IPC message.
        The function places an ID on the message struct which can be used to unregister the message.
        
        Args:
            msg (IPCMsg): An IPCMsg structure to register.
        """
        AG_register_msg(msg)

    @CheckInit
    def unregister_msg(self, msg: IPCMsg) -> None:
        """
        Unregister an IPC message by its message object.
        
        Args:
            msg (IPCMsg): The IPCMsg object to unregister.
        """
        AG_unregister_msg(msg.msg_id)

    @CheckInit
    def send_msg_request(self, msg_handle: str, msg_data: str) -> None:
        """
        Send an IPC message request to another process instance.
        
        This function can be used to send an IPC message to a live instance of the program. 
        This could typically be used to forward data to the program from other secondary instances before exiting, 
        e.g., command line arguments. Only registered messages that are found by the same handle provided 
        will be called with the message data passed to them.
        
        Args:
            msg_handle (str): The message handle identifier.
            msg_data (str): The message data to send.
        """
        AG_send_msg_request(msg_handle, msg_data)

    @CheckInit
    def focus_window(self, window_name: str) -> None:
        """
        Focus a window by its name.
        
        Args:
            window_name (str): The name of the window to focus.
        """
        AG_focus_window(window_name)

    @CheckInit
    def get_process_id(self) -> int:
        """
        Retrieve the current process ID.
        
        Returns:
            int: The current process ID.
        """
        return AG_get_process_id()


__all__ = [
    "AG_init",
    "AG_release",
    "AG_is_loaded",
    "AG_is_primary_instance",
    "AG_create_IPCMsg",
    "AG_register_msg",
    "AG_unregister_msg",
    "AG_send_msg_request",
    "AG_get_process_id",
    "AG_focus_window",
    "IPCMsg"
]