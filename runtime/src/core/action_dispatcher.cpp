#include "core/action_dispatcher.h"

json ActionDispatcher::dispatch(const json &request)
{
	int version = request.value("version", 0);
	std::string action = request.value("action", "");

	if (version != 1)
	{
		return {
				{"status", "error"},
				{"error", "unsupported protocol version"}};
	}

	if (action == "ping")
	{
		return handle_ping(request);
	}
	else if (action == "infer")
	{
		return handle_infer(request);
	}

	return {
			{"status", "error"},
			{"error", "unknown action"}};
}

json ActionDispatcher::handle_ping(const json &)
{
	return {
			{"status", "ok"},
			{"action", "ping"},
			{"result", "pong"}};
}

json ActionDispatcher::handle_infer(const json &request)
{
	std::string input = request.value("input", "");

	// Stub inference
	return {
			{"status", "ok"},
			{"action", "infer"},
			{"result", "echo: " + input}};
}
