C++ API Reference
==================

The C++ API provides direct access to the AppGuard library functions.

Core Functions
--------------

.. doxygenfunction:: AG_init

.. doxygenfunction:: AG_release

.. doxygenfunction:: AG_is_loaded

.. doxygenfunction:: AG_is_primary_instance

IPC Functions
-------------

.. doxygenfunction:: AG_create_IPCMsg

.. doxygenfunction:: AG_register_msg

.. doxygenfunction:: AG_unregister_msg

.. doxygenfunction:: AG_send_msg_request

Utility Functions
-----------------

.. doxygenfunction:: AG_get_process_id

.. doxygenfunction:: AG_focus_window

Data Structures
---------------

.. doxygenstruct:: IPCMsg
   :members:

.. doxygenstruct:: IPCMsgData
   :members:

Type Definitions
----------------

.. doxygentypedef:: IPCMsgCallback

.. doxygentypedef:: AppOnQuitCallback

Macros
------

.. doxygendefine:: APPGUARD_API