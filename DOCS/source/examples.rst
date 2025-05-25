Examples
========

This page contains practical examples of using AppGuard in various scenarios.

Basic Usage
-----------

Python Basic Example
~~~~~~~~~~~~~~~~~~~~

.. code-block:: python

   from app_guard import AppGuard
   import time

   def on_quit():
       print("Another instance is running, exiting...")

   def main():
       # Initialize AppGuard
       AppGuard.init("MyUniqueApp", on_quit, quit_immediate=True)
       
       if AppGuard.is_primary_instance():
           print("Primary instance started")
           
           # Your main application logic
           while True:
               print("Running...")
               time.sleep(5)
       else:
           print("Secondary instance detected")

   if __name__ == "__main__":
       main()

C++ Basic Example
~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include "AppGuard.h"
   #include <iostream>
   #include <thread>
   #include <chrono>

   void on_quit() {
       std::cout << "Another instance is running, exiting..." << std::endl;
   }

   int main() {
       AG_init("MyUniqueApp", on_quit, true);
       
       if (AG_is_primary_instance()) {
           std::cout << "Primary instance started" << std::endl;
           
           // Main application loop
           while (true) {
               std::cout << "Running..." << std::endl;
               std::this_thread::sleep_for(std::chrono::seconds(5));
           }
       } else {
           std::cout << "Secondary instance detected" << std::endl;
       }
       
       AG_release();
       return 0;
   }

IPC Communication
-----------------

Python IPC Example
~~~~~~~~~~~~~~~~~~~

.. code-block:: python

   from app_guard import AppGuard
   import sys

   def handle_message(msg_data):
       print(f"Received message: {msg_data['msg_data']}")

   def on_quit():
       print("Forwarding to primary instance...")

   def main():
       app = AppGuard()
       
       if len(sys.argv) > 1:
           # Secondary instance - send message and quit
           AppGuard.init("MyApp", on_quit, quit_immediate=False)
           
           if not AppGuard.is_primary_instance():
               message = " ".join(sys.argv[1:])
               app.send_msg_request("command_line", message)
               AppGuard.release()
               return
       
       # Primary instance
       AppGuard.init("MyApp", None, quit_immediate=True)
       
       if AppGuard.is_primary_instance():
           # Register message handler
           msg = app.create_ipc_msg("command_line", handle_message)
           app.register_msg(msg)
           
           print("Primary instance running, waiting for messages...")
           
           # Keep running
           try:
               while True:
                   time.sleep(1)
           except KeyboardInterrupt:
               app.unregister_msg(msg)
               AppGuard.release()

   if __name__ == "__main__":
       main()

Advanced Examples
-----------------

*Note: Additional examples will be added here covering:*

- GUI applications with window focusing
- Service applications with configuration updates
- Multi-threaded applications
- Error handling and recovery
- Cross-platform considerations