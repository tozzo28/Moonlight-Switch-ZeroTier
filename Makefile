# Default: original XITRIX Moonlight-Switch UI plus process-local ZeroTier.
# The working legacy UI remains buildable with make LEGACY_UI=1 -j2.
.DEFAULT_GOAL := all
LEGACY_UI ?= 0
ifeq ($(LEGACY_UI),1)
LEGACY_MAKEFILE := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))Makefile.legacy
include $(LEGACY_MAKEFILE)
.PHONY: force-legacy-output
$(OUTPUT).elf $(OUTPUT).nacp: force-legacy-output
else
export DEVKITPRO ?= /opt/devkitpro
MODERN_BUILD := modern/build/switch
ZSTD_ARCHIVE := modern/extern/zstd/lib/libzstd.a
SWITCH_ARCH := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE

.PHONY: all modern clean
all: modern

$(ZSTD_ARCHIVE):
	+$(MAKE) -C modern/extern/zstd/lib libzstd.a \
		CC=$(DEVKITPRO)/devkitA64/bin/aarch64-none-elf-gcc \
		AR=$(DEVKITPRO)/devkitA64/bin/aarch64-none-elf-ar \
		CFLAGS='-O2 $(SWITCH_ARCH) -ffunction-sections -fdata-sections -D__SWITCH__'

$(MODERN_BUILD)/Makefile: modern/CMakeLists.txt modern/EmbeddedZeroTier.cmake $(ZSTD_ARCHIVE)
	cmake -S modern -B $(MODERN_BUILD) -DPLATFORM_SWITCH=ON -DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_TOOLCHAIN_FILE=$(DEVKITPRO)/cmake/Switch.cmake \
		-DZSTD_LIBRARY=$(CURDIR)/$(ZSTD_ARCHIVE)

modern: $(MODERN_BUILD)/Makefile
	+$(MAKE) -C $(MODERN_BUILD) Moonlight.nro
	cp $(MODERN_BUILD)/Moonlight.nro moonlight.nro
	cp $(MODERN_BUILD)/Moonlight.elf moonlight.elf
	cp $(MODERN_BUILD)/Moonlight.nacp moonlight.nacp

clean:
	cmake --build $(MODERN_BUILD) --target clean
endif
