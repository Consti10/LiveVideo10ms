//
// Created by Consti10 on 14/05/2019.
//

#ifndef FPV_VR_PRIVATE_MDEBUG_H
#define FPV_VR_PRIVATE_MDEBUG_H

#include "android/log.h"
#include <string.h>

//#define TAG_MDEBUG "MDebug"
//#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG_MDEBUG, __VA_ARGS__)

namespace LOG{
    // taken from https://android.googlesource.com/platform/system/core/+/android-2.1_r1/liblog/logd_write.c
    static constexpr const auto ANDROID_LOG_BUFF_SIZE=1024;
    static constexpr auto DEFAULT_TAG="NoTag";
    static void D(const char* fmt,...) {
        va_list argptr;
        va_start(argptr, fmt);
        __android_log_vprint(ANDROID_LOG_DEBUG,DEFAULT_TAG,fmt,argptr);
        va_end(argptr);
    }
    static void E(const char* fmt,...) {
        va_list argptr;
        va_start(argptr, fmt);
        __android_log_vprint(ANDROID_LOG_ERROR,DEFAULT_TAG,fmt,argptr);
        va_end(argptr);
    }
    static void LOL(const char* fmt,...) {
        const auto ANDROID_LOG_BUFF_SIZE=1024;
        char buffer[ANDROID_LOG_BUFF_SIZE];
        va_list argptr;
        va_start(argptr, fmt);
        vsprintf (buffer,fmt, argptr);
        vsnprintf(buffer,ANDROID_LOG_BUFF_SIZE, fmt, argptr);
        va_end(argptr);
        __android_log_print(ANDROID_LOG_DEBUG,"HA","%s",buffer);
    }
}

class MDebug{
public:
    //Splits debug messages that exceed the android log maximum length into smaller log(s)
    static void log(const std::string& message,const std::string& tag="NoTag"){
        constexpr int MAX_L=1024;
        if(message.length()>MAX_L){
            __android_log_print(ANDROID_LOG_DEBUG,tag.c_str(),"%s",message.substr(0,MAX_L).c_str());
            log(message.substr(MAX_L),tag);
        }else{
            __android_log_print(ANDROID_LOG_DEBUG,tag.c_str(),"%s",message.c_str());
        }
    }
};

namespace SOMETHING_LONG_TO_HIDE_STUFF{
    static void test(){
        LOG::D("TestTAG","TestText %d",1);
        LOG::D("TestText %d",1);
        const char*LOL="LOL";
        LOG::D("TestText %s",LOL);
        LOG::LOL("HALLO %s %d",LOL,2);
    }
}

#endif //FPV_VR_PRIVATE_MDEBUG_H
