ROOT := ../..
ZT := $(ROOT)/src/zerotier
OUT := $(abspath $(ROOT)/.build-tests/zerotier)
INC := -I$(ZT) -I$(ZT)/include -I$(ZT)/lwip-port -I$(ZT)/lwip/src/include
CXXFLAGS := -std=c++17 -O1 -g -Wall -Wextra -pthread $(INC)
WRAP := socket connect bind close shutdown setsockopt getsockopt getsockname getpeername send recv sendto recvfrom sendmsg recvmsg poll select fcntl ioctl read write
.PHONY: all build run
all: run
build:
	mkdir -p $(OUT)
	$(CC) -std=c11 -O1 -g $(INC) -c $(ZT)/net/lwip_unity.c -o $(OUT)/lwip.o
	$(CXX) $(CXXFLAGS) test_vnet.cpp $(ZT)/net/vnet.cpp -o $(OUT)/test_vnet
	$(CXX) $(CXXFLAGS) test_transport.cpp $(ZT)/StateStore.cpp $(ZT)/Transport.cpp $(ZT)/net/vnet.cpp $(ZT)/net/tcp_stack.cpp $(OUT)/lwip.o $(foreach f,$(WRAP),-Wl,--wrap=$(f)) -o $(OUT)/test_transport
run: build
	$(OUT)/test_vnet
	$(OUT)/test_transport
	$(OUT)/test_transport custom-network

HTTP_INC := -I$(ROOT)/src -I$(ROOT)/src/libgamestream -I$(ROOT)/src/crypto -I$(ROOT)/src/utils -I$(ROOT)/third_party/moonlight-common-c/src
.PHONY: http-build http-test
http-build:
	mkdir -p $(OUT)
	$(CXX) $(CXXFLAGS) $(HTTP_INC) -DUSE_MBEDTLS -DMOONLIGHT_CONNECTION_LOG='"/tmp/moonlight-http-test.log"' test_http_cancel.cpp $(ROOT)/src/libgamestream/http.cpp $(ROOT)/src/crypto/Data.cpp $$(pkg-config --cflags --libs libcurl) -o $(OUT)/test_http_cancel
http-test: http-build
	$(OUT)/test_http_cancel

.PHONY: launch-test
launch-test:
	mkdir -p $(OUT)
	$(CXX) $(CXXFLAGS) $(HTTP_INC) -DUSE_MBEDTLS -ffunction-sections -fdata-sections test_launch.cpp $(ROOT)/src/libgamestream/client.cpp $(ROOT)/src/libgamestream/xml.cpp $(ROOT)/src/crypto/Data.cpp -Wl,--gc-sections -lexpat -o $(OUT)/test_launch
	$(OUT)/test_launch

ENET_SOURCE := $(ROOT)/modern/extern/moonlight-common-c/enet
ENET_HOST := $(OUT)/enet-host
.PHONY: enet-build enet-test
enet-build:
	mkdir -p $(OUT)
	cmake -S $(ENET_SOURCE) -B $(ENET_HOST) -DCMAKE_BUILD_TYPE=Debug -DENET_NO_INSTALL=ON
	+$(MAKE) -C $(ENET_HOST)
	$(CC) -std=c11 -O1 -g $(INC) -c $(ZT)/net/lwip_unity.c -o $(OUT)/lwip.o
	$(CXX) $(CXXFLAGS) -DTEST_MODERN_ENET -I$(ENET_SOURCE)/include test_transport.cpp $(ZT)/StateStore.cpp $(ZT)/Transport.cpp $(ZT)/net/vnet.cpp $(ZT)/net/tcp_stack.cpp $(OUT)/lwip.o $(ENET_HOST)/libenet.a $(foreach f,$(WRAP),-Wl,--wrap=$(f)) -o $(OUT)/test_transport_enet
enet-test: enet-build
	$(OUT)/test_transport_enet
	$(OUT)/test_transport_enet custom-network

.PHONY: config-test
config-test:
	mkdir -p $(OUT)
	$(CXX) $(CXXFLAGS) test_options.cpp -o $(OUT)/test_options
	$(OUT)/test_options
