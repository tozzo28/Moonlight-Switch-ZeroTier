#include "GameStreamClient.hpp"
#include "Settings.hpp"
#include "http.h"
#include "Logger.hpp"
#include <atomic>
#include <condition_variable>
#include "WakeOnLanManager.hpp"
#include <nanogui/nanogui.h>
#include <thread>
#include <mutex>
#include <algorithm>
#include <vector>
#include <iostream>
#include <fstream>
#include <future>
#include <unistd.h>

#if defined(__linux) || defined(__APPLE__)
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>

#elif defined(__SWITCH__)
#include <switch.h>
#endif

static std::mutex m_async_mutex;
static std::condition_variable m_async_ready;
static std::vector<std::function<void()>> m_tasks;
static std::atomic<bool> task_loop_active{false};

static void deliver_callback(std::function<void()> callback) {
    if (!task_loop_active.load()) return;
    nanogui::async([callback] {
        if (task_loop_active.load()) callback();
    });
}

static void task_loop() {
    while (true) {
        std::vector<std::function<void()>> tasks;
        {
            std::unique_lock<std::mutex> lock(m_async_mutex);
            m_async_ready.wait(lock, [] { return !task_loop_active.load() || !m_tasks.empty(); });
            if (!task_loop_active.load()) break;
            tasks.swap(m_tasks);
        }
        for (auto &task : tasks) {
            if (!task_loop_active.load()) break;
            task();
        }
    }
    http_diagnostic("Task worker stopped");
}

#ifdef __SWITCH__
static Thread task_loop_thread;
static bool task_thread_created = false;
static void start_task_loop() {
    Result result = threadCreate(&task_loop_thread, [](void*) { task_loop(); },
                                 nullptr, nullptr, 0x10000, 0x2C, -2);
    if (R_SUCCEEDED(result)) {
        task_thread_created = true;
        result = threadStart(&task_loop_thread);
    }
    if (R_FAILED(result)) {
        task_loop_active.store(false);
        if (task_thread_created) { threadClose(&task_loop_thread); task_thread_created = false; }
        http_diagnostic("Task worker start failed: 0x%x", result);
    }
}
#else
static std::thread task_loop_thread;
static void start_task_loop() { task_loop_thread = std::thread(task_loop); }
#endif

void perform_async(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(m_async_mutex);
        if (!task_loop_active.load()) return;
        m_tasks.push_back(std::move(task));
    }
    m_async_ready.notify_one();
}

void GameStreamClient::start() {
    http_begin_requests();
    task_loop_active.store(true);
    start_task_loop();
}

void GameStreamClient::stop() {
    // Abort curl before joining the worker: pairing can otherwise wait 120s.
    http_cancel_requests();
    {
        std::lock_guard<std::mutex> lock(m_async_mutex);
        task_loop_active.store(false);
        m_tasks.clear();
    }
    m_async_ready.notify_all();
    http_diagnostic("Shutdown: cancelling requests and joining worker");
#ifdef __SWITCH__
    if (task_thread_created) {
        threadWaitForExit(&task_loop_thread);
        threadClose(&task_loop_thread);
        task_thread_created = false;
    }
#else
    if (task_loop_thread.joinable()) task_loop_thread.join();
#endif
    http_cleanup();
    http_diagnostic("Shutdown: task worker joined");
}

static uint32_t get_my_ip_address() {
    uint32_t address = 0;
#if defined(__linux) || defined(__APPLE__)
    struct ifreq ifr;
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, "en0", IFNAMSIZ - 1);
    
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    ioctl(fd, SIOCGIFADDR, &ifr);
    close(fd);
    
    address = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;
#elif defined(__SWITCH__)
    nifmGetCurrentIpAddress(&address);
#endif
    return address;
}

std::vector<std::string> GameStreamClient::host_addresses_for_find() {
    std::vector<std::string> addresses;
    
    uint32_t address = get_my_ip_address();
    bool isSucceed = address != 0;
    
    if (isSucceed) {
        int a = address & 0xFF;
        int b = (address >> 8) & 0xFF;
        int c = (address >> 16) & 0xFF;
        int d = (address >> 24) & 0xFF;
        
        for (int i = 0; i < 256; i++) {
            if (i == d) {
                continue;
            }
            addresses.push_back(std::to_string(a) + "." + std::to_string(b) + "." + std::to_string(c) + "." + std::to_string(i));
        }
    }
    return addresses;
}

bool GameStreamClient::can_find_host() {
    return get_my_ip_address() != 0;
}

void GameStreamClient::find_host(ServerCallback<Host> callback) {
    perform_async([this, callback] {
        auto addresses = host_addresses_for_find();
        
        if (addresses.empty()) {
            deliver_callback([callback] { callback(GSResult<Host>::failure("Can't obtain IP address...")); });
        } else {
            bool found = false;
            
            for (int i = 0; i < addresses.size() && task_loop_active.load(); i++) {
                SERVER_DATA server_data;
                
                int status = gs_init(&server_data, addresses[i], true);
                if (status == GS_OK) {
                    found = true;
                    
                    Host host;
                    host.address = addresses[i];
                    host.hostname = server_data.hostname;
                    host.mac = server_data.mac;
                    deliver_callback([callback, host] { callback(GSResult<Host>::success(host)); });
                    break;
                }
            }
            
            if (!found) {
                deliver_callback([callback] { callback(GSResult<Host>::failure("Host PC not found...")); });
            }
        }
    });
}

