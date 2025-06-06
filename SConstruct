import platform as pf
import os, sys, shutil
import subprocess

from SCons.Script import Environment, Exit, Builder, Action, Configure
from SCons.Script import Variables, BoolVariable, EnumVariable, ARGUMENTS

import sysconfig 

def get_platform():
    if sys.platform == 'win32': return 'win32'
    if sys.platform == 'darwin': return 'darwin'
    return 'linux'

def platform_to_str(platform_val):
    if platform_val == 'win32': return 'windows'
    if platform_val == 'darwin': return 'macos'
    return 'linux'

def detect_host_arch():
    machine = pf.machine().lower()
    if machine in ['x86_64', 'amd64']: return 'x86_64'
    if machine in ['i386', 'i686']: return 'x86'
    if machine in ['arm64', 'aarch64']: return 'arm64'
    print(f"Warning: Unrecognized architecture '{machine}'. Defaulting to x86_64.")
    return 'x86_64'

def detect_compiler_from_env(env_obj, platform_str):
    compiler_arg = env_obj.get('compiler_arg', 'default')
    if compiler_arg == 'msvc': return 'msvc'
    if compiler_arg == 'clang': return 'clang'
    if compiler_arg == 'gcc': return 'gcc'
    cc_tool = str(env_obj.get('CC', '')).lower()
    if 'cl' in cc_tool: return 'msvc'
    if platform_str == 'macos': return 'clang'
    if 'clang' in cc_tool: return 'clang'
    return 'gcc'

def build_cleanup_action(target, source, env_obj):
    build_dir = os.path.join(os.getcwd(), 'build')
    if env_obj.get('cleanup_scons_build_dir', False) and os.path.exists(build_dir):
        print(f"INFO: Cleaning up build artifact directory: {build_dir}")
        try: shutil.rmtree(build_dir)
        except Exception as e: print(f"Error removing {build_dir}: {e}"); return 1
    return 0

def build_python_package(target, source, env):
    cwd = os.getcwd()
    python_exe = sys.executable
    try:
        env_vars = os.environ.copy()
        env_vars["PYTHON_BUILD_TYPE"] = env.get('PYTHON_BUILD_TYPE', 'release')
        print(f"INFO: Running: {python_exe} setup.py build_ext --inplace")
        result = subprocess.run([python_exe, "setup.py", "build_ext", "--inplace"],
                              capture_output=True, text=True, cwd=cwd, env=env_vars)
        print("--- setup.py build_ext STDOUT ---"); print(result.stdout if result.stdout else "<no stdout>")
        print("--- setup.py build_ext STDERR ---"); print(result.stderr if result.stderr else "<no stderr>")
        if result.returncode != 0:
            print(f"ERROR: Python extension build failed (setup.py error code {result.returncode}).")
            return 1
        if not os.path.exists(target[0].path):
            print(f"ERROR: Expected Python extension target file {target[0].path} was NOT created by setup.py.")
            guard_dir = os.path.join(cwd, os.path.dirname(target[0].path))
            if os.path.exists(guard_dir): print(f"Files in {guard_dir}: {os.listdir(guard_dir)}")
            return 1
        if env.get('PYTHON_BUILD_WHEEL', False):
            print(f"INFO: Running: {python_exe} setup.py bdist_wheel")
            wheel_result = subprocess.run([python_exe, "setup.py", "bdist_wheel"],
                                        capture_output=True, text=True, cwd=cwd, env=env_vars)
            print("--- setup.py bdist_wheel STDOUT ---"); print(wheel_result.stdout if wheel_result.stdout else "<no stdout>")
            print("--- setup.py bdist_wheel STDERR ---"); print(wheel_result.stderr if wheel_result.stderr else "<no stderr>")
            if wheel_result.returncode != 0:
                print(f"ERROR: Wheel build failed (setup.py error code {wheel_result.returncode}).")
                return 1
        print("INFO: Python package action completed successfully.")
        return 0
    except Exception as e: print(f"ERROR: Exception during Python package build: {e}"); return 1


