#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include "core/action_dispatcher.h"

using json = nlohmann::json;

class SocketServer
{
public:
	SocketServer(const std::string &socket_path, ActionDispatcher &dispatcher);
	~SocketServer();

	void run();

private:
	std::string socket_path_;
	int server_fd_;
	ActionDispatcher &dispatcher_;

	void handle_client(int client_fd);
};
