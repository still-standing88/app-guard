# AppGuard

Cross-platform C++ library for application instance management and inter-process communication.

## Overview

AppGuard provides a simple yet powerful solution for managing application instances and enabling communication between processes. Whether you're building a desktop application that should only run once, or need to pass data between multiple instances of your program, AppGuard has you covered.
Disclaimer: While this library has  an IPC mechanism, it is only for data exchange between app instances. It is not ment to be a robust IPC implementation. 
If you want a stable IPC solution, use an other library that can handle IPC more efficiently.

## Key Features

### Single Instance Management
- Automatically detect if another instance of your application is already running
- Configurable behavior: quit immediately or handle manually
- Cross-platform mutex-based implementation

### Inter-Process Communication
- Send messages between application instances
- Callback-based message handling
- Support for structured message routing
- Thread-safe message delivery

### Cross-Platform Support
- Windows (Win32 API)
- Linux and macOS (System v messages. Unix based implementation)

### Language Bindings
- Native C++ API
- Python bindings with Pythonic interface
- Consistent API across languages

## Use Cases

### Desktop Applications
Ensure your GUI application only runs one instance, and bring the existing window to focus when users try to launch it again.

### Command-Line Tools
Forward command-line arguments from secondary instances to the primary instance for processing.

# Building
Fork this repo
```bash
git clone https://github.com/still-standing88/app-guard
```

Install requirements, AppGuard requires SCons,, setuptools, and  pybind11 to build with python bindings

```bash
pip install -r requirements.txt
scons
```


## Documentation
Documentation can be found [here](https://still-standing88.github.io/app-guard-docs/)

## License

This project is licensed under the MIT License - see the LICENSE file for details.

- GitHub: [@still-standing88](https://github.com/still-standing88)
- Project: [app-guard](https://github.com/still-standing88/app-guard)