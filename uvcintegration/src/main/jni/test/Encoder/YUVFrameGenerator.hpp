//
// Created by geier on 25/05/2020.
//

#ifndef FPV_VR_OS_YUVFRAMEGENERATOR_HPP
#define FPV_VR_OS_YUVFRAMEGENERATOR_HPP

#include <MyColorSpaces.hpp>
#include <LOL.hpp>

// Values taken from
// https://developer.android.com/reference/android/media/MediaCodecInfo.CodecCapabilities
namespace MediaCodecInfo{
    namespace CodecCapabilities{
        static constexpr int COLOR_Format24bitRGB888=12;

        static constexpr int COLOR_FormatYUV420Flexible=2135033992;
        static constexpr int COLOR_FormatYUV420Planar=19;
        static constexpr int COLOR_FormatYUV420PackedPlanar=20;
        static constexpr int COLOR_FormatYUV420SemiPlanar=21;
        static constexpr int COLOR_FormatYUV420PackedSemiPlanar=39;
        static constexpr int COLOR_TI_FormatYUV420PackedSemiPlanar=2130706688;

        static constexpr int COLOR_FormatYUV422Flexible= 2135042184;
        static constexpr int COLOR_FormatYUV422Planar=22;
        static constexpr int COLOR_FormatYUV422PackedPlanar=23;
        static constexpr int COLOR_FormatYUV422SemiPlanar=24;
        static constexpr int COLOR_FormatYUV422PackedSemiPlanar=40;

        static constexpr int COLOR_FormatYUV444Flexible=2135181448;
    }
};
// https://chromium.googlesource.com/libyuv/libyuv/+/HEAD/docs/formats.md
//  I420, NV12 and NV21 are half width, half height
//  I422, NV16 and NV61 are half width, full height
//  I444, NV24 and NV42 are full width, full height
namespace ImageFormatXXX{
    // NV16 seems to work ?
    static constexpr int NV16=16;
    static constexpr int NV21=21;
    //static constexpr int NV24=
    static constexpr int YUV_420_888=35;
    // does not work for ANativeWindow
    //Surface: dequeueBuffer failed (Out of memory)
    static constexpr int YUV_422_888=39;
}

// taken from https://android.googlesource.com/platform/cts/+/3661c33/tests/tests/media/src/android/media/cts/EncodeDecodeTest.java
// and translated to cpp
namespace YUVFrameGenerator{
    static bool isSemiPlanarYUV(const int colorFormat) {
        using namespace MediaCodecInfo::CodecCapabilities;
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
    // YUV values for purple
    static constexpr uint8_t PURPLE_YUV[]={120, 160, 200};

    // creates a purple rectangle with w=width/4 and h=height/2 that moves 1 square forward with frameIndex/8
    static void generateFrame(int frameIndex, int colorFormat, uint8_t* frameData,size_t frameDataSize) {
        using namespace MyColorSpaces;
        // Full width/height for luma ( Y )
        constexpr size_t WIDTH=640;
        constexpr size_t HEIGHT=480;
        constexpr size_t FRAME_BUFFER_SIZE_B= WIDTH * HEIGHT * 12 / 8;
        if(frameDataSize < FRAME_BUFFER_SIZE_B){
            MLOGE<<"Frame buffer size not suefficcient";
            return;
        }
        // Half width / height for chroma (U,V btw Cb,Cr)
        constexpr size_t HALF_WIDTH= WIDTH / 2;
        constexpr size_t HALF_HEIGHT= HEIGHT / 2;

        auto framebuffer=YUV420SemiPlanar(frameData,WIDTH,HEIGHT);

        boolean semiPlanar = isSemiPlanarYUV(colorFormat);
        // Set to zero.  In YUV this is a dull green.
        std::memset(frameData, 0, FRAME_BUFFER_SIZE_B);

        constexpr int COLORED_RECT_W= WIDTH / 4;
        constexpr int COLORED_RECT_H= HEIGHT / 2;

        //frameIndex %= 8;
        frameIndex = (frameIndex / 8) % 8;    // use this instead for debug -- easier to see
        int startX;
        const int startY=frameIndex<4 ? 0 : HEIGHT / 2;
        if (frameIndex < 4) {
            startX = frameIndex * COLORED_RECT_W;
        } else {
            startX = frameIndex % 4 * COLORED_RECT_W;
        }
        // fill the wanted area with purple color
        for (int x = startX; x < startX + COLORED_RECT_W; x++) {
            for (int y = startY; y < startY + COLORED_RECT_H; y++) {
                if (semiPlanar) {
                    // full-size Y, followed by UV pairs at half resolution
                    // e.g. Nexus 4 OMX.qcom.video.encoder.avc COLOR_FormatYUV420SemiPlanar
                    // e.g. Galaxy Nexus OMX.TI.DUCATI1.VIDEO.H264E
                    //        OMX_TI_COLOR_FormatYUV420PackedSemiPlanar
                    //frameData[y * WIDTH + x] = (uint8_t) TEST_Y;
                    framebuffer.Y(x,y)= PURPLE_YUV[0];
                    const bool even=(x % 2) == 0 && (y % 2) == 0;
                    if (even) {
                        framebuffer.U(x/2,y/2)= PURPLE_YUV[1];
                        framebuffer.V(x/2,y/2)=PURPLE_YUV[2];
                        //frameData[WIDTH * HEIGHT + y * HALF_WIDTH + x] = TEST_U;
                        //frameData[WIDTH * HEIGHT + y * HALF_WIDTH + x + 1] = TEST_V;
                    }
                } else {
                    // full-size Y, followed by quarter-size U and quarter-size V
                    // e.g. Nexus 10 OMX.Exynos.AVC.Encoder COLOR_FormatYUV420Planar
                    // e.g. Nexus 7 OMX.Nvidia.h264.encoder COLOR_FormatYUV420Planar
                    // NOT TESTED !
                    frameData[y * WIDTH + x] = PURPLE_YUV[0];
                    if ((x & 0x01) == 0 && (y & 0x01) == 0) {
                        frameData[WIDTH * HEIGHT + (y / 2) * HALF_WIDTH + (x / 2)] = PURPLE_YUV[1];
                        frameData[WIDTH * HEIGHT + HALF_WIDTH * (HEIGHT / 2) +
                                  (y / 2) * HALF_WIDTH + (x / 2)] = PURPLE_YUV[2];
                    }
                }
            }
        }
    }

    // For some reason HEIGHT comes before WIDTH here ?!
    // The Y plane has full resolution.
    //auto& YPlane = *static_cast<uint8_t (*)[HEIGHT][WIDTH]>(static_cast<void*>(frameData));
    // The CbCrPlane only has half resolution in both x and y direction ( 4:2:0 )
    // CbCrPlane[y][x][0] == Cb (U) value for pixel x,y and
    // CbCrPlane[y][x][1] == Cr (V) value for pixel x,y
    //auto& CbCrPlane = *static_cast<uint8_t(*)[HALF_HEIGHT][HALF_WIDTH][2]>(static_cast<void*>(&frameData[WIDTH * HEIGHT]));
    // Check - YUV420 has 12 bit per pixel (1.5 bytes)
    //static_assert(sizeof(YPlane)+sizeof(CbCrPlane) == FRAME_BUFFER_SIZE_B);
}
#endif //FPV_VR_OS_YUVFRAMEGENERATOR_HPP
