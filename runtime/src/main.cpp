#include <iostream>
#include "ipc/socket_server.h"
#include "core/tool_registry.h"
#include "tools/list_dir_tool.h"

int main()
{
	std::cout << "[forge-runtime] starting...\n";

	// 1. Tool registry
	ToolRegistry registry;
	registry.register_tool(std::make_unique<ListDirTool>());

	// 2. Dispatcher
	ActionDispatcher dispatcher(registry);

	// 3. IPC server
	SocketServer server("/tmp/forge-ai.sock", dispatcher);
	server.run();

	return 0;
}
