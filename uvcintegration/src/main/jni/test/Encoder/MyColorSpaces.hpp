//
// Created by geier on 27/05/2020.
//

#ifndef FPV_VR_OS_MYCOLORSPACES_HPP
#define FPV_VR_OS_MYCOLORSPACES_HPP

#include <cstdint>
#include <array>
#include <array>
#include <optional>

// Watch out: For the data layout height comes before width !!
// I like to use it the other way around - image data = [width][height]
namespace MyColorSpaces{

    // RGB 888 ,no special packing
    template<size_t WIDTH,size_t HEIGHT> //,size_t STRIDE
    class RGB{
    public:
        RGB(void* data1):data((uint8_t*)data1){}
        RGB():ownedData(WIDTH*HEIGHT*3),data(ownedData->data()){};
        std::optional<std::vector<uint8_t>> ownedData={};
        uint8_t* data;
        //using LOL=uint8_t(*)[HEIGHT][WIDTH][3];
        uint8_t& R(size_t w,size_t h){
            return (*static_cast<uint8_t(*)[HEIGHT][WIDTH][3]>(static_cast<void*>(data)))[h][w][0];
        }
        uint8_t& G(size_t w,size_t h){
            return (*static_cast<uint8_t(*)[HEIGHT][WIDTH][3]>(static_cast<void*>(data)))[h][w][1];
        }
        uint8_t& B(size_t w,size_t h){
            return (*static_cast<uint8_t(*)[HEIGHT][WIDTH][3]>(static_cast<void*>(data)))[h][w][2];
        }
        void clear(const int frameIndex){
            constexpr uint8_t red[3]={255,0,0};
            constexpr uint8_t green[3]={0,255,0};
            constexpr uint8_t blue[3]={0,255,0};
            const auto color= (frameIndex / 60) % 2 == 0 ? red : green;
            for(size_t w=0;w<WIDTH;w++){
                for(size_t h=0;h<HEIGHT;h++){
                    R(w,h)=color[0];
                    G(w,h)=color[1];
                    B(w,h)=color[2];
                }
            }
        }
    };

    // YUV 420 either Planar or SemiPlanar (U,V packed together in pairs of 2)
    // see https://developer.android.com/reference/android/graphics/ImageFormat.html#YUV_420_888
    template<size_t WIDTH,size_t HEIGHT,bool PLANAR>
    class YUV420{
    public:
        YUV420(void* data1):data((uint8_t*)data1){}
        uint8_t* data;
        const size_t LUMA_SIZE_B=WIDTH*HEIGHT;
        const size_t HALF_WIDTH=WIDTH/2;
        const size_t HALF_HEIGHT=HEIGHT/2;
        // All these functions return a reference to the Y (U,V) value at position (w,h)
        // Since the Y plane has full resolution w can be in the range [0,WIDTH[ and h in [0,HEIGHT[
        // but the U,V plane (both in PLANAR and PACKED mode) is only in the range of [0,HALF_WIDTH[ / [0,HALF_HEIGHT[
        uint8_t& Y(size_t w,size_t h){
            auto& tmp=*static_cast<uint8_t(*)[HEIGHT][WIDTH]>(static_cast<void*>(data));
            return tmp[h][w];
        }
        uint8_t& U(size_t w,size_t h){
            if constexpr (PLANAR){
                auto& tmp=(*static_cast<uint8_t(*)[HALF_HEIGHT][HALF_WIDTH]>(static_cast<void*>(&data[LUMA_SIZE_B])));
                return (*static_cast<uint8_t(*)[HALF_HEIGHT][HALF_WIDTH]>(static_cast<void*>(&data[LUMA_SIZE_B])))[h][w];
            }
            auto& tmp=*static_cast<uint8_t(*)[HALF_HEIGHT][HALF_WIDTH][2]>(static_cast<void*>(&data[LUMA_SIZE_B]));
            return tmp[h][w][0];
        }
        uint8_t& V(size_t w,size_t h){
            if constexpr (PLANAR){
                auto& tmp=*static_cast<uint8_t(*)[HALF_HEIGHT][HALF_WIDTH]>(static_cast<void*>(&data[LUMA_SIZE_B+HALF_WIDTH*HALF_HEIGHT]));
                return tmp[h][w];
            }
            auto& tmp=*static_cast<uint8_t(*)[HALF_HEIGHT][HALF_WIDTH][2]>(static_cast<void*>(&data[LUMA_SIZE_B]));
            return tmp[h][w][1];
        }
        void clear(uint8_t y,uint8_t u,uint8_t v){
            for(size_t w=0;w<WIDTH;w++){
                for(size_t h=0;h<HEIGHT;h++){
                    Y(w,h)=y;
                }
            }
            for(size_t w=0;w<HALF_WIDTH;w++){
                for(size_t h=0;h<HALF_HEIGHT;h++){
                    U(w,h)=u;
                    V(w,h)=v;
                }
            }
        }
    };
    template<size_t WIDTH,size_t HEIGHT>
    using YUV420Planar = YUV420<WIDTH,HEIGHT,true>;
    template<size_t WIDTH,size_t HEIGHT>
    using YUV420SemiPlanar = YUV420<WIDTH,HEIGHT,false>;



