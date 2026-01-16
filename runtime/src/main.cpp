#include <iostream>
#include "ipc/socket_server.h"

int main() {
  std::cout << "[forge-runtime] starting...\n";

  SocketServer server("/tmp/forge-ai.sock");
  server.run();

  return 0;
}
