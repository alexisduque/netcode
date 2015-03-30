#pragma once

#include <cstdint>

#include "netcode/detail/galois_field.hh"

namespace ntc { namespace detail {

/*------------------------------------------------------------------------------------------------*/

/// @internal
/// @brief Compute the coefficient for the repair being built.
std::uint32_t
coefficient(const galois_field& gf, std::uint32_t repair_id, std::uint32_t src_id)
noexcept;

/*------------------------------------------------------------------------------------------------*/

}} // namespace ntc::detail