    // Y plane has full width & height
    // U and V plane both have half width and full height
    template<size_t WIDTH,size_t HEIGHT>
    class YUV422Planar{
    public:
        uint8_t planeY[HEIGHT][WIDTH];
        uint8_t planeU[HEIGHT][WIDTH/2];
        uint8_t planeV[HEIGHT][WIDTH/2];
        std::array<uint8_t,2> getUVHalf(size_t xHalf,size_t yHalf)const{
            // linear interpolation between y0,y1 ..
            const auto U=planeU[yHalf*2][xHalf];
            const auto U2=planeU[yHalf*2+1][xHalf];
            const int diff=std::abs((int)U2-(int)U);
            if(diff > 2){
                //MLOGD<<"U1 "<<(int)U<<" U2 "<<(int)U2;
            }
            const auto V=planeV[yHalf*2][xHalf];
            return {U,V};
        }
    }__attribute__((packed));
    //
    template<size_t WIDTH,size_t HEIGHT>
    class YUV422SemiPlanar{
    public:
        uint8_t planeY[HEIGHT][WIDTH];
        uint8_t planeUV[HEIGHT][WIDTH/2][2];
    }__attribute__((packed));
    //
    static_assert(sizeof(YUV422Planar<640,480>)==640*480*16/8);
    //static_assert(sizeof(YUV420SemiPlanar<640,480>)==640*480*12/8);
    //static_assert(sizeof(YUV420SemiPlanar<640,480>)*16/12==sizeof(YUV422Planar<640,480>));
    static_assert(sizeof(YUV422SemiPlanar<640,480>)==sizeof(YUV422Planar<640,480>));

    //
    static void copyTo(const YUV422Planar<640,480>& in,YUV422SemiPlanar<640,480>& out){
        // copy Y component (easy)
        memcpy(out.planeY,in.planeY, sizeof(out.planeY));
        // copy CbCr component ( loop needed)
        // copy CbCr component ( loop needed)
        for(int i=0;i<640/2;i++){
            for(int j=0;j<480;j++){
                out.planeUV[j][i][0]=in.planeU[j][i];
                out.planeUV[j][i][1]=in.planeV[j][i];
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

    template<size_t WIDTH,size_t HEIGHT>
    static void copyTo(RGB<WIDTH,HEIGHT>& in,YUV420SemiPlanar<WIDTH,HEIGHT>& out){
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

    /*template<size_t WIDTH,size_t HEIGHT>
    static void copyTo(const RGBA<WIDTH,HEIGHT>& in,YUV420SemiPlanar<WIDTH,HEIGHT,true>& out){
        for(size_t w=0;w<WIDTH;w++){
            for(size_t h=0;h<HEIGHT;h++){
                //const uint8_t* rgb=in.planeRGB[h[w];
                //const auto YUV=convertToYUV(rgb);
                //out.planeY[h][w]=YUV[0];
                //out.planeUV[h][w/2][0]=YUV[1];
                //out.planeUV[h][w/2][1]=YUV[2];
                out.planeY[h][w]=120;
            }
        }
        for(size_t w=0;w<WIDTH/2;w++){
            for(size_t h=0;h<HEIGHT/2;h++){
                out.planeUV[h][w][0]=160;
                out.planeUV[h][w][1]=200;
            }
        }
    }*/


}


#endif //FPV_VR_OS_MYCOLORSPACES_HPP
