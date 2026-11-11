#include "Lifecycle.hpp"
#include <cassert>
#include <cstdio>

int main() {
    embedded::Options defaults;
    uint64_t network = 42;
    uint32_t peer = 42;
    assert(!defaults.enabled);
    assert(defaults.network.empty() && defaults.peer.empty());
    assert(!embedded::parseNetwork(defaults.network, network) && network == 42);
    assert(!embedded::parsePeer(defaults.peer, peer) && peer == 42);
    assert(embedded::parseNetwork("ABCDEF0123456789", network) && network == 0xabcdef0123456789ULL);
    assert(embedded::parsePeer("172.27.0.18", peer) && peer == 0xac1b0012);
    for (const auto* input : {"", "0000000000000000", "0123456789abcde", "0123456789abcdef3", "0123456789abcdeg", " 123456789abcdef"}) {
        network = 42;
        assert(!embedded::parseNetwork(input, network) && network == 42);
    }
    for (const auto* input : {"", "0.0.0.0", "256.0.0.1", "1.2.3", "1.2.3.4.5", "1..2.3", ".1.2.3", "1.2.3.", "1.2.3.4 ", "-1.2.3.4", "127.0.0.1", "224.0.0.1", "255.255.255.255"}) {
        peer = 42;
        assert(!embedded::parsePeer(input, peer) && peer == 42);
    }
    puts("ZeroTier options: existing defaults, new networks and invalid input OK");
}
