//
// Created by geier on 25/05/2020.
//

#ifndef FPV_VR_OS_ANDROIDCOLORFORMATS_HPP
#define FPV_VR_OS_ANDROIDCOLORFORMATS_HPP

#include <HALPixelFormats.hpp>

//
// includes Android color format constants ( e.g. int values )
// In java, these constants are declared in multiple locations but often mean the same

// Values taken from
// https://developer.android.com/reference/android/media/MediaCodecInfo.CodecCapabilities
namespace MediaCodecInfo{
    namespace CodecCapabilities{
        static constexpr int COLOR_Format24bitRGB888=12;
        //
        static constexpr int COLOR_FormatYUV420Flexible=2135033992;
        static constexpr int COLOR_FormatYUV420Planar=19;
        static constexpr int COLOR_FormatYUV420PackedPlanar=20;
        static constexpr int COLOR_FormatYUV420SemiPlanar=21;
        static constexpr int COLOR_FormatYUV420PackedSemiPlanar=39;
        static constexpr int COLOR_TI_FormatYUV420PackedSemiPlanar=2130706688;
        //
        static constexpr int COLOR_FormatYUV422Flexible= 2135042184;
        static constexpr int COLOR_FormatYUV422Planar=22;
        static constexpr int COLOR_FormatYUV422PackedPlanar=23;
        static constexpr int COLOR_FormatYUV422SemiPlanar=24;
        static constexpr int COLOR_FormatYUV422PackedSemiPlanar=40;
        //
        static constexpr int COLOR_FormatYUV444Flexible=2135181448;
    }
    static bool isSemiPlanarYUV(const int colorFormat) {
        using namespace CodecCapabilities;
        switch (colorFormat) {
            case COLOR_FormatYUV420Planar:
            case COLOR_FormatYUV420PackedPlanar:
                return false;
            case COLOR_FormatYUV420SemiPlanar:
            case COLOR_FormatYUV420PackedSemiPlanar:
            case COLOR_TI_FormatYUV420PackedSemiPlanar:
                return true;
            default:
                MLOGE<<"unknown format "<< colorFormat;
                return true;
        }
    }
};
// https://chromium.googlesource.com/libyuv/libyuv/+/HEAD/docs/formats.md
//  I420, NV12 and NV21 are half width, half height
//  I422, NV16 and NV61 are half width, full height
//  I444, NV24 and NV42 are full width, full height
namespace ImageFormat{
    //https://developer.android.com/reference/android/graphics/ImageFormat#YUY2
    static constexpr int YUY2=20;
    //https://developer.android.com/reference/android/graphics/ImageFormat#YV12
    static constexpr int YV12=842094169;
    static constexpr int NV21=21;
    // NV16 seems to work ?
    static constexpr int NV16=16;
    // https://developer.android.com/reference/android/graphics/ImageFormat#YUV_420_888
    // I think this one should work on all devices (either planar or semiplanar)
    static constexpr int YUV_420_888=35;
    // does not work for ANativeWindow
    //Surface: dequeueBuffer failed (Out of memory)
    // https://developer.android.com/reference/android/graphics/ImageFormat#YUV_422_888
    static constexpr int YUV_422_888=39;
    // https://developer.android.com/reference/android/graphics/ImageFormat#YUV_444_888
    static constexpr int YUV_444_888=40;
}
static_assert(ImageFormat::YUV_420_888 == AHARDWAREBUFFER_FORMAT_Y8Cb8Cr8_420);


#endif //FPV_VR_OS_ANDROIDCOLORFORMATS_HPP
