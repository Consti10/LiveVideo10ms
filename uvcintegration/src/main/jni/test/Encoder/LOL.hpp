//
// Created by geier on 29/05/2020.
//

#ifndef FPV_VR_OS_LOL_HPP
#define FPV_VR_OS_LOL_HPP

// https://android.googlesource.com/platform/hardware/libhardware/+/28147965b4f3b57893897924bdd2fc8fdc695f84/include/hardware/hardware.h
namespace TodoRename{
    /**
 * pixel format definitions
 */
    enum {
        HAL_PIXEL_FORMAT_RGBA_8888          = 1,
        HAL_PIXEL_FORMAT_RGBX_8888          = 2,
        HAL_PIXEL_FORMAT_RGB_888            = 3,
        HAL_PIXEL_FORMAT_RGB_565            = 4,
        HAL_PIXEL_FORMAT_BGRA_8888          = 5,
        HAL_PIXEL_FORMAT_RGBA_5551          = 6,
        HAL_PIXEL_FORMAT_RGBA_4444          = 7,
        /* 0x8 - 0xF range unavailable */
        HAL_PIXEL_FORMAT_YCbCr_422_SP       = 0x10,     // NV16
        HAL_PIXEL_FORMAT_YCrCb_420_SP       = 0x11,     // NV21
        HAL_PIXEL_FORMAT_YCbCr_422_P        = 0x12,     // IYUV
        HAL_PIXEL_FORMAT_YCbCr_420_P        = 0x13,     // YUV9
        HAL_PIXEL_FORMAT_YCbCr_422_I        = 0x14,     // YUY2
        /* 0x15 reserved */
        HAL_PIXEL_FORMAT_CbYCrY_422_I       = 0x16,     // UYVY
        /* 0x17 reserved */
        /* 0x18 - 0x1F range unavailable */
        HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED = 0x20,     // NV12_adreno_tiled
        HAL_PIXEL_FORMAT_YCbCr_420_SP       = 0x21,     // NV12
        HAL_PIXEL_FORMAT_YCrCb_422_SP       = 0x22,     // NV61
    };
/* fourcc mapping for the YUV formats. see http://www.fourcc.org */
    enum {
        HAL_PIXEL_FORMAT_NV16               = HAL_PIXEL_FORMAT_YCbCr_422_SP,
        HAL_PIXEL_FORMAT_NV21               = HAL_PIXEL_FORMAT_YCrCb_420_SP,
        HAL_PIXEL_FORMAT_IYUV               = HAL_PIXEL_FORMAT_YCbCr_422_P,
        HAL_PIXEL_FORMAT_YUV9               = HAL_PIXEL_FORMAT_YCbCr_420_P,
        HAL_PIXEL_FORMAT_YUY2               = HAL_PIXEL_FORMAT_YCbCr_422_I,
        HAL_PIXEL_FORMAT_UYVY               = HAL_PIXEL_FORMAT_CbYCrY_422_I,
        HAL_PIXEL_FORMAT_NV12_ADRENO_TILED  = HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED,
        HAL_PIXEL_FORMAT_NV12               = HAL_PIXEL_FORMAT_YCbCr_420_SP,
        HAL_PIXEL_FORMAT_NV61               = HAL_PIXEL_FORMAT_YCrCb_422_SP
    };
}



#endif //FPV_VR_OS_LOL_HPP