host_arch = detect_host_arch()
vars = Variables()
vars.Add(EnumVariable('build_mode', 'Build mode for main C++ library (static/shared)', 'static', ('static', 'shared')))
vars.Add(EnumVariable('build_type', 'Build type (debug/release)', 'release', ('debug', 'release')))
vars.Add(EnumVariable('arch', 'Target architecture', host_arch, ('x86_64', 'arm64', 'x86')))
vars.Add('compiler_arg', 'Compiler (msvc, clang, gcc, default)', 'default')
vars.Add(BoolVariable('cleanup_scons_build_dir', 'Delete build intermediates directory.', False))
vars.Add(BoolVariable('build_python', 'Build Python package', True))
vars.Add(BoolVariable('python_wheel', 'Create Python wheel', False))

init_env = Environment(variables=vars, PLATFORM=get_platform())
conf = Configure(init_env)

platform_name = platform_to_str(conf.env['PLATFORM'])
target_arch = conf.env.get('arch')
compiler = detect_compiler_from_env(conf.env, platform_name)
build_mode = conf.env.get('build_mode')
build_type = conf.env.get('build_type') 
build_python = conf.env.get('build_python')

print(f"Platform: {platform_name}, Arch: {target_arch}, Compiler: {compiler}, Build Type: {build_type}")
print(f"Main C++ Lib Mode: {build_mode}, Build Python: {build_python}")

conf.env.Append(CXXFLAGS=['/std:c++17'] if compiler == 'msvc' else ['-std=c++17'])
if compiler == 'msvc':
    conf.env.Append(CXXFLAGS=['/EHsc', '/W3'], CPPDEFINES=['_CRT_SECURE_NO_WARNINGS', '_WINSOCK_DEPRECATED_NO_WARNINGS'])
    conf.env.Append(CCFLAGS=['/MTd'] if build_type == 'debug' else ['/MT'])  
elif compiler == 'clang' or compiler == 'gcc':
    conf.env.Append(CXXFLAGS=['-Wall', '-Wextra', '-Wno-unused-parameter', '-Wno-deprecated-declarations'])

if build_type == 'debug':
    conf.env.Append(CPPDEFINES=['DEBUG', '_DEBUG']) 
    if compiler == 'msvc': conf.env.Append(CCFLAGS=['/Zi', '/Od'], LINKFLAGS=['/DEBUG'])
    else: conf.env.Append(CXXFLAGS=['-g', '-O0'])
else: 
    conf.env.Append(CPPDEFINES=['NDEBUG'])
    if compiler == 'msvc': conf.env.Append(CCFLAGS=['/O2', '/GL'], LINKFLAGS=['/LTCG'])
    else: conf.env.Append(CXXFLAGS=['-O2'])
conf.env.Append(CPPDEFINES=['UNICODE', '_UNICODE'])

platform_libs = [] 
if platform_name == 'windows':
    if '64' in target_arch: conf.env.Append(CPPDEFINES=['_WIN64'])
    conf.env.Append(CPPDEFINES=['WIN32', '_WIN32_WINNT=0x0601']) 
    platform_libs.extend(['User32', 'Ole32'])
elif platform_name == 'macos':
    conf.env.Append(CPPDEFINES=['MACOS', '__MACOSX_CORE__'])
    conf.env.Append(FRAMEWORKS=['CoreFoundation', 'ApplicationServices'])
    platform_libs.append('c')
elif platform_name == 'linux':
    conf.env.Append(CPPDEFINES=['LINUX'])
    conf.env.Append(LIBPATH=['/usr/lib', '/usr/lib64', '/usr/lib/x86_64-linux-gnu', '/lib/x86_64-linux-gnu'])
    platform_libs.extend(['pthread'])
    if conf.CheckLibWithHeader('X11', ['X11/Xlib.h', 'X11/Xatom.h', 'X11/Xutil.h'], 'c'):
        print("SCONS_INFO: X11 dev files FOUND. Defining APP_HAS_X11_SUPPORT_LNX_UTIL=1 for C++ and SCons will link -lX11.")
        conf.env.Append(CPPDEFINES=['APP_HAS_X11_SUPPORT_LNX_UTIL=1']) 
        platform_libs.append('X11')
    else:
        print("SCONS_INFO: X11 dev files NOT FOUND. Defining APP_HAS_X11_SUPPORT_LNX_UTIL=0 for C++.")
        conf.env.Append(CPPDEFINES=['APP_HAS_X11_SUPPORT_LNX_UTIL=0'])
