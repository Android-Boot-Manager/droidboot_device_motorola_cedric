# top level project rules for the msm8916 project
#
LOCAL_DIR := $(GET_LOCAL_DIR)

OUTBOOTIMG := $(BUILDDIR)/lk2nd.img

# Droidboot patch, include droidboot_gui to build
DROIDBOOT_PLATFORM=qcom
MODULES += app/droidboot_gui

# Enable fastboot display menu
ENABLE_FBCON_DISPLAY_MSG := 1
DISABLE_CRYPTO := 1

include $(LOCAL_DIR)/msm8916.mk

# Use maximum verbosity
DEBUG := 2
DEFINES += DISABLE_CRYPTO=1
DEFINES += LK_LOG_BUF_SIZE=16384

# Avoid writing device info
DEFINES += SAFE_MODE=1
# Display as unlocked by default
DEFINES += DEFAULT_UNLOCK=1

# Use continuous splash from primary bootloader for display
DISPLAY_USE_CONTINUOUS_SPLASH := 1

APPSBOOTHEADER: $(OUTBOOTIMG)
ANDROID_BOOT_BASE := 0x80000000
ANDROID_BOOT_CMDLINE := lk2nd
