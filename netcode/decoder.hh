#pragma once

#include <chrono>
#include <functional>

#include "netcode/detail/decoder.hh"
#include "netcode/detail/packet_type.hh"
#include "netcode/detail/packetizer.hh"
#include "netcode/detail/repair.hh"
#include "netcode/detail/source.hh"
#include "netcode/configuration.hh"
#include "netcode/errors.hh"

namespace ntc {

/*------------------------------------------------------------------------------------------------*/

/// @brief The class to interact with on the receiver side.
/// @ingroup ntc_decoder
template <typename PacketHandler, typename DataHandler>
class decoder final
{
public:

  /// @brief The type of the handler that processes data ready to be sent on the network.
  using packet_handler_type = PacketHandler;

  /// @brief The type of the handler that processes decoded or received data.
  using data_handler_type = DataHandler;

public:

  /// @brief Can't copy-construct an encoder.
  decoder(const decoder&) = delete;

  /// @brief Can't copy an encoder.
  decoder& operator=(const decoder&) = delete;

  /// @brief Constructor.
  template <typename PacketHandler_, typename DataHandler_>
  decoder( PacketHandler_&& packet_handler, DataHandler_&& data_handler
         , configuration conf)
    : m_conf{conf}
    , m_last_ack_date(std::chrono::steady_clock::now())
    , m_ack{}
    , m_decoder{ conf.galois_field_size()
              , std::bind(&decoder::handle_source, this, std::placeholders::_1)
              , conf.in_order()}
    , m_packet_handler(std::forward<PacketHandler_>(packet_handler))
    , m_data_handler(std::forward<DataHandler_>(data_handler))
    , m_packetizer{m_packet_handler}
    , m_nb_received_repairs{0}
    , m_nb_received_sources{0}
    , m_nb_sent_ack{0}
  {
    // Let's reserve some memory for the ack, it will most likely avoid memory re-allocations.
    m_ack.source_ids().reserve(128);
  }

  /// @brief Constructor with a default configuration.
  template <typename PacketHandler_, typename DataHandler_>
  decoder(PacketHandler_&& packet_handler, DataHandler_&& data_handler)
    : decoder{ std::forward<PacketHandler_>(packet_handler)
             , std::forward<DataHandler_>(data_handler)
             , configuration{}}
  {}

  /// @brief Notify the encoder of a new incoming packet.
  /// @param packet The incoming packet.
  /// @param max_len The maximum number of bytes to read from @p packet.
  /// @return The number of bytes that have been read.
  /// @throw overflow_error when the number of read bytes > @p max_len.
  /// @throw packet_type_error when the packet has an incorrect type.
  /// @note To fulfill memory alignement requirements, a copy will occur.
  std::size_t
  operator()(const char* packet, std::size_t max_len)
  {
    switch (detail::get_packet_type(packet))
    {
      case detail::packet_type::repair:
      {
        ++m_nb_received_repairs;
        ++m_ack.nb_packets();
        auto res = m_packetizer.read_repair(packet, max_len);
        m_decoder(std::move(res.first));
        return res.second;
      }

      case detail::packet_type::source:
      {
        ++m_nb_received_sources;
        ++m_ack.nb_packets();
        auto res = m_packetizer.read_source(packet, max_len);
        m_decoder(std::move(res.first));
        return res.second;
      }

      default:
      {
        throw packet_type_error{};
      }
    }
  }

  /// @brief Notify the encoder of a new incoming packet.
  /// @param packet The incoming packet stored in a vector.
  /// @return The number of bytes that have been read.
  /// @throw packet_type_error when the packet has an incorrect type.
  /// @note To fulfill memory alignement requirements, a copy will occur.
  std::size_t
  operator()(const std::vector<char>& packet)
  {
    return operator()(packet.data(), packet.size());
  }

  /// @brief Get the data handler.
  const packet_handler_type&
  packet_handler()
  const noexcept
  {
    return m_packet_handler;
  }

  /// @brief Get the data handler.
  packet_handler_type&
  packet_handler()
  noexcept
  {
    return m_packet_handler;
  }