conf.env.Append(LIBS=platform_libs)

if platform_name == 'windows':
    if target_arch == 'x86_64': conf.env.Append(LINKFLAGS=['/MACHINE:X64'])
elif platform_name == 'macos':
    if target_arch == 'arm64': conf.env.Append(CCFLAGS=['-arch', 'arm64'], LINKFLAGS=['-arch', 'arm64'])
    elif target_arch == 'x86_64': conf.env.Append(CCFLAGS=['-arch', 'x86_64'], LINKFLAGS=['-arch', 'x86_64'])
    conf.env.Append(CCFLAGS=['-mmacosx-version-min=10.15'], LINKFLAGS=['-mmacosx-version-min=10.15'])
elif platform_name == 'linux':
    if target_arch == 'x86_64': conf.env.Append(CCFLAGS=['-march=x86-64'])

env = conf.Finish()
inc_dir = os.path.join(os.getcwd(), 'include'); src_dir = os.path.join(os.getcwd(), 'src')
env.Replace(CPPPATH=[inc_dir, src_dir]) 

build_root = 'build'; obj_frag = os.path.join(platform_name, target_arch, build_type)
lib_name = 'AppGuard'
main_lib_out = os.path.join('lib', platform_name, target_arch, build_type, build_mode)
main_lib_obj = os.path.join(build_root, obj_frag, build_mode, 'appguard_lib_main_obj')

bin_out = os.path.join('bin', platform_name, target_arch, build_type) 
test_exe_guard_obj = os.path.join(build_root, obj_frag, 'test_exe_appguard_obj')
test_exe_example_obj = os.path.join(build_root, obj_frag, 'test_exe_example_obj')
py_static_out = os.path.join('lib', platform_name, target_arch, build_type, 'static_for_python')
py_static_obj = os.path.join(build_root, obj_frag, 'static_for_python_obj')

for path in [main_lib_out, bin_out, py_static_out, main_lib_obj, test_exe_guard_obj, test_exe_example_obj, py_static_obj]:
    if not os.path.exists(path): os.makedirs(path, exist_ok=True)

lib_src_files = ['AppGuard.cpp', 'AppInstance.cpp', 'IPCWatcher.cpp', 'PlatformIPCWatcher.cpp', 'utils.cpp']

env_main = env.Clone()
main_lib_objs = []
is_shared_posix = (build_mode == 'shared' and (platform_name == 'linux' or platform_name == 'macos'))

if build_mode == 'static':
    env_main.AppendUnique(CPPDEFINES=['APPGUARD_STATIC'])
    if platform_name == 'linux' or platform_name == 'macos':
        if not any('-fPIC' in str(f) for f in env_main.get('CCFLAGS',[])): env_main.Append(CCFLAGS=['-fPIC'])
else:
    env_main.AppendUnique(CPPDEFINES=['APPGUARD_EXPORTS'])
    if platform_name == 'windows': env_main.Append(LINKFLAGS=['/DLL'])
    elif is_shared_posix:
        if not any('-fPIC' in str(f) for f in env_main.get('CCFLAGS',[])): env_main.Append(CCFLAGS=['-fPIC'])
        if compiler == 'clang' or compiler == 'gcc': env_main.Append(CXXFLAGS=['-fvisibility=hidden', '-fvisibility-inlines-hidden'])

for src in lib_src_files:
    obj_path = os.path.join(main_lib_obj, src.replace('.cpp', env_main.subst('$OBJSUFFIX')))
    src_path = os.path.join(src_dir, src)
    if is_shared_posix:
        main_lib_objs.append(env_main.SharedObject(target=obj_path, source=src_path))
    else:
        main_lib_objs.append(env_main.Object(target=obj_path, source=src_path))

main_lib_node = None
if build_mode == 'static':
    main_lib_node = env_main.StaticLibrary(target=os.path.join(main_lib_out, lib_name), source=main_lib_objs)
