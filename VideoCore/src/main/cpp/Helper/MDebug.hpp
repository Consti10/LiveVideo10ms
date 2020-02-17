//
// Created by Consti10 on 14/05/2019.
//

#ifndef FPV_VR_PRIVATE_MDEBUG_H
#define FPV_VR_PRIVATE_MDEBUG_H

#include "android/log.h"
#include <string.h>

#define TAG_MDEBUG "MDebug"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG_MDEBUG, __VA_ARGS__)

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

#endif //FPV_VR_PRIVATE_MDEBUG_H
