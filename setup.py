import platform as pf

import sys
import os

from setuptools import Extension, setup, find_packages
import pybind11


def get_readme():
    try:
        with open("./readme.md", 'r', encoding='utf-8') as file:
            return file.read()
    except FileNotFoundError:
        return "AppGuard Python Wrapper (pybind11)"

def detect_platform():
    if sys.platform == 'win32': return 'windows'
    if sys.platform == 'darwin': return 'macos'
    return 'linux'

def detect_arch():
    machine = pf.machine().lower()
    if machine in ['x86_64', 'amd64']: return 'x86_64'
    if machine in ['i386', 'i686']: return 'x86' 
    if machine in ['arm64', 'aarch64']: return 'arm64'
    print(f"Warning: Unrecognized architecture '{machine}'. Defaulting to x86_64.")
    return 'x86_64'

def find_static_library_for_python(build_type_hint=None):
    platform = detect_platform()
    arch = detect_arch()
    
    build_types = []
    if build_type_hint and build_type_hint in ['release', 'debug']:
        build_types.append(build_type_hint)
    build_types.extend(['release', 'debug'])

    dir_names = ['static_for_python', 'static']

    for build_attempt in list(dict.fromkeys(build_types)):
        for subdir in dir_names:
            lib_dir_base = os.path.join('lib', platform, arch, build_attempt, subdir)
            
            if platform == 'windows':
                lib_file_path = os.path.join(lib_dir_base, 'AppGuard.lib')
            else:
                lib_file_path = os.path.join(lib_dir_base, 'libAppGuard.a')
            
            if os.path.exists(lib_file_path):
                return lib_dir_base, lib_file_path, build_attempt
    
    raise FileNotFoundError(
        f"SETUP.PY_ERROR: Could not find AppGuard static library for Python in expected paths for "
        f"platform {platform}, arch {arch}"
    )

current_platform = detect_platform()
current_arch = detect_arch()
lib_build_type_from_env = os.environ.get('PYTHON_BUILD_TYPE') 
print(f"SETUP.PY_INFO: PYTHON_BUILD_TYPE from env: {lib_build_type_from_env}")

try:
    static_lib_dir_found, static_lib_file_path, resolved_lib_build_type = find_static_library_for_python(lib_build_type_from_env)
except FileNotFoundError as e:
    print(f"SETUP.PY_ERROR: {e}")

sources = ["./app_guard/bindings.cpp"] 
extra_compile_args = ["-D__APPGUARD_STATIC"] 

cpp_std_flag = '/std:c++17' if current_platform == "windows" else '-std=c++17'
extra_compile_args.append(cpp_std_flag)

link_libraries = []
library_dirs_list = [os.path.abspath(static_lib_dir_found)]
extra_link_args_platform = []


if current_platform == "windows":
    crt_flag = "/MTd" if resolved_lib_build_type == 'debug' else '/MT'
    extra_compile_args.extend([crt_flag, "/EHsc", "/D_WIN32", "/DUNICODE", "/D_UNICODE", "/W3"])

    link_libraries.append('AppGuard')
    extra_link_args_platform.extend(["User32.lib", "Ole32.lib"]) 

elif current_platform == "macos":
    extra_compile_args.extend(["-fvisibility=hidden", "-Wall", "-Wextra", "-DMACOS", "-D__MACOSX_CORE__", "-mmacosx-version-min=10.15"])

    link_libraries.append('AppGuard')
    extra_link_args_platform.extend(["-framework", "CoreFoundation", "-framework", "ApplicationServices"])

else:
    extra_compile_args.extend(["-fvisibility=hidden", "-Wall", "-Wextra", "-DLINUX"])
    link_libraries.append('AppGuard')

    extra_link_args_platform.extend(["-lpthread", "-lrt", "-lX11"])


ext_modules = [
    Extension(
        name="app_guard.AppGuard",
        sources=sources,
        include_dirs=[
            os.path.abspath("./include/"), 
            pybind11.get_include(),
        ],
        library_dirs=library_dirs_list,
        libraries=link_libraries,
        extra_compile_args=extra_compile_args,
        extra_link_args=extra_link_args_platform,
        language="c++"
    )
]

setup(
    name="AppGuard",
    version="1.0.2",
    author="still-standing88",
    url="https://github.com/still-standing88/app-guard",
    description="Python wrapper for the AppGuard library (pybind11)",
    long_description=get_readme(),
    long_description_content_type="text/markdown",
    license="MIT", 
    packages=find_packages(where=".", include=["app_guard*"]),
    ext_modules=ext_modules,
    classifiers=[
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
        "License :: OSI Approved :: MIT License",
        "Operating System :: Microsoft :: Windows",
        "Operating System :: MacOS :: MacOS X",
        "Operating System :: POSIX :: Linux",
    ],
    python_requires=">=3.8",
    setup_requires=['pybind11>=2.6'], 
    install_requires=[],
    zip_safe=False,
)