else:
    shared_name = env_main.subst('$SHLIBPREFIX') + lib_name + env_main.subst('$SHLIBSUFFIX') if (platform_name == 'linux' or platform_name == 'macos') else lib_name + env_main.subst('$SHLIBSUFFIX')
    main_lib_node = env_main.SharedLibrary(target=os.path.join(main_lib_out, shared_name), source=main_lib_objs)

env_test = env.Clone()
if platform_name == 'linux' or platform_name == 'macos':
    ccflags = list(env_test.get('CCFLAGS', []))
    env_test.Replace(CCFLAGS=[f for f in ccflags if '-fPIC' not in str(f)])
env_test.AppendUnique(CPPDEFINES=['APPGUARD_STATIC'])
if compiler == 'msvc': 
    ccf=env_test.get('CCFLAGS',[]); fcf=[f for f in ccf if not f.startswith('/MD')]; env_test.Replace(CCFLAGS=fcf)
    if build_type=='debug':env_test.Append(CCFLAGS=['/MTd'])
    else:env_test.Append(CCFLAGS=['/MT'])
test_guard_objs = [env_test.Object(target=os.path.join(test_exe_guard_obj, s.replace('.cpp',env_test['OBJSUFFIX'])),source=os.path.join(src_dir,s)) for s in lib_src_files]
test_example_obj = env_test.Object(target=os.path.join(test_exe_example_obj,'test'+env_test['OBJSUFFIX']),source=os.path.join(os.getcwd(),'examples','test.cpp'))
test_exe_node = env_test.Program(target=os.path.join(bin_out,'AppGuardTest'), source=test_guard_objs + [test_example_obj])

py_ext_nodes = []
py_static_node = None
if build_python:
    env_py = env.Clone()
    env_py.AppendUnique(CPPDEFINES=['APPGUARD_STATIC'])
    if platform_name == 'linux' or platform_name == 'macos':
        if not any('-fPIC' in str(f) for f in env_py.get('CCFLAGS',[])): env_py.Append(CCFLAGS=['-fPIC'])
    print(f"SCONS_INFO: env_py CCFLAGS (for Python's static lib): {env_py.get('CCFLAGS')}")
    py_static_objs = [env_py.Object(target=os.path.join(py_static_obj,s.replace('.cpp',env_py['OBJSUFFIX'])),source=os.path.join(src_dir,s)) for s in lib_src_files]
    py_static_node = env_py.StaticLibrary(
        target=os.path.join(py_static_out, lib_name), 
        source=py_static_objs
    )
    env_setup = env.Clone()
    env_setup['PYTHON_BUILD_TYPE'] = build_type
    mod_path="app_guard.AppGuard";parts=mod_path.split('.');base_name=parts[-1];pkg_dir=parts[0]
    ext_suffix=sysconfig.get_config_var('EXT_SUFFIX')
    py_ext_name = f"{base_name}{ext_suffix if ext_suffix else ('.pyd' if platform_name == 'windows' else '.so')}"
    py_ext_path = os.path.join(pkg_dir, py_ext_name)
    py_build_node = env_setup.Command(
        target=py_ext_path, 
        source=[py_static_node],
        action=Action(build_python_package, "Building Python package via setup.py...")
    )
    py_ext_nodes.append(py_build_node)
    env.Requires(py_build_node, py_static_node)

if env.get('cleanup_scons_build_dir', False) and main_lib_node:
    env.AddPostAction(main_lib_node, build_cleanup_action)
targets = []
if main_lib_node: targets.append(main_lib_node)
if test_exe_node: targets.append(test_exe_node)
if build_python: targets.extend(py_ext_nodes) 
Default(targets)

print("--- Build Summary ---")
if main_lib_node: print(f"Main C++ Library: {main_lib_node[0].path}")
if test_exe_node: print(f"Test Executable: {test_exe_node[0].path}")
if build_python:
    if py_static_node: print(f"Static Lib for Python: {py_static_node[0].path}")
    if py_ext_nodes and py_ext_nodes[0] and len(py_ext_nodes[0]) > 0: 
        print(f"Python Extension: {py_ext_nodes[0][0].path}") 
    else: print(f"Python Extension: [Node not available/built or error in SCons target setup]")