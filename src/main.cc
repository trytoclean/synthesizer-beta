#include "echo_server.h"

#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
  try {
    std::uint16_t port = 12345;
    if (argc > 1) {
      port = static_cast<std::uint16_t>(std::stoi(argv[1]));
    }

    boost::asio::io_context io_context;
    libtcp::EchoServer server(io_context, port);
    server.start();

    std::cout << "Echo server listening on port " << server.port() << '\n';
    io_context.run();
  } catch (const std::exception& ex) {
    std::cerr << "Server error: " << ex.what() << '\n';
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
