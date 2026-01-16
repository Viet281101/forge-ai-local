#pragma once

#include <string>

class SocketServer {
public:
	explicit SocketServer(const std::string& socket_path);
	~SocketServer();

	void run();

private:
	std::string socket_path_;
	int server_fd_;

	void handle_client(int client_fd);
};
