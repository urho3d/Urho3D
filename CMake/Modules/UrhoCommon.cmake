#
# Copyright (c) 2008-2020 the Urho3D project.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

# Save the initial values of CC and CXX environment variables
if (NOT CMAKE_CROSSCOMPILING)
    set (SAVED_CC $ENV{CC} CACHE INTERNAL "Initial value for CC")
    set (SAVED_CXX $ENV{CXX} CACHE INTERNAL "Initial value for CXX")
endif ()

# Limit the supported build configurations
set (URHO3D_BUILD_CONFIGURATIONS Release RelWithDebInfo Debug)
set (DOC_STRING "Specify CMake build configuration (single-configuration generator only), possible values are Release (default), RelWithDebInfo, and Debug")
if (CMAKE_CONFIGURATION_TYPES)
    # For multi-configurations generator, such as VS and Xcode
    set (CMAKE_CONFIGURATION_TYPES ${URHO3D_BUILD_CONFIGURATIONS} CACHE STRING ${DOC_STRING} FORCE)
    unset (CMAKE_BUILD_TYPE)
else ()
    # For single-configuration generator, such as Unix Makefile generator
    if (CMAKE_BUILD_TYPE STREQUAL "")
        # If not specified then default to Release
        set (CMAKE_BUILD_TYPE Release)
    endif ()
    set (CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING ${DOC_STRING} FORCE)
endif ()

# Define other useful variables not defined by CMake
if (CMAKE_GENERATOR STREQUAL Xcode)
    set (XCODE TRUE)
elseif (CMAKE_GENERATOR STREQUAL Ninja)
    set (NINJA TRUE)
elseif (CMAKE_GENERATOR MATCHES Visual)
    set (VS TRUE)
endif ()

# Rightfully we could have performed this inside a CMake/iOS toolchain file but we don't have one nor need for one for now
if (IOS)
    set (CMAKE_CROSSCOMPILING TRUE)
    set (CMAKE_XCODE_EFFECTIVE_PLATFORMS -iphoneos -iphonesimulator)
    set (CMAKE_OSX_SYSROOT iphoneos)    # Set Base SDK to "Latest iOS"
    # This is a CMake hack in order to make standard CMake check modules that use try_compile() internally work on iOS platform
    # The injected "flags" are not compiler flags, they are actually CMake variables meant for another CMake subprocess that builds the source file being passed in the try_compile() command
    # CAVEAT: these injected "flags" must always be kept at the end of the string variable, i.e. when adding more compiler flags later on then those new flags must be prepended in front of these flags instead
    set (CMAKE_REQUIRED_FLAGS ";-DSmileyHack=byYaoWT;-DCMAKE_MACOSX_BUNDLE=1;-DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED=0;-DCMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY=")
    if (NOT IOS_SYSROOT)
        execute_process (COMMAND xcodebuild -version -sdk ${CMAKE_OSX_SYSROOT} Path OUTPUT_VARIABLE IOS_SYSROOT OUTPUT_STRIP_TRAILING_WHITESPACE)   # Obtain iOS sysroot path
        set (IOS_SYSROOT ${IOS_SYSROOT} CACHE INTERNAL "Path to iOS system root")
    endif ()
    set (CMAKE_FIND_ROOT_PATH ${IOS_SYSROOT})
    set (IPHONEOS_DEPLOYMENT_TARGET "" CACHE STRING "Specify iOS deployment target (iOS platform only); default to latest installed iOS SDK if not specified, the minimum supported target is 3.0 due to constraint from SDL library")
    if (DEPLOYMENT_TARGET_SAVED AND NOT ${IPHONEOS_DEPLOYMENT_TARGET}: STREQUAL DEPLOYMENT_TARGET_SAVED)
        string (REPLACE : "" DEPLOYMENT_TARGET_SAVED ${DEPLOYMENT_TARGET_SAVED})
        set (IPHONEOS_DEPLOYMENT_TARGET "${DEPLOYMENT_TARGET_SAVED}" CACHE STRING "Specify iOS deployment target (iOS platform only); default to latest installed iOS SDK if not specified, the minimum supported target is 3.0 due to constraint from SDL library" FORCE)
        message (FATAL_ERROR "IPHONEOS_DEPLOYMENT_TARGET cannot be changed after the initial configuration/generation. "
            "Auto reverting to its initial value. If you wish to change it then the build tree would have to be regenerated from scratch.")
    endif ()
    set (CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET ${IPHONEOS_DEPLOYMENT_TARGET})
    set (DEPLOYMENT_TARGET_SAVED ${IPHONEOS_DEPLOYMENT_TARGET}: CACHE INTERNAL "Last known deployment target")    # with sentinel so it does not appear empty even when the default target is used
    set (CMAKE_XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES)
    # Workaround what appears to be a bug in CMake/Xcode generator, ensure the CMAKE_OSX_DEPLOYMENT_TARGET is set to empty for iOS build
    set (CMAKE_OSX_DEPLOYMENT_TARGET)
    unset (CMAKE_OSX_DEPLOYMENT_TARGET CACHE)
elseif (TVOS)
    set (CMAKE_CROSSCOMPILING TRUE)
    set (CMAKE_XCODE_EFFECTIVE_PLATFORMS -appletvos -appletvsimulator)
    set (CMAKE_OSX_SYSROOT appletvos)    # Set Base SDK to "Latest tvOS"
    set (CMAKE_REQUIRED_FLAGS ";-DSmileyHack=byYaoWT;-DCMAKE_MACOSX_BUNDLE=1;-DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED=0;-DCMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY=")
    if (NOT TVOS_SYSROOT)
        execute_process (COMMAND xcodebuild -version -sdk ${CMAKE_OSX_SYSROOT} Path OUTPUT_VARIABLE TVOS_SYSROOT OUTPUT_STRIP_TRAILING_WHITESPACE)   # Obtain tvOS sysroot path
        set (TVOS_SYSROOT ${TVOS_SYSROOT} CACHE INTERNAL "Path to tvOS system root")
    endif ()
    set (CMAKE_FIND_ROOT_PATH ${TVOS_SYSROOT})
    set (APPLETVOS_DEPLOYMENT_TARGET "" CACHE STRING "Specify AppleTV OS deployment target (tvOS platform only); default to latest installed tvOS SDK if not specified, the minimum supported target is 9.0")
    set (CMAKE_XCODE_ATTRIBUTE_APPLETVOS_DEPLOYMENT_TARGET ${APPLETVOS_DEPLOYMENT_TARGET})
    set (CMAKE_XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES)
    # Just in case it has similar bug for tvOS build
    set (CMAKE_OSX_DEPLOYMENT_TARGET)
    unset (CMAKE_OSX_DEPLOYMENT_TARGET CACHE)
