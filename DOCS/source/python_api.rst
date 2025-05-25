Python API Reference
====================

The Python API provides a high-level, Pythonic interface to the AppGuard library.

AppGuard Class
--------------

.. automodule:: app_guard
   :members:
   :undoc-members:
   :show-inheritance:

.. autoclass:: app_guard.AppGuard
   :members:
   :undoc-members:
   :show-inheritance:

Exceptions
----------

.. autoexception:: app_guard.NotLoadedError
   :members:
   :show-inheritance:

.. autoexception:: app_guard.AppGuardError
   :members:
   :show-inheritance:

Data Structures
---------------

.. autoclass:: app_guard.IPCMsg
   :members:
   :undoc-members:
   :show-inheritance:

Low-Level Functions
-------------------

These functions provide direct access to the C++ library functions. Generally, you should use the AppGuard class instead.

.. autofunction:: app_guard.AG_init

.. autofunction:: app_guard.AG_release

.. autofunction:: app_guard.AG_is_loaded

.. autofunction:: app_guard.AG_is_primary_instance

.. autofunction:: app_guard.AG_create_IPCMsg

.. autofunction:: app_guard.AG_register_msg

.. autofunction:: app_guard.AG_unregister_msg

.. autofunction:: app_guard.AG_send_msg_request

.. autofunction:: app_guard.AG_get_process_id

.. autofunction:: app_guard.AG_focus_window