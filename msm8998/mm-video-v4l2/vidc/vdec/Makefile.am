AM_CFLAGS = -Wall
AM_CFLAGS += -Wundef
AM_CFLAGS += -Wstrict-prototypes
AM_CFLAGS += -Wno-trigraphs
AM_CFLAGS += -g -O3

AM_CPPFLAGS := -D__alignx\(x\)=__attribute__\(\(__aligned__\(x\)\)\)
AM_CPPFLAGS += -D__align=__alignx
AM_CPPFLAGS += -Dinline=__inline
AM_CPPFLAGS += -DIMAGE_APPS_PROC
AM_CPPFLAGS += -DCDECL
AM_CPPFLAGS += -DT_ARM
AM_CPPFLAGS += -DNO_ARM_CLZ
AM_CPPFLAGS += -UENABLE_DEBUG_LOW
AM_CPPFLAGS += -DENABLE_DEBUG_HIGH
AM_CPPFLAGS += -DENABLE_DEBUG_ERROR
AM_CPPFLAGS += -UINPUT_BUFFER_LOG
AM_CPPFLAGS += -UOUTPUT_BUFFER_LOG
AM_CPPFLAGS += -Wno-parentheses
AM_CPPFLAGS += -D_ANDROID_ICS_
AM_CPPFLAGS += -D_MSM8974_
AM_CPPFLAGS += -DPROCESS_EXTRADATA_IN_OUTPUT_PORT
AM_CPPFLAGS += -DMAX_RES_1080P
AM_CPPFLAGS += -DMAX_RES_1080P_EBI

if TARGET_MSM8960
AM_CPPFLAGS += -DMAX_RES_1080P
AM_CPPFLAGS += -DMAX_RES_1080P_EBI
AM_CPPFLAGS += -DPROCESS_EXTRADATA_IN_OUTPUT_PORT
AM_CPPFLAGS += -DUSE_ION
endif

if TARGET_B_FAMILY
AM_CPPFLAGS += -DMAX_RES_1080P
AM_CPPFLAGS += -DMAX_RES_1080P_EBI
AM_CPPFLAGS += -DPROCESS_EXTRADATA_IN_OUTPUT_PORT
AM_CPPFLAGS += -DUSE_ION
AM_CPPFLAGS += -D_MSM8974_
#AM_CPPFLAGS += -D_HEVC_USE_ADSP_HEAP_
endif

AM_CPPFLAGS += -I./inc
AM_CPPFLAGS += -I../common/inc
AM_CPPFLAGS += -I../../../libc2dcolorconvert
AM_CPPFLAGS += -I../../../mm-core/inc
AM_CPPFLAGS += -I../../../mm-core/src/common

c_sources = src/frameparser.cpp
c_sources += src/h264_utils.cpp
c_sources += src/ts_parser.cpp
c_sources += src/mp4_utils.cpp

if TARGET_MSM8960
c_sources += src/omx_vdec.cpp
endif

if TARGET_B_FAMILY
c_sources += src/omx_vdec_msm8974.cpp
endif

c_sources += ../common/src/extra_data_handler.cpp
c_sources += ../common/src/vidc_color_converter.cpp

lib_LTLIBRARIES = libOmxVdec.la
libOmxVdec_la_SOURCES = $(c_sources)
libOmxVdec_la_CFLAGS = $(AM_CFLAGS) $(AM_CPPFLAGS) -fPIC
libOmxVdec_la_CFLAGS += ../libc2d2colorconvert/libc2dcolorconvert.la
libOmxVdec_la_LDFLAGS = -shared -version-info $(OMXVIDEO_LIBRARY_VERSION)

bin_PROGRAMS = mm-vdec-omx-test

mm_vdec_omx_test_CPPFLAGS = -I./inc
mm_vdec_omx_test_CPPFLAGS += -I../../../mm-core/inc
mm_vdec_omx_test_CPPFLAGS += -I../common/inc
mm_vdec_omx_test_CPPFLAGS += -DUSE_ION
mm_vdec_omx_test_CPPFLAGS += -D_NOFBMEM_

if TARGET_B_FAMILY
mm_vdec_omx_test_CPPFLAGS += -D_MSM8974_
endif

mm_vdec_omx_test_SOURCES := src/queue.c
mm_vdec_omx_test_SOURCES += test/omx_vdec_test.cpp
mm_vdec_omx_test_LDADD = -ldl -lpthread -lOmxCore
mm_vdec_omx_test_LDADD +=  ./libOmxVdec.la

