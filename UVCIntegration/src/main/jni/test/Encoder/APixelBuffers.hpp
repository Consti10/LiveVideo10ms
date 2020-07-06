//
// Created by geier on 27/05/2020.
//

#ifndef FPV_VR_OS_MYCOLORSPACES_HPP
#define FPV_VR_OS_MYCOLORSPACES_HPP

#include <cstdint>
#include <array>
#include <optional>

/***
 * Helps with the most common data layout for android HW btw. pixel buffers. Use with buffers obtained by MediaCodec, Surface ( ANativeWindow ) usw
 * WARNING: The data layout is always [HEIGHT][WIDTH] for these color formats, but since I am used to
 * [WIDTH][HEIGHT] the data is indexed this way.
 * INFO: Every class also has a constructors that takes a void* - when created this way the instance does not own the buffer data
 */
namespace APixelBuffers{
    // RGB 888 ,no special packing
    // By default, STRIDE==WIDTH
    class RGB{
    public:
        RGB(void* data1,const size_t W,const size_t H,const size_t STRIDE1):data(data1),WIDTH(W),HEIGHT(H),STRIDE(STRIDE1){}
        RGB(const size_t W,const size_t H,const ssize_t STRIDE1=-1):ownedData(WIDTH*HEIGHT*3),data(ownedData->data()),WIDTH(W),HEIGHT(H),STRIDE(STRIDE1<0 ? WIDTH : STRIDE1){};
        const size_t WIDTH,HEIGHT;
        const size_t STRIDE;
        std::optional<std::vector<uint8_t>> ownedData={};
        void* data;
        uint8_t& R(size_t w,size_t h){
            return (*static_cast<uint8_t(*)[HEIGHT][STRIDE][3]>(data))[h][w][0];
        }
        uint8_t& G(size_t w,size_t h){
            return (*static_cast<uint8_t(*)[HEIGHT][STRIDE][3]>(data))[h][w][1];
        }
        uint8_t& B(size_t w,size_t h){
            return (*static_cast<uint8_t(*)[HEIGHT][STRIDE][3]>(data))[h][w][2];
        }
        void drawPattern(const int frameIndex){
            constexpr uint8_t red[3]={255,0,0};
            constexpr uint8_t green[3]={0,255,0};
            constexpr uint8_t blue[3]={0,0,255};
            const auto color= (frameIndex / 60) % 2 == 0 ? red : green;
            for(size_t w=0;w<WIDTH;w++){
                for(size_t h=0;h<HEIGHT;h++){
                    const auto color2=(h/8)%2==0 ? color : blue;
                    R(w,h)=color2[0];
                    G(w,h)=color2[1];
                    B(w,h)=color2[2];
                }
            }
        }
    };
    // YUV 420 either Planar or SemiPlanar (U,V packed together in pairs of 2)
    // see https://developer.android.com/reference/android/graphics/ImageFormat.html#YUV_420_888
    template<bool PLANAR>
    class YUV420{
    public:
        static constexpr const size_t BITS_PER_PIXEL=12;
        YUV420(void* data1,const size_t W,const size_t H):data(data1),WIDTH(W),HEIGHT(H){}
        const size_t WIDTH,HEIGHT;
        void* data;
        const size_t LUMA_SIZE_B=WIDTH*HEIGHT;
        const size_t HALF_WIDTH=WIDTH/2;
        const size_t HALF_HEIGHT=HEIGHT/2;
        const size_t SIZE_BYTES=WIDTH*HEIGHT*BITS_PER_PIXEL/8;
        // No matter if planar or semiplanar, the chroma data always starts at this offset
        void* chromaData=&static_cast<uint8_t*>(data)[LUMA_SIZE_B];
        // All these functions return a reference to the Y (U,V) value at position (w,h)
        // Since the Y plane has full resolution w can be in the range [0,WIDTH[ and h in [0,HEIGHT[
        // but the U,V plane (both in PLANAR and PACKED mode) is only in the range of [0,HALF_WIDTH[ / [0,HALF_HEIGHT[
        uint8_t& Y(size_t w,size_t h){
            return (*static_cast<uint8_t(*)[HEIGHT][WIDTH]>(data))[h][w];
        }
        // half width (h2) and half height (h2)
        // Planar means [2][..][..] and Semi-planar means [..][..][2]
        uint8_t& U(size_t w2, size_t h2){
            if constexpr (PLANAR){
                auto& tmp=(*static_cast<uint8_t(*)[2][HALF_HEIGHT][HALF_WIDTH]>(chromaData));
                return tmp[0][h2][w2];
            }
            auto& tmp=*static_cast<uint8_t(*)[HALF_HEIGHT][HALF_WIDTH][2]>(chromaData);
            return tmp[h2][w2][0];
        }
        uint8_t& V(size_t w2, size_t h2){
            if constexpr (PLANAR){
                auto& tmp=*static_cast<uint8_t(*)[2][HALF_HEIGHT][HALF_WIDTH]>(chromaData);
                return tmp[1][h2][w2];
            }
            auto& tmp=*static_cast<uint8_t(*)[HALF_HEIGHT][HALF_WIDTH][2]>(chromaData);
            return tmp[h2][w2][1];
        }
        void clear(uint8_t y,uint8_t u,uint8_t v){
            for(size_t w=0;w<WIDTH;w++){
                for(size_t h=0;h<HEIGHT;h++){
                    Y(w,h)=y;
                }
            }
            for(size_t halfW=0; halfW < HALF_WIDTH; halfW++){
                for(size_t halfH=0; halfH < HALF_HEIGHT; halfH++){
                    U(halfW, halfH)=u;
                    V(halfW, halfH)=v;
                }
            }
        }
    };
    using YUV420Planar = YUV420<true>;
    using YUV420SemiPlanar = YUV420<false>;

