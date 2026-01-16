#include "ipc/socket_server.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

SocketServer::SocketServer(const std::string &socket_path)
		: socket_path_(socket_path), server_fd_(-1) {}

SocketServer::~SocketServer()
{
	if (server_fd_ >= 0)
		close(server_fd_);
	unlink(socket_path_.c_str());
}

void SocketServer::run()
{
	server_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
	if (server_fd_ < 0)
	{
		perror("socket");
		return;
	}

	sockaddr_un addr{};
	addr.sun_family = AF_UNIX;
	std::strncpy(addr.sun_path, socket_path_.c_str(), sizeof(addr.sun_path) - 1);

	unlink(socket_path_.c_str());

	if (bind(server_fd_, (sockaddr *)&addr, sizeof(addr)) < 0)
	{
		perror("bind");
		return;
	}

	if (listen(server_fd_, 1) < 0)
	{
		perror("listen");
		return;
	}

	std::cout << "[forge-runtime] listening on " << socket_path_ << "\n";

	while (true)
	{
		int client_fd = accept(server_fd_, nullptr, nullptr);
		if (client_fd < 0)
		{
			perror("accept");
			continue;
		}

		handle_client(client_fd);
		close(client_fd);
	}
}

void SocketServer::handle_client(int client_fd)
{
	char buffer[4096];
	ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);
	if (n <= 0)
		return;

	buffer[n] = '\0';

	std::cout << "[request raw]\n"
						<< buffer << "\n";

	json response;

	try
	{
		json request = json::parse(buffer);
		response = dispatcher_.dispatch(request);
	}
	catch (const std::exception &)
	{
		response = {
				{"status", "error"},
				{"error", "invalid json"}};
	}

	std::string out = response.dump();
	write(client_fd, out.c_str(), out.size());
}
