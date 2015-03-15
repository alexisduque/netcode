#pragma once

#include "netcode/detail/symbol_buffer.hh"
#include "netcode/types.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief A source packet holding a user's symbol.
class source final
{
public:

  /// @brief Constructor.
  source(std::uint32_t id, symbol_buffer&& buffer, std::size_t user_size)
    : id_{id}
    , symbol_buffer_{std::move(buffer)}
    , user_size_{user_size}
  {}

  /// @brief Get this source's identifier.
  std::uint32_t
  id()
  const noexcept
  {
    return id_;
  }

  /// @brief Get the bytes of the symbol.
  const symbol_buffer&
  buffer()
  const noexcept
  {
    return symbol_buffer_;
  }

  /// @brief Get the number of bytes really used by the user's symbol.
  std::size_t
  user_size()
  const noexcept
  {
    return user_size_;
  }

private:

  /// @brief This source's unique identifier.
  std::uint32_t id_;

  /// @brief This source's symbol.
  symbol_buffer symbol_buffer_;

  /// @brief The number of bytes really used by the user's symbol in the buffer.
  std::size_t user_size_;
};

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
