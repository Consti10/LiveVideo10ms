//
// Created by Consti10 on 14/05/2019.
//

#ifndef FPV_VR_PRIVATE_MDEBUG_H
#define FPV_VR_PRIVATE_MDEBUG_H

#include "android/log.h"
#include <string.h>
#include <sstream>

// See https://medium.com/@geierconstantinabc/best-way-to-log-in-android-native-code-c-style-7461005610f6
// Handles logging in all my android studio projects with native code
// inspired by https://android.googlesource.com/platform/system/core/+/refs/heads/master/base/include/android-base/logging.h
// Most notable difference : I have LOGD(TAG) they have LOG(SEVERITY)
class AndroidLogger{
public:
    // TODO Chrome university https://www.youtube.com/watch?v=UNJrgsQXvCA
    // can we make these functions more performant (constructor and << ) ?
    AndroidLogger(const android_LogPriority priority,const std::string& TAG):M_PRIORITY(priority),M_TAG(TAG) {}
    ~AndroidLogger() {
        logBigMessage(stream.str());
    }
private:
    std::stringstream stream;
    const std::string M_TAG;
    const android_LogPriority M_PRIORITY;
    // taken from https://android.googlesource.com/platform/system/core/+/android-2.1_r1/liblog/logd_write.c
    static constexpr const auto ANDROID_LOG_BUFF_SIZE=1024;
    //Splits debug messages that exceed the android log maximum length into smaller log(s)
    //Recursive declaration
    void logBigMessage(const std::string& message){
        if(message.length()>ANDROID_LOG_BUFF_SIZE){
            __android_log_print(M_PRIORITY,M_TAG.c_str(),"%s",message.substr(0,ANDROID_LOG_BUFF_SIZE).c_str());
            logBigMessage(message.substr(ANDROID_LOG_BUFF_SIZE));
        }else{
            __android_log_print(M_PRIORITY,M_TAG.c_str(),"%s",message.c_str());
        }
    }
    // the non-member function operator<< will now have access to private members
    template <typename T>
    friend AndroidLogger& operator<<(AndroidLogger& record, T&& t);
};
template <typename T>
AndroidLogger& operator<<(AndroidLogger& record, T&& t) {
    record.stream << std::forward<T>(t);
    return record;
}
template <typename T>
AndroidLogger& operator<<(AndroidLogger&& record, T&& t) {
    return record << std::forward<T>(t);
}
// These are usefully so we don't have to write AndroidLogger(ANDROID_LOG_DEBUG,"MyTAG") every time
static AndroidLogger LOGD(const std::string& TAG="NoTag"){
    return AndroidLogger(ANDROID_LOG_DEBUG,TAG);
}
static AndroidLogger LOGE(const std::string& TAG="NoTag"){
    return AndroidLogger(ANDROID_LOG_ERROR,TAG);
}

// print some example LOGs
namespace TEST_LOGGING_ON_ANDROID{
    static void test2(){
        __android_log_print(ANDROID_LOG_DEBUG,"TAG","Before");
        AndroidLogger(ANDROID_LOG_DEBUG,"MyTAG")<<"Hello World I "<<1<<" F "<<0.0f<<" X";
        __android_log_print(ANDROID_LOG_DEBUG,"TAG","After");
    }
}

/*static void test(){
       LOG::D("TestText %d",1);
       LOG::D("TestText %d",1);
       const char*LOL="LOL";
       LOG::D("TestText %s",LOL);
       LOG::LOL("HALLO %s %d",LOL,2);
   }*/
//#define TAG_MDEBUG "MDebug"
//#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG_MDEBUG, __VA_ARGS__)

/*namespace LOG{
    // taken from https://android.googlesource.com/platform/system/core/+/android-2.1_r1/liblog/logd_write.c
    static constexpr const auto ANDROID_LOG_BUFF_SIZE=1024;
    static constexpr auto DEFAULT_TAG="NoTag";
    static void D(const char* fmt,...)__attribute__((__format__(printf, 1, 2))) {
        va_list argptr;
        va_start(argptr, fmt);
        __android_log_vprint(ANDROID_LOG_DEBUG,DEFAULT_TAG,fmt,argptr);
        va_end(argptr);
    }
    static void E(const char* fmt,...)__attribute__((__format__(printf, 1, 2))) {
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
}*/
#endif //FPV_VR_PRIVATE_MDEBUG_H
