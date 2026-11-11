// Real host curl against a loopback HTTP server that deliberately waits like
// Sunshine's getservercert endpoint before the user confirms a PIN.
#include "http.h"
#include "client.h"
#include "Logger.hpp"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <atomic>
#include <thread>
#include <chrono>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>

static std::string last_error;
void gs_set_error(std::string value) { last_error = value; }
std::string gs_error() { return last_error; }
void Logger::info(const char*, const char*, ...) {}
void Logger::error(const char*, const char*, ...) {}
void Logger::debug(const char*, const char*, ...) {}
void Logger::warning(const char*, const char*, ...) {}
void Logger::fatal(const char*, const char*, ...) {}

struct Server {
    int fd;
    uint16_t port;
    std::atomic<bool> received{false}, release{false};
    std::thread thread;
    Server(bool reply) {
        fd = socket(AF_INET, SOCK_STREAM, 0);
        assert(fd >= 0);
        sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        assert(bind(fd, (sockaddr*)&addr, sizeof(addr)) == 0);
        socklen_t len = sizeof(addr);
        assert(getsockname(fd, (sockaddr*)&addr, &len) == 0);
        port = ntohs(addr.sin_port);
        assert(listen(fd, 1) == 0);
        thread = std::thread([this, reply] {
            int client = accept(fd, nullptr, nullptr);
            assert(client >= 0);
            char request[4096];
            assert(recv(client, request, sizeof(request), 0) > 0);
            received.store(true);
            if (reply) {
                const char response[] = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\nConnection: close\r\n\r\nOK";
                assert(send(client, response, sizeof(response)-1, 0) == sizeof(response)-1);
            } else {
                while (!release.load()) std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            close(client);
        });
    }
    ~Server() { release.store(true); thread.join(); close(fd); }
    std::string url() const { return "http://127.0.0.1:" + std::to_string(port) + "/pair?phrase=getservercert&salt=PRIVATE-MARKER"; }
};
static bool wait(std::atomic<bool>& flag, int milliseconds) {
    auto start = std::chrono::steady_clock::now();
    while (!flag.load()) {
        if (std::chrono::steady_clock::now()-start > std::chrono::milliseconds(milliseconds)) return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return true;
}
int main() {
    remove("/tmp/moonlight-http-test.log");
    http_begin_requests();
    assert(http_init("/tmp/nonexistent-moonlight-key") == 0);
    {
        Server server(false);
        std::atomic<bool> complete{false};
        int result = 0;
        std::thread request([&] {
            Data data;
            result = http_request(server.url(), &data, HTTPRequestTimeoutLong);
            complete.store(true);
        });
        assert(wait(server.received, 5000));
        const auto start = std::chrono::steady_clock::now();
        http_cancel_requests();
        assert(wait(complete, 5000));
        request.join();
        assert(result != 0 && http_requests_cancelled());
        assert(gs_error().find("cancelled") != std::string::npos);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count();
        printf("Pending 120-second request cancelled in %lld ms\n", (long long)ms);
        Data ignored;
        assert(http_request(server.url(), &ignored, HTTPRequestTimeoutLong) != 0);
    }
    http_begin_requests();
    {
        Server server(true);
        Data data;
        assert(http_request(server.url(), &data, HTTPRequestTimeoutLow) == 0);
        assert(data.size() == 2 && std::string((char*)data.bytes(), data.size()) == "OK");
    }
    http_cleanup();
    // Cleanup is idempotent and a fresh session can be initialized afterward.
    http_cleanup();
    assert(http_init("/tmp/nonexistent-moonlight-key") == 0);
    http_cleanup();
    std::ifstream file("/tmp/moonlight-http-test.log");
    const std::string log((std::istreambuf_iterator<char>(file)), {});
    assert(log.find("cancelled=1") != std::string::npos);
    assert(log.find("PRIVATE-MARKER") == std::string::npos);
    puts("http: cancellation, rejected requests after shutdown, restart, cleanup and redacted diagnostics OK");
}
