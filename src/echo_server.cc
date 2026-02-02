#include "echo_server.h"

#include <array>
#include <utility>

namespace libtcp {

class EchoSession : public std::enable_shared_from_this<EchoSession> {
public:
  explicit EchoSession(EchoServer::tcp::socket socket)
      : socket_(std::move(socket)) {}

  void start() { do_read(); }

private:
  void do_read() {
    auto self = shared_from_this();
    socket_.async_read_some(boost::asio::buffer(buffer_),
                            [self](const boost::system::error_code& ec,
                                   std::size_t bytes_transferred) {
                              if (!ec) {
                                self->do_write(bytes_transferred);
                              }
                            });
  }

  void do_write(std::size_t length) {
    auto self = shared_from_this();
    boost::asio::async_write(socket_,
                             boost::asio::buffer(buffer_.data(), length),
                             [self](const boost::system::error_code& ec,
                                    std::size_t /*bytes_transferred*/) {
                               if (!ec) {
                                 self->do_read();
                               }
                             });
  }

  EchoServer::tcp::socket socket_;
  std::array<char, 4096> buffer_{};
};

EchoServer::EchoServer(boost::asio::io_context& io_context, std::uint16_t port)
    : io_context_(io_context),
      acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {}

void EchoServer::start() {
  do_accept();
}

std::uint16_t EchoServer::port() const {
  return acceptor_.local_endpoint().port();
}

void EchoServer::do_accept() {
  acceptor_.async_accept(
      [this](const boost::system::error_code& ec, tcp::socket socket) {
        if (!ec) {
          std::make_shared<EchoSession>(std::move(socket))->start();
        }
        do_accept();
      });
}

}  // namespace libtcp
