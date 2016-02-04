ifneq ($(filter msm8909,$(TARGET_BOARD_PLATFORM)),)
include $(call all-named-subdir-makefiles,msm8909)
else
include $(call all-named-subdir-makefiles,qcwcn)
endif