bool GameStreamClient::can_wake_up_host(const Host &host) {
    return WakeOnLanManager::instance().can_wake_up_host(host);
}

void GameStreamClient::wake_up_host(const Host &host, ServerCallback<bool> callback) {
    perform_async([this, host, callback] {
        auto result = WakeOnLanManager::instance().wake_up_host(host);
        
        if (result.isSuccess()) {
            usleep(5'000'000);
            deliver_callback([callback, result] { callback(result); });
        } else {
            deliver_callback([callback, result] { callback(result); });
        }
    });
}

void GameStreamClient::connect(const std::string &address, ServerCallback<SERVER_DATA> callback) {
    m_server_data[address] = SERVER_DATA();
    
    perform_async([this, address, callback] {
        int status = gs_init(&m_server_data[address], address);
        
        deliver_callback([this, address, callback, status] {
            if (status == GS_OK) {
                Host host;
                host.address = address;
                host.hostname = m_server_data[address].hostname;
                host.mac = m_server_data[address].mac;
                Settings::instance().add_host(host);
                callback(GSResult<SERVER_DATA>::success(m_server_data[address]));
            } else {
                callback(GSResult<SERVER_DATA>::failure(gs_error()));
            }
        });
    });
}

void GameStreamClient::pair(const std::string &address, const std::string &pin, ServerCallback<bool> callback) {
    if (m_server_data.count(address) == 0) {
        callback(GSResult<bool>::failure("Firstly call connect()..."));
        return;
    }
    
    perform_async([this, address, pin, callback] {
        int status = gs_pair(&m_server_data[address], (char *)pin.c_str());
        
        const auto error = gs_error();
        deliver_callback([callback, status, error] {
            if (status == GS_OK) {
                callback(GSResult<bool>::success(true));
            } else {
                callback(GSResult<bool>::failure(error));
            }
        });
    });
}

void GameStreamClient::applist(const std::string &address, ServerCallback<AppInfoList> callback) {
    if (m_server_data.count(address) == 0) {
        callback(GSResult<AppInfoList>::failure("Firstly call connect() & pair()..."));
        return;
    }
    
    perform_async([this, address, callback] {
        PAPP_LIST list;
        
        int status = gs_applist(&m_server_data[address], &list);
        
        AppInfoList app_list;
        
        while (list) {
            app_list.push_back({ .name = list->name, .app_id = list->id });
            list = list->next;
        }
        
        std::sort(app_list.begin(), app_list.end(), [](AppInfo a, AppInfo b) { return a.name < b.name; });
        
        deliver_callback([this, app_list, callback, status] {
            if (status == GS_OK) {
                callback(GSResult<AppInfoList>::success(app_list));
            } else {
                callback(GSResult<AppInfoList>::failure(gs_error()));
            }
        });
    });
}

void GameStreamClient::app_boxart(const std::string &address, int app_id, ServerCallback<Data> callback) {
    if (m_server_data.count(address) == 0) {
        callback(GSResult<Data>::failure("Firstly call connect() & pair()..."));
        return;
    }
    
    perform_async([this, address, app_id, callback] {
        Data data;
        int status = gs_app_boxart(&m_server_data[address], app_id, &data);
        
        deliver_callback([this, callback, data, status] {
            if (status == GS_OK) {
                callback(GSResult<Data>::success(data));
            } else {
                callback(GSResult<Data>::failure(gs_error()));
            }
        });
    });
}

void GameStreamClient::start(const std::string &address, STREAM_CONFIGURATION config, int app_id, ServerCallback<STREAM_CONFIGURATION> callback) {
    if (m_server_data.count(address) == 0) {
        callback(GSResult<STREAM_CONFIGURATION>::failure("Firstly call connect() & pair()..."));
        return;
    }
    
    m_config = config;
    
    perform_async([this, address, app_id, callback] {
        int status = gs_start_app(&m_server_data[address], &m_config, app_id, Settings::instance().sops(), Settings::instance().play_audio(), 0x1);
        
        const auto error = gs_error();
        if (status != GS_OK) http_diagnostic("Launch error: code=%d message=%s", status, error.c_str());
        deliver_callback([this, callback, status, error] {
            if (status == GS_OK) {
                callback(GSResult<STREAM_CONFIGURATION>::success(m_config));
            } else {
                callback(GSResult<STREAM_CONFIGURATION>::failure(error));
            }
        });
    });
}

void GameStreamClient::quit(const std::string &address, ServerCallback<bool> callback) {
    if (m_server_data.count(address) == 0) {
        callback(GSResult<bool>::failure("Firstly call connect() & pair()..."));
        return;
    }
    
    auto server_data = m_server_data[address];
    
    perform_async([this, server_data, callback] {
        int status = gs_quit_app((PSERVER_DATA)&server_data);
        
        deliver_callback([this, callback, status] {
            if (status == GS_OK) {
                callback(GSResult<bool>::success(true));
            } else {
                callback(GSResult<bool>::failure(gs_error()));
            }
        });
    });
}
