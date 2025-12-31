cmake_minimum_required(VERSION 3.5)
project(ExampleModule LANGUAGES CXX)

# Build a shared library for the example module.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options to control produced filenames and Windows exports
option(REMOVE_LIB_PREFIX_ON_UNIX "Remove the 'lib' prefix on Unix-like systems (produces ExampleModule.so instead of libExampleModule.so)" ON)
option(WINDOWS_EXPORT_ALL_SYMBOLS_OPTION "When building on Windows, export all symbols automatically (convenient for DLLs)" ON)

# The module sources: collect everything under src/ so adding/removing files
# there doesn't require editing this CMakeLists.
file(GLOB_RECURSE MODULE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

if(NOT MODULE_SOURCES)
    message(FATAL_ERROR "No source files found in ${CMAKE_CURRENT_SOURCE_DIR}/src")
endif()

add_library(ExampleModule SHARED ${MODULE_SOURCES})

# Make sure the module can find FractalCORE.hpp and other headers. Add the
# repository header folder and src so local includes work.
target_include_directories(ExampleModule PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# By default CMake will produce libExampleModule.so on Unix. Some engines
# expect the module file to be named without the lib prefix (e.g. "ExampleModule.so").
# If the option REMOVE_LIB_PREFIX_ON_UNIX is ON, remove the prefix on Unix to match that convention.
if(UNIX AND REMOVE_LIB_PREFIX_ON_UNIX)
    set_target_properties(ExampleModule PROPERTIES PREFIX "")
endif()

# Ensure the produced file base name is ExampleModule on all platforms; the
# loader will append .so/.dll as appropriate for the platform.
set_target_properties(ExampleModule PROPERTIES OUTPUT_NAME "ExampleModule")

# Optional: export all symbols on Windows to avoid needing an explicit
# DEF file when building a DLL. Controlled by WINDOWS_EXPORT_ALL_SYMBOLS_OPTION.
if(WIN32 AND WINDOWS_EXPORT_ALL_SYMBOLS_OPTION)
    set_target_properties(ExampleModule PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# Installation (optional): place module into lib or modules folder. Uncomment
# to enable installation.
#install(TARGETS ExampleModule LIBRARY DESTINATION lib)

# Usage:
#   mkdir build && cd build
#   cmake ..
#   cmake --build . --config Release
#
# On Linux this will produce `ExampleModule.so` (no lib prefix if enabled). On Windows
# the produced file will be `ExampleModule.dll`.
