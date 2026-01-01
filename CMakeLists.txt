cmake_minimum_required(VERSION 3.5)
project(AmethystXRenderEngineModule LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(REMOVE_LIB_PREFIX_ON_UNIX "Remove the 'lib' prefix on Unix-like systems (produces AmethystXRenderEngineModule.so instead of libAmethystXRenderEngineModule.so)" ON)
option(WINDOWS_EXPORT_ALL_SYMBOLS_OPTION "When building on Windows, export all symbols automatically (convenient for DLLs)" ON)
option(USE_GLFW "Link with GLFW and enable example GL demo code" ON)
option(USE_GLAD "Compile and link glad from external/glad (if present)" ON)

# Collect module sources
file(GLOB_RECURSE MODULE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

if(NOT MODULE_SOURCES)
    message(FATAL_ERROR "No source files found in ${CMAKE_CURRENT_SOURCE_DIR}/src")
endif()

add_library(AmethystXRenderEngineModule SHARED ${MODULE_SOURCES})

# Include paths for project headers
target_include_directories(AmethystXRenderEngineModule PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Optionally remove lib prefix on Unix
if(UNIX AND REMOVE_LIB_PREFIX_ON_UNIX)
    set_target_properties(AmethystXRenderEngineModule PROPERTIES PREFIX "")
endif()

set_target_properties(AmethystXRenderEngineModule PROPERTIES OUTPUT_NAME "AmethystXRenderEngineModule")

if(WIN32 AND WINDOWS_EXPORT_ALL_SYMBOLS_OPTION)
    set_target_properties(AmethystXRenderEngineModule PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# Link common system libs
find_package(Threads REQUIRED)
target_link_libraries(AmethystXRenderEngineModule PRIVATE ${CMAKE_THREAD_LIBS_INIT} dl)

# GLFW integration (optional)
if(USE_GLFW)
    # Try pkg-config first (works on many Linux/Termux setups)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(GLFW_PKG QUIET glfw3)
    endif()

    if(GLFW_PKG_FOUND)
        message(STATUS "Found GLFW via pkg-config")
        target_include_directories(AmethystXRenderEngineModule PRIVATE ${GLFW_PKG_INCLUDE_DIRS})
        target_link_libraries(AmethystXRenderEngineModule PRIVATE ${GLFW_PKG_LIBRARIES})
        target_compile_options(AmethystXRenderEngineModule PRIVATE ${GLFW_PKG_CFLAGS_OTHER})
    else()
        # Fallback to CMake package
        find_package(glfw3 QUIET)
        if(glfw3_FOUND)
            message(STATUS "Found GLFW via find_package")
            target_link_libraries(AmethystXRenderEngineModule PRIVATE glfw)
        else()
            message(WARNING "GLFW not found. Build will continue but GLFW-dependent code will fail at link time.")
        endif()
    endif()
endif()
# glad integration
if(USE_GLAD)
    set(GLAD_SRC "${CMAKE_CURRENT_SOURCE_DIR}/external/glad/src/glad.c")
    set(GLAD_INC "${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include")
    
    if(EXISTS ${GLAD_SRC})
        message(STATUS "Including glad from ${GLAD_SRC}")
        add_library(glad STATIC ${GLAD_SRC})
        target_include_directories(glad PUBLIC ${GLAD_INC})
        set_target_properties(glad PROPERTIES POSITION_INDEPENDENT_CODE ON)
        target_link_libraries(AmethystXRenderEngineModule PRIVATE glad)
    else()
        message(WARNING "glad not found in external/glad!")
    endif()
endif()

# Helpful compile flags for Unix (optional)
if(UNIX)
    target_compile_options(AmethystXRenderEngineModule PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Installation (optional)
# install(TARGETS AmethystXRenderEngineModule LIBRARY DESTINATION lib)

# Usage instructions
message(STATUS "To build: mkdir build && cd build && cmake -G Ninja .. && cmake --build .")
