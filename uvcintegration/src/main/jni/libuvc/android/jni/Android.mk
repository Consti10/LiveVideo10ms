#/*********************************************************************
#* Software License Agreement (BSD License)
#*
#* Copyright (C)2014-2017 saki@serenegiant <t_saki@serenegiant.com>
#*  All rights reserved.
#*
#*  Redistribution and use in source and binary forms, with or without
#*  modification, are permitted provided that the following conditions
#*  are met:
#*
#*   * Redistributions of source code must retain the above copyright
#*     notice, this list of conditions and the following disclaimer.
#*   * Redistributions in binary form must reproduce the above
#*     copyright notice, this list of conditions and the following
#*     disclaimer in the documentation and/or other materials provided
#*     with the distribution.
#*   * Neither the name of the author nor other contributors may be
#*     used to endorse or promote products derived from this software
#*     without specific prior written permission.
#*
#*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
#*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
#*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
#*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
#*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
#*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
#*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
#*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
#*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#*  POSSIBILITY OF SUCH DAMAGE.
#*********************************************************************/

######################################################################
# libuvc_static.a (static library with static link to libusb1.0), no link to libjpeg
######################################################################
LOCAL_PATH	:= $(call my-dir)/../..
include $(CLEAR_VARS)

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/.. \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/include/libuvc

LOCAL_EXPORT_C_INCLUDES := \
	$(LOCAL_PATH)/ \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/include/libuvc

LOCAL_CFLAGS := $(LOCAL_C_INCLUDES:%=-I%)
LOCAL_CFLAGS += -DANDROID_NDK
LOCAL_CFLAGS += -DLOG_NDEBUG
LOCAL_CFLAGS += -DUVC_DEBUGGING

LOCAL_EXPORT_LDLIBS := -llog

LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES += libusb1.0_static

LOCAL_SRC_FILES := \
	src/ctrl.c \
	src/device.c \
	src/diag.c \
	src/frame.c \
	src/frame-mjpeg.c \
	src/init.c \
	src/stream.c

LOCAL_MODULE := libuvc_static
include $(BUILD_STATIC_LIBRARY)