  /// @brief Get the data handler.
  const data_handler_type&
  data_handler()
  const noexcept
  {
    return m_data_handler;
  }

  /// @brief Get the data handler.
  data_handler_type&
  data_handler()
  noexcept
  {
    return m_data_handler;
  }

  /// @brief Get the total number of decoded sources.
  std::size_t
  nb_decoded()
  const noexcept
  {
    return m_decoder.nb_decoded();
  }

  /// @brief Get the number of times the full decoding failed.
  std::size_t
  nb_failed_full_decodings()
  const noexcept
  {
    return m_decoder.nb_failed_full_decodings();
  }

  /// @brief Get the current number of missing sources.
  std::size_t
  nb_missing_sources()
  const noexcept
  {
    return m_decoder.missing_sources().size();
  }

  /// @brief Get the total number of received repairs.
  std::size_t
  nb_received_repairs()
  const noexcept
  {
    return m_nb_received_repairs;
  }

  /// @brief Get the total number of received sources.
  std::size_t
  nb_received_sources()
  const noexcept
  {
    return m_nb_received_sources;
  }

  /// @brief Get the number of sent ack.
  std::size_t
  nb_sent_acks()
  const noexcept
  {
    return m_nb_sent_ack;
  }

  /// @brief Get the number of repairs that were dropped because they wereuseless.
  std::size_t
  nb_useless_repairs()
  const noexcept
  {
    return m_decoder.nb_useless_repairs();
  }

  /// @brief Force the generation of an ack.
  void
  generate_ack()
  {
    // Add all currently known source identifiers to the ack to be sent.
    for (const auto& id_src : m_decoder.sources())
    {
      m_ack.source_ids().insert(m_ack.source_ids().end(), id_src.first);
    }

    // Ask packetizer to handle the bytes of the new ack (will be routed to user's handler).
    m_packetizer.write_ack(m_ack);
    ++m_nb_sent_ack;

    // Start a fresh new ack.
    m_ack.reset();
  }

  /// @brief Generate an ack if needed.
  void
  maybe_ack()
  {
    if (m_ack.nb_packets() >= m_conf.ack_nb_packets())
    {
      generate_ack();
      m_last_ack_date = std::chrono::steady_clock::now();
    }
    else if (m_conf.ack_frequency() != std::chrono::milliseconds{0})
    {
      const auto now = std::chrono::steady_clock::now();
      if ((now - m_last_ack_date) >= m_conf.ack_frequency())
      {
        generate_ack();
        m_last_ack_date = now;
      }
    }
  }

  /// @brief Get the configuration (mutable).
  configuration&
  conf()
  noexcept
  {
    return m_conf;
  }

  /// @brief Get the configuration.
  const configuration&
  conf()
  const noexcept
  {
    return m_conf;
  }

private:

  /// @brief Callback given to the real encoder to be notified when a source is processed.
  void
  handle_source(const detail::source& src)
  {
    // Ask user to read the bytes of this new source.
    m_data_handler(src.buffer().data(), src.user_size());

    // Send an ack if necessary.
    maybe_ack();
  }

private:

  /// @brief The configuration.
  configuration m_conf;

  /// @brief The last time an ack was sent.
  std::chrono::steady_clock::time_point m_last_ack_date;

  /// @brief Re-use the same memory to prepare an ack packet.
  detail::ack m_ack;

  /// @brief The component that rebuilds sources using repairs.
  detail::decoder m_decoder;

  /// @brief The user's handler to output data on network.
  packet_handler_type m_packet_handler;

  /// @brief The user's handler to read decoded data.
  data_handler_type m_data_handler;

  /// @brief How to read and write packets.
  detail::packetizer<packet_handler_type> m_packetizer;

  /// @brief The counter of received repairs.
  std::size_t m_nb_received_repairs;

  /// @brief The counter of received sources.
  std::size_t m_nb_received_sources;

  /// @brief The number of ack sent back to the encoder.
  std::size_t m_nb_sent_ack;
};

/*------------------------------------------------------------------------------------------------*/

} // namespace ntc
