#pragma once

#include <algorithm>
#include <cstring>
#include <new> // nothrow
#include <stdexcept>

#include "netcode/c/error.h"
#include "netcode/errors.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief Apply function @p fn and fill @p error if necessary
template <typename Fn>
auto
__attribute__((nonnull))
check_error(Fn&& fn, ntc_error* error)
noexcept
-> decltype(fn())
{
  error->type = ntc_no_error;

  try
  {
    return fn();
  }

  catch (const ntc::packet_type_error&)
  {
    error->type = ntc_packet_type_error;
  }

  catch (const ntc::overflow_error&)
  {
    error->type = ntc_overflow_error;
  }

  catch (const std::bad_alloc&)
  {
    error->type = ntc_no_memory;
  }

  catch (const std::exception& e)
  {
    error->type = ntc_unknown_error;
    const auto sz = std::strlen(e.what());
    delete error->message;
    error->message = new (std::nothrow) char[sz];
    if (error->message)
    {
      std::copy_n(e.what(), sz, error->message);
    }
  }

  catch (...)
  {
    error->type = ntc_unknown_error;
    delete error->message;
  }

  // Return the default value of the type returned by fn when an error occurs.
  return decltype(fn())();
}

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
