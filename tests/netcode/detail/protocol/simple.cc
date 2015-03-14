#include <algorithm> // copy_n

#include "tests/catch.hpp"

#include "netcode/detail/protocol/simple.hh"

/*------------------------------------------------------------------------------------------------*/

using namespace ntc;

/*------------------------------------------------------------------------------------------------*/

namespace /* unnamed */ {

struct handler
{
  std::size_t bytes_ = 0;
  char data_[2048];

  void
  on_ready_data(std::size_t len, const char* data)
  {
    std::copy_n(data, len, data_ + bytes_);
    bytes_ += len;
  }
};

} // namespace unnamed

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("An ack is (de)serialized by protocol::simple", "[serialization][ack][simple]" )
{
  detail::handler_derived<handler> h{handler{}};
  detail::protocol::simple serializer{h};

  const detail::ack a_in{{0,1,2,3}};

  serializer.write_ack(a_in);

  REQUIRE(h.handler_.bytes_ == ( sizeof(std::uint8_t)      // type
                               + sizeof(std::uint16_t)     // nb nb source ids
                               + 4 * sizeof(std::uint32_t) // identifiers
                               )
         );

  const auto a_out = serializer.read_ack(h.handler_.data_);
  REQUIRE(a_in.source_ids() == a_out.source_ids());
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("A repair is (de)serialized by protocol::simple", "[serialization][repair][simple]" )
{
  detail::handler_derived<handler> h{handler{}};
  detail::protocol::simple serializer{h};

  const detail::repair r_in{42, {0,1,2,3}, detail::symbol_buffer{'a', 'b', 'c'}};

  serializer.write_repair(r_in);

  REQUIRE(h.handler_.bytes_ == ( sizeof(std::uint8_t)      // type
                               + sizeof(std::uint32_t)     // id
                               + sizeof(std::uint16_t)     // nb source ids
                               + 4 * sizeof(std::uint32_t) // identifiers
                               + sizeof(std::uint16_t)     // symbol length
                               + 3                         // symbol
                               )
         );

  const auto r_out = serializer.read_repair(h.handler_.data_);
  REQUIRE(r_in.id() == r_out.id());
  REQUIRE(r_in.source_ids() == r_out.source_ids());
  REQUIRE(r_in.buffer() == r_out.buffer());
}

/*------------------------------------------------------------------------------------------------*/