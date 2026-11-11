#pragma once
#include <atomic>
#include <mutex>
#include <deque>
#include <vector>
#include <ZeroTierOne.h>
#include "net/vnet.hpp"
#include "net/tcp_stack.hpp"
#include "Lifecycle.hpp"
namespace embedded {
// No network or peer is selected until the user configures them.
struct Frame { uint64_t mac; uint16_t type; std::vector<unsigned char> data; };
struct Runtime {
    std::mutex lock;
    ztnx::net::VNet vnet;
    ztnx::net::TcpStack *tcp = ztnx::net::TcpStack::instance();
    std::deque<Frame> output;
    std::atomic<bool> running{false};
    bool subscribe = false;
    unsigned generation = 0;
    ZT_Node *node = nullptr;
    int wire = -1;
    uint64_t network = 0;
    uint32_t peer = 0;
};
Runtime &runtime();
void setTransportEnabled(bool enabled);
void stop();
uint32_t ticks();
}