    // Y plane has full width & height
    // U and V plane both have half width and full height
    template<bool PLANAR>
    class YUV422{
    public:
        static constexpr const size_t BITS_PER_PIXEL=16;
        YUV422(void* data1,const size_t W,const size_t H):data(data1),WIDTH(W),HEIGHT(H){}
        YUV422(const size_t W,const size_t H):ownedData(W*H*16/8),data(ownedData->data()),WIDTH(W),HEIGHT(H){}
        const size_t WIDTH,HEIGHT;
        std::optional<std::vector<uint8_t>> ownedData={};
        void* data;
        const size_t LUMA_SIZE_B=WIDTH*HEIGHT;
        const size_t HALF_WIDTH=WIDTH/2;
        // we do not need Half height ! const size_t HALF_HEIGHT=HEIGHT/2;
        void* chromaData=&static_cast<uint8_t*>(data)[LUMA_SIZE_B];
        uint8_t& Y(size_t w,size_t h){
            auto& tmp=*static_cast<uint8_t(*)[HEIGHT][WIDTH]>(data);
            return tmp[h][w];
        }
        uint8_t& U(size_t w,size_t h2){
            if constexpr (PLANAR){
                auto& tmp=(*static_cast<uint8_t(*)[2][HEIGHT][HALF_WIDTH]>(chromaData));
                return tmp[0][h2][w];
            }
            auto& tmp=*static_cast<uint8_t(*)[HEIGHT][HALF_WIDTH][2]>(chromaData);
            return tmp[h2][w][0];
        }
        uint8_t& V(size_t w,size_t h2){
            if constexpr (PLANAR){
                auto& tmp=*static_cast<uint8_t(*)[2][HEIGHT][HALF_WIDTH]>(chromaData);
                return tmp[1][h2][w];
            }
            auto& tmp=*static_cast<uint8_t(*)[HEIGHT][HALF_WIDTH][2]>(chromaData);
            return tmp[h2][w][1];
        }
        void clear(uint8_t y,uint8_t u,uint8_t v){
            for(size_t w=0;w<WIDTH;w++){
                for(size_t h=0;h<HEIGHT;h++){
                    Y(w,h)=y;
                }
            }
            for(size_t w=0;w<HALF_WIDTH;w++){
                for(size_t h=0;h<HEIGHT;h++){
                    U(w,h)=u;
                    V(w,h)=v;
                }
            }
        }
    };
    using YUV422Planar = YUV422<true>;
    using YUV422SemiPlanar = YUV422<false>;
    //
    static void copyTo(YUV422Planar& in,YUV420SemiPlanar& out){
        assert(in.WIDTH==out.WIDTH && in.HEIGHT==out.HEIGHT);
        const size_t WIDTH=in.WIDTH;
        const size_t HEIGHT=in.HEIGHT;
        // copy Y component (easy)
        memcpy(out.data,in.data,WIDTH*HEIGHT);
        // copy CbCr component ( loop needed)
        for(int i=0;i<WIDTH/2;i++){
            for(int j=0;j<HEIGHT/2;j++){
                out.U(i,j)=in.U(i,j*2);
                out.V(i,j)=in.V(i,j*2);
            }
        }
    }
    // from https://stackoverflow.com/questions/1737726/how-to-perform-rgb-yuv-conversion-in-c-c
    // RGB -> YUV
    #define CLIP(X) ( (X) > 255 ? 255 : (X) < 0 ? 0 : X)
    #define RGB2Y(R, G, B) CLIP(( (  66 * (R) + 129 * (G) +  25 * (B) + 128) >> 8) +  16)
    #define RGB2U(R, G, B) CLIP(( ( -38 * (R) -  74 * (G) + 112 * (B) + 128) >> 8) + 128)
    #define RGB2V(R, G, B) CLIP(( ( 112 * (R) -  94 * (G) -  18 * (B) + 128) >> 8) + 128)
    static std::array<uint8_t,3> convertToYUV(const std::array<uint8_t,3> rgb){
        uint8_t Y=RGB2Y(rgb[0],rgb[1],rgb[2]);
        uint8_t U=RGB2U(rgb[0],rgb[1],rgb[2]);
        uint8_t V=RGB2V(rgb[0],rgb[1],rgb[2]);
        return {Y,U,V};
    }
    static void copyTo(RGB& in,YUV420SemiPlanar& out){
        assert(in.WIDTH==out.WIDTH && in.HEIGHT==out.HEIGHT);
        const size_t WIDTH=in.WIDTH;
        const size_t HEIGHT=in.HEIGHT;
        for(size_t w=0;w<WIDTH;w++){
            for(size_t h=0;h<HEIGHT;h++){
                const std::array<uint8_t,3> rgb={in.R(w,h),in.G(w,h),in.B(w,h)};
                const auto yuv=convertToYUV(rgb);
                out.Y(w,h)=yuv[0];
            }
        }
        for(size_t w=0;w<WIDTH;w++){
            for(size_t h=0;h<HEIGHT/2;h++){
                const std::array<uint8_t,3> rgb={in.R(w,h),in.G(w,h),in.B(w,h)};
                const auto yuv=convertToYUV(rgb);
                out.U(w,h)=yuv[1];
                out.V(w,h)=yuv[2];
            }
        }
    }
}

