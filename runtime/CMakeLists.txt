cmake_minimum_required(VERSION 3.16)
project(forge_runtime LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Find packages
find_package(Threads REQUIRED)

# ============================================================================
# llama.cpp configuration
# ============================================================================
set(LLAMA_BUILD_TESTS OFF CACHE BOOL "llama: build tests")
set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "llama: build examples")
set(LLAMA_BUILD_SERVER OFF CACHE BOOL "llama: build server")
set(LLAMA_NATIVE ON CACHE BOOL "llama: enable native optimizations")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "build shared libraries")

# Path to llama.cpp
set(LLAMA_CPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/llama.cpp")

# Check if llama.cpp exists
if(NOT EXISTS "${LLAMA_CPP_DIR}/CMakeLists.txt")
	message(FATAL_ERROR
		"llama.cpp not found at ${LLAMA_CPP_DIR}\n"
		"Please run: git submodule update --init --recursive"
	)
endif()

message(STATUS "Found llama.cpp at: ${LLAMA_CPP_DIR}")

# Add llama.cpp
add_subdirectory(${LLAMA_CPP_DIR} llama.cpp EXCLUDE_FROM_ALL)

# ============================================================================
# Main executable
# ============================================================================
add_executable(forge_runtime
	src/main.cpp
	src/ipc/socket_server.cpp
	src/core/action_dispatcher.cpp
	src/core/tool_registry.cpp
	src/llm/llama_engine.cpp
	src/llm/llama_config.cpp
	src/tools/list_dir_tool.cpp
	src/tools/argument_validator.cpp
)

# Include directories
target_include_directories(forge_runtime PRIVATE
    include
    ${LLAMA_CPP_DIR}/include
    ${LLAMA_CPP_DIR}/ggml/include
)

# Link libraries
target_link_libraries(forge_runtime PRIVATE
	llama
	Threads::Threads
)

# Compiler flags
target_compile_options(forge_runtime PRIVATE
	$<$<CONFIG:Release>:-O3 -march=native -mtune=native>
	$<$<CONFIG:Debug>:-g -O0>
	-Wall -Wextra
)

# Filesystem library for older compilers
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
	target_link_libraries(forge_runtime PRIVATE stdc++fs)
endif()

# Installation
install(TARGETS forge_runtime DESTINATION bin)

# Print configuration summary
message(STATUS "==========================================")
message(STATUS "Forge Runtime Configuration:")
message(STATUS "  Build type:        ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard:      ${CMAKE_CXX_STANDARD}")
message(STATUS "  llama.cpp dir:     ${LLAMA_CPP_DIR}")
message(STATUS "  Install prefix:    ${CMAKE_INSTALL_PREFIX}")
message(STATUS "==========================================")
