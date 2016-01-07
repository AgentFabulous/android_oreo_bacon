ifneq ($(filter msm8909 ,$(TARGET_BOARD_PLATFORM)),)
wlan-dirs := qcwcn wcnss-service
include $(call all-named-subdir-makefiles,$(wlan-dirs))
endif
