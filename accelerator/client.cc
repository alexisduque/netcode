#include <iostream>

#include "accelerator/transcoder.hh"

/*------------------------------------------------------------------------------------------------*/

class client
{
public:

  client( const ntc::configuration& conf, std::uint16_t app_port, const std::string& addr
        , const std::string& port)
    : io_()
    , app_socket_(io_, udp::endpoint{udp::v4(), app_port}) // a server socket
    , app_endpoint_()
    , socket_(io_, udp::endpoint(udp::v4(), 0))            // a client socket
    , endpoint_()
    , transcoder_(conf, io_, app_socket_, app_endpoint_, socket_, endpoint_)
  {
    udp::resolver resolver(io_);
    endpoint_ = *resolver.resolve({udp::v4(), addr, port});
  }

  void
  operator()()
  {
    io_.run();
  }

private:

  /// @brief
  asio::io_service io_;

  /// @brief
  udp::socket app_socket_;

  /// @brief
  udp::endpoint app_endpoint_;

  /// @brief
  udp::socket socket_;

  /// @brief
  udp::endpoint endpoint_;

  /// @brief
  transcoder transcoder_;
};

/*------------------------------------------------------------------------------------------------*/

int
main(int argc, char** argv)
{
  if (argc != 4)
  {
    std::cerr << "Usage: " << argv[0] << " app_port server_ip server_port\n";
    return 1;
  }
  try
  {
    client c(ntc::configuration{}, std::atoi(argv[1]), argv[2], argv[3]);
    c();
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << '\n';
  }
  return 0;
}

/*------------------------------------------------------------------------------------------------*/
