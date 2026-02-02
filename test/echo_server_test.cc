#include <echo_server.h>
#include <gtest/gtest.h>

TEST(EchoServerTest, StartsOnEphemeralPort) {
  boost::asio::io_context io_context;
  libtcp::EchoServer server(io_context, 0);
  server.start();

  EXPECT_GT(server.port(), 0);
}
