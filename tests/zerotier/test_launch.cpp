// Production launch validation and XML parsing; HTTP is a controlled fixture.
#include "client.h"
#include "http.h"
#include "errors.h"
#include "Logger.hpp"
#include <cassert>
#include <cstring>
#include <cstdio>
static int requests = 0;
static std::string request_url;
static std::string response = "<root status_code=\"200\"><gamesession>1</gamesession></root>";
int http_request(std::string url, Data *data, HTTPRequestTimeout) {
    ++requests;
    request_url = url;
    *data = Data((char*)response.data(), response.size());
    return GS_OK;
}
void http_diagnostic(const char*, ...) {}
void Logger::info(const char*, const char*, ...) {}
void Logger::error(const char*, const char*, ...) {}
void Logger::debug(const char*, const char*, ...) {}
void Logger::warning(const char*, const char*, ...) {}
void Logger::fatal(const char*, const char*, ...) {}
static SERVER_DATA server() {
    SERVER_DATA value = {};
    value.serverInfo.address = "192.0.2.18";
    value.paired = true;
    return value;
}
int main() {
    STREAM_CONFIGURATION config = {};
    config.width = 1280; config.height = 720; config.fps = 60;
    config.audioConfiguration = AUDIO_CONFIGURATION_STEREO;
    auto no_modes = server();
    assert(gs_start_app(&no_modes, &config, 1, true, false, 1) == GS_OK);
    assert(requests == 1 && request_url.find("/launch?") != std::string::npos);
    assert(request_url.find("mode=1280x720x60") != std::string::npos);
    assert(no_modes.currentGame == 1);
    // Both listed applications use the same preflight and can reach launch.
    no_modes.currentGame = 0;
    assert(gs_start_app(&no_modes, &config, 2, true, false, 1) == GS_OK);
    assert(requests == 2 && request_url.find("appid=2") != std::string::npos);
    DISPLAY_MODE supported = {720,1280,60,nullptr};
    auto listed = server(); listed.modes = &supported;
    assert(gs_start_app(&listed, &config, 1, true, false, 1) == GS_OK);
    int before = requests;
    supported.refresh = 30; listed.currentGame = 0;
    assert(gs_start_app(&listed, &config, 1, true, false, 1) == GS_NOT_SUPPORTED_MODE);
    assert(requests == before && listed.currentGame == 0);
    // Explicitly ignoring advertised modes must also work with SOPS enabled.
    supported.width = 1920; supported.height = 1080; listed.unsupported = true;
    assert(gs_start_app(&listed, &config, 1, true, false, 1) == GS_OK);
    auto failed = server();
    response = "<root status_code=\"503\" status_message=\"Encoder unavailable\"/>";
    assert(gs_start_app(&failed, &config, 1, true, false, 1) == GS_ERROR);
    assert(failed.currentGame == 0 && gs_error() == "Encoder unavailable");
    response = "<root status_code=\"200\"><gamesession>0</gamesession></root>";
    assert(gs_start_app(&failed, &config, 1, true, false, 1) == GS_FAILED);
    assert(failed.currentGame == 0);
    response = "<root status_code=\"200\"/>";
    assert(gs_start_app(&failed, &config, 1, true, false, 1) != GS_OK);
    assert(failed.currentGame == 0);
    puts("launch: absent mode list, Desktop/Steam requests, advertised modes, override and server rejection OK");
}
