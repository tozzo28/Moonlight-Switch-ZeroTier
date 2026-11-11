ROOT := ../..
APP := $(ROOT)/modern/app
BRLS := $(ROOT)/modern/extern/borealis/library
OUT := $(abspath $(ROOT)/.build-tests/modern)
CXXFLAGS := -std=c++20 -O1 -g -Wall -pthread -Ishim -I$(BRLS)/include
HTTP_INCLUDES := -I$(APP)/src/libgamestream -I$(APP)/src/crypto -I$(ROOT)/modern/extern/moonlight-common-c/src -I$(ROOT)/src
.PHONY: gestures http-build http-test

gestures:
	mkdir -p $(OUT)
	$(CXX) $(CXXFLAGS) -I$(APP)/include/gestures test_gestures.cpp $(APP)/src/gestures/click_gesture_recognizer.cpp $(APP)/src/gestures/fingers_gesture_recognizer.cpp $(APP)/src/gestures/two_finger_scroll_recognizer.cpp $(BRLS)/lib/core/geometry.cpp -o $(OUT)/test_gestures
	$(OUT)/test_gestures

http-build:
	mkdir -p $(OUT)
	$(CXX) $(CXXFLAGS) $(HTTP_INCLUDES) -DUSE_MBEDTLS_CRYPTO -DMOONLIGHT_CONNECTION_LOG='"/tmp/moonlight-http-test.log"' $(ROOT)/tests/zerotier/test_http_cancel.cpp $(APP)/src/libgamestream/http.cpp $(APP)/src/crypto/Data.cpp $$(pkg-config --cflags --libs libcurl) -o $(OUT)/test_http_cancel

http-test: http-build
	$(OUT)/test_http_cancel

.PHONY: resume-test
resume-test:
	mkdir -p $(OUT)
	$(CXX) $(CXXFLAGS) $(HTTP_INCLUDES) -DUSE_MBEDTLS_CRYPTO -ffunction-sections -fdata-sections test_resume.cpp $(APP)/src/libgamestream/client.cpp $(APP)/src/libgamestream/xml.cpp $(APP)/src/crypto/Data.cpp -Wl,--gc-sections -lexpat -o $(OUT)/test_resume
	$(OUT)/test_resume
