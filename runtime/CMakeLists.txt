cmake_minimum_required(VERSION 3.16)
project(forge_runtime LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(forge_runtime
	src/main.cpp
	src/ipc/socket_server.cpp
	src/core/action_dispatcher.cpp
	src/core/tool_registry.cpp
	src/tools/list_dir_tool.cpp
)

target_include_directories(forge_runtime PRIVATE
	include
)
