#pragma once

#include "netcode/packet.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief An encoder's source packet holding a user's symbol
class encoder_source final
{
public:

  /// @brief Can't copy-construct a source
  encoder_source(const encoder_source&) = delete;

  /// @brief Can't copy a source
  encoder_source& operator=(const encoder_source&) = delete;

  /// @brief Can move-construct a source
  encoder_source(encoder_source&&) = default;

  /// @brief Can move a source
  encoder_source& operator=(encoder_source&&) = default;

  /// @brief Constructor
  encoder_source(std::uint32_t id, detail::byte_buffer&& p)
    : m_id{id}
    , m_symbol_buffer{std::move(p)}
  {}

  /// @brief Get this source's identifier
  std::uint32_t
  id()
  const noexcept
  {
    return m_id;
  }

  /// @brief Get the bytes of the symbol
  const detail::byte_buffer&
  symbol()
  const noexcept
  {
    return m_symbol_buffer;
  }

  /// @brief Get the bytes of the symbol
  detail::byte_buffer&
  symbol()
  noexcept
  {
    return m_symbol_buffer;
  }

  /// @brief Get the number of bytes in the user's symbol
  std::uint16_t
  size()
  const noexcept
  {
    return static_cast<std::uint16_t>(m_symbol_buffer.size());
  }

private:

  /// @brief This source's unique identifier
  std::uint32_t m_id;

  /// @brief This source's symbol
  detail::byte_buffer m_symbol_buffer;
};

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief A decoder source packet holding a user's symbol
///
/// To avoid copies, a source on the decoder side is constructed using directly the packet received
/// from the network.
class decoder_source final
{
public:

  /// @brief Can't copy-construct a source
  decoder_source(const decoder_source&) = delete;

  /// @brief Can't copy a source
  decoder_source& operator=(const decoder_source&) = delete;

  /// @brief Can move-construct a source
  decoder_source(decoder_source&&) = default;

  /// @brief Can move a source
  decoder_source& operator=(decoder_source&&) = default;

  /// @brief Constructor
  decoder_source(std::uint32_t id, packet&& p, std::size_t symbol_size)
    : m_id{id}
    , m_symbol_buffer{std::move(p)}
    , m_symbol_size{static_cast<std::uint16_t>(symbol_size)}
  {}

  /// @brief Get this source's identifier
  std::uint32_t
  id()
  const noexcept
  {
    return m_id;
  }

  /// @brief Get the bytes of the symbol
  const char*
  symbol()
  const noexcept
  {
    return m_symbol_buffer.symbol();
  }

  /// @brief Get the bytes of the symbol
  char*
  symbol()
  noexcept
  {
    return m_symbol_buffer.symbol();
  }

  /// @brief Get the number of bytes in the user's symbol
  std::uint16_t
  symbol_size()
  const noexcept
  {
    return m_symbol_size;
  }

private:

  /// @brief This source's unique identifier
  std::uint32_t m_id;

  /// @brief This source's symbol
  packet m_symbol_buffer;

  /// @brief This source's symbol size
  std::uint16_t m_symbol_size;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
