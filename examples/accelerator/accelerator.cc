#include <chrono>
#include <cstring> // strncmp
#include <iostream>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#define ASIO_STANDALONE
#include <asio.hpp>
#pragma GCC diagnostic pop

#include "accelerator/transcoder.hh"

/*------------------------------------------------------------------------------------------------*/

int
main(int argc, char** argv)
{
  using asio::ip::address_v4;
  using asio::ip::udp;

  const auto usage = [&]
  {
    std::cerr << "Usage\n";
    std::cerr << argv[0] << " server server_port app_ip app_port\n";
    std::cerr << argv[0] << " client app_port server_ip server_port\n";
    std::exit(1);
  };

  if (argc != 5)
  {
    usage();
  }
  try
  {
    asio::io_service io;

    // Connection with the proxied application.
    udp::socket app_socket{io};
    udp::endpoint app_endpoint;

    // Encoded tunnel.
    udp::socket socket{io};
    udp::endpoint endpoint;

    // The transcoder is fully symmetric as it handles two-ways communications. Still, UDP
    // requires a client and a server.
    if (std::strncmp(argv[1], "server", 7) == 0)
    {
      const auto server_port = static_cast<unsigned short>(std::atoi(argv[2]));
      const auto app_url = argv[3];
      const auto app_port = argv[4];

      app_socket = udp::socket{io, udp::endpoint(udp::v4(), 0)}; // a client socket
      app_socket.set_option(asio::socket_base::receive_buffer_size{8192*64});
      app_socket.set_option(asio::socket_base::send_buffer_size{8192*64});

      socket = udp::socket{io, udp::endpoint{udp::v4(), server_port}};  // a server socket
      socket.set_option(asio::socket_base::receive_buffer_size{8192*64});
      socket.set_option(asio::socket_base::send_buffer_size{8192*64});

      udp::resolver resolver(io);
      app_endpoint = *resolver.resolve({udp::v4(), app_url, app_port});
    }
    else if (std::strncmp(argv[1], "client", 7) == 0)
    {
      const auto app_port = static_cast<unsigned short>(std::atoi(argv[2]));
      const auto server_url = argv[3];
      const auto server_port = argv[4];

      app_socket = udp::socket{io, udp::endpoint{udp::v4(), app_port}}; // a server socket
      app_socket.set_option(asio::socket_base::receive_buffer_size{8192*64});
      app_socket.set_option(asio::socket_base::send_buffer_size{8192*64});

      socket = udp::socket{io, udp::endpoint(udp::v4(), 0)};            // a client socket
      socket.set_option(asio::socket_base::receive_buffer_size{8192*64});
      socket.set_option(asio::socket_base::send_buffer_size{8192*64});

      udp::resolver resolver(io);
      endpoint = *resolver.resolve({udp::v4(), server_url, server_port});
    }
    else
    {
      usage();
    }

    // Create and configure the transcoder that handles encoding/decoding. The client/server status
    // is completely transparent to the transcoder.
    transcoder t{io, app_socket, app_endpoint, socket, endpoint};

    // Launch the event loop (runs forever).
    io.run();
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << '\n';
  }
  return 0;
}

/*------------------------------------------------------------------------------------------------*/
