

######################################################################
#
######################################################################
LOCAL_PATH	:= $(call my-dir)
include $(CLEAR_VARS)

######################################################################
#
######################################################################
CFLAGS := -Werror

LOCAL_CFLAGS += -DANDROID_NDK
LOCAL_CFLAGS += -DACCESS_RAW_DESCRIPTORS
LOCAL_CFLAGS += -O3 -fstrict-aliasing -fprefetch-loop-arrays

LOCAL_LDLIBS += -llog
LOCAL_LDLIBS += -landroid
LOCAL_LDLIBS += -lmediandk
LOCAL_LDLIBS += -lOpenMAXAL
#LOCAL_LDLIBS += -ljnigraphics


#DIR_VideoTelemetryShared := $(LOCAL_PATH)/../../../../../../LiveVideo10ms/VideoTelemetryShared
DIR_VideoTelemetryShared := $(V_CORE_DIR)/../VideoTelemetryShared
# Android.mk doesnt work with absolute paths
# DIR_VideoTelemetryShared := $(V_CORE_DIR)/../VideoTelemetryShared

LOCAL_C_INCLUDES := $(DIR_VideoTelemetryShared)/Helper
LOCAL_C_INCLUDES += $(DIR_VideoTelemetryShared)/InputOutput
LOCAL_C_INCLUDES += $(LOCAL_PATH)/Encoder

# If we remove dependency of libusb here we can build both libusb and libuvc as static libraries uvc usb1.0
# Then include libuvc as a static library here
LOCAL_STATIC_LIBRARIES +=libuvc_static
# jpeg-turbo can be static, too since it is only needed for the UVCReceiverDecoder (not for libuvc anymore)
LOCAL_STATIC_LIBRARIES +=libjpeg-turbo

# leave this one. By default, the build system generates ARM target binaries in thumb mode, where each instruction is 16 bits wide
LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
		UVCReceiverDecoder.cpp \
		Encoder/SimpleTranscoder.cpp \
		ColorFormatTester.cpp \
		#$(DIR_VideoTelemetryShared)/Helper/ZDummy.cpp \
		#$(DIR_VideoTelemetryShared)/InputOutput/ZDummy.cpp \

LOCAL_MODULE    := UVCReceiverDecoder
# Here we need a shared library since it has to be bundled with the .apk (here are the native bindings)
include $(BUILD_SHARED_LIBRARY)