elseif (XCODE)
    set (CMAKE_OSX_SYSROOT macosx)    # Set Base SDK to "Latest OS X"
    if (CMAKE_OSX_DEPLOYMENT_TARGET)
        if (CMAKE_OSX_DEPLOYMENT_TARGET VERSION_LESS 10.9)
            message (FATAL_ERROR "The minimum supported CMAKE_OSX_DEPLOYMENT_TARGET is 10.9.")
        endif ()
    else ()
        # If not set, set to current running build system OS version by default
        execute_process (COMMAND sw_vers -productVersion OUTPUT_VARIABLE CURRENT_OSX_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
        string (REGEX REPLACE ^\([^.]+\\.[^.]+\).* \\1 CMAKE_OSX_DEPLOYMENT_TARGET ${CURRENT_OSX_VERSION})
        set (CMAKE_OSX_DEPLOYMENT_TARGET ${CMAKE_OSX_DEPLOYMENT_TARGET} CACHE STRING "Specify macOS deployment target (macOS platform only); default to current running macOS if not specified, the minimum supported target is 10.9")
    endif ()
    if (DEPLOYMENT_TARGET_SAVED AND NOT CMAKE_OSX_DEPLOYMENT_TARGET STREQUAL DEPLOYMENT_TARGET_SAVED)
        set (CMAKE_OSX_DEPLOYMENT_TARGET ${DEPLOYMENT_TARGET_SAVED} CACHE STRING "Specify macOS deployment target (macOS platform only); default to current running macOS if not specified, the minimum supported target is 10.9" FORCE)
        message (FATAL_ERROR "CMAKE_OSX_DEPLOYMENT_TARGET cannot be changed after the initial configuration/generation. "
            "Auto reverting to its initial value. If you wish to change it then the build tree would have to be regenerated from scratch.")
    endif ()
    set (DEPLOYMENT_TARGET_SAVED ${CMAKE_OSX_DEPLOYMENT_TARGET} CACHE INTERNAL "Last known deployment target")
endif ()

# To support Android Gradle build system on Windows host system
if (ANDROID AND GRADLE_BUILD_DIR)
    file (TO_CMAKE_PATH "${GRADLE_BUILD_DIR}" GRADLE_BUILD_DIR)
endif ()

include (CheckHost)
include (CheckCompilerToolchain)

# Extra linker flags for linking against indirect dependencies (linking shared lib with dependencies)
if (RPI)
    # Extra linker flags for Raspbian because it installs VideoCore libraries in the "/opt/vc/lib" directory (no harm in doing so for other distros)
    set (INDIRECT_DEPS_EXE_LINKER_FLAGS "${INDIRECT_DEPS_EXE_LINKER_FLAGS} -Wl,-rpath-link,\"${CMAKE_SYSROOT}/opt/vc/lib\"")      # CMAKE_SYSROOT is empty when not cross-compiling
elseif (APPLE AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.0.0)
    set (INDIRECT_DEPS_EXE_LINKER_FLAGS "${INDIRECT_DEPS_EXE_LINKER_FLAGS} -Wl,-no_weak_imports")
endif ()
if (ARM AND CMAKE_SYSTEM_NAME STREQUAL Linux AND CMAKE_CROSSCOMPILING)
    # Cannot do this in the toolchain file because CMAKE_LIBRARY_ARCHITECTURE is not yet defined when CMake is processing toolchain file
    set (INDIRECT_DEPS_EXE_LINKER_FLAGS "${INDIRECT_DEPS_EXE_LINKER_FLAGS} -Wl,-rpath-link,\"${CMAKE_SYSROOT}/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}\":\"${CMAKE_SYSROOT}/lib/${CMAKE_LIBRARY_ARCHITECTURE}\"")
endif ()
set (CMAKE_REQUIRED_FLAGS "${INDIRECT_DEPS_EXE_LINKER_FLAGS} ${CMAKE_REQUIRED_FLAGS}")
set (CMAKE_EXE_LINKER_FLAGS "${INDIRECT_DEPS_EXE_LINKER_FLAGS} ${CMAKE_EXE_LINKER_FLAGS}")

# Define all supported build options
include (CMakeDependentOption)
cmake_dependent_option (IOS "Setup build for iOS platform" FALSE "XCODE" FALSE)
cmake_dependent_option (TVOS "Setup build for tvOS platform" FALSE "XCODE" FALSE)
cmake_dependent_option (URHO3D_64BIT "Enable 64-bit build, the default is set based on the native ABI of the chosen compiler toolchain" "${NATIVE_64BIT}" "NOT MSVC AND NOT ANDROID AND NOT (ARM AND NOT IOS) AND NOT WEB AND NOT POWERPC" "${NATIVE_64BIT}")     # Intentionally only enable the option for iOS but not for tvOS as the latter is 64-bit only
option (URHO3D_ANGELSCRIPT "Enable AngelScript scripting support" TRUE)
option (URHO3D_IK "Enable inverse kinematics support" TRUE)
option (URHO3D_LUA "Enable additional Lua scripting support" TRUE)
option (URHO3D_NAVIGATION "Enable navigation support" TRUE)
cmake_dependent_option (URHO3D_NETWORK "Enable networking support" TRUE "NOT WEB" FALSE)
option (URHO3D_PHYSICS "Enable physics support" TRUE)
option (URHO3D_URHO2D "Enable 2D graphics and physics support" TRUE)
option (URHO3D_GLES3 "Enable GLES3" FALSE)
option (URHO3D_WEBP "Enable WebP support" TRUE)
if (ARM AND NOT ANDROID AND NOT RPI AND NOT APPLE)
    set (ARM_ABI_FLAGS "" CACHE STRING "Specify ABI compiler flags (ARM on Linux platform only); e.g. Orange-Pi Mini 2 could use '-mcpu=cortex-a7 -mfpu=neon-vfpv4'")
endif ()
if ((RPI AND "${RPI_ABI}" MATCHES NEON) OR (ARM AND (APPLE OR URHO3D_64BIT OR "${ARM_ABI_FLAGS}" MATCHES neon)))    # Stringify in case RPI_ABI/ARM_ABI_FLAGS is not set explicitly
    # TODO: remove this logic when the compiler flags are set in each toolchain file, such that the CheckCompilerToolchain can perform the check automatically
    set (NEON 1)
endif ()
# For Raspbery Pi, find Broadcom VideoCore IV firmware
if (RPI)
    # TODO: this logic is earmarked to be moved into SDL's CMakeLists.txt when refactoring the library dependency handling
    find_package (VideoCore REQUIRED)
    include_directories (SYSTEM ${VIDEOCORE_INCLUDE_DIRS})
    link_directories (${VIDEOCORE_LIBRARY_DIRS})
endif ()
if (CMAKE_PROJECT_NAME STREQUAL Urho3D)
    set (URHO3D_LIB_TYPE STATIC CACHE STRING "Specify Urho3D library type, possible values are STATIC (default) and SHARED (not available for Emscripten)")
    # Non-Windows platforms always use OpenGL, the URHO3D_OPENGL variable will always be forced to TRUE, i.e. it is not an option at all
    # Windows platform has URHO3D_OPENGL as an option, MSVC compiler default to FALSE (i.e. prefers Direct3D) while MinGW compiler default to TRUE
    if (MINGW)
        set (DEFAULT_OPENGL TRUE)
    endif ()
    cmake_dependent_option (URHO3D_OPENGL "Use OpenGL instead of Direct3D (Windows platform only)" "${DEFAULT_OPENGL}" WIN32 TRUE)
    # On Windows platform Direct3D11 can be optionally chosen
    # Using Direct3D11 on non-MSVC compiler may require copying and renaming Microsoft official libraries (.lib to .a), else link failures or non-functioning graphics may result
    cmake_dependent_option (URHO3D_D3D11 "Use Direct3D11 instead of Direct3D9 (Windows platform only); overrides URHO3D_OPENGL option" FALSE "WIN32" FALSE)
    if (X86 OR WEB)
        # TODO: Rename URHO3D_SSE to URHO3D_SIMD
        if (MINGW AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9.1)
            if (NOT DEFINED URHO3D_SSE)     # Only give the warning once during initial configuration
                # Certain MinGW versions fail to compile SSE code. This is the initial guess for known "bad" version range, and can be tightened later
                message (WARNING "Disabling SSE by default due to MinGW version. It is recommended to upgrade to MinGW with GCC >= 4.9.1. "
                    "You can also try to re-enable SSE with CMake option -DURHO3D_SSE=1, but this may result in compile errors.")
            endif ()
            set (URHO3D_DEFAULT_SIMD FALSE)
        else ()
            set (URHO3D_DEFAULT_SIMD ${HAVE_SSE})
        endif ()
        # It is not possible to turn SSE off on 64-bit MSVC and it appears it is also not able to do so safely on 64-bit GCC
        cmake_dependent_option (URHO3D_SSE "Enable SIMD instruction set (32-bit Web and Intel platforms only, including Android on Intel Atom); default to true on Intel and false on Web platform; the effective SSE level could be higher, see also URHO3D_DEPLOYMENT_TARGET and CMAKE_OSX_DEPLOYMENT_TARGET build options" "${URHO3D_DEFAULT_SIMD}" "NOT URHO3D_64BIT" TRUE)
    endif ()
    cmake_dependent_option (URHO3D_HASH_DEBUG "Enable StringHash reversing and hash collision detection at the expense of memory and performance penalty" FALSE "NOT CMAKE_BUILD_TYPE STREQUAL Release" FALSE)
    cmake_dependent_option (URHO3D_3DNOW "Enable 3DNow! instruction set (Linux platform only); should only be used for older CPU with (legacy) 3DNow! support" "${HAVE_3DNOW}" "X86 AND CMAKE_SYSTEM_NAME STREQUAL Linux AND NOT URHO3D_SSE" FALSE)
    cmake_dependent_option (URHO3D_MMX "Enable MMX instruction set (32-bit Linux platform only); the MMX is effectively enabled when 3DNow! or SSE is enabled; should only be used for older CPU with MMX support" "${HAVE_MMX}" "X86 AND CMAKE_SYSTEM_NAME STREQUAL Linux AND NOT URHO3D_64BIT AND NOT URHO3D_SSE AND NOT URHO3D_3DNOW" FALSE)
    # For completeness sake - this option is intentionally not documented as we do not officially support PowerPC (yet)
    cmake_dependent_option (URHO3D_ALTIVEC "Enable AltiVec instruction set (PowerPC only)" "${HAVE_ALTIVEC}" POWERPC FALSE)
    cmake_dependent_option (URHO3D_LUAJIT "Enable Lua scripting support using LuaJIT (check LuaJIT's CMakeLists.txt for more options)" FALSE "NOT WEB" FALSE)
    cmake_dependent_option (URHO3D_LUAJIT_AMALG "Enable LuaJIT amalgamated build (LuaJIT only); default to true when LuaJIT is enabled" TRUE URHO3D_LUAJIT FALSE)
    cmake_dependent_option (URHO3D_SAFE_LUA "Enable Lua C++ wrapper safety checks (Lua/LuaJIT only)" FALSE URHO3D_LUA FALSE)
    if (NOT CMAKE_BUILD_TYPE STREQUAL Release AND NOT CMAKE_CONFIGURATION_TYPES)
        set (DEFAULT_LUA_RAW TRUE)
    endif ()
    cmake_dependent_option (URHO3D_LUA_RAW_SCRIPT_LOADER "Prefer loading raw script files from the file system before falling back on Urho3D resource cache. Useful for debugging (e.g. breakpoints), but less performant (Lua/LuaJIT only)" "${DEFAULT_LUA_RAW}" URHO3D_LUA FALSE)
    option (URHO3D_PLAYER "Build Urho3D script player" TRUE)
    option (URHO3D_SAMPLES "Build sample applications" TRUE)
    option (URHO3D_UPDATE_SOURCE_TREE "Enable commands to copy back some of the generated build artifacts from build tree to source tree to facilitate devs to push them as part of a commit (for library devs with push right only)")
    option (URHO3D_BINDINGS "Enable API binding generation support for script subsystems")
    cmake_dependent_option (URHO3D_CLANG_TOOLS "Build Clang tools (native on host system only)" FALSE "NOT CMAKE_CROSSCOMPILING" FALSE)
    mark_as_advanced (URHO3D_UPDATE_SOURCE_TREE URHO3D_BINDINGS URHO3D_CLANG_TOOLS)
    cmake_dependent_option (URHO3D_TOOLS "Build tools (native, RPI, and ARM on Linux only)" TRUE "NOT IOS AND NOT TVOS AND NOT ANDROID AND NOT WEB" FALSE)
    cmake_dependent_option (URHO3D_EXTRAS "Build extras (native, RPI, and ARM on Linux only)" FALSE "NOT IOS AND NOT TVOS AND NOT ANDROID AND NOT WEB" FALSE)
    option (URHO3D_DOCS "Generate documentation as part of normal build")
    option (URHO3D_DOCS_QUIET "Generate documentation as part of normal build, suppress generation process from sending anything to stdout")
    option (URHO3D_PCH "Enable PCH support" TRUE)
    cmake_dependent_option (URHO3D_DATABASE_ODBC "Enable Database support with ODBC, requires vendor-specific ODBC driver" FALSE "NOT IOS AND NOT TVOS AND NOT ANDROID AND NOT WEB;NOT MSVC OR NOT MSVC_VERSION VERSION_LESS 1900" FALSE)
    option (URHO3D_DATABASE_SQLITE "Enable Database support with SQLite embedded")
    # Enable file watcher support for automatic resource reloads by default.
    option (URHO3D_FILEWATCHER "Enable filewatcher support" TRUE)
    option (URHO3D_TESTING "Enable testing support")
    # By default this option is off (i.e. we use the MSVC dynamic runtime), this can be switched on if using Urho3D as a STATIC library
    cmake_dependent_option (URHO3D_STATIC_RUNTIME "Use static C/C++ runtime libraries and eliminate the need for runtime DLLs installation (VS only)" FALSE "MSVC" FALSE)
    if (((URHO3D_LUA AND NOT URHO3D_LUAJIT) OR URHO3D_DATABASE_SQLITE) AND NOT ANDROID AND NOT IOS AND NOT TVOS AND NOT WEB AND NOT WIN32)
        # Find GNU Readline development library for Lua interpreter and SQLite's isql
        find_package (Readline)
    endif ()
    if (CPACK_SYSTEM_NAME STREQUAL Linux)
        cmake_dependent_option (URHO3D_USE_LIB64_RPM "Enable 64-bit RPM CPack generator using /usr/lib64 and disable all other generators (Debian-based host only)" FALSE "URHO3D_64BIT AND NOT HAS_LIB64" FALSE)
        cmake_dependent_option (URHO3D_USE_LIB_DEB "Enable 64-bit DEB CPack generator using /usr/lib and disable all other generators (Redhat-based host only)" FALSE "URHO3D_64BIT AND HAS_LIB64" FALSE)
    endif ()
    # Set to search in 'lib' or 'lib64' based on the chosen ABI
    if (NOT CMAKE_HOST_WIN32)
        set_property (GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS ${URHO3D_64BIT})
    endif ()
else ()
    set (URHO3D_LIB_TYPE "" CACHE STRING "Specify Urho3D library type, possible values are STATIC (default) and SHARED (not available for Emscripten)")
    set (URHO3D_HOME "" CACHE PATH "Path to Urho3D build tree or SDK installation location (downstream project only)")
    if (URHO3D_PCH OR URHO3D_UPDATE_SOURCE_TREE OR URHO3D_SAMPLES OR URHO3D_TOOLS)
        # Just reference it to suppress "unused variable" CMake warning on downstream projects using this CMake module
    endif ()
    if (CMAKE_PROJECT_NAME MATCHES ^Urho3D-ExternalProject-)
        set (URHO3D_SSE ${HAVE_SSE})
    else ()
        # All Urho3D downstream projects require Urho3D library, so find Urho3D library here now
        find_package (Urho3D REQUIRED)
        include_directories (${URHO3D_INCLUDE_DIRS})
    endif ()
endif ()
cmake_dependent_option (URHO3D_PACKAGING "Enable resources packaging support" FALSE "NOT WEB" TRUE)
# Enable profiling by default. If disabled, autoprofileblocks become no-ops and the Profiler subsystem is not instantiated.
option (URHO3D_PROFILING "Enable profiling support" TRUE)
# Enable logging by default. If disabled, LOGXXXX macros become no-ops and the Log subsystem is not instantiated.
option (URHO3D_LOGGING "Enable logging support" TRUE)
# Enable threading by default, except for Emscripten because its thread support is yet experimental
if (NOT WEB)
    set (THREADING_DEFAULT TRUE)
endif ()
option (URHO3D_THREADING "Enable thread support, on Web platform default to 0, on other platforms default to 1" ${THREADING_DEFAULT})
if (URHO3D_TESTING)
    if (WEB)
        set (DEFAULT_TIMEOUT 10)
        if (EMSCRIPTEN)
            set (EMSCRIPTEN_EMRUN_BROWSER firefox CACHE STRING "Specify the particular browser to be spawned by emrun during testing (Emscripten only), use 'emrun --list_browsers' command to get the list of possible values")
        endif ()
    else ()
        set (DEFAULT_TIMEOUT 5)
    endif ()
    set (URHO3D_TEST_TIMEOUT ${DEFAULT_TIMEOUT} CACHE STRING "Number of seconds to test run the executables (when testing support is enabled only), default to 10 on Web platform and 5 on other platforms")
else ()
    unset (URHO3D_TEST_TIMEOUT CACHE)
    if (EMSCRIPTEN_EMRUN_BROWSER)   # Suppress unused variable warning at the same time
        unset (EMSCRIPTEN_EMRUN_BROWSER CACHE)
    endif ()
endif ()
# Structured exception handling and minidumps on MSVC only
cmake_dependent_option (URHO3D_MINIDUMPS "Enable minidumps on crash (VS only)" TRUE "MSVC" FALSE)
# By default Windows platform setups main executable as Windows application with WinMain() as entry point
cmake_dependent_option (URHO3D_WIN32_CONSOLE "Use console main() instead of WinMain() as entry point when setting up Windows executable targets (Windows platform only)" FALSE "WIN32" FALSE)
cmake_dependent_option (URHO3D_MACOSX_BUNDLE "Use MACOSX_BUNDLE when setting up macOS executable targets (Xcode/macOS platform only)" FALSE "XCODE AND NOT ARM" FALSE)
if (CMAKE_CROSSCOMPILING AND NOT ANDROID AND NOT APPLE)
    set (URHO3D_SCP_TO_TARGET "" CACHE STRING "Use scp to transfer executables to target system (RPI and generic ARM cross-compiling build only), SSH digital key must be setup first for this to work, typical value has a pattern of usr@tgt:remote-loc")
else ()
    unset (URHO3D_SCP_TO_TARGET CACHE)
endif ()
if (MINGW AND CMAKE_CROSSCOMPILING)
    set (MINGW_PREFIX "" CACHE STRING "Prefix path to MinGW cross-compiler tools (MinGW cross-compiling build only)")
    set (MINGW_SYSROOT "" CACHE PATH "Path to MinGW system root (MinGW only); should only be used when the system root could not be auto-detected")
    # When cross-compiling then we are most probably in Unix-alike host environment which should not have problem to handle long include dirs
    # This change is required to keep ccache happy because it does not like the CMake generated include response file
    foreach (lang C CXX)
        foreach (cat OBJECTS INCLUDES)
            unset (CMAKE_${lang}_USE_RESPONSE_FILE_FOR_${cat})
        endforeach ()
    endforeach ()
endif ()
if (RPI)
    if (NOT RPI_SUPPORTED_ABIS)
        set (RPI_SUPPORTED_ABIS armeabi-v6)
        if (CMAKE_CROSSCOMPILING)
            # We have no way to know for sure so just give all the available options to user
            list (APPEND RPI_SUPPORTED_ABIS armeabi-v7a "armeabi-v7a with NEON" "armeabi-v7a with VFPV4")
        else ()
            # If not cross-compiling then we should be on the host system (device) itself, so below command is safe to be executed
            execute_process (COMMAND uname -m OUTPUT_VARIABLE HOST_MACHINE ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
            if (HOST_MACHINE MATCHES ^armv7)
                list (APPEND RPI_SUPPORTED_ABIS armeabi-v7a "armeabi-v7a with NEON" "armeabi-v7a with VFPV4")
                if (NOT RPI_ABI)
                    set (RPI_ABI armeabi-v7a)   # Set default to this specific target device
                endif ()
            endif ()
        endif ()
        set (RPI_SUPPORTED_ABIS ${RPI_SUPPORTED_ABIS} CACHE INTERNAL "Supported target ABIs for RPI build")
    endif ()
    if (CMAKE_CROSSCOMPILING)
        set (RPI_PREFIX "" CACHE STRING "Prefix path to Raspberry Pi cross-compiler tools (RPI cross-compiling build only)")
        set (RPI_SYSROOT "" CACHE PATH "Path to Raspberry Pi system root (RPI cross-compiling build only)")
    endif ()
    if (RPI_ABI)
        list (FIND RPI_SUPPORTED_ABIS ${RPI_ABI} RPI_ABI_FOUND_INDEX)
        if (RPI_ABI_FOUND_INDEX EQUAL -1)
            string (REPLACE ";" "\", \"" PRINTABLE_RPI_SUPPORTED_ABIS "${RPI_SUPPORTED_ABIS}")  # Stringify for string replace to work
            if (NOT CMAKE_CROSSCOMPILING)
                set (MSG_STR " by this Raspberry Pi device")
            endif ()
            message (FATAL_ERROR "Specified RPI_ABI = \"${RPI_ABI}\" is not supported${MSG_STR}. Supported values are: \"${PRINTABLE_RPI_SUPPORTED_ABIS}\".")
        endif ()
    else ()
        set (RPI_ABI armeabi-v6)
    endif ()
    set (RPI_ABI ${RPI_ABI} CACHE STRING "Specify target ABI (RPI platform only), possible values are armeabi-v6 (default for RPI 1), armeabi-v7a (default for RPI 2), armeabi-v7a with NEON, and armeabi-v7a with VFPV4" FORCE)
endif ()
if (EMSCRIPTEN)     # CMAKE_CROSSCOMPILING is always true for Emscripten
    set (EMSCRIPTEN_ROOT_PATH "" CACHE PATH "Root path to Emscripten cross-compiler tools (Emscripten only)")
    set (EMSCRIPTEN_SYSROOT "" CACHE PATH "Path to Emscripten system root (Emscripten only)")
    option (EMSCRIPTEN_AUTO_SHELL "Auto adding a default HTML shell-file when it is not explicitly specified (Emscripten only)" TRUE)
    option (EMSCRIPTEN_ALLOW_MEMORY_GROWTH "Enable memory growing based on application demand, default to true as there should be little or no overhead (Emscripten only)" TRUE)
    math (EXPR EMSCRIPTEN_TOTAL_MEMORY "128 * 1024 * 1024")
    set (EMSCRIPTEN_TOTAL_MEMORY ${EMSCRIPTEN_TOTAL_MEMORY} CACHE STRING "Specify the total size of memory to be used (Emscripten only); default to 128 MB, must be in multiple of 64 KB")
    option (EMSCRIPTEN_SHARE_DATA "Enable sharing data file support (Emscripten only)")
else ()
    set (SHARED SHARED)
endif ()
# Constrain the build option values in cmake-gui, if applicable
set_property (CACHE URHO3D_LIB_TYPE PROPERTY STRINGS STATIC ${SHARED})
if (NOT CMAKE_CONFIGURATION_TYPES)
    set_property (CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${URHO3D_BUILD_CONFIGURATIONS})
endif ()
if (RPI)
    set_property (CACHE RPI_ABI PROPERTY STRINGS ${RPI_SUPPORTED_ABIS})
endif ()
# Handle mutually exclusive options and implied options
if (URHO3D_D3D11)
    set (URHO3D_OPENGL 0)
    unset (URHO3D_OPENGL CACHE)
endif ()
if (URHO3D_DATABASE_ODBC)
    set (URHO3D_DATABASE_SQLITE 0)
    unset (URHO3D_DATABASE_SQLITE CACHE)
endif ()
if (URHO3D_DATABASE_SQLITE OR URHO3D_DATABASE_ODBC)
    set (URHO3D_DATABASE 1)
endif ()
if (URHO3D_LUAJIT)
    set (JIT JIT)
    set (URHO3D_LUA 1)
endif ()
if (EMSCRIPTEN)
    set (URHO3D_LIB_TYPE STATIC)
    unset (URHO3D_LIB_TYPE CACHE)
endif ()

# Union all the sysroot variables into one so it can be referred to generically later
set (SYSROOT ${CMAKE_SYSROOT} ${MINGW_SYSROOT} ${IOS_SYSROOT} ${TVOS_SYSROOT} CACHE INTERNAL "Path to system root of the cross-compiling target")  # SYSROOT is empty for native build

# Clang tools building
if (URHO3D_CLANG_TOOLS OR URHO3D_BINDINGS)
    # Ensure LLVM/Clang is installed
    find_program (LLVM_CONFIG NAMES llvm-config llvm-config-64 llvm-config-32 HINTS $ENV{LLVM_CLANG_ROOT}/bin DOC "LLVM config tool" NO_CMAKE_FIND_ROOT_PATH)
    if (NOT LLVM_CONFIG)
        message (FATAL_ERROR "Could not find LLVM/Clang installation")
    endif ()
endif ()
if (URHO3D_CLANG_TOOLS)
    # Require C++11 standard and no precompiled-header
    set (URHO3D_PCH 0)
    set (URHO3D_LIB_TYPE SHARED)
    # Set build options that would maximise the AST of Urho3D library
    foreach (OPT
            URHO3D_ANGELSCRIPT
            URHO3D_DATABASE_SQLITE
            URHO3D_FILEWATCHER
            URHO3D_IK
            URHO3D_LOGGING
            URHO3D_LUA
            URHO3D_NAVIGATION
            URHO3D_NETWORK
            URHO3D_PHYSICS
            URHO3D_PROFILING
            URHO3D_URHO2D)
        set (${OPT} 1)
    endforeach ()
    foreach (OPT URHO3D_TESTING URHO3D_LUAJIT URHO3D_DATABASE_ODBC)
        set (${OPT} 0)
    endforeach ()
endif ()

# Coverity scan does not support PCH
if ($ENV{COVERITY_SCAN_BRANCH})
    set (URHO3D_PCH 0)
endif ()

# Enable testing
if (URHO3D_TESTING)
    enable_testing ()
endif ()

# Default library type is STATIC
if (URHO3D_LIB_TYPE)
    string (TOUPPER ${URHO3D_LIB_TYPE} URHO3D_LIB_TYPE)
endif ()
if (NOT URHO3D_LIB_TYPE STREQUAL SHARED)
    set (URHO3D_LIB_TYPE STATIC)
    if (MSVC)
        # This define will be baked into the export header for MSVC compiler
        set (URHO3D_STATIC_DEFINE 1)
    else ()
        # Only define it on the fly when necessary (both SHARED and STATIC libs can coexist) for other compiler toolchains
        add_definitions (-DURHO3D_STATIC_DEFINE)
    endif ()
endif ()

if (URHO3D_DATABASE_ODBC)
    find_package (ODBC REQUIRED)
endif ()

# Define preprocessor macros (for building the Urho3D library) based on the configured build options
foreach (OPT
        URHO3D_ANGELSCRIPT
        URHO3D_DATABASE
        URHO3D_FILEWATCHER
        URHO3D_IK
        URHO3D_LOGGING
        URHO3D_LUA
        URHO3D_MINIDUMPS
        URHO3D_NAVIGATION
        URHO3D_NETWORK
        URHO3D_PHYSICS
        URHO3D_PROFILING
        URHO3D_THREADING
        URHO3D_URHO2D
        URHO3D_GLES3
        URHO3D_WEBP
        URHO3D_WIN32_CONSOLE)
    if (${OPT})
        add_definitions (-D${OPT})
    endif ()
endforeach ()

# TODO: The logic below is earmarked to be moved into SDL's CMakeLists.txt when refactoring the library dependency handling, until then ensure the DirectX package is not being searched again in external projects such as when building LuaJIT library
if (WIN32 AND NOT CMAKE_PROJECT_NAME MATCHES ^Urho3D-ExternalProject-)
    set (DIRECTX_REQUIRED_COMPONENTS)
    set (DIRECTX_OPTIONAL_COMPONENTS DInput DSound XInput)
    if (NOT URHO3D_OPENGL)
        if (URHO3D_D3D11)
            list (APPEND DIRECTX_REQUIRED_COMPONENTS D3D11)
        else ()
            list (APPEND DIRECTX_REQUIRED_COMPONENTS D3D)
        endif ()
    endif ()
    find_package (DirectX REQUIRED ${DIRECTX_REQUIRED_COMPONENTS} OPTIONAL_COMPONENTS ${DIRECTX_OPTIONAL_COMPONENTS})
    if (DIRECTX_FOUND)
        include_directories (SYSTEM ${DIRECTX_INCLUDE_DIRS})   # These variables may be empty when WinSDK or MinGW is being used
        link_directories (${DIRECTX_LIBRARY_DIRS})
    endif ()
endif ()

# Platform and compiler specific options
if (CMAKE_CXX_COMPILER_ID MATCHES GNU)
    # Use gnu++11/gnu++0x instead of c++11/c++0x as the latter does not work as expected when cross compiling
    if (VERIFIED_SUPPORTED_STANDARD)
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=${VERIFIED_SUPPORTED_STANDARD}")
    else ()
        foreach (STANDARD gnu++11 gnu++0x)  # Fallback to gnu++0x on older GCC version
            execute_process (COMMAND ${CMAKE_COMMAND} -E echo COMMAND ${CMAKE_CXX_COMPILER} -std=${STANDARD} -E - RESULT_VARIABLE GCC_EXIT_CODE OUTPUT_QUIET ERROR_QUIET)
            if (GCC_EXIT_CODE EQUAL 0)
                set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=${STANDARD}")
                set (VERIFIED_SUPPORTED_STANDARD ${STANDARD} CACHE INTERNAL "GNU extension of C++11 standard that is verified to be supported by the chosen compiler")
                break ()
            endif ()
        endforeach ()
        if (NOT GCC_EXIT_CODE EQUAL 0)
            message (FATAL_ERROR "Your GCC version ${CMAKE_CXX_COMPILER_VERSION} is too old to enable C++11 standard")
        endif ()
    endif ()
elseif (CMAKE_CXX_COMPILER_ID MATCHES Clang)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif (MSVC80)
    message (FATAL_ERROR "Your MSVC version is too told to enable C++11 standard")
endif ()
if (APPLE)
    if (IOS)
        # iOS-specific setup
        add_definitions (-DIOS)
        if (URHO3D_64BIT)
            set (CMAKE_OSX_ARCHITECTURES $(ARCHS_STANDARD))
        else ()
            message (WARNING "URHO3D_64BIT=0 for iOS is a deprecated option and should not be used as we are phasing out 32-bit only mode")
            set (CMAKE_OSX_ARCHITECTURES $(ARCHS_STANDARD_32_BIT))
        endif ()
    elseif (TVOS)
        # tvOS-specific setup
        add_definitions (-DTVOS)
        set (CMAKE_OSX_ARCHITECTURES $(ARCHS_STANDARD))
    else ()
        if (XCODE)
            # macOS-specific setup
            if (URHO3D_64BIT)
                if (URHO3D_UNIVERSAL)
                    message (WARNING "URHO3D_UNIVERSAL=1 for macOS is a deprecated option and should not be used as we are phasing out macOS universal binary mode")
                    set (CMAKE_OSX_ARCHITECTURES $(ARCHS_STANDARD_32_64_BIT))
                else ()
                    set (CMAKE_OSX_ARCHITECTURES $(ARCHS_STANDARD))
                endif ()
            else ()
                message (WARNING "URHO3D_64BIT=0 for macOS is a deprecated option and should not be used as we are phasing out 32-bit only mode")
                set (CMAKE_OSX_ARCHITECTURES $(ARCHS_STANDARD_32_BIT))
            endif ()
        endif ()
    endif ()
    # Common macOS, iOS, and tvOS bundle setup
    if (URHO3D_MACOSX_BUNDLE OR (APPLE AND ARM))
        # Only set the bundle properties to its default when they are not explicitly specified by user
        if (NOT MACOSX_BUNDLE_GUI_IDENTIFIER)
            set (MACOSX_BUNDLE_GUI_IDENTIFIER com.github.urho3d.\${PRODUCT_NAME:rfc1034identifier:lower})
        endif ()
        if (NOT MACOSX_BUNDLE_BUNDLE_NAME)
            set (MACOSX_BUNDLE_BUNDLE_NAME \${PRODUCT_NAME})
        endif ()
    endif ()
endif ()
if (MSVC)
    # VS-specific setup
    add_definitions (-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)
    if (URHO3D_STATIC_RUNTIME)
        set (RELEASE_RUNTIME /MT)
        set (DEBUG_RUNTIME /MTd)
    endif ()
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
    set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${DEBUG_RUNTIME}")
    set (CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELEASE} ${RELEASE_RUNTIME} /fp:fast /Zi /GS-")
    set (CMAKE_C_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELWITHDEBINFO})
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
    set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${DEBUG_RUNTIME}")
    set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELEASE} ${RELEASE_RUNTIME} /fp:fast /Zi /GS- /D _SECURE_SCL=0")
    set (CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
    # Visual Studio 2012 onward enables the SSE2 by default, however, we must set the flag to IA32 if user intention is to turn the SIMD off
    if (NOT URHO3D_SSE)
        set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /arch:IA32")
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:IA32")
    endif ()
    set (CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /OPT:REF /OPT:ICF /DEBUG")
    set (CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /OPT:REF /OPT:ICF")
else ()
    # GCC/Clang-specific setup
    set (CMAKE_CXX_VISIBILITY_PRESET hidden)
    set (CMAKE_VISIBILITY_INLINES_HIDDEN true)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-offsetof")
    if (NOT ANDROID)    # Most of the flags are already setup in Android toolchain file
        if (ARM AND CMAKE_SYSTEM_NAME STREQUAL Linux)
            # Common compiler flags for aarch64-linux-gnu and arm-linux-gnueabihf, we do not support ARM on Windows for now
            set (ARM_CFLAGS "${ARM_CFLAGS} -fsigned-char -pipe")
            if (NOT URHO3D_64BIT)
                # We only support armhf distros, so turn on hard-float by default
                set (ARM_CFLAGS "${ARM_CFLAGS} -mfloat-abi=hard -Wno-psabi")
            endif ()
            # The configuration is done here instead of in CMake toolchain file because we also support native build which does not use toolchain file at all
            if (RPI)
                # RPI-specific setup
                add_definitions (-DRPI)
                if (RPI_ABI MATCHES ^armeabi-v7a)
                    set (ARM_CFLAGS "${ARM_CFLAGS} -mcpu=cortex-a7")
                    if (RPI_ABI MATCHES NEON)
                        set (ARM_CFLAGS "${ARM_CFLAGS} -mfpu=neon-vfpv4")
                    elseif (RPI_ABI MATCHES VFPV4)
                        set (ARM_CFLAGS "${ARM_CFLAGS} -mfpu=vfpv4")
                    else ()
                        set (ARM_CFLAGS "${ARM_CFLAGS} -mfpu=vfpv4-d16")
                    endif ()
                else ()
                    set (ARM_CFLAGS "${ARM_CFLAGS} -mcpu=arm1176jzf-s -mfpu=vfp")
                endif ()
            else ()
                # Generic ARM-specific setup
                add_definitions (-DGENERIC_ARM)
                if (URHO3D_64BIT)
                    # aarch64 has only one valid arch so far
                    set (ARM_CFLAGS "${ARM_CFLAGS} -march=armv8-a")
                elseif (URHO3D_ANGELSCRIPT)
                    # Angelscript seems to fail to compile using Thumb states, so force to use ARM states by default
                    set (ARM_CFLAGS "${ARM_CFLAGS} -marm")
                endif ()
                if (ARM_ABI_FLAGS)
                    # Instead of guessing all the possible ABIs, user would have to specify the ABI compiler flags explicitly via ARM_ABI_FLAGS build option
                    set (ARM_CFLAGS "${ARM_CFLAGS} ${ARM_ABI_FLAGS}")
                endif ()
            endif ()
            set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ARM_CFLAGS}")
            set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ARM_CFLAGS}")
        else ()
            if (NOT XCODE AND NOT WEB AND NOT E2K)
                set (CMAKE_C_FLAGS "-mtune=generic ${CMAKE_C_FLAGS}")
                set (CMAKE_CXX_FLAGS "-mtune=generic ${CMAKE_CXX_FLAGS}")
            endif ()
            if (URHO3D_SSE AND NOT XCODE AND NOT WEB)
                # This may influence the effective SSE level when URHO3D_SSE is on as well
                set (URHO3D_DEPLOYMENT_TARGET native CACHE STRING "Specify the minimum CPU type on which the target binaries are to be deployed (non-ARM platform only), see GCC/Clang's -march option for possible values; Use 'generic' for targeting a wide range of generic processors")
                if (NOT URHO3D_DEPLOYMENT_TARGET STREQUAL generic)
                    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=${URHO3D_DEPLOYMENT_TARGET}")
                    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=${URHO3D_DEPLOYMENT_TARGET}")
                endif ()
            endif ()
            # We don't add these flags directly here for Xcode because we support Mach-O universal binary build
            # The compiler flags will be added later conditionally when the effective arch is i386 during build time (using XCODE_ATTRIBUTE target property)
            if (NOT XCODE)
                if (URHO3D_64BIT)
                    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse3")
                    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse3")
                else ()
                    # Not the compiler native ABI, this could only happen on multilib-capable compilers
                    if (NATIVE_64BIT)
                        set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
                        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
                    endif ()
                    # The effective SSE level could be higher, see also URHO3D_DEPLOYMENT_TARGET and CMAKE_OSX_DEPLOYMENT_TARGET build options
                    # The -mfpmath=sse is not set in global scope but it may be set in local scope when building LuaJIT sub-library for x86 arch
                    if (URHO3D_SSE)
                        if (HAVE_SSE3)
                            set (SIMD_FLAG -msse3)
                        elseif (HAVE_SSE2)
                            set (SIMD_FLAG -msse2)
                        else ()
                            set (SIMD_FLAG -msse)
                        endif ()
                        set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SIMD_FLAG}")
                        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SIMD_FLAG}")
                    endif ()
                endif ()
                if (NOT URHO3D_SSE)
                    if (CMAKE_CXX_COMPILER_ID MATCHES Clang)
                        # Clang enables SSE support for i386 ABI by default, so use the '-mno-sse' compiler flag to nullify that and make it consistent with GCC
                        set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mno-sse")
                        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mno-sse")
                    endif ()
                    if (URHO3D_MMX)
                        set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mmmx")
                        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmmx")
                    endif ()
                    if (URHO3D_3DNOW)
                        set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m3dnow")
                        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m3dnow")
                    endif ()
                endif ()
                # For completeness sake only as we do not support PowerPC (yet)
                if (URHO3D_ALTIVEC)
                    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -maltivec")
                    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -maltivec")
                endif ()
            endif ()
        endif ()
        if (WEB)
            if (EMSCRIPTEN)
                # Emscripten-specific setup
                set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-warn-absolute-paths -Wno-unknown-warning-option")
                set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-warn-absolute-paths -Wno-unknown-warning-option")
                if (URHO3D_THREADING)
                    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s USE_PTHREADS=1")
                    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_PTHREADS=1")
                endif ()
                # Since version 1.37.25 emcc reduces default runtime exports, but we need "Pointer_stringify" so it needs to be explicitly declared now
                # (See https://github.com/kripken/emscripten/commit/3bc1f9f08b9f420680124af703c787244468cedc for more detail)
                # Since version 1.37.28 emcc reduces default runtime exports, but we need "FS" so it needs to be explicitly requested now
                # (See https://github.com/kripken/emscripten/commit/f2191c1223e8261bf45f4e27d2ba4d2e9d8b3341 for more detail)
                # Since version 1.39.5 emcc disables deprecated find event target behavior by default; we revert the flag for now until the support is removed
                # (See https://github.com/emscripten-core/emscripten/commit/948af470be12559367e7629f31cf7c841fbeb2a9#diff-291d81f9d42b322a89881b6d91f7a122 for more detail)
                set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s EXTRA_EXPORTED_RUNTIME_METHODS=\"['Pointer_stringify']\" -s FORCE_FILESYSTEM=1 -s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=0")
                set (CMAKE_C_FLAGS_RELEASE "-Oz -DNDEBUG")
                set (CMAKE_CXX_FLAGS_RELEASE "-Oz -DNDEBUG")
                # Remove variables to make the -O3 regalloc easier
                set (CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -O3 -s AGGRESSIVE_VARIABLE_ELIMINATION=1")
                # Preserve LLVM debug information, show line number debug comments, and generate source maps; always disable exception handling codegen
                set (CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -g4 -s DISABLE_EXCEPTION_CATCHING=1")
            endif ()
        elseif (MINGW)
            # MinGW-specific setup
            set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static -static-libgcc -fno-keep-inline-dllexport")
            set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -static-libgcc -static-libstdc++ -fno-keep-inline-dllexport")
            if (NOT URHO3D_64BIT)
                set (CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")
                set (CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
                # Prevent auto-vectorize optimization when using -O2, unless stack realign is being enforced globally
                if (URHO3D_SSE)
                    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mstackrealign")
                    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mstackrealign")
                else ()
                    set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fno-tree-slp-vectorize -fno-tree-vectorize")
                    set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-tree-slp-vectorize -fno-tree-vectorize")
                endif ()
            endif ()
        else ()
            # Not Android and not Emscripten and not MinGW derivative
            set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")     # This will emit '-DREENTRANT' to compiler and '-lpthread' to linker on Linux and Mac OSX platform
            set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread") # However, it may emit other equivalent compiler define and/or linker flag on other *nix platforms
        endif ()
        set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG -D_DEBUG")
        set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -D_DEBUG")
    endif ()
    if (CMAKE_CXX_COMPILER_ID MATCHES Clang)
        # Clang-specific
        set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Qunused-arguments")
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Qunused-arguments")
        if (NINJA OR "$ENV{USE_CCACHE}")    # Stringify to guard against undefined environment variable
            # When ccache support is on, these flags keep the color diagnostics pipe through ccache output and suppress Clang warning due ccache internal preprocessing step
            set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fcolor-diagnostics")
            set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")
        endif ()
        if ((NOT APPLE AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0.1) OR (APPLE AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 11.0.0))
            # Workaround for Clang 7.0.1 and above until the Bullet upstream has fixed the Clang 7 diagnostic checks issue (see https://github.com/bulletphysics/bullet3/issues/2114)
            # ditto for AppleClang 11.0.0 and above
            set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-argument-outside-range")
            set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-argument-outside-range")
        endif ()
    else ()
        # GCC-specific
        if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.9.1)
            set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fdiagnostics-color=auto")
            set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=auto")
        endif ()
    endif ()
endif ()
# LuaJIT specific - extra linker flags for linking against LuaJIT (adapted from LuaJIT's original Makefile)
if (URHO3D_LUAJIT)
    if (URHO3D_64BIT AND APPLE AND NOT ARM)
        # 64-bit macOS: it simply won't work without these flags; if you are reading this comment then you may want to know the following also
        # it's recommended to rebase all (self-compiled) shared libraries which are loaded at runtime on OSX/x64 (e.g. C extension modules for Lua), see: man rebase
        set (LUAJIT_EXE_LINKER_FLAGS_APPLE "-pagezero_size 10000 -image_base 100000000")
        set (LUAJIT_SHARED_LINKER_FLAGS_APPLE "-image_base 7fff04c4a000")
        if (NOT XCODE)
            set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LUAJIT_EXE_LINKER_FLAGS_APPLE}")
            set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${LUAJIT_SHARED_LINKER_FLAGS_APPLE}")
        endif ()
    elseif (URHO3D_LIB_TYPE STREQUAL STATIC AND NOT WIN32 AND NOT APPLE)    # The original condition also checks: AND NOT SunOS AND NOT PS3
        # We assume user may want to load C modules compiled for plain Lua with require(), so we have to ensure all the public symbols are exported when linking with Urho3D (and therefore LuaJIT) statically
        # Note: this implies that loading such modules on Windows platform may only work with SHARED library type
        set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-E")
    endif ()
endif ()
# Trim the leading white space in the compiler/linker flags, if any
foreach (TYPE C CXX EXE_LINKER SHARED_LINKER)
    string (REGEX REPLACE "^ +" "" CMAKE_${TYPE}_FLAGS "${CMAKE_${TYPE}_FLAGS}")
endforeach ()

include (CMakeParseArguments)

# Macro for adjusting target output name by dropping _suffix from the target name
macro (adjust_target_name)
    if (TARGET_NAME MATCHES _.*$)
        string (REGEX REPLACE _.*$ "" OUTPUT_NAME ${TARGET_NAME})
        set_target_properties (${TARGET_NAME} PROPERTIES OUTPUT_NAME ${OUTPUT_NAME})
    endif ()
endmacro ()

# Macro for checking the SOURCE_FILES variable is properly initialized
macro (check_source_files)
    if (NOT SOURCE_FILES)
        if (NOT ${ARGN} STREQUAL "")
            message (FATAL_ERROR ${ARGN})
        else ()
            message (FATAL_ERROR "Could not configure and generate the project file because no source files have been defined yet. "
                "You can define the source files explicitly by setting the SOURCE_FILES variable in your CMakeLists.txt; or "
                "by calling the define_source_files() macro which would by default glob all the C++ source files found in the same scope of "
                "CMakeLists.txt where the macro is being called and the macro would set the SOURCE_FILES variable automatically. "
                "If your source files are not located in the same directory as the CMakeLists.txt or your source files are "
                "more than just C++ language then you probably have to pass in extra arguments when calling the macro in order to make it works. "
                "See the define_source_files() macro definition in the CMake/Modules/UrhoCommon.cmake for more detail.")
        endif ()
    endif ()
endmacro ()

# Macro for setting symbolic link on platform that supports it
macro (create_symlink SOURCE DESTINATION)
    cmake_parse_arguments (ARG "FALLBACK_TO_COPY" "BASE" "" ${ARGN})
    # Make absolute paths so they work more reliably on cmake-gui
    if (IS_ABSOLUTE ${SOURCE})
        set (ABS_SOURCE ${SOURCE})
    else ()
        set (ABS_SOURCE ${CMAKE_SOURCE_DIR}/${SOURCE})
    endif ()
    if (IS_ABSOLUTE ${DESTINATION})
        set (ABS_DESTINATION ${DESTINATION})
    else ()
        if (ARG_BASE)
            set (ABS_DESTINATION ${ARG_BASE}/${DESTINATION})
        else ()
            set (ABS_DESTINATION ${CMAKE_BINARY_DIR}/${DESTINATION})
        endif ()
    endif ()
    if (CMAKE_HOST_WIN32)
        if (IS_DIRECTORY ${ABS_SOURCE})
            set (SLASH_D /D)
        else ()
            unset (SLASH_D)
        endif ()
        if (HAS_MKLINK)
            if (NOT EXISTS ${ABS_DESTINATION})
                # Have to use string-REPLACE as file-TO_NATIVE_PATH does not work as expected with MinGW on "backward slash" host system
                string (REPLACE / \\ BACKWARD_ABS_DESTINATION ${ABS_DESTINATION})
                string (REPLACE / \\ BACKWARD_ABS_SOURCE ${ABS_SOURCE})
                execute_process (COMMAND cmd /C mklink ${SLASH_D} ${BACKWARD_ABS_DESTINATION} ${BACKWARD_ABS_SOURCE} OUTPUT_QUIET ERROR_QUIET)
            endif ()
        elseif (ARG_FALLBACK_TO_COPY)
            if (SLASH_D)
                set (COMMAND COMMAND ${CMAKE_COMMAND} -E copy_directory ${ABS_SOURCE} ${ABS_DESTINATION})
            else ()
                set (COMMAND COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ABS_SOURCE} ${ABS_DESTINATION})
            endif ()
            # Fallback to copy only one time
            execute_process (${COMMAND})
            if (TARGET ${TARGET_NAME})
                # Fallback to copy every time the target is built
                add_custom_command (TARGET ${TARGET_NAME} POST_BUILD ${COMMAND})
            endif ()
        else ()
            message (WARNING "Unable to create symbolic link on this host system, you may need to manually copy file/dir from \"${SOURCE}\" to \"${DESTINATION}\"")
        endif ()
    else ()
        get_filename_component (DIRECTORY ${ABS_DESTINATION} DIRECTORY)
        file (RELATIVE_PATH REL_SOURCE ${DIRECTORY} ${ABS_SOURCE})
        execute_process (COMMAND ${CMAKE_COMMAND} -E create_symlink ${REL_SOURCE} ${ABS_DESTINATION})
    endif ()
endmacro ()

# Macro for adding additional make clean files
macro (add_make_clean_files)
    get_directory_property (ADDITIONAL_MAKE_CLEAN_FILES ADDITIONAL_MAKE_CLEAN_FILES)
    set_directory_properties (PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${ADDITIONAL_MAKE_CLEAN_FILES};${ARGN}")
endmacro ()

# *** THIS IS A DEPRECATED MACRO ***
# Macro for defining external library dependencies
# The purpose of this macro is emulate CMake to set the external library dependencies transitively
# It works for both targets setup within Urho3D project and downstream projects that uses Urho3D as external static/shared library
# *** THIS IS A DEPRECATED MACRO ***
macro (define_dependency_libs TARGET)
    # ThirdParty/SDL external dependency
    if (${TARGET} MATCHES SDL|Urho3D)
        if (WIN32)
            list (APPEND LIBS user32 gdi32 winmm imm32 ole32 oleaut32 setupapi version uuid)
        elseif (APPLE)
            list (APPEND LIBS iconv)
        elseif (ANDROID)
            list (APPEND LIBS dl log android)
        else ()
            # Linux
            if (NOT WEB)
                list (APPEND LIBS dl m rt)
            endif ()
            if (RPI)
                list (APPEND ABSOLUTE_PATH_LIBS ${VIDEOCORE_LIBRARIES})
            endif ()
        endif ()
    endif ()

    # ThirdParty/Civetweb external dependency
    if (${TARGET} MATCHES Civetweb|Urho3D)
        if (WIN32)
            list (APPEND LIBS ws2_32)
        endif ()
    endif ()

    # ThirdParty/SLikeNet external dependency
    if (${TARGET} MATCHES SLikeNet|Urho3D)
        if (WIN32)
            list (APPEND LIBS iphlpapi)
        endif ()
    endif ()

    # Urho3D/LuaJIT external dependency
    if (URHO3D_LUAJIT AND ${TARGET} MATCHES LuaJIT|Urho3D)
        if (NOT WIN32 AND NOT WEB)
            list (APPEND LIBS dl m)
        endif ()
    endif ()

    # Urho3D external dependency
    if (${TARGET} STREQUAL Urho3D)
        # Core
        if (WIN32)
            list (APPEND LIBS winmm)
            if (URHO3D_MINIDUMPS)
                list (APPEND LIBS dbghelp)
            endif ()
        elseif (APPLE)
            if (ARM)
                list (APPEND LIBS "-framework AudioToolbox" "-framework AVFoundation" "-framework CoreAudio" "-framework CoreBluetooth" "-framework CoreGraphics" "-framework CoreMotion" "-framework Foundation" "-framework GameController" "-framework OpenGLES" "-framework QuartzCore" "-framework UIKit")
            else ()
                list (APPEND LIBS "-framework AudioToolbox" "-framework Carbon" "-framework Cocoa" "-framework CoreFoundation" "-framework SystemConfiguration" "-framework CoreAudio" "-framework CoreBluetooth" "-framework CoreServices" "-framework CoreVideo" "-framework ForceFeedback" "-framework IOKit" "-framework OpenGL")
            endif ()
        endif ()

        # Graphics
        if (URHO3D_OPENGL)
            if (NOT (ANDROID OR WEB OR IOS OR TVOS))
                set (URHO3D_GLES3 FALSE)
            endif()
            if (APPLE)
                # Do nothing
            elseif (WIN32)
                list (APPEND LIBS opengl32)
            elseif (RPI)
                list (APPEND LIBS brcmGLESv2)
            elseif (ANDROID OR ARM)
                if (URHO3D_GLES3)
                    list (APPEND LIBS GLESv3)
                else ()
                    list (APPEND LIBS GLESv1_CM GLESv2)
                endif ()
            else ()
                list (APPEND LIBS GL)
            endif ()
        elseif (DIRECT3D_LIBRARIES)
            list (APPEND LIBS ${DIRECT3D_LIBRARIES})
        endif ()

        # Database
        if (URHO3D_DATABASE_ODBC)
            list (APPEND LIBS ${ODBC_LIBRARIES})
        endif ()

        # This variable value can either be 'Urho3D' target or an absolute path to an actual static/shared Urho3D library or empty (if we are building the library itself)
        # The former would cause CMake not only to link against the Urho3D library but also to add a dependency to Urho3D target
        if (URHO3D_LIBRARIES)
            if (WIN32 AND URHO3D_LIBRARIES_DBG AND URHO3D_LIBRARIES_REL AND TARGET ${TARGET_NAME})
                # Special handling when both debug and release libraries are found
                target_link_libraries (${TARGET_NAME} debug ${URHO3D_LIBRARIES_DBG} optimized ${URHO3D_LIBRARIES_REL})
            else ()
                if (TARGET ${TARGET}_universal)
                    add_dependencies (${TARGET_NAME} ${TARGET}_universal)
                endif ()
                list (APPEND ABSOLUTE_PATH_LIBS ${URHO3D_LIBRARIES})
            endif ()
        endif ()
    endif ()
endmacro ()

# Macro for defining source files with optional arguments as follows:
#  GLOB_CPP_PATTERNS <list> - Use the provided globbing patterns for CPP_FILES instead of the default *.cpp
#  GLOB_H_PATTERNS <list> - Use the provided globbing patterns for H_FILES instead of the default *.h
#  EXCLUDE_PATTERNS <list> - Use the provided regex patterns for excluding the unwanted matched source files
#  EXTRA_CPP_FILES <list> - Include the provided list of files into CPP_FILES result
#  EXTRA_H_FILES <list> - Include the provided list of files into H_FILES result
#  PCH <list> - Enable precompiled header support on the defined source files using the specified header file, the list is "<path/to/header> [C++|C]"
#  RECURSE - Option to glob recursively
#  GROUP - Option to group source files based on its relative path to the corresponding parent directory
macro (define_source_files)
    # Source files are defined by globbing source files in current source directory and also by including the extra source files if provided
    cmake_parse_arguments (ARG "RECURSE;GROUP" "" "PCH;EXTRA_CPP_FILES;EXTRA_H_FILES;GLOB_CPP_PATTERNS;GLOB_H_PATTERNS;EXCLUDE_PATTERNS" ${ARGN})
    if (NOT ARG_GLOB_CPP_PATTERNS)
        set (ARG_GLOB_CPP_PATTERNS *.cpp)    # Default glob pattern
    endif ()
    if (NOT ARG_GLOB_H_PATTERNS)
        set (ARG_GLOB_H_PATTERNS *.h)
    endif ()
    if (ARG_RECURSE)
        set (ARG_RECURSE _RECURSE)
    else ()
        unset (ARG_RECURSE)
    endif ()
    file (GLOB${ARG_RECURSE} CPP_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${ARG_GLOB_CPP_PATTERNS})
    file (GLOB${ARG_RECURSE} H_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${ARG_GLOB_H_PATTERNS})
    if (ARG_EXCLUDE_PATTERNS)
        set (CPP_FILES_WITH_SENTINEL ";${CPP_FILES};")  # Stringify the lists
        set (H_FILES_WITH_SENTINEL ";${H_FILES};")
        foreach (PATTERN ${ARG_EXCLUDE_PATTERNS})
            foreach (LOOP RANGE 1)
                string (REGEX REPLACE ";${PATTERN};" ";;" CPP_FILES_WITH_SENTINEL "${CPP_FILES_WITH_SENTINEL}")
                string (REGEX REPLACE ";${PATTERN};" ";;" H_FILES_WITH_SENTINEL "${H_FILES_WITH_SENTINEL}")
            endforeach ()
        endforeach ()
        set (CPP_FILES ${CPP_FILES_WITH_SENTINEL})      # Convert strings back to lists, extra sentinels are harmless
        set (H_FILES ${H_FILES_WITH_SENTINEL})
    endif ()
    list (APPEND CPP_FILES ${ARG_EXTRA_CPP_FILES})
    list (APPEND H_FILES ${ARG_EXTRA_H_FILES})
    set (SOURCE_FILES ${CPP_FILES} ${H_FILES})
    # Optionally enable PCH
    if (ARG_PCH)
        enable_pch (${ARG_PCH})
    endif ()
    # Optionally group the sources based on their physical subdirectories
    if (ARG_GROUP)
        foreach (CPP_FILE ${CPP_FILES})
            get_filename_component (PATH ${CPP_FILE} PATH)
            if (PATH)
                string (REPLACE / \\ PATH ${PATH})
                source_group ("Source Files\\${PATH}" FILES ${CPP_FILE})
            endif ()
        endforeach ()
        foreach (H_FILE ${H_FILES})
            get_filename_component (PATH ${H_FILE} PATH)
            if (PATH)
                string (REPLACE / \\ PATH ${PATH})
                source_group ("Header Files\\${PATH}" FILES ${H_FILE})
            endif ()
        endforeach ()
    endif ()
endmacro ()

# Macro for defining resource directories with optional arguments as follows:
#  GLOB_PATTERNS <list> - Use the provided globbing patterns for resource directories, default to "${CMAKE_SOURCE_DIR}/bin/*Data"
#  EXCLUDE_PATTERNS <list> - Use the provided regex patterns for excluding the unwanted matched directories
#  EXTRA_DIRS <list> - Include the provided list of directories into globbing result
#  HTML_SHELL <value> - An absolute path to the HTML shell file (only applicable for Web platform)
macro (define_resource_dirs)
    check_source_files ("Could not call define_resource_dirs() macro before define_source_files() macro.")
    cmake_parse_arguments (ARG "" "HTML_SHELL" "GLOB_PATTERNS;EXCLUDE_PATTERNS;EXTRA_DIRS" ${ARGN})
    if (WEB AND ARG_HTML_SHELL)
        add_html_shell (${ARG_HTML_SHELL})
    endif ()
    # If not explicitly specified then use the Urho3D project structure convention
    if (NOT ARG_GLOB_PATTERNS)
        set (ARG_GLOB_PATTERNS ${CMAKE_SOURCE_DIR}/bin/*Data)
    endif ()
    file (GLOB GLOB_RESULTS ${ARG_GLOB_PATTERNS})
    unset (GLOB_DIRS)
    foreach (DIR ${GLOB_RESULTS})
        if (IS_DIRECTORY ${DIR})
            list (APPEND GLOB_DIRS ${DIR})
        endif ()
    endforeach ()
    if (ARG_EXCLUDE_PATTERNS)
        set (GLOB_DIRS_WITH_SENTINEL ";${GLOB_DIRS};")  # Stringify the lists
        foreach (PATTERN ${ARG_EXCLUDE_PATTERNS})
            foreach (LOOP RANGE 1)
                string (REGEX REPLACE ";${PATTERN};" ";;" GLOB_DIRS_WITH_SENTINEL "${GLOB_DIRS_WITH_SENTINEL}")
            endforeach ()
        endforeach ()
        set (GLOB_DIRS ${GLOB_DIRS_WITH_SENTINEL})      # Convert strings back to lists, extra sentinels are harmless
    endif ()
    list (APPEND RESOURCE_DIRS ${GLOB_DIRS})
    foreach (DIR ${ARG_EXTRA_DIRS})
        if (EXISTS ${DIR})
            list (APPEND RESOURCE_DIRS ${DIR})
        endif ()
    endforeach ()
    source_group ("Resource Dirs" FILES ${RESOURCE_DIRS})
    # Populate all the variables required by resource packaging, if the build option is enabled
    if (URHO3D_PACKAGING AND RESOURCE_DIRS)
        foreach (DIR ${RESOURCE_DIRS})
            get_filename_component (NAME ${DIR} NAME)
            if (ANDROID)
                set (RESOURCE_${DIR}_PATHNAME ${CMAKE_BINARY_DIR}/assets/${NAME}.pak)
            else ()
                set (RESOURCE_${DIR}_PATHNAME ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${NAME}.pak)
            endif ()
            list (APPEND RESOURCE_PAKS ${RESOURCE_${DIR}_PATHNAME})
            if (EMSCRIPTEN AND NOT EMSCRIPTEN_SHARE_DATA)
                # Set the custom EMCC_OPTION property to preload the *.pak individually
                set_source_files_properties (${RESOURCE_${DIR}_PATHNAME} PROPERTIES EMCC_OPTION preload-file EMCC_FILE_ALIAS "${NAME}.pak")
            endif ()
        endforeach ()
        set_property (SOURCE ${RESOURCE_PAKS} PROPERTY GENERATED TRUE)
        if (WEB)
            if (EMSCRIPTEN)
                # Set the custom EMCC_OPTION property to peload the generated shared data file
                if (EMSCRIPTEN_SHARE_DATA)
                    set (SHARED_RESOURCE_JS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_PROJECT_NAME}.js)
                    list (APPEND SOURCE_FILES ${SHARED_RESOURCE_JS} ${SHARED_RESOURCE_JS}.data)
                    # DEST_BUNDLE_DIR may be empty when macro caller does not wish to install anything
                    if (DEST_BUNDLE_DIR)
                        install (FILES ${SHARED_RESOURCE_JS} ${SHARED_RESOURCE_JS}.data DESTINATION ${DEST_BUNDLE_DIR})
                    endif ()
                    # Define a custom command for generating a shared data file
                    if (RESOURCE_PAKS)
                        # When sharing a single data file, all main targets are assumed to use a same set of resource paks
                        foreach (FILE ${RESOURCE_PAKS})
                            get_filename_component (NAME ${FILE} NAME)
                            list (APPEND PAK_NAMES ${NAME})
                        endforeach ()
                        if (CMAKE_BUILD_TYPE STREQUAL Debug)
                            set (SEPARATE_METADATA --separate-metadata)
                        endif ()
                        add_custom_command (OUTPUT ${SHARED_RESOURCE_JS} ${SHARED_RESOURCE_JS}.data
                            COMMAND ${EMPACKAGER} ${SHARED_RESOURCE_JS}.data --preload ${PAK_NAMES} --js-output=${SHARED_RESOURCE_JS} --use-preload-cache ${SEPARATE_METADATA}
                            DEPENDS RESOURCE_CHECK ${RESOURCE_PAKS}
                            WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                            COMMENT "Generating shared data file")
                    endif ()
                endif ()
            endif ()
        endif ()
    endif ()
    if (XCODE)
        if (NOT RESOURCE_FILES)
            # Default app bundle icon
            set (RESOURCE_FILES ${CMAKE_SOURCE_DIR}/bin/Data/Textures/UrhoIcon.icns)
            if (ARM)
                # Default app icon on the iOS/tvOS home screen
                list (APPEND RESOURCE_FILES ${CMAKE_SOURCE_DIR}/bin/Data/Textures/UrhoIcon.png)
            endif ()
        endif ()
        # Group them together under 'Resources' in Xcode IDE
        source_group (Resources FILES ${RESOURCE_PAKS} ${RESOURCE_FILES})     # RESOURCE_PAKS could be empty if packaging is not requested
        # But only use either paks or dirs
        if (RESOURCE_PAKS)
            set_source_files_properties (${RESOURCE_PAKS} ${RESOURCE_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
        else ()
            set_source_files_properties (${RESOURCE_DIRS} ${RESOURCE_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
        endif ()
    endif ()
    list (APPEND SOURCE_FILES ${RESOURCE_DIRS} ${RESOURCE_PAKS} ${RESOURCE_FILES})
endmacro()

# Macro for adding a HTML shell-file when targeting Web platform
macro (add_html_shell)
    check_source_files ("Could not call add_html_shell() macro before define_source_files() macro.")
    if (EMSCRIPTEN)
        if (NOT ${ARGN} STREQUAL "")
            set (HTML_SHELL ${ARGN})
        else ()
            # Create Urho3D custom HTML shell that also embeds our own project logo
            if (NOT EXISTS ${CMAKE_BINARY_DIR}/Source/shell.html)
                file (READ ${EMSCRIPTEN_ROOT_PATH}/src/shell.html HTML_SHELL)
                string (REPLACE "<!doctype html>" "<!-- This is a generated file. DO NOT EDIT!-->\n\n<!doctype html>" HTML_SHELL "${HTML_SHELL}")     # Stringify to preserve semicolons
                string (REPLACE "<body>" "<body>\n<script>document.body.innerHTML=document.body.innerHTML.replace(/^#!.*\\n/, '');</script>\n<a href=\"https://urho3d.github.io\" title=\"Urho3D Homepage\"><img src=\"https://urho3d.github.io/assets/images/logo.png\" alt=\"link to https://urho3d.github.io\" height=\"80\" width=\"160\" /></a>\n" HTML_SHELL "${HTML_SHELL}")
                file (WRITE ${CMAKE_BINARY_DIR}/Source/shell.html "${HTML_SHELL}")
            endif ()
            set (HTML_SHELL ${CMAKE_BINARY_DIR}/Source/shell.html)
        endif ()
        list (APPEND SOURCE_FILES ${HTML_SHELL})
        set_source_files_properties (${HTML_SHELL} PROPERTIES EMCC_OPTION shell-file)
    endif ()
endmacro ()

include (GenerateExportHeader)

# Macro for precompiling header (On MSVC, the dummy C++ or C implementation file for precompiling the header file would be generated if not already exists)
# This macro should be called before the CMake target has been added
# Typically, user should indirectly call this macro by using the 'PCH' option when calling define_source_files() macro
macro (enable_pch HEADER_PATHNAME)
    # No op when PCH support is not enabled
    if (URHO3D_PCH)
        # Get the optional LANG parameter to indicate whether the header should be treated as C or C++ header, default to C++
        if ("${ARGN}" STREQUAL C) # Stringify as the LANG parameter could be empty
            set (EXT c)
            set (LANG C)
            set (LANG_H c-header)
        else ()
            # This is the default
            set (EXT cpp)
            set (LANG CXX)
            set (LANG_H c++-header)
        endif ()
        # Relative path is resolved using CMAKE_CURRENT_SOURCE_DIR
        if (IS_ABSOLUTE ${HEADER_PATHNAME})
            set (ABS_HEADER_PATHNAME ${HEADER_PATHNAME})
        else ()
            set (ABS_HEADER_PATHNAME ${CMAKE_CURRENT_SOURCE_DIR}/${HEADER_PATHNAME})
        endif ()
        # Determine the precompiled header output filename
        get_filename_component (HEADER_FILENAME ${HEADER_PATHNAME} NAME)
        if (CMAKE_COMPILER_IS_GNUCXX)
            # GNU g++
            set (PCH_FILENAME ${HEADER_FILENAME}.gch)
        else ()
            # Clang or MSVC
            set (PCH_FILENAME ${HEADER_FILENAME}.pch)
        endif ()

        if (MSVC)
            get_filename_component (NAME_WE ${HEADER_FILENAME} NAME_WE)
            if (TARGET ${TARGET_NAME})
                if (VS)
                    # VS is multi-config, the exact path is only known during actual build time based on effective build config
                    set (PCH_PATHNAME "$(IntDir)${PCH_FILENAME}")
                else ()
                    set (PCH_PATHNAME ${CMAKE_CURRENT_BINARY_DIR}/${PCH_FILENAME})
                endif ()
                foreach (FILE ${SOURCE_FILES})
                    if (FILE MATCHES \\.${EXT}$)
                        if (FILE MATCHES ${NAME_WE}\\.${EXT}$)
                            # Precompiling header file
                            set_property (SOURCE ${FILE} APPEND_STRING PROPERTY COMPILE_FLAGS " /Fp${PCH_PATHNAME} /Yc${HEADER_FILENAME}")     # Need a leading space for appending
                        else ()
                            # Using precompiled header file
                            set_property (SOURCE ${FILE} APPEND_STRING PROPERTY COMPILE_FLAGS " /Fp${PCH_PATHNAME} /Yu${HEADER_FILENAME} /FI${HEADER_FILENAME}")
                        endif ()
                    endif ()
                endforeach ()
                unset (${TARGET_NAME}_HEADER_PATHNAME)
            else ()
                # The target has not been created yet, so set an internal variable to come back here again later
                set (${TARGET_NAME}_HEADER_PATHNAME ${ARGV})
                # But proceed to add the dummy C++ or C implementation file if necessary
                set (${LANG}_FILENAME ${NAME_WE}.${EXT})
                get_filename_component (PATH ${HEADER_PATHNAME} PATH)
                if (PATH)
                    set (PATH ${PATH}/)
                endif ()
                list (FIND SOURCE_FILES ${PATH}${${LANG}_FILENAME} ${LANG}_FILENAME_FOUND)
                if (${LANG}_FILENAME_FOUND STREQUAL -1)
                    if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/${${LANG}_FILENAME})
                        # Only generate it once so that its timestamp is not touched unnecessarily
                        file (WRITE ${CMAKE_CURRENT_BINARY_DIR}/${${LANG}_FILENAME} "// This is a generated file. DO NOT EDIT!\n\n#include \"${HEADER_FILENAME}\"")
                    endif ()
                    list (INSERT SOURCE_FILES 0 ${${LANG}_FILENAME})
                endif ()
            endif ()
        elseif (XCODE)
            if (TARGET ${TARGET_NAME})
                # Precompiling and using precompiled header file
                set_target_properties (${TARGET_NAME} PROPERTIES XCODE_ATTRIBUTE_GCC_PRECOMPILE_PREFIX_HEADER YES XCODE_ATTRIBUTE_GCC_PREFIX_HEADER ${ABS_HEADER_PATHNAME})
                unset (${TARGET_NAME}_HEADER_PATHNAME)
            else ()
                # The target has not been created yet, so set an internal variable to come back here again later
                set (${TARGET_NAME}_HEADER_PATHNAME ${ARGV})
            endif ()
        else ()
            # GCC or Clang
            if (TARGET ${TARGET_NAME})
                # Precompiling header file
                get_directory_property (COMPILE_DEFINITIONS COMPILE_DEFINITIONS)
                get_directory_property (INCLUDE_DIRECTORIES INCLUDE_DIRECTORIES)
                get_target_property (TYPE ${TARGET_NAME} TYPE)
                if (TYPE MATCHES SHARED)
                    list (APPEND COMPILE_DEFINITIONS ${TARGET_NAME}_EXPORTS)
                    if (LANG STREQUAL CXX)
                        _test_compiler_hidden_visibility ()
                    endif ()
                endif ()
                # Use PIC flags as necessary, except when compiling using MinGW which already uses PIC flags for all codes
                if (NOT MINGW)
                    get_target_property (PIC ${TARGET_NAME} POSITION_INDEPENDENT_CODE)
                    if (PIC)
                        set (PIC_FLAGS -fPIC)
                    endif ()
                endif ()
                string (REPLACE ";" " -D" COMPILE_DEFINITIONS "-D${COMPILE_DEFINITIONS}")
                string (REPLACE "\"" "\\\"" COMPILE_DEFINITIONS ${COMPILE_DEFINITIONS})
                string (REPLACE ";" "\" -I\"" INCLUDE_DIRECTORIES "-I\"${INCLUDE_DIRECTORIES}\"")
                if (CMAKE_SYSROOT)
                    set (SYSROOT_FLAGS "--sysroot=\"${CMAKE_SYSROOT}\"")
                endif ()
                # Make sure the precompiled headers are not stale by creating custom rules to re-compile the header as necessary
                file (MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${PCH_FILENAME})
                set (ABS_PATH_PCH ${CMAKE_CURRENT_BINARY_DIR}/${HEADER_FILENAME})
                foreach (CONFIG ${CMAKE_CONFIGURATION_TYPES} ${CMAKE_BUILD_TYPE})   # These two vars are mutually exclusive
                    # Generate *.rsp containing configuration specific compiler flags
                    string (TOUPPER ${CONFIG} UPPERCASE_CONFIG)
                    file (WRITE ${ABS_PATH_PCH}.${CONFIG}.pch.rsp.new "${COMPILE_DEFINITIONS} ${SYSROOT_FLAGS} ${CLANG_${LANG}_FLAGS} ${CMAKE_${LANG}_FLAGS} ${CMAKE_${LANG}_FLAGS_${UPPERCASE_CONFIG}} ${COMPILER_HIDDEN_VISIBILITY_FLAGS} ${COMPILER_HIDDEN_INLINE_VISIBILITY_FLAGS} ${PIC_FLAGS} ${INCLUDE_DIRECTORIES} -c -x ${LANG_H}")
                    execute_process (COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ABS_PATH_PCH}.${CONFIG}.pch.rsp.new ${ABS_PATH_PCH}.${CONFIG}.pch.rsp)
                    file (REMOVE ${ABS_PATH_PCH}.${CONFIG}.pch.rsp.new)
                    if (NOT ${TARGET_NAME}_PCH_DEPS)
                        if (NOT CMAKE_CURRENT_SOURCE_DIR EQUAL CMAKE_CURRENT_BINARY_DIR)
                            # Create a dummy initial PCH file in the Out-of-source build tree to keep CLion happy
                            execute_process (COMMAND ${CMAKE_COMMAND} -E touch ${ABS_PATH_PCH})
                        endif ()
                        # Determine the dependency list
                        execute_process (COMMAND ${CMAKE_${LANG}_COMPILER} @${ABS_PATH_PCH}.${CONFIG}.pch.rsp -MTdeps -MM -MF ${ABS_PATH_PCH}.d ${ABS_HEADER_PATHNAME} RESULT_VARIABLE ${LANG}_COMPILER_EXIT_CODE)
                        if (NOT ${LANG}_COMPILER_EXIT_CODE EQUAL 0)
                            message (FATAL_ERROR "Could not generate dependency list for PCH. There is something wrong with your compiler toolchain. "
                                "Ensure its bin path is in the PATH environment variable or ensure CMake can find CC/CXX in your build environment.")
                        endif ()
                        file (STRINGS ${ABS_PATH_PCH}.d ${TARGET_NAME}_PCH_DEPS)
                        string (REGEX REPLACE "^deps: *| *\\; *" ";" ${TARGET_NAME}_PCH_DEPS ${${TARGET_NAME}_PCH_DEPS})
                        string (REGEX REPLACE "\\\\ " "\ " ${TARGET_NAME}_PCH_DEPS "${${TARGET_NAME}_PCH_DEPS}")    # Need to stringify the second time to preserve the semicolons
                    endif ()
                    # Create the rule that depends on the included headers
                    add_custom_command (OUTPUT ${HEADER_FILENAME}.${CONFIG}.pch.trigger
                        COMMAND ${CMAKE_${LANG}_COMPILER} @${ABS_PATH_PCH}.${CONFIG}.pch.rsp -o ${PCH_FILENAME}/${PCH_FILENAME}.${CONFIG} ${ABS_HEADER_PATHNAME}
                        COMMAND ${CMAKE_COMMAND} -E touch ${HEADER_FILENAME}.${CONFIG}.pch.trigger
                        DEPENDS ${ABS_PATH_PCH}.${CONFIG}.pch.rsp ${${TARGET_NAME}_PCH_DEPS}
                        COMMENT "Precompiling header file '${HEADER_FILENAME}' for ${CONFIG} configuration")
                    add_make_clean_files (${PCH_FILENAME}/${PCH_FILENAME}.${CONFIG})
                endforeach ()
                # Using precompiled header file
                set (CMAKE_${LANG}_FLAGS "${CMAKE_${LANG}_FLAGS} -include \"${ABS_PATH_PCH}\"")
                unset (${TARGET_NAME}_HEADER_PATHNAME)
            else ()
                # The target has not been created yet, so set an internal variable to come back here again later
                set (${TARGET_NAME}_HEADER_PATHNAME ${ARGV})
                # But proceed to add the dummy source file(s) to trigger the custom command output rule
                if (CMAKE_CONFIGURATION_TYPES)
                    # Multi-config, trigger all rules and let the compiler to choose which precompiled header is suitable to use
                    foreach (CONFIG ${CMAKE_CONFIGURATION_TYPES})
                        list (APPEND TRIGGERS ${HEADER_FILENAME}.${CONFIG}.pch.trigger)
                    endforeach ()
                else ()
                    # Single-config, just trigger the corresponding rule matching the current build configuration
                    set (TRIGGERS ${HEADER_FILENAME}.${CMAKE_BUILD_TYPE}.pch.trigger)
                endif ()
                list (APPEND SOURCE_FILES ${TRIGGERS})
            endif ()
        endif ()
    endif ()
endmacro ()

# Macro for finding file in Urho3D build tree or Urho3D SDK
macro (find_Urho3D_file VAR NAME)
    # Pass the arguments to the actual find command
    cmake_parse_arguments (ARG "" "DOC;MSG_MODE" "HINTS;PATHS;PATH_SUFFIXES" ${ARGN})
    find_file (${VAR} ${NAME} HINTS ${ARG_HINTS} PATHS ${ARG_PATHS} PATH_SUFFIXES ${ARG_PATH_SUFFIXES} DOC ${ARG_DOC} NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
    mark_as_advanced (${VAR})  # Hide it from cmake-gui in non-advanced mode
    if (NOT ${VAR} AND ARG_MSG_MODE)
        message (${ARG_MSG_MODE}
            "Could not find ${VAR} file in the Urho3D build tree or Urho3D SDK. "
            "Please reconfigure and rebuild your Urho3D build tree or reinstall the SDK for the correct target platform.")
    endif ()
endmacro ()

# Macro for finding tool in Urho3D build tree or Urho3D SDK
macro (find_Urho3D_tool VAR NAME)
    # Pass the arguments to the actual find command
    cmake_parse_arguments (ARG "" "DOC;MSG_MODE" "HINTS;PATHS;PATH_SUFFIXES" ${ARGN})
    find_program (${VAR} ${NAME} HINTS ${ARG_HINTS} PATHS ${ARG_PATHS} PATH_SUFFIXES ${ARG_PATH_SUFFIXES} DOC ${ARG_DOC} NO_DEFAULT_PATH)
    mark_as_advanced (${VAR})  # Hide it from cmake-gui in non-advanced mode
    if (NOT ${VAR})
        set (${VAR} ${CMAKE_BINARY_DIR}/bin/tool/${NAME})
        if (ARG_MSG_MODE AND NOT CMAKE_PROJECT_NAME STREQUAL Urho3D)
            message (${ARG_MSG_MODE}
                "Could not find ${VAR} tool in the Urho3D build tree or Urho3D SDK. Your project may not build successfully without this tool. "
                "You may have to first rebuild the Urho3D in its build tree or reinstall Urho3D SDK to get this tool built or installed properly. "
                "Alternatively, copy the ${VAR} executable manually into bin/tool subdirectory in your own project build tree.")
        endif ()
    endif ()
endmacro ()

# Macro for setting up header files installation for the SDK and the build tree (only support subset of install command arguments)
#  FILES <list> - File list to be installed
#  DIRECTORY <list> - Directory list to be installed
#  FILES_MATCHING - Option to perform file pattern matching on DIRECTORY list
#  USE_FILE_SYMLINK - Option to use file symlinks on the matched files found in the DIRECTORY list
#  BUILD_TREE_ONLY - Option to install the header files into the build tree only
#  PATTERN <list> - Pattern list to be used in file pattern matching option
#  BASE <value> - An absolute base path to be prepended to the destination path when installing to build tree, default to build tree
#  DESTINATION <value> - A relative destination path to be installed to
#  ACCUMULATE <value> - Accumulate the header files into the specified CMake variable, implies USE_FILE_SYMLINK when input list is a directory
macro (install_header_files)
    # Need to check if the destination variable is defined first because this macro could be called by downstream project that does not wish to install anything
    if (DEST_INCLUDE_DIR)
        # Parse the arguments for the underlying install command for the SDK
        cmake_parse_arguments (ARG "FILES_MATCHING;USE_FILE_SYMLINK;BUILD_TREE_ONLY" "BASE;DESTINATION;ACCUMULATE" "FILES;DIRECTORY;PATTERN" ${ARGN})
        unset (INSTALL_MATCHING)
        if (ARG_FILES)
            set (INSTALL_TYPE FILES)
            set (INSTALL_SOURCES ${ARG_FILES})
        elseif (ARG_DIRECTORY)
            set (INSTALL_TYPE DIRECTORY)
            set (INSTALL_SOURCES ${ARG_DIRECTORY})
            if (ARG_FILES_MATCHING)
                set (INSTALL_MATCHING FILES_MATCHING)
                # Our macro supports PATTERN <list> but CMake's install command does not, so convert the list to: PATTERN <value1> PATTERN <value2> ...
                foreach (PATTERN ${ARG_PATTERN})
                    list (APPEND INSTALL_MATCHING PATTERN ${PATTERN})
                endforeach ()
            endif ()
        else ()
            message (FATAL_ERROR "Couldn't setup install command because the install type is not specified.")
        endif ()
        if (NOT ARG_DESTINATION)
            message (FATAL_ERROR "Couldn't setup install command because the install destination is not specified.")
        endif ()
        if (NOT ARG_BUILD_TREE_ONLY AND NOT CMAKE_PROJECT_NAME MATCHES ^Urho3D-ExternalProject-)
            install (${INSTALL_TYPE} ${INSTALL_SOURCES} DESTINATION ${ARG_DESTINATION} ${INSTALL_MATCHING})
        endif ()

        # Reparse the arguments for the create_symlink macro to "install" the header files in the build tree
        if (NOT ARG_BASE)
            # Use build tree as base path
            if (ANDROID AND GRADLE_BUILD_DIR)
                set (ARG_BASE ${GRADLE_BUILD_DIR}/tree/${CMAKE_BUILD_TYPE}/${ANDROID_ABI})
            else ()
                set (ARG_BASE ${CMAKE_BINARY_DIR})
            endif ()
        endif ()
        foreach (INSTALL_SOURCE ${INSTALL_SOURCES})
            if (NOT IS_ABSOLUTE ${INSTALL_SOURCE})
                set (INSTALL_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/${INSTALL_SOURCE})
            endif ()
            if (INSTALL_SOURCE MATCHES /$)
                # Source is a directory
                if (ARG_USE_FILE_SYMLINK OR ARG_ACCUMULATE OR BASH_ON_WINDOWS)
                    # Use file symlink for each individual files in the source directory
                    if (IS_SYMLINK ${ARG_DESTINATION} AND NOT CMAKE_HOST_WIN32)
                        execute_process (COMMAND ${CMAKE_COMMAND} -E remove ${ARG_DESTINATION})
                    endif ()
                    set (GLOBBING_EXPRESSION RELATIVE ${INSTALL_SOURCE})
                    if (ARG_FILES_MATCHING)
                        foreach (PATTERN ${ARG_PATTERN})
                            list (APPEND GLOBBING_EXPRESSION ${INSTALL_SOURCE}${PATTERN})
                        endforeach ()
                    else ()
                        list (APPEND GLOBBING_EXPRESSION ${INSTALL_SOURCE}*)
                    endif ()
                    file (GLOB_RECURSE NAMES ${GLOBBING_EXPRESSION})
                    foreach (NAME ${NAMES})
                        get_filename_component (PATH ${ARG_DESTINATION}/${NAME} PATH)
                        # Recreate the source directory structure in the destination path
                        if (NOT EXISTS ${ARG_BASE}/${PATH})
                            file (MAKE_DIRECTORY ${ARG_BASE}/${PATH})
                        endif ()
                        create_symlink (${INSTALL_SOURCE}${NAME} ${ARG_DESTINATION}/${NAME} BASE ${ARG_BASE} FALLBACK_TO_COPY)
                        if (ARG_ACCUMULATE)
                            list (APPEND ${ARG_ACCUMULATE} ${ARG_DESTINATION}/${NAME})
                        endif ()
                    endforeach ()
                else ()
                    # Use a single symlink pointing to the source directory
                    if (NOT IS_SYMLINK ${ARG_DESTINATION} AND NOT CMAKE_HOST_WIN32)
                        execute_process (COMMAND ${CMAKE_COMMAND} -E remove_directory ${ARG_DESTINATION})
                    endif ()
                    create_symlink (${INSTALL_SOURCE} ${ARG_DESTINATION} BASE ${ARG_BASE} FALLBACK_TO_COPY)
                endif ()
            else ()
                # Source is a file (it could also be actually a directory to be treated as a "file", i.e. for creating symlink pointing to the directory)
                get_filename_component (NAME ${INSTALL_SOURCE} NAME)
                create_symlink (${INSTALL_SOURCE} ${ARG_DESTINATION}/${NAME} BASE ${ARG_BASE} FALLBACK_TO_COPY)
                if (ARG_ACCUMULATE)
                    list (APPEND ${ARG_ACCUMULATE} ${ARG_DESTINATION}/${NAME})
                endif ()
            endif ()
        endforeach ()
    endif ()
endmacro ()

# Macro for setting common output directories
macro (set_output_directories OUTPUT_PATH)
    cmake_parse_arguments (ARG LOCAL "" "" ${ARGN})
    if (ARG_LOCAL)
        unset (SCOPE)
        unset (OUTPUT_DIRECTORY_PROPERTIES)
    else ()
        set (SCOPE CMAKE_)
    endif ()
    foreach (TYPE ${ARG_UNPARSED_ARGUMENTS})
        set (${SCOPE}${TYPE}_OUTPUT_DIRECTORY ${OUTPUT_PATH})
        list (APPEND OUTPUT_DIRECTORY_PROPERTIES ${TYPE}_OUTPUT_DIRECTORY ${${TYPE}_OUTPUT_DIRECTORY})
        foreach (CONFIG ${CMAKE_CONFIGURATION_TYPES})
            string (TOUPPER ${CONFIG} CONFIG)
            set (${SCOPE}${TYPE}_OUTPUT_DIRECTORY_${CONFIG} ${OUTPUT_PATH})
            list (APPEND OUTPUT_DIRECTORY_PROPERTIES ${TYPE}_OUTPUT_DIRECTORY_${CONFIG} ${${TYPE}_OUTPUT_DIRECTORY_${CONFIG}})
        endforeach ()
        if (TYPE STREQUAL RUNTIME AND NOT ${OUTPUT_PATH} STREQUAL .)
            file (RELATIVE_PATH REL_OUTPUT_PATH ${CMAKE_BINARY_DIR} ${OUTPUT_PATH})
            set (DEST_RUNTIME_DIR ${REL_OUTPUT_PATH})
        endif ()
    endforeach ()
    if (ARG_LOCAL)
        list (APPEND TARGET_PROPERTIES ${OUTPUT_DIRECTORY_PROPERTIES})
    endif ()
endmacro ()

# Macro for setting up an executable target
# Macro arguments:
#  PRIVATE - setup executable target without installing it
#  TOOL - setup a tool executable target
#  NODEPS - setup executable target without defining Urho3D dependency libraries
#  WIN32/MACOSX_BUNDLE/EXCLUDE_FROM_ALL - see CMake help on add_executable() command
# CMake variables:
#  SOURCE_FILES - list of source files
#  INCLUDE_DIRS - list of directories for include search path
#  LIBS - list of dependent libraries that are built internally in the project
#  ABSOLUTE_PATH_LIBS - list of dependent libraries that are external to the project
#  LINK_DEPENDS - list of additional files on which a target binary depends for linking (Makefile-based generator only)
#  LINK_FLAGS - list of additional link flags
#  TARGET_PROPERTIES - list of target properties
macro (setup_executable)
    cmake_parse_arguments (ARG "PRIVATE;TOOL;NODEPS" "" "" ${ARGN})
    check_source_files ()
    add_executable (${TARGET_NAME} ${ARG_UNPARSED_ARGUMENTS} ${SOURCE_FILES})
    set (RUNTIME_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    if (ARG_PRIVATE)
        set_output_directories (. LOCAL RUNTIME PDB)
        set (RUNTIME_DIR .)
    endif ()
    if (ARG_TOOL)
        list (APPEND TARGET_PROPERTIES XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH YES)
        if (NOT ARG_PRIVATE AND NOT DEST_RUNTIME_DIR MATCHES tool)
            set_output_directories (${CMAKE_BINARY_DIR}/bin/tool LOCAL RUNTIME PDB)
            set (RUNTIME_DIR ${CMAKE_BINARY_DIR}/bin/tool)
        endif ()
    endif ()
    if (NOT ARG_NODEPS)
        define_dependency_libs (Urho3D)
    endif ()
    if (XCODE AND LUAJIT_EXE_LINKER_FLAGS_APPLE)
        # Xcode universal build linker flags when targeting 64-bit OSX with LuaJIT enabled
        list (APPEND TARGET_PROPERTIES XCODE_ATTRIBUTE_OTHER_LDFLAGS[arch=x86_64] "${LUAJIT_EXE_LINKER_FLAGS_APPLE} $(OTHER_LDFLAGS)")
    endif ()
    _setup_target ()

    if (URHO3D_SCP_TO_TARGET)
        add_custom_command (TARGET ${TARGET_NAME} POST_BUILD COMMAND scp $<TARGET_FILE:${TARGET_NAME}> ${URHO3D_SCP_TO_TARGET} || exit 0
            COMMENT "Scp-ing ${TARGET_NAME} executable to target system")
    endif ()
    if (WIN32 AND NOT ARG_NODEPS AND URHO3D_LIB_TYPE STREQUAL SHARED)
        # Make a copy of the Urho3D DLL to the runtime directory in the build tree
        if (TARGET Urho3D)
            add_custom_command (TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Urho3D> ${RUNTIME_DIR})
            add_make_clean_files (${RUNTIME_DIR}/$<TARGET_FILE_NAME:Urho3D>)
        else ()
            foreach (DLL ${URHO3D_DLL})
                add_custom_command (TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DLL} ${RUNTIME_DIR})
                add_make_clean_files (${RUNTIME_DIR}/${DLL})
            endforeach ()
        endif ()
    endif ()
    if (DIRECT3D_DLL AND NOT ARG_NODEPS)
        # Make a copy of the D3D DLL to the runtime directory in the build tree
        add_custom_command (TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DIRECT3D_DLL} ${RUNTIME_DIR})
        add_make_clean_files (${RUNTIME_DIR}/${DIRECT3D_DLL})
    endif ()
    # Need to check if the destination variable is defined first because this macro could be called by downstream project that does not wish to install anything
    if (NOT ARG_PRIVATE)
        if (WEB AND DEST_BUNDLE_DIR)
            set (LOCATION $<TARGET_FILE_DIR:${TARGET_NAME}>)
            unset (FILES)
            foreach (EXT data html html.map js wasm)
                list (APPEND FILES ${LOCATION}/${TARGET_NAME}.${EXT})
            endforeach ()
            install (FILES ${FILES} DESTINATION ${DEST_BUNDLE_DIR} OPTIONAL)
        elseif (DEST_RUNTIME_DIR AND (DEST_BUNDLE_DIR OR NOT (IOS OR TVOS)))
            install (TARGETS ${TARGET_NAME} RUNTIME DESTINATION ${DEST_RUNTIME_DIR} BUNDLE DESTINATION ${DEST_BUNDLE_DIR})
            if (WIN32 AND NOT ARG_NODEPS AND URHO3D_LIB_TYPE STREQUAL SHARED AND NOT URHO3D_DLL_INSTALLED)
                if (TARGET Urho3D)
                    install (FILES $<TARGET_FILE:Urho3D> DESTINATION ${DEST_RUNTIME_DIR})
                else ()
                    install (FILES ${URHO3D_DLL} DESTINATION ${DEST_RUNTIME_DIR})
                endif ()
                set (URHO3D_DLL_INSTALLED TRUE)
            endif ()
            if (DIRECT3D_DLL AND NOT DIRECT3D_DLL_INSTALLED)
                # Make a copy of the D3D DLL to the runtime directory in the installed location
                install (FILES ${DIRECT3D_DLL} DESTINATION ${DEST_RUNTIME_DIR})
                set (DIRECT3D_DLL_INSTALLED TRUE)
            endif ()
        endif ()
    endif ()
endmacro ()

# Macro for setting up a library target
# Macro arguments:
#  NODEPS - setup library target without defining Urho3D dependency libraries (applicable for downstream projects)
#  STATIC/SHARED/MODULE/EXCLUDE_FROM_ALL - see CMake help on add_library() command
# CMake variables:
#  SOURCE_FILES - list of source files
#  INCLUDE_DIRS - list of directories for include search path
#  LIBS - list of dependent libraries that are built internally in the project
#  ABSOLUTE_PATH_LIBS - list of dependent libraries that are external to the project
#  LINK_DEPENDS - list of additional files on which a target binary depends for linking (Makefile-based generator only)
#  LINK_FLAGS - list of additional link flags
#  TARGET_PROPERTIES - list of target properties
macro (setup_library)
    cmake_parse_arguments (ARG NODEPS "" "" ${ARGN})
    check_source_files ()
    add_library (${TARGET_NAME} ${ARG_UNPARSED_ARGUMENTS} ${SOURCE_FILES})
    get_target_property (LIB_TYPE ${TARGET_NAME} TYPE)
    if (NOT ARG_NODEPS AND NOT PROJECT_NAME STREQUAL Urho3D)
        define_dependency_libs (Urho3D)
    endif ()
    if (XCODE AND LUAJIT_SHARED_LINKER_FLAGS_APPLE AND LIB_TYPE STREQUAL SHARED_LIBRARY)
        list (APPEND TARGET_PROPERTIES XCODE_ATTRIBUTE_OTHER_LDFLAGS[arch=x86_64] "${LUAJIT_SHARED_LINKER_FLAGS_APPLE} $(OTHER_LDFLAGS)")    # Xcode universal build linker flags when targeting 64-bit OSX with LuaJIT enabled
    endif ()
    _setup_target ()

    if (PROJECT_NAME STREQUAL Urho3D)
        # Accumulate all the dependent static libraries that are used in building the Urho3D library itself
        if (NOT ${TARGET_NAME} STREQUAL Urho3D AND LIB_TYPE STREQUAL STATIC_LIBRARY)
            set (STATIC_LIBRARY_TARGETS ${STATIC_LIBRARY_TARGETS} ${TARGET_NAME} PARENT_SCOPE)
            # When performing Xcode CI build suppress all the warnings for 3rd party libraries because there are just too many of them
            if (XCODE AND DEFINED ENV{CI})
                set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")
                set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
            endif ()
        endif ()
    elseif (URHO3D_SCP_TO_TARGET)
        add_custom_command (TARGET ${TARGET_NAME} POST_BUILD COMMAND scp $<TARGET_FILE:${TARGET_NAME}> ${URHO3D_SCP_TO_TARGET} || exit 0
            COMMENT "Scp-ing ${TARGET_NAME} library to target system")
    endif ()
endmacro ()

# Macro for setting up an executable target with resources to copy/package/bundle/preload
# Macro arguments:
#  NODEPS - setup executable target without defining Urho3D dependency libraries
#  NOBUNDLE - do not use MACOSX_BUNDLE even when URHO3D_MACOSX_BUNDLE build option is enabled
#  WIN32/MACOSX_BUNDLE/EXCLUDE_FROM_ALL - see CMake help on add_executable() command
# CMake variables:
#  RESOURCE_DIRS - list of resource directories (will be packaged into *.pak when URHO3D_PACKAGING build option is set)
#  RESOURCE_FILES - list of additional resource files (will not be packaged into *.pak in any case)
#  SOURCE_FILES - list of source files
#  INCLUDE_DIRS - list of directories for include search path
#  LIBS - list of dependent libraries that are built internally in the project
#  ABSOLUTE_PATH_LIBS - list of dependent libraries that are external to the project
#  LINK_DEPENDS - list of additional files on which a target binary depends for linking (Makefile-based generator only)
#  LINK_FLAGS - list of additional link flags
#  TARGET_PROPERTIES - list of target properties
macro (setup_main_executable)
    cmake_parse_arguments (ARG "NOBUNDLE;MACOSX_BUNDLE;WIN32" "" "" ${ARGN})
    if (NOT RESOURCE_DIRS)
        define_resource_dirs ()
    endif ()
    if (ANDROID)
        # Setup target as main shared library
        setup_library (SHARED)
        if (DEST_LIBRARY_DIR)
            install (TARGETS ${TARGET_NAME} LIBRARY DESTINATION ${DEST_LIBRARY_DIR} ARCHIVE DESTINATION ${DEST_LIBRARY_DIR})
        endif ()
    else ()
        # Setup target as executable
        if (WIN32)
            if (NOT URHO3D_WIN32_CONSOLE OR ARG_WIN32)
                set (EXE_TYPE WIN32)
            endif ()
            list (APPEND TARGET_PROPERTIES DEBUG_POSTFIX _d)
        elseif (IOS)
            set (EXE_TYPE MACOSX_BUNDLE)
            list (APPEND TARGET_PROPERTIES XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY 1,2 MACOSX_BUNDLE_INFO_PLIST iOSBundleInfo.plist.template)
        elseif (TVOS)
            set (EXE_TYPE MACOSX_BUNDLE)
            list (APPEND TARGET_PROPERTIES XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY 3 MACOSX_BUNDLE_INFO_PLIST tvOSBundleInfo.plist.template)
        elseif (APPLE)
            if ((URHO3D_MACOSX_BUNDLE OR ARG_MACOSX_BUNDLE) AND NOT ARG_NOBUNDLE)
                set (EXE_TYPE MACOSX_BUNDLE)
                list (APPEND TARGET_PROPERTIES MACOSX_BUNDLE_INFO_PLIST MacOSXBundleInfo.plist.template)
            endif ()
        elseif (WEB)
            if (EMSCRIPTEN)
                # Output to HTML when a HTML shell-file is being added in source files list
                foreach (FILE ${SOURCE_FILES})
                    get_property (EMCC_OPTION SOURCE ${FILE} PROPERTY EMCC_OPTION)
                    if (EMCC_OPTION STREQUAL shell-file)
                        list (APPEND TARGET_PROPERTIES SUFFIX .html)
                        break ()
                    endif ()
                endforeach ()
                # Auto adding the HTML shell-file if necessary
                if (NOT EMCC_OPTION STREQUAL shell-file)
                    if (URHO3D_TESTING OR EMSCRIPTEN_AUTO_SHELL)
                        add_html_shell ()
                        list (APPEND TARGET_PROPERTIES SUFFIX .html)
                        set (HAS_SHELL_FILE 1)
                    endif ()
                endif ()
            endif ()
        endif ()
        setup_executable (${EXE_TYPE} ${ARG_UNPARSED_ARGUMENTS})
        if (HAS_SHELL_FILE)
            get_target_property (LOCATION ${TARGET_NAME} LOCATION)
            get_filename_component (NAME_WE ${LOCATION} NAME_WE)
            add_make_clean_files ($<TARGET_FILE_DIR:${TARGET_NAME}>/${NAME_WE}.js $<TARGET_FILE_DIR:${TARGET_NAME}>/${NAME_WE}.wasm)
        endif ()
    endif ()
    # Setup custom resource checker target
    if ((EXE_TYPE STREQUAL MACOSX_BUNDLE OR URHO3D_PACKAGING) AND RESOURCE_DIRS)
        if (URHO3D_PACKAGING)
            # Urho3D project builds the PackageTool as required; downstream project uses PackageTool found in the Urho3D build tree or Urho3D SDK
            find_Urho3d_tool (PACKAGE_TOOL PackageTool
                HINTS ${CMAKE_BINARY_DIR}/bin/tool ${URHO3D_HOME}/bin/tool
                DOC "Path to PackageTool" MSG_MODE WARNING)
            if (CMAKE_PROJECT_NAME STREQUAL Urho3D)
                set (PACKAGING_DEP DEPENDS PackageTool)
            endif ()
            set (PACKAGING_COMMENT " and packaging")
        endif ()
        # Share a same custom target that checks for a same resource dirs list
        foreach (DIR ${RESOURCE_DIRS})
            string (MD5 MD5 ${DIR})
            set (MD5ALL ${MD5ALL}${MD5})
            if (CMAKE_HOST_WIN32)
                # On Windows host, always assumes there are changes so resource dirs would be repackaged in each build, however, still make sure the *.pak timestamp is not altered unnecessarily
                if (URHO3D_PACKAGING)
                    set (PACKAGING_COMMAND && echo Packaging ${DIR}... && ${PACKAGE_TOOL} ${DIR} ${RESOURCE_${DIR}_PATHNAME}.new -c -q && ${CMAKE_COMMAND} -E copy_if_different ${RESOURCE_${DIR}_PATHNAME}.new ${RESOURCE_${DIR}_PATHNAME} && ${CMAKE_COMMAND} -E remove ${RESOURCE_${DIR}_PATHNAME}.new)
                endif ()
                list (APPEND COMMANDS COMMAND ${CMAKE_COMMAND} -E touch ${DIR} ${PACKAGING_COMMAND})
            else ()
                # On Unix-like hosts, detect the changes in the resource directory recursively so they are only repackaged and/or rebundled (Xcode only) as necessary
                if (URHO3D_PACKAGING)
                    set (PACKAGING_COMMAND && echo Packaging ${DIR}... && ${PACKAGE_TOOL} ${DIR} ${RESOURCE_${DIR}_PATHNAME} -c -q)
                    set (OUTPUT_COMMAND test -e ${RESOURCE_${DIR}_PATHNAME} || \( true ${PACKAGING_COMMAND} \))
                else ()
                    set (OUTPUT_COMMAND true)   # Nothing to output
                endif ()
                list (APPEND COMMANDS COMMAND echo Checking ${DIR}... && bash -c \"\(\( `find ${DIR} -newer ${DIR} |wc -l` \)\)\" && touch -cm ${DIR} ${PACKAGING_COMMAND} || ${OUTPUT_COMMAND})
            endif ()
            add_make_clean_files (${RESOURCE_${DIR}_PATHNAME})
        endforeach ()
        string (MD5 MD5ALL ${MD5ALL})
        # Ensure the resource check is done before building the main executable target
        if (NOT RESOURCE_CHECK_${MD5ALL})
            set (RESOURCE_CHECK RESOURCE_CHECK)
            while (TARGET ${RESOURCE_CHECK})
                string (RANDOM RANDOM)
                set (RESOURCE_CHECK RESOURCE_CHECK_${RANDOM})
            endwhile ()
            set (RESOURCE_CHECK_${MD5ALL} ${RESOURCE_CHECK} CACHE INTERNAL "Resource check hash map")
        endif ()
        if (NOT TARGET ${RESOURCE_CHECK_${MD5ALL}})
            add_custom_target (${RESOURCE_CHECK_${MD5ALL}} ALL ${COMMANDS} ${PACKAGING_DEP} COMMENT "Checking${PACKAGING_COMMENT} resource directories")
        endif ()
        add_dependencies (${TARGET_NAME} ${RESOURCE_CHECK_${MD5ALL}})
    endif ()
    # Only need to install the resource directories once in case they are referenced by multiple targets
    if (RESOURCE_DIRS AND DEST_SHARE_DIR)
        foreach (DIR ${RESOURCE_DIRS})
            list (FIND INSTALLED_RESOURCE_DIRS ${DIR} FOUND_INDEX)
            if (FOUND_INDEX EQUAL -1)
                install (DIRECTORY ${DIR} DESTINATION ${DEST_SHARE_DIR}/Resources)
                list (APPEND INSTALLED_RESOURCE_DIRS ${DIR})
            endif ()
            # This cache variable is used to keep track of whether a resource directory has been instructed to be installed by CMake or not
            set (INSTALLED_RESOURCE_DIRS ${INSTALLED_RESOURCE_DIRS} CACHE INTERNAL "Installed resource dirs")
        endforeach ()
    endif ()
    # Define a custom command for stripping the main target executable for Release build configuration,
    # but only for platforms that support it and require it (for Android, let Android plugin handle it)
    if (CMAKE_BUILD_TYPE STREQUAL Release AND NOT ANDROID AND NOT WEB AND NOT MSVC)
        add_custom_command (TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_STRIP} $<TARGET_FILE:${TARGET_NAME}>)
    endif ()
endmacro ()

# This cache variable is used to keep track of whether a resource directory has been instructed to be installed by CMake or not
unset (INSTALLED_RESOURCE_DIRS CACHE)

# Macro for setting up dependency lib for compilation and linking of a target (to be used internally)
macro (_setup_target)
    # Include directories
    include_directories (${INCLUDE_DIRS})
    # Link libraries
    define_dependency_libs (${TARGET_NAME})
    target_link_libraries (${TARGET_NAME} ${ABSOLUTE_PATH_LIBS} ${LIBS})
    # Enable PCH if requested
    if (${TARGET_NAME}_HEADER_PATHNAME)
        enable_pch (${${TARGET_NAME}_HEADER_PATHNAME})
    endif ()
    # Extra compiler flags for Xcode which are dynamically changed based on active arch in order to support Mach-O universal binary targets
    # We don't add the ABI flag for Xcode because it automatically passes '-arch i386' compiler flag when targeting 32 bit which does the same thing as '-m32'
    if (XCODE)
        # Speed up build when in Debug configuration by building active arch only
        list (FIND TARGET_PROPERTIES XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH ATTRIBUTE_ALREADY_SET)
        if (ATTRIBUTE_ALREADY_SET EQUAL -1)
            list (APPEND TARGET_PROPERTIES XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH $<$<CONFIG:Debug>:YES>)
        endif ()
        if (NOT URHO3D_SSE)
            # Nullify the Clang default so that it is consistent with GCC
            list (APPEND TARGET_PROPERTIES XCODE_ATTRIBUTE_OTHER_CFLAGS[arch=i386] "-mno-sse $(OTHER_CFLAGS)")
            list (APPEND TARGET_PROPERTIES XCODE_ATTRIBUTE_OTHER_CPLUSPLUSFLAGS[arch=i386] "-mno-sse $(OTHER_CPLUSPLUSFLAGS)")
        endif ()
    endif ()
    # Extra linker flags for Emscripten
    if (EMSCRIPTEN)
        # These flags are set only once in the main executable
        if (NOT LIB_TYPE)   # LIB_TYPE is empty for executable target
            list (APPEND LINK_FLAGS "-s TOTAL_MEMORY=${EMSCRIPTEN_TOTAL_MEMORY}")
            if (EMSCRIPTEN_ALLOW_MEMORY_GROWTH)
                list (APPEND LINK_FLAGS "-s ALLOW_MEMORY_GROWTH=1 --no-heap-copy")
            endif ()
            if (EMSCRIPTEN_SHARE_DATA)
                list (APPEND LINK_FLAGS "--pre-js \"${CMAKE_BINARY_DIR}/Source/pak-loader.js\"")
            endif ()
            if (URHO3D_TESTING)
                list (APPEND LINK_FLAGS --emrun)
            else ()
                # If not using EMRUN then we need to include the emrun_prejs.js manually in order to process the request parameters as app's arguments correctly
                list (APPEND LINK_FLAGS "--pre-js \"${EMSCRIPTEN_ROOT_PATH}/src/emrun_prejs.js\"")
            endif ()
            # These flags are here instead of in the CMAKE_EXE_LINKER_FLAGS so that they do not interfere with the auto-detection logic during initial configuration
            list (APPEND LINK_FLAGS "-s NO_EXIT_RUNTIME=1 -s ERROR_ON_UNDEFINED_SYMBOLS=1")
        endif ()
        # Pass additional source files to linker with the supported flags, such as: js-library, pre-js, post-js, embed-file, preload-file, shell-file
        foreach (FILE ${SOURCE_FILES})
            get_property (EMCC_OPTION SOURCE ${FILE} PROPERTY EMCC_OPTION)
            if (EMCC_OPTION)
                unset (EMCC_FILE_ALIAS)
                unset (EMCC_EXCLUDE_FILE)
                unset (USE_PRELOAD_CACHE)
                if (EMCC_OPTION STREQUAL embed-file OR EMCC_OPTION STREQUAL preload-file)
                    get_property (EMCC_FILE_ALIAS SOURCE ${FILE} PROPERTY EMCC_FILE_ALIAS)
                    if (EMCC_FILE_ALIAS)
                        set (EMCC_FILE_ALIAS "@\"${EMCC_FILE_ALIAS}\"")
                    endif ()
                    get_property (EMCC_EXCLUDE_FILE SOURCE ${FILE} PROPERTY EMCC_EXCLUDE_FILE)
                    if (EMCC_EXCLUDE_FILE)
                        set (EMCC_EXCLUDE_FILE " --exclude-file \"${EMCC_EXCLUDE_FILE}\"")
                    else ()
                        list (APPEND LINK_DEPENDS ${FILE})
                    endif ()
                    if (EMCC_OPTION STREQUAL preload-file)
                        set (USE_PRELOAD_CACHE " --use-preload-cache")
                    endif ()
                endif ()
                list (APPEND LINK_FLAGS "--${EMCC_OPTION} \"${FILE}\"${EMCC_FILE_ALIAS}${EMCC_EXCLUDE_FILE}${USE_PRELOAD_CACHE}")
            endif ()
        endforeach ()
    endif ()
    # Set additional linker dependencies (only work for Makefile-based generator according to CMake documentation)
    if (LINK_DEPENDS)
        string (REPLACE ";" "\;" LINK_DEPENDS "${LINK_DEPENDS}")        # Stringify for string replacement
        list (APPEND TARGET_PROPERTIES LINK_DEPENDS "${LINK_DEPENDS}")  # Stringify with semicolons already escaped
        unset (LINK_DEPENDS)
    endif ()
    # Set additional linker flags
    if (LINK_FLAGS)
        string (REPLACE ";" " " LINK_FLAGS "${LINK_FLAGS}")
        list (APPEND TARGET_PROPERTIES LINK_FLAGS ${LINK_FLAGS})
        unset (LINK_FLAGS)
    endif ()
    if (TARGET_PROPERTIES)
        set_target_properties (${TARGET_NAME} PROPERTIES ${TARGET_PROPERTIES})
        unset (TARGET_PROPERTIES)
    endif ()
    # Create symbolic links in the build tree
    if (NOT ANDROID AND NOT URHO3D_PACKAGING)
        # Ensure the asset root directory exist before creating the symlinks
        file (MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
        foreach (I ${RESOURCE_DIRS})
            get_filename_component (NAME ${I} NAME)
            if (NOT EXISTS ${CMAKE_BINARY_DIR}/bin/${NAME} AND EXISTS ${I})
                create_symlink (${I} ${CMAKE_BINARY_DIR}/bin/${NAME} FALLBACK_TO_COPY)
            endif ()
        endforeach ()
    endif ()
    # Workaround CMake/Xcode generator bug where it always appends '/build' path element to SYMROOT attribute and as such the items in Products are always rendered as red in the Xcode as if they are not yet built
    if (NOT DEFINED ENV{TRAVIS})
        if (XCODE AND NOT CMAKE_PROJECT_NAME MATCHES ^Urho3D-ExternalProject-)
            file (MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/build)
            get_target_property (LOCATION ${TARGET_NAME} LOCATION)
            string (REGEX REPLACE "^.*\\$\\(CONFIGURATION\\)" $(CONFIGURATION) SYMLINK ${LOCATION})
            get_filename_component (DIRECTORY ${SYMLINK} PATH)
            add_custom_command (TARGET ${TARGET_NAME} POST_BUILD
                COMMAND mkdir -p ${DIRECTORY} && ln -sf $<TARGET_FILE:${TARGET_NAME}> ${DIRECTORY}/$<TARGET_FILE_NAME:${TARGET_NAME}>
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/build)
        endif ()
    endif ()
endmacro()

# Macro for setting up a test case
macro (setup_test)
    if (URHO3D_TESTING)
        cmake_parse_arguments (ARG "" NAME OPTIONS ${ARGN})
        if (NOT ARG_NAME)
            set (ARG_NAME ${TARGET_NAME})
        endif ()
        list (APPEND ARG_OPTIONS -timeout ${URHO3D_TEST_TIMEOUT})
        if (WEB)
            if (EMSCRIPTEN)
                math (EXPR EMRUN_TIMEOUT "2 * ${URHO3D_TEST_TIMEOUT}")
                add_test (NAME ${ARG_NAME} COMMAND ${EMRUN} --browser ${EMSCRIPTEN_EMRUN_BROWSER} --timeout ${EMRUN_TIMEOUT} --kill_exit ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET_NAME}.html ${ARG_OPTIONS})
            endif ()
        else ()
            add_test (NAME ${ARG_NAME} COMMAND ${TARGET_NAME} ${ARG_OPTIONS})
        endif ()
    endif ()
endmacro ()

# Macro for setting up linter
macro (setup_lint)
    if (URHO3D_LINT)
        find_program (CLANG_TIDY clang-tidy NO_CMAKE_FIND_ROOT_PATH)
        if (CLANG_TIDY)
            set (URHO3D_PCH 0)
            set (CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY} -config=)
            set (CMAKE_EXPORT_COMPILE_COMMANDS 1)
        else ()
            message (FATAL_ERROR "Ensure clang-tidy host tool is installed and can be found in the PATH environment variable.")
        endif ()
    endif ()
endmacro ()

# Macro for resetting the linter (intended to be called in a child scope where its parent scope has the linter setup)
macro (reset_lint)
    unset (CMAKE_CXX_CLANG_TIDY)
    unset (CMAKE_EXPORT_COMPILE_COMMANDS)
endmacro ()

# Set common binary output directory if not already set (note that this module can be included in an external project which may already have DEST_RUNTIME_DIR preset)
if (NOT DEST_RUNTIME_DIR)
    set_output_directories (${CMAKE_BINARY_DIR}/bin RUNTIME PDB)
endif ()

if (WEB)
    if (EMSCRIPTEN_SHARE_DATA AND NOT EXISTS ${CMAKE_BINARY_DIR}/Source/pak-loader.js)
        file (WRITE ${CMAKE_BINARY_DIR}/Source/pak-loader.js "var Module;if(typeof Module==='undefined')Module=eval('(function(){try{return Module||{}}catch(e){return{}}})()');var s=document.createElement('script');s.src='${CMAKE_PROJECT_NAME}.js';document.body.appendChild(s);Module['preRun'].push(function(){Module['addRunDependency']('${CMAKE_PROJECT_NAME}.js.loader')});s.onload=function(){Module['removeRunDependency']('${CMAKE_PROJECT_NAME}.js.loader')};")
    endif ()
endif ()

# Warn user if PATH environment variable has not been correctly set for using ccache
if (NOT CMAKE_HOST_WIN32 AND "$ENV{USE_CCACHE}")
    if (APPLE)
        set (WHEREIS brew info ccache)
    else ()
        set (WHEREIS whereis -b ccache)
    endif ()
    execute_process (COMMAND ${WHEREIS} COMMAND grep -o \\S*lib\\S* RESULT_VARIABLE EXIT_CODE OUTPUT_VARIABLE CCACHE_SYMLINK ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
    if (EXIT_CODE EQUAL 0 AND NOT $ENV{PATH} MATCHES "${CCACHE_SYMLINK}")  # Need to stringify because CCACHE_SYMLINK variable could be empty when the command failed
        message (WARNING "The lib directory containing the ccache symlinks (${CCACHE_SYMLINK}) has not been added in the PATH environment variable. "
            "This is required to enable ccache support for native compiler toolchain. CMake has been configured to use the actual compiler toolchain instead of ccache. "
            "In order to rectify this, the build tree must be regenerated after the PATH environment variable has been adjusted accordingly.")
    endif ()
endif ()

# Post-CMake fixes
if (IOS)
    # TODO: can be removed when CMake minimum required has reached 3.4
    if (CMAKE_VERSION VERSION_LESS 3.4)
        # Due to a bug in the CMake/Xcode generator (fixed in 3.4) that prevents iOS targets (library and bundle) to be installed correctly
        # (see http://public.kitware.com/Bug/bug_relationship_graph.php?bug_id=12506&graph=dependency),
        # below temporary fix is required to work around the bug
        list (APPEND POST_CMAKE_FIXES COMMAND sed -i '' 's/\$$\(EFFECTIVE_PLATFORM_NAME\)//g' ${CMAKE_BINARY_DIR}/CMakeScripts/install_postBuildPhase.make* || exit 0)
    endif ()
elseif (TVOS)
    # Almost the same bug as iOS one above but not quite, most probably because CMake does not support AppleTV platform yet
    list (APPEND POST_CMAKE_FIXES COMMAND sed -i '' 's/\)\$$\(EFFECTIVE_PLATFORM_NAME\)/\) -DEFFECTIVE_PLATFORM_NAME=$$\(EFFECTIVE_PLATFORM_NAME\)/g' ${CMAKE_BINARY_DIR}/CMakeScripts/install_postBuildPhase.make* || exit 0)
    add_custom_target (APPLETV_POST_CMAKE_FIX COMMAND sed -i '' -E 's,\(Debug|RelWithDebInfo|Release\)/,$$\(CONFIGURATION\)$$\(EFFECTIVE_PLATFORM_NAME\)/,g' ${CMAKE_BINARY_DIR}/Source/Urho3D/CMakeScripts/Urho3D_cmakeRulesBuildPhase.make* || exit 0)
endif ()
if (POST_CMAKE_FIXES)
    add_custom_target (POST_CMAKE_FIXES ALL ${POST_CMAKE_FIXES} COMMENT "Applying post-cmake fixes")
endif ()