// taken from https://android.googlesource.com/platform/cts/+/3661c33/tests/tests/media/src/android/media/cts/EncodeDecodeTest.java
// and translated to cpp
namespace YUVFrameGenerator{
    using namespace APixelBuffers;
    static constexpr uint8_t PURPLE_YUV[]={120, 160, 200};

    // creates a purple rectangle with w=width/4 and h=height/2 that moves 1 square forward with frameIndex/8
    template<bool PLANAR>
    static void generateFrame(YUV420<PLANAR>& framebuffer,int frameIndex){
        assert(framebuffer.WIDTH % 4==0 && framebuffer.HEIGHT % 2 ==0);
        const size_t WIDTH=framebuffer.WIDTH;
        const size_t HEIGHT=framebuffer.HEIGHT;
        const size_t FRAME_BUFFER_SIZE_B= WIDTH * HEIGHT * framebuffer.BITS_PER_PIXEL / 8;

        // Set to zero.  In YUV this is a dull green.
        std::memset(framebuffer.data, 0, FRAME_BUFFER_SIZE_B);

        const size_t COLORED_RECT_W= WIDTH / 4;
        const size_t COLORED_RECT_H= HEIGHT / 2;

        frameIndex = (frameIndex / 8) % 8;    // use this instead for debug -- easier to see
        size_t startX;
        const int startY=frameIndex<4 ? 0 : HEIGHT / 2;
        if (frameIndex < 4) {
            startX = frameIndex * COLORED_RECT_W;
        } else {
            startX = frameIndex % 4 * COLORED_RECT_W;
        }
        // fill the wanted area with purple color
        for (size_t x = startX; x < startX + COLORED_RECT_W; x++) {
            for (size_t y = startY; y < startY + COLORED_RECT_H; y++) {
                framebuffer.Y(x, y) = PURPLE_YUV[0];
                const bool even = (x % 2) == 0 && (y % 2) == 0;
                if (even) {
                    framebuffer.U(x / 2, y / 2) = PURPLE_YUV[1];
                    framebuffer.V(x / 2, y / 2) = PURPLE_YUV[2];
                }
            }
        }
    }
}


#endif //FPV_VR_OS_MYCOLORSPACES_HPP
