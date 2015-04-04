#include "tests/catch.hpp"

#include "netcode/detail/buffer.hh"

/*------------------------------------------------------------------------------------------------*/

using namespace ntc;

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("A byte_buffer is aligned on 16 bytes")
{
  const auto b0 = detail::byte_buffer{};
  REQUIRE((reinterpret_cast<std::size_t>(&b0[0]) % 16ul) == 0ul);

  const auto b1 = b0;
  REQUIRE((reinterpret_cast<std::size_t>(&b1[0]) % 16ul) == 0ul);
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("A byte_buffer is 0-out when resized")
{
  auto b = detail::byte_buffer{0,1,2,3,4,5,6,7,8,9};
  REQUIRE(b.size() == 10);

  for (char i = 0; i < 10; ++i)
  {
    REQUIRE(b[i] == i);
  }

  b.resize(3);
  REQUIRE(b.size() == 3);
  REQUIRE(b[0] == 0);
  REQUIRE(b[1] == 1);
  REQUIRE(b[2] == 2);

  b.resize(10);
  REQUIRE(b.size() == 10);
  REQUIRE(b[0] == 0);
  REQUIRE(b[1] == 1);
  REQUIRE(b[2] == 2);
  // Values 0ed in the mean time.
  for (char i = 3; i < 10; ++i)
  {
    REQUIRE(b[i] == 0);
  }
}

/*------------------------------------------------------------------------------------------------*/
