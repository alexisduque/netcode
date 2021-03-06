#include <algorithm>
#include <thread>
#include <vector>

#include <catch.hpp>
#include "tests/netcode/common.hh"
#include "tests/netcode/launch.hh"

#include "netcode/decoder.hh"
#include "netcode/encoder.hh"
#include "netcode/detail/packet_type.hh"

#include <iostream>

/*------------------------------------------------------------------------------------------------*/

using namespace ntc;

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Decoder gives a correct source to user")
{
  launch([](std::uint8_t gf_size)
  {
    encoder<packet_handler> enc{gf_size, packet_handler{}};
    decoder<packet_handler, data_handler> dec{ gf_size, in_order::yes, packet_handler{}
                                             , data_handler{}};

    auto& enc_packet_handler = enc.packet_handler();
    auto& dec_data_handler = dec.data_handler();

    const auto s0 = {'a','b','c','d'};
    enc(data{begin(s0), end(s0)});

    // Send serialized data to decoder.
    dec(enc_packet_handler[0]);

    REQUIRE(dec_data_handler[0].size() == s0.size());
    REQUIRE(std::equal(begin(s0), end(s0), begin(dec_data_handler[0])));
  });
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Decoder repairs a lost source")
{
  launch([](std::uint8_t gf_size)
  {
    encoder<packet_handler> enc{gf_size, packet_handler{}};
    enc.set_rate(1);

    decoder<packet_handler, data_handler> dec{ gf_size, in_order::yes, packet_handler{}
                                             , data_handler{}};

    auto& enc_packet_handler = enc.packet_handler();
    auto& dec_data_handler = dec.data_handler();

    // Give a source to the encoder.
    const auto s0 = {'a','b','c','d'};
    enc(data{begin(s0), end(s0)});

    // Skip first source.
    auto repair = enc_packet_handler[1];

    // Send repair to decoder.
    REQUIRE(dec(repair));
    REQUIRE(dec.nb_received_repairs() == 1);
    REQUIRE(dec.nb_received_sources() == 0);
    REQUIRE(dec.nb_decoded() == 1);
    REQUIRE(dec_data_handler[0].size() == s0.size());
    REQUIRE(std::equal(begin(s0), end(s0), begin(dec_data_handler[0])));
  });
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Decoder generate correct ack")
{
  launch([](std::uint8_t gf_size)
  {
    encoder<packet_handler> enc{gf_size, packet_handler{}};
    enc.set_rate(100);

    decoder<packet_handler, data_handler> dec{ gf_size, in_order::yes, packet_handler{}
                                             , data_handler{}};
    dec.set_ack_frequency(std::chrono::milliseconds{100});

    auto& enc_packet_handler = enc.packet_handler();
    auto& dec_packet_handler = dec.packet_handler();

    // Give a source to the encoder.
    const auto s0 = {'a','b','c','d'};
    enc(data{begin(s0), end(s0)});
    REQUIRE(enc.window() == 1);

    // Send source to decoder.
    dec(enc_packet_handler[0]);

    SECTION("Force ack")
    {
      // Now force the sending of an ack.
      dec.generate_ack();
      REQUIRE(dec.nb_sent_acks() == 1);

      // Sent it to the encoder.
      enc(dec_packet_handler[0]);
      REQUIRE(enc.window() == 0); // Source was correctly removed from the encoder window.
    }

    SECTION("Wait for trigger")
    {
      // Wait long enough just to be sure.
      std::this_thread::sleep_for(std::chrono::milliseconds{200});
      dec.maybe_ack();
      REQUIRE(dec.nb_sent_acks() == 1);

      // Sent it to the encoder.
      enc(dec_packet_handler[0]);
      REQUIRE(enc.window() == 0); // Source was correctly removed from the encoder window.
    }
  });
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Decoder generate acks when N packets are received")
{
  launch([](std::uint8_t gf_size)
  {
    encoder<packet_handler> enc{gf_size, packet_handler{}};
    enc.set_rate(100);

    decoder<packet_handler, data_handler> dec{ gf_size, in_order::yes, packet_handler{}
                                             , data_handler{}};
    dec.set_ack_frequency(std::chrono::milliseconds{0});
    dec.set_ack_nb_packets(4);

    auto& enc_packet_handler = enc.packet_handler();
    auto& dec_packet_handler = dec.packet_handler();

    // Give a source to the encoder.
    const auto s0 = {'a','b','c','d','e','f','g','i'};
    enc(data{begin(s0), end(s0)});
    enc(data{begin(s0), end(s0)});
    enc(data{begin(s0), end(s0)});
    enc(data{begin(s0), end(s0)});
    REQUIRE(enc.window() == 4);
    REQUIRE(enc_packet_handler.nb_packets() == 4);
    REQUIRE(detail::get_packet_type(enc_packet_handler[0]) == detail::packet_type::source);
    REQUIRE(detail::get_packet_type(enc_packet_handler[1]) == detail::packet_type::source);
    REQUIRE(detail::get_packet_type(enc_packet_handler[2]) == detail::packet_type::source);
    REQUIRE(detail::get_packet_type(enc_packet_handler[3]) == detail::packet_type::source);

    // Send sources to decoder.
    dec(enc_packet_handler[0]);
    dec(enc_packet_handler[1]);
    dec(enc_packet_handler[2]);
    dec(enc_packet_handler[3]);

    REQUIRE(dec_packet_handler.nb_packets() == 1);
    REQUIRE(detail::get_packet_type(dec_packet_handler[0]) == detail::packet_type::ack);
  });
}

/*------------------------------------------------------------------------------------------------*/

void
test_case_0(ntc::in_order order)
{
  launch([&](std::uint8_t gf_size)
  {
    encoder<packet_handler> enc{gf_size, packet_handler{}};
    enc.set_rate(4);
    enc.set_window_size(3);

    decoder<packet_handler, data_handler> dec{gf_size, order, packet_handler{}, data_handler{}};
    dec.set_ack_frequency(std::chrono::milliseconds{0});

    auto& enc_handler = enc.packet_handler();
    auto& dec_data_handler = dec.data_handler();

    // Packets will be stored in enc_handler.vec.
    const auto s0 = {'a','a','a','a'};
    enc(data{begin(s0), end(s0)});
    REQUIRE(enc.window() == 1);

    const auto s1 = {'b','b','b','b'};
    enc(data{begin(s1), end(s1)});
    REQUIRE(enc.window() == 2);

    const auto s2 = {'c','c','c','c','c','c','c','c'};
    enc(data{begin(s2), end(s2)});
    REQUIRE(enc.window() == 3);

    const auto s3 = {'d','d','d','d'};
    enc(data{begin(s3), end(s3)});
    REQUIRE(enc.window() == 3);

    REQUIRE(enc_handler.nb_packets() == 5 /* 4 src + 1 repair */);
    REQUIRE(detail::get_packet_type(enc_handler[0]) == detail::packet_type::source);
    REQUIRE(detail::get_packet_type(enc_handler[1]) == detail::packet_type::source);
    REQUIRE(detail::get_packet_type(enc_handler[2]) == detail::packet_type::source);
    REQUIRE(detail::get_packet_type(enc_handler[3]) == detail::packet_type::source);
    REQUIRE(detail::get_packet_type(enc_handler[4]) == detail::packet_type::repair);

    // Now send to decoder.
    // Lost first source.
    dec(enc_handler[1]);
    dec(enc_handler[2]);
    dec(enc_handler[3]);
    // Because the encoder's window is 3, the incoming repair only encode s1, s2 and s3. Thus,
    // s0 is completely lost and cannot be recovered.
    dec(enc_handler[4]);
    REQUIRE(dec.nb_received_sources() == 3);
    REQUIRE(dec.nb_received_repairs() == 1);
    REQUIRE(dec.nb_missing_sources() == 0);
    REQUIRE(dec.nb_decoded() == 0);

    // Sources were correctly given to the user handler.
    REQUIRE(dec_data_handler.nb_data() == 3);
    REQUIRE(std::equal(begin(s1), end(s1), begin(dec_data_handler[0])));
    REQUIRE(std::equal(begin(s2), end(s2), begin(dec_data_handler[1])));
    REQUIRE(std::equal(begin(s3), end(s3), begin(dec_data_handler[2])));
  });
}


TEST_CASE("In order decoder: lost packet with an encoder's limited window")
{
  test_case_0(ntc::in_order::yes);
}

TEST_CASE("Out of order decoder: lost packet with an encoder's limited window")
{
  test_case_0(ntc::in_order::no);
}

/*------------------------------------------------------------------------------------------------*/

void
test_non_systematic(ntc::in_order order)
{
  launch({8,16,32}, [&](std::uint8_t gf_size)
  {
    encoder<packet_handler> enc{gf_size, packet_handler{}};
    enc.set_rate(4);
    enc.set_code_type(systematic::no);

    decoder<packet_handler, data_handler> dec{gf_size, order, packet_handler{}, data_handler{}};
    dec.set_ack_frequency(std::chrono::milliseconds{0});

    auto& enc_handler = enc.packet_handler();
    auto& dec_data_handler = dec.data_handler();

    // Packets will be stored in enc_handler.vec.
    const auto s0 = {'a','a','a','a'};
    enc(data{begin(s0), end(s0)});
    REQUIRE(enc.window() == 1);

    const auto s1 = {'b','b','b','b','b','b','b','b'};
    enc(data{begin(s1), end(s1)});
    REQUIRE(enc.window() == 2);

    const auto s2 = {'c','c','c','c','c','c','c','c','c','c','c','c'};
    enc(data{begin(s2), end(s2)});
    REQUIRE(enc.window() == 3);

    const auto s3 = {'d','d','d','d'};
    enc(data{begin(s3), end(s3)});

    // Only repairs
    REQUIRE(enc_handler.nb_packets() == 5 /* repairs */);
    REQUIRE(detail::get_packet_type(enc_handler[0]) == detail::packet_type::repair);
    REQUIRE(detail::get_packet_type(enc_handler[1]) == detail::packet_type::repair);
    REQUIRE(detail::get_packet_type(enc_handler[2]) == detail::packet_type::repair);
    REQUIRE(detail::get_packet_type(enc_handler[3]) == detail::packet_type::repair);
    REQUIRE(detail::get_packet_type(enc_handler[4]) == detail::packet_type::repair);

    SECTION("Lost first repair")
    {
      // Now send to decoder.
      // Lost first repair.
      dec(enc_handler[1]);
      dec(enc_handler[2]);
      dec(enc_handler[3]);
      dec(enc_handler[4]);

      REQUIRE(dec.nb_received_sources() == 0);
      REQUIRE(dec.nb_received_repairs() == 4);
      REQUIRE(dec.nb_missing_sources() == 0);
      REQUIRE(dec.nb_decoded() == 4);

      // All sources were correctly given to the user handler.
      REQUIRE(dec_data_handler.nb_data() == 4);
      REQUIRE(std::equal(begin(s0), end(s0), begin(dec_data_handler[0])));
      REQUIRE(std::equal(begin(s1), end(s1), begin(dec_data_handler[1])));
      REQUIRE(std::equal(begin(s2), end(s2), begin(dec_data_handler[2])));
      REQUIRE(std::equal(begin(s3), end(s3), begin(dec_data_handler[3])));
    }
    SECTION("Lost a repair")
    {
      // Now send to decoder.
      // Lost repair 2.
      dec(enc_handler[0]);
      dec(enc_handler[1]);
      dec(enc_handler[3]);
      dec(enc_handler[4]);

      REQUIRE(dec.nb_received_sources() == 0);
      REQUIRE(dec.nb_received_repairs() == 4);
      REQUIRE(dec.nb_missing_sources() == 0);
      REQUIRE(dec.nb_decoded() == 4);

      // All sources were correctly given to the user handler.
      REQUIRE(dec_data_handler.nb_data() == 4);
      REQUIRE(std::equal(begin(s0), end(s0), begin(dec_data_handler[0])));
      REQUIRE(std::equal(begin(s1), end(s1), begin(dec_data_handler[1])));
      REQUIRE(std::equal(begin(s2), end(s2), begin(dec_data_handler[2])));
      REQUIRE(std::equal(begin(s3), end(s3), begin(dec_data_handler[3])));
    }
    SECTION("Lost last repair")
    {
      // Now send to decoder.
      // Lost last repair.
      dec(enc_handler[0]);
      dec(enc_handler[1]);
      dec(enc_handler[2]);
      dec(enc_handler[3]);

      REQUIRE(dec.nb_received_sources() == 0);
      REQUIRE(dec.nb_received_repairs() == 4);
      REQUIRE(dec.nb_missing_sources() == 0);
      REQUIRE(dec.nb_decoded() == 4);

      // All sources were correctly given to the user handler.
      REQUIRE(dec_data_handler.nb_data() == 4);
      REQUIRE(std::equal(begin(s0), end(s0), begin(dec_data_handler[0])));
      REQUIRE(std::equal(begin(s1), end(s1), begin(dec_data_handler[1])));
      REQUIRE(std::equal(begin(s2), end(s2), begin(dec_data_handler[2])));
      REQUIRE(std::equal(begin(s3), end(s3), begin(dec_data_handler[3])));
    }
  });
}

TEST_CASE("In order decoder: non systematic code")
{
  test_non_systematic(ntc::in_order::yes);
}

TEST_CASE("Out of order decoder: non systematic code")
{
  test_non_systematic(ntc::in_order::no);
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Decoder invalid read scenario")
{
  launch([](std::uint8_t gf_size)
  {
    encoder<packet_handler> enc{gf_size, packet_handler{}};
    enc.set_rate(3);
    enc.set_code_type(systematic::no);

    decoder<packet_handler, data_handler> dec{ gf_size, in_order::yes, packet_handler{}
                                             , data_handler{}};
    dec.set_ack_frequency(std::chrono::milliseconds{0});

    auto& enc_handler = enc.packet_handler();
    auto& dec_data_handler = dec.data_handler();

    // Packets will be stored in enc_handler.vec.
    const auto s0 = {'a','a','a','a'};
    enc(data{begin(s0), end(s0)});

    const auto s1 = {'b','b','b','b'};
    enc(data{begin(s1), end(s1)});

    const auto s2 = {'c','c','c','c'};
    enc(data{begin(s2), end(s2)});

    // Only repairs
    REQUIRE(enc_handler.nb_packets() == 4 /* repairs */);
    REQUIRE(detail::get_packet_type(enc_handler[0]) == detail::packet_type::repair);
    REQUIRE(detail::get_packet_type(enc_handler[1]) == detail::packet_type::repair);
    REQUIRE(detail::get_packet_type(enc_handler[2]) == detail::packet_type::repair);
    REQUIRE(detail::get_packet_type(enc_handler[3]) == detail::packet_type::repair);

    SECTION("Lost 1st repair")
    {
      dec(enc_handler[1]);
      dec(enc_handler[2]);
      dec(enc_handler[3]);

      REQUIRE(dec.nb_received_sources() == 0);
      REQUIRE(dec.nb_received_repairs() == 3);
      REQUIRE(dec.nb_decoded() == 3);

      // All sources were correctly given to the user handler.
      REQUIRE(dec_data_handler.nb_data() == 3);
      REQUIRE(std::equal(begin(s0), end(s0), begin(dec_data_handler[0])));
      REQUIRE(std::equal(begin(s1), end(s1), begin(dec_data_handler[1])));
      REQUIRE(std::equal(begin(s2), end(s2), begin(dec_data_handler[2])));
    }
  });
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("In order decoder")
{
  launch([](std::uint8_t gf_size)
  {
    encoder<packet_handler> enc{gf_size, packet_handler{}};
    enc.set_rate(4);

    decoder<packet_handler, data_handler> dec{ gf_size, in_order::yes, packet_handler{}
                                             , data_handler{}};
    dec.set_ack_frequency(std::chrono::milliseconds{0});

    auto& enc_handler = enc.packet_handler();
    auto& dec_data_handler = dec.data_handler();

    // Packets will be stored in enc_handler.vec.
    const auto s0 = {'a','a','a','a'};
    enc(data{begin(s0), end(s0)});
    const auto s1 = {'b','b','b','b'};
    enc(data{begin(s1), end(s1)});
    const auto s2 = {'c','c','c','c'};
    enc(data{begin(s2), end(s2)});
    const auto s3 = {'d','d','d','d'};
    enc(data{begin(s3), end(s3)});

    REQUIRE(enc_handler.nb_packets() == 5 /* 4 src + 1 repair */);
    REQUIRE(detail::get_packet_type(enc_handler[0]) == detail::packet_type::source);
    REQUIRE(detail::get_packet_type(enc_handler[1]) == detail::packet_type::source);
    REQUIRE(detail::get_packet_type(enc_handler[2]) == detail::packet_type::source);
    REQUIRE(detail::get_packet_type(enc_handler[3]) == detail::packet_type::source);
    REQUIRE(detail::get_packet_type(enc_handler[4]) == detail::packet_type::repair);

    SECTION("Wrong order of sources")
    {
      // Now send to decoder in wrong order.
      dec(enc_handler[0]);
      dec(enc_handler[3]);
      dec(enc_handler[1]);
      dec(enc_handler[2]);
      dec(enc_handler[4]);
      REQUIRE(dec.nb_received_sources() == 4);
      REQUIRE(dec.nb_received_repairs() == 1);
      REQUIRE(dec.nb_missing_sources() == 0);
      REQUIRE(dec.nb_decoded() == 0);

      // Sources were given to in the correct order the user handler.
      REQUIRE(dec_data_handler.nb_data() == 4);
      REQUIRE(std::equal(begin(s0), end(s0), begin(dec_data_handler[0])));
      REQUIRE(std::equal(begin(s1), end(s1), begin(dec_data_handler[1])));
      REQUIRE(std::equal(begin(s2), end(s2), begin(dec_data_handler[2])));
      REQUIRE(std::equal(begin(s3), end(s3), begin(dec_data_handler[3])));
    }

    SECTION("Reverse order of sources")
    {
      // Now send to decoder in wrong order.
      dec(enc_handler[3]);
      dec(enc_handler[2]);
      dec(enc_handler[1]);
      dec(enc_handler[0]);
      dec(enc_handler[4]); // << repair
      REQUIRE(dec.nb_received_sources() == 4);
      REQUIRE(dec.nb_received_repairs() == 1);
      REQUIRE(dec.nb_missing_sources() == 0);
      REQUIRE(dec.nb_decoded() == 0);

      // Sources were given to in the correct order the user handler.
      REQUIRE(dec_data_handler.nb_data() == 4);
      REQUIRE(std::equal(begin(s0), end(s0), begin(dec_data_handler[0])));
      REQUIRE(std::equal(begin(s1), end(s1), begin(dec_data_handler[1])));
      REQUIRE(std::equal(begin(s2), end(s2), begin(dec_data_handler[2])));
      REQUIRE(std::equal(begin(s3), end(s3), begin(dec_data_handler[3])));
    }

    SECTION("Repair in middle")
    {
      dec(enc_handler[0]); // s0
      dec(enc_handler[4]); // << repair
      dec(enc_handler[3]); // s3
      dec(enc_handler[1]); // s1
      dec(enc_handler[2]); // s2, will be reconstructed
      REQUIRE(dec.nb_received_sources() == 4);
      REQUIRE(dec.nb_received_repairs() == 1);
      REQUIRE(dec.nb_missing_sources() == 0);
      REQUIRE(dec.nb_decoded() == 1); //

      // Sources were given in the correct order the user handler.
      REQUIRE(dec_data_handler.nb_data() == 4);
      REQUIRE(std::equal(begin(s0), end(s0), begin(dec_data_handler[0])));
      REQUIRE(std::equal(begin(s1), end(s1), begin(dec_data_handler[1])));
      REQUIRE(std::equal(begin(s2), end(s2), begin(dec_data_handler[2])));
      REQUIRE(std::equal(begin(s3), end(s3), begin(dec_data_handler[3])));
    }
  });
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("In order decoder, missing sources")
{
  launch([](std::uint8_t gf_size)
  {
    encoder<packet_handler> enc{gf_size, packet_handler{}};
    enc.set_window_size(3);
    enc.set_rate(3);

    decoder<packet_handler, data_handler> dec{ gf_size, in_order::yes, packet_handler{}
                                             , data_handler{}};
    dec.set_ack_frequency(std::chrono::milliseconds{0});

    auto& enc_handler = enc.packet_handler();
    auto& dec_data_handler = dec.data_handler();

    // Packets will be stored in enc_handler.vec.
    const auto s0 = {'a','a','a','a'};
    enc(data{begin(s0), end(s0)});
    const auto s1 = {'b','b','b','b','b','b','b','b','b','b','b','b','b','b','b','b'};
    enc(data{begin(s1), end(s1)});
    const auto s2 = {'c','c','c','c','c','c','c','c'};
    enc(data{begin(s2), end(s2)});
    const auto s3 = {'d','d','d','d'};
    enc(data{begin(s3), end(s3)});
    const auto s4 = {'e','e','e','e','e','e','e','e','e','e','e','e'};
    enc(data{begin(s4), end(s4)});
    const auto s5 = {'f','f','f','f'};
    enc(data{begin(s5), end(s5)});

    REQUIRE(enc_handler.nb_packets() == 8 /* 6 src + 2 repair */);

    REQUIRE(detail::get_packet_type(enc_handler[0]) == detail::packet_type::source);
    REQUIRE(detail::get_packet_type(enc_handler[1]) == detail::packet_type::source);
    REQUIRE(detail::get_packet_type(enc_handler[2]) == detail::packet_type::source);
    REQUIRE(detail::get_packet_type(enc_handler[3]) == detail::packet_type::repair);

    REQUIRE(detail::get_packet_type(enc_handler[4]) == detail::packet_type::source);
    REQUIRE(detail::get_packet_type(enc_handler[5]) == detail::packet_type::source);
    REQUIRE(detail::get_packet_type(enc_handler[6]) == detail::packet_type::source);
    REQUIRE(detail::get_packet_type(enc_handler[7]) == detail::packet_type::repair);

    SECTION("Right order")
    {
      // s1 and s2 are lost, unable to reconstruct them.
      dec(enc_handler[0]);
      dec(enc_handler[3]); // << repair
      REQUIRE(dec.nb_missing_sources() == 2);

      dec(enc_handler[4]);
      dec(enc_handler[5]);
      dec(enc_handler[6]);
      dec(enc_handler[7]); // << repair, outdating sources 0, 1 and 2

      REQUIRE(dec.nb_received_sources() == 4);
      REQUIRE(dec.nb_received_repairs() == 2);
      REQUIRE(dec.nb_missing_sources() == 0);
      REQUIRE(dec.nb_decoded() == 0);

      // Sources were given in the correct order the user handler.
      REQUIRE(dec_data_handler.nb_data() == 4);
      REQUIRE(std::equal(begin(s0), end(s0), begin(dec_data_handler[0])));
      REQUIRE(std::equal(begin(s3), end(s3), begin(dec_data_handler[1])));
      REQUIRE(std::equal(begin(s4), end(s4), begin(dec_data_handler[2])));
      REQUIRE(std::equal(begin(s5), end(s5), begin(dec_data_handler[3])));
    }

    SECTION("Wrong order 1")
    {
      // s1 and s2 are lost, unable to reconstruct them.
      dec(enc_handler[0]); // s0
      dec(enc_handler[4]); // s3
      dec(enc_handler[5]); // s4
      dec(enc_handler[6]); // s5
      dec(enc_handler[7]); // << repair for 3, 4 and 5; outdating sources 0, 1 and 2
      dec(enc_handler[3]); // << repair for 0, 1 and 2

      REQUIRE(dec.nb_received_sources() == 4);
      REQUIRE(dec.nb_received_repairs() == 2);
      REQUIRE(dec.nb_missing_sources() == 0);
      REQUIRE(dec.nb_decoded() == 0);

      // Sources were given in the correct order the user handler.
      REQUIRE(dec_data_handler.nb_data() == 4);
      REQUIRE(std::equal(begin(s0), end(s0), begin(dec_data_handler[0])));
      REQUIRE(std::equal(begin(s3), end(s3), begin(dec_data_handler[1])));
      REQUIRE(std::equal(begin(s4), end(s4), begin(dec_data_handler[2])));
      REQUIRE(std::equal(begin(s5), end(s5), begin(dec_data_handler[3])));
    }

    SECTION("Wrong order 2")
    {
      // s1 and s2 are lost, unable to reconstruct them.
      dec(enc_handler[4]); // s3
      dec(enc_handler[5]); // s4
      dec(enc_handler[6]); // s5
      dec(enc_handler[7]); // << repair for 3, 4 and 5; outdating sources 0, 1 and 2
      dec(enc_handler[3]); // << repair for 0, 1 and 2
      dec(enc_handler[0]); // s0

      REQUIRE(dec.nb_received_sources() == 4);
      REQUIRE(dec.nb_received_repairs() == 2);
      REQUIRE(dec.nb_missing_sources() == 0);
      REQUIRE(dec.nb_decoded() == 0);

      // Sources were given in the correct order the user handler.
      REQUIRE(dec_data_handler.nb_data() == 3);
      REQUIRE(std::equal(begin(s3), end(s3), begin(dec_data_handler[0])));
      REQUIRE(std::equal(begin(s4), end(s4), begin(dec_data_handler[1])));
      REQUIRE(std::equal(begin(s5), end(s5), begin(dec_data_handler[2])));
    }

    SECTION("Wrong order 3")
    {
      // s1 and s2 are lost, unable to reconstruct them.
      dec(enc_handler[5]); // s4
      dec(enc_handler[6]); // s5
      dec(enc_handler[7]); // << repair for 3, 4 and 5; outdating sources 0, 1 and 2
      dec(enc_handler[3]); // << repair for 0, 1 and 2
      dec(enc_handler[0]); // s0
      dec(enc_handler[4]); // s3

      REQUIRE(dec.nb_received_sources() == 4);
      REQUIRE(dec.nb_received_repairs() == 2);
      REQUIRE(dec.nb_missing_sources() == 0);
      REQUIRE(dec.nb_decoded() == 1);

      // Sources were given in the correct order the user handler.
      REQUIRE(dec_data_handler.nb_data() == 3);
      REQUIRE(std::equal(begin(s3), end(s3), begin(dec_data_handler[0])));
      REQUIRE(std::equal(begin(s4), end(s4), begin(dec_data_handler[1])));
      REQUIRE(std::equal(begin(s5), end(s5), begin(dec_data_handler[2])));
    }

    SECTION("Outdated sources")
    {
      // s0 is lost.
      // dec(enc_handler[0]); // s0
      dec(enc_handler[1]); // s1
      dec(enc_handler[2]); // s2
      // r0 is lost
      // dec(enc_handler[3]); // << repair for 0, 1 and 2
      dec(enc_handler[4]); // s3
      dec(enc_handler[5]); // s4
      dec(enc_handler[6]); // s5
      dec(enc_handler[7]); // << repair for s3, s4 and s5, outdating s0, s1 and s2

      // Source s0 was never received and the repair which could have decoded it was also missing.
      // Thus, the library will give all sources available from s1.

      REQUIRE(dec.nb_received_sources() == 5);
      REQUIRE(dec.nb_received_repairs() == 1);
      REQUIRE(dec.nb_missing_sources() == 0);
      REQUIRE(dec.nb_decoded() == 0);

      // Sources were given in the correct order the user handler.
      REQUIRE(dec_data_handler.nb_data() == 5);
      REQUIRE(std::equal(begin(s1), end(s1), begin(dec_data_handler[0])));
      REQUIRE(std::equal(begin(s2), end(s2), begin(dec_data_handler[1])));
      REQUIRE(std::equal(begin(s3), end(s3), begin(dec_data_handler[2])));
      REQUIRE(std::equal(begin(s4), end(s4), begin(dec_data_handler[3])));
      REQUIRE(std::equal(begin(s5), end(s5), begin(dec_data_handler[4])));
    }
  });
}

/*------------------------------------------------------------------------------------------------*/

TEST_CASE("Decoder rejects ack")
{
  launch([](std::uint8_t gf_size)
  {
    const auto data = std::vector<char>(100, 'x');

    decoder<packet_handler, data_handler> dec{ gf_size, in_order::yes, packet_handler{}
                                             , data_handler{}};
    packet_handler h;
    detail::packetizer<packet_handler> serializer{h};

    SECTION("ack")
    {
      serializer.write_ack(detail::ack{{0,1,2,3}, 33});
      REQUIRE_THROWS_AS(dec(h[0]), packet_type_error);
    }

    SECTION("Garbage")
    {
      REQUIRE_THROWS_AS(dec(packet{33,35,1,0}), packet_type_error);
    }
  });
}

/*------------------------------------------------------------------------------------------------*/
