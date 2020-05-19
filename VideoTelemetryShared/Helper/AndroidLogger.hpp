//
// Created by Consti10 on 14/05/2019.
//

#ifndef FPV_VR_PRIVATE_MDEBUG_H
#define FPV_VR_PRIVATE_MDEBUG_H

#include "android/log.h"
#include <string.h>
#include <sstream>

// remove any old c style definitions that might have slip trough some header files
#ifdef LOGD
#undef LOGD
#endif
#ifdef LOGE
#undef LOGE
#endif

class Example{
public:
    const std::string TAG;
    Example(std::string tag):TAG(std::move(tag)){};
};
// See https://medium.com/@geierconstantinabc/best-way-to-log-in-android-native-code-c-style-7461005610f6
// Handles logging in all my android studio projects with native code
// inspired by https://android.googlesource.com/platform/system/core/+/refs/heads/master/base/include/android-base/logging.h
// Most notable difference : I have LOGD(TAG) they have LOG(SEVERITY)
class AndroidLogger{
public:
    // Chrome university https://www.youtube.com/watch?v=UNJrgsQXvCA
    // 'New style C++ ' https://google.github.io/styleguide/cppguide.html
    AndroidLogger(const android_LogPriority priority,const std::string TAG):M_PRIORITY(priority),M_TAG(std::move(TAG)) {}
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

// taken from https://stackoverflow.com/questions/1666802/is-there-a-class-macro-in-c
namespace PrettyFunctionHelper{
    inline std::string className(const std::string& prettyFunction){
        size_t colons = prettyFunction.find("::");
        if (colons == std::string::npos)
            return "UnknownClassName";
        size_t begin = prettyFunction.substr(0,colons).rfind(" ") + 1;
        size_t end = colons - begin;
        return prettyFunction.substr(begin,end);
    }
}
#ifndef __CLASS_NAME__
#define __CLASS_NAME__ PrettyFunctionHelper::className(__PRETTY_FUNCTION__)
#endif

// These are usefully so we don't have to write AndroidLogger(ANDROID_LOG_DEBUG,"MyTAG") every time
static AndroidLogger LOGD(const std::string& TAG="NoTag"){
    return AndroidLogger(ANDROID_LOG_DEBUG,TAG);
}
static AndroidLogger LOGE(const std::string& TAG="NoTag"){
    return AndroidLogger(ANDROID_LOG_ERROR,TAG);
}

// When using macros we can have the class name as an tag (with pretty function workaround)
#define MLOGD LOGD(__CLASS_NAME__)
#define MLOGE LOGE(__CLASS_NAME__)


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
