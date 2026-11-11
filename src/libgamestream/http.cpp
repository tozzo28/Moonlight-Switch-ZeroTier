/*
 * This file is part of Moonlight Embedded.
 *
 * Copyright (C) 2015 Iwan Timmer
 *
 * Moonlight is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Moonlight is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Moonlight; if not, see <http://www.gnu.org/licenses/>.
 */

#include "http.h"
#include "errors.h"
#include "client.h"
#include "CryptoManager.hpp"
#include "Logger.hpp"

#include <stdbool.h>
#include <string.h>
#include <curl/curl.h>
#include <atomic>
#include <mutex>
#include <chrono>
#include <cstdarg>
#include <cstdint>
#include <algorithm>

static CURL *curl;
static std::atomic<bool> stopping{false};
static std::mutex diagnostic_mutex;
#ifndef MOONLIGHT_CONNECTION_LOG
#define MOONLIGHT_CONNECTION_LOG "sdmc:/switch/moonlight/connection.log"
#endif

void http_diagnostic(const char *format, ...) {
    std::lock_guard<std::mutex> lock(diagnostic_mutex);
    FILE *file = fopen(MOONLIGHT_CONNECTION_LOG, "a");
    if (!file) return;
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    fprintf(file, "[%lld] ", (long long)ms);
    va_list args;
    va_start(args, format);
    vfprintf(file, format, args);
    va_end(args);
    fputc('\n', file);
    fclose(file);
}

void http_begin_requests() {
    stopping.store(false);
    // Bound diagnostics across launches; never log URLs with query secrets.
    FILE *file = fopen(MOONLIGHT_CONNECTION_LOG, "rb");
    if (file) {
        fseek(file, 0, SEEK_END);
        long length = ftell(file);
        fclose(file);
        if (length > 128 * 1024) {
            const std::string previous = std::string(MOONLIGHT_CONNECTION_LOG) + ".old";
            remove(previous.c_str());
            rename(MOONLIGHT_CONNECTION_LOG, previous.c_str());
        }
    }
    http_diagnostic("Application requests started");
}
void http_cancel_requests() { stopping.store(true); }
bool http_requests_cancelled() { return stopping.load(); }

static int transfer_progress(void *, curl_off_t, curl_off_t, curl_off_t, curl_off_t) {
    return stopping.load() ? 1 : 0;
}

struct HTTP_DATA {
    char *memory = (char *)calloc(1, 1);
    size_t size = 0;
    ~HTTP_DATA() { free(memory); }
};

static size_t _write_curl(void *contents, size_t size, size_t nmemb, void *userp) {
    if (size && nmemb > SIZE_MAX / size) return 0;
    const size_t realsize = size * nmemb;
    auto *mem = static_cast<HTTP_DATA *>(userp);
    if (realsize > SIZE_MAX - mem->size - 1) return 0;
    char *new_memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
    if (!new_memory) return 0;
    mem->memory = new_memory;
    memcpy(mem->memory + mem->size, contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}

int http_init(const std::string key_directory) {
    if (!curl) {
        curl_global_init(CURL_GLOBAL_ALL);
        Logger::info("Curl", "%s", curl_version());
    } else {
        return GS_OK;
    }
    
    curl = curl_easy_init();
    
    if (!curl)
        return GS_FAILED;
    
    char certificateFilePath[4096];
    sprintf(certificateFilePath, "%s/%s", key_directory.c_str(), CERTIFICATE_FILE_NAME);
    
    char keyFilePath[4096];
    sprintf(&keyFilePath[0], "%s/%s", key_directory.c_str(), KEY_FILE_NAME);
    
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_SSLENGINE_DEFAULT, 1L);
    curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, "PEM");
    curl_easy_setopt(curl, CURLOPT_SSLCERT, certificateFilePath);
    curl_easy_setopt(curl, CURLOPT_SSLKEYTYPE, "PEM");
    curl_easy_setopt(curl, CURLOPT_SSLKEY, keyFilePath);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _write_curl);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_SESSIONID_CACHE, 0L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, transfer_progress);
    http_diagnostic("curl initialized: %s", curl_version());
    
    return GS_OK;
}

int http_request(const std::string url, Data* data, HTTPRequestTimeout timeout) {
    if (stopping.load()) {
        gs_set_error("Request cancelled: application is closing");
        return GS_FAILED;
    }
    if (!curl || !data) return GS_FAILED;
    const auto endpoint = url.substr(0, url.find('?'));
    Logger::info("Curl", "Request: %s", endpoint.c_str());
    http_diagnostic("HTTP begin %s timeout=%ld", endpoint.c_str(), (long)timeout);
    HTTP_DATA http_data;
    if (!http_data.memory) return GS_OUT_OF_MEMORY;
    char error_buffer[CURL_ERROR_SIZE] = {};
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &http_data);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)timeout);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, std::min<long>((long)timeout, 10));
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
    const CURLcode result = curl_easy_perform(curl);
    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, nullptr);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, nullptr);
    http_diagnostic("HTTP end %s curl=%d status=%ld bytes=%zu cancelled=%d",
        endpoint.c_str(), (int)result, status, http_data.size, (int)stopping.load());
    if (result != CURLE_OK || stopping.load()) {
        if (stopping.load()) gs_set_error("Request cancelled: application is closing");
        else if (result == CURLE_OPERATION_TIMEDOUT && url.find("phrase=getservercert") != std::string::npos)
            gs_set_error("Pairing timed out. Enter PIN 0000 in Sunshine > PIN on your PC, then try again.");
        else gs_set_error(error_buffer[0] ? error_buffer : curl_easy_strerror(result));
        Logger::error("Curl", "%s", gs_error().c_str());
        return GS_FAILED;
    }
    *data = Data(http_data.memory, http_data.size);
    Logger::info("Curl", "Response: HTTP %ld, %zu bytes", status, http_data.size);
    return GS_OK;
}

void http_cleanup() {
    if (curl) { curl_easy_cleanup(curl); curl = nullptr; }
    http_diagnostic("HTTP cleanup complete");
}
