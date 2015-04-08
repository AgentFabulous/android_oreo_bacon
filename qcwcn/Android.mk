ifeq ($(filter msm8916,$(TARGET_BOARD_PLATFORM)),)
ifeq ($(BOARD_WLAN_DEVICE),qcwcn)
    include $(call all-subdir-makefiles)
endif
endif
