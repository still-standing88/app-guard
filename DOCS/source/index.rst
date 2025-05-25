AppGuard Documentation
======================

Welcome to AppGuard's documentation!

AppGuard is a cross-platform C++ library with Python bindings for application instance management and inter-process communication (IPC). It ensures single-instance applications and provides seamless communication between processes.

.. toctree::
   :maxdepth: 2
   :caption: Contents:

   python_api
   cpp_api
   examples

Key Features
------------

* **Single Instance Management**: Ensure only one instance of your application runs at a time
* **Inter-Process Communication**: Send messages between application instances
* **Cross-Platform**: Works on Windows, Linux, and macOS
* **Python & C++ APIs**: Use from both Python and C++ applications
* **Thread-Safe**: Built with thread safety in mind
* **Lightweight**: Minimal overhead and dependencies

Quick Start
-----------

Python
~~~~~~

.. code-block:: python

   from app_guard import AppGuard

   def on_quit():
       print("Secondary instance detected, quitting...")

   # Initialize AppGuard
   AppGuard.init("MyApp", on_quit, quit_immediate=True)

   if AppGuard.is_primary_instance():
       print("Primary instance running")
       # Your main application logic here
   else:
       print("Secondary instance, will quit")

C++
~~~

.. code-block:: cpp

   #include "AppGuard.h"

   void on_quit() {
       printf("Secondary instance detected, quitting...\n");
   }

   int main() {
       AG_init("MyApp", on_quit, true);
       
       if (AG_is_primary_instance()) {
           printf("Primary instance running\n");
           // Your main application logic here
       }
       
       AG_release();
       return 0;
   }

Installation
------------

.. code-block:: bash

   pip install app-guard

Or build from source:

.. code-block:: bash

   git clone https://github.com/still-standing88/app-guard
   cd app-guard
   pip install -r requirements.txt
   scons

Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`