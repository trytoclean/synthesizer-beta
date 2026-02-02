#ifndef LIBTCP_ECHO_SERVER_H
#define LIBTCP_ECHO_SERVER_H

#include <boost/asio.hpp>
#include <cstdint>
#include <memory>

namespace libtcp {

class EchoServer {
 public:
  using tcp = boost::asio::ip::tcp;

  EchoServer(boost::asio::io_context& io_context, std::uint16_t port);

  void start();
  std::uint16_t port() const;

 private:
  void do_accept();

  boost::asio::io_context& io_context_;
  tcp::acceptor acceptor_;
};

}  // namespace libtcp

#endif  // LIBTCP_ECHO_SERVER_H
