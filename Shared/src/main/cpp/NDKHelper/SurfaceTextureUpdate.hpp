//
// Created by geier on 31/01/2020.
//

#ifndef FPV_VR_OS_SURFACETEXTUREUPDATE_HPP
#define FPV_VR_OS_SURFACETEXTUREUPDATE_HPP

//It makes absolutely no sense to call java code via ndk that is then calling ndk code again - but when building
//for pre-android 9 (api 28), there is no other way around
//If api<28 we have to use java to update the surface texture
//#include <android/api-level.h>
//#if __ANDROID_API__< __ANDROID_API_P__ //28
#define FPV_VR_USE_JAVA_FOR_SURFACE_TEXTURE_UPDATE
//#endif

#include <jni.h>
#include <android/surface_texture_jni.h>
#ifdef FPV_VR_USE_JAVA_FOR_SURFACE_TEXTURE_UPDATE
#include <android/surface_texture.h>
#else
#include <android/surface_texture_jni.h>
#include <android/surface_texture.h>

#undef __ANDROID_API__
#define __ANDROID_API__ 28
#undef __INTRODUCED_IN
#define __INTRODUCED_IN(api_level) __attribute__((annotate("introduced_in=21")))

#include <android/surface_texture_jni.h>
#include <android/surface_texture.h>

#undef __ANDROID_API__
#undef __INTRODUCED_IN
#define __ANDROID_API__ 21
#define __INTRODUCED_IN(api_level) __attribute__((annotate("introduced_in=" #api_level)))

#endif //FPV_VR_USE_JAVA_FOR_SURFACE_TEXTURE_UPDATE

#include <optional>
#include <chrono>
#include "TimeHelper.hpp"
#include "NDKThreadHelper.hpp"

//Helper for calling the ASurfaceTexture_XXX method with a fallback for minApi<28
// 03.06.2020 confirmed that the ASurfaceTexture_XXX methods call native code directly (not java)

class SurfaceTextureUpdate {
private:
    jmethodID updateTexImageMethodId;
    jmethodID getTimestampMethodId;
    //Only valid hen not using FPV_VR_USE_JAVA_FOR_SURFACE_TEXTURE_UPDATE
    ASurfaceTexture* mSurfaceTexture;
    // set later (not in constructor)
    jobject weakGlobalRefSurfaceTexture;
    int textureId;
public:
    AvgCalculator2 delayToUpdate2{60};
    // look up all the method ids
    SurfaceTextureUpdate(JNIEnv* env){
        jclass jcSurfaceTexture = env->FindClass("android/graphics/SurfaceTexture");
        MLOGD<<"SurfaceTextureStart";
        if ( jcSurfaceTexture == nullptr ) {
            MLOGD<< "FindClass( SurfaceTexture ) failed";
        }
        // find the constructor that takes an int
        jmethodID constructor = env->GetMethodID( jcSurfaceTexture, "<init>", "(I)V" );
        if ( constructor == nullptr) {
            MLOGD<<"GetMethodID( <init> ) failed";
        }
        //get the java methods that can be called with a valid surfaceTexture instance and JNI env
        updateTexImageMethodId = env->GetMethodID( jcSurfaceTexture, "updateTexImage", "()V" );
        if ( !updateTexImageMethodId ) {
            MLOGD<<"couldn't get updateTexImageMethodId";
        }
        getTimestampMethodId = env->GetMethodID( jcSurfaceTexture, "getTimestamp", "()J" );
        if ( !getTimestampMethodId ) {
            MLOGD<<"couldn't get getTimestampMethodId";
        }
    }
    /**
     * set the wrapped SurfaceTexture. this has to be delayed (cannot be done in constructor)
     * @param surfaceTexture1 when nullptr delete previosuly aquired reference, else create new reference
     */
    void setSurfaceTextureAndId(JNIEnv* env,jobject surfaceTexture1,jint textureId1){
        textureId=textureId1;
#ifdef FPV_VR_USE_JAVA_FOR_SURFACE_TEXTURE_UPDATE
        if(surfaceTexture1==nullptr){
            assert(weakGlobalRefSurfaceTexture!=nullptr);
            env->DeleteWeakGlobalRef(weakGlobalRefSurfaceTexture);
            weakGlobalRefSurfaceTexture=nullptr;
        }else{
            weakGlobalRefSurfaceTexture = env->NewWeakGlobalRef(surfaceTexture1);
        }
#else
        mSurfaceTexture=ASurfaceTexture_fromSurfaceTexture(env,surfaceTexture1);
#endif
    }
    void updateFromSurfaceTextureHolder(JNIEnv* env,jobject surfaceTextureHolder){
        // You are going to see an IDE error here since this module does not know RenderingXCore
        // However, if you use this class in a module that also depends on RenderingXCore it will work without issues
        jclass jcSurfaceTextureHolder = env->FindClass("constantin/renderingx/core/xglview/SurfaceTextureHolder");
        assert(jcSurfaceTextureHolder);
        jmethodID jmGetTextureId = env->GetMethodID(jcSurfaceTextureHolder, "getTextureId", "()I" );
        assert(jmGetTextureId);
        jmethodID jmGetSurfaceTexture=env->GetMethodID(jcSurfaceTextureHolder, "getSurfaceTexture", "()Landroid/graphics/SurfaceTexture;" );
        assert(jmGetTextureId);
        setSurfaceTextureAndId(env,env->CallObjectMethod(surfaceTextureHolder,jmGetSurfaceTexture),env->CallIntMethod(surfaceTextureHolder,jmGetTextureId));
        // call once such that the target is set properly for the texture
        updateTexImageJAVA(env);
    }
    void updateTexImageJAVA(JNIEnv* env) {
#ifdef FPV_VR_USE_JAVA_FOR_SURFACE_TEXTURE_UPDATE
        env->CallVoidMethod(weakGlobalRefSurfaceTexture, updateTexImageMethodId );
#else
        ASurfaceTexture_updateTexImage(mSurfaceTexture);
#endif
    };
    long getTimestamp(JNIEnv* env){
#ifdef FPV_VR_USE_JAVA_FOR_SURFACE_TEXTURE_UPDATE
        return env->CallLongMethod(weakGlobalRefSurfaceTexture, getTimestampMethodId);
#else
        return ASurfaceTexture_getTimestamp(mSurfaceTexture);
#endif
    }
    // on success, returns delay between producer enqueueing buffer and consumer (gl) dequeueing it
    // on failure (no new image available) returns std::nullopt
    std::optional<std::chrono::steady_clock::duration> updateAndCheck(JNIEnv* env){
        const long oldTimestamp=getTimestamp(env);
        updateTexImageJAVA(env);
        const long newTimestamp=getTimestamp(env);
        if(newTimestamp!=oldTimestamp){
            const auto diff=newTimestamp-oldTimestamp;
            //MLOGD<<"Diff "<<MyTimeHelper::R(std::chrono::nanoseconds(diff));
            const auto delay=std::chrono::steady_clock::now().time_since_epoch()-std::chrono::nanoseconds(newTimestamp);
            delayToUpdate2.add(delay);
            //MLOGD<<"ST delay "<<delayToUpdate2.getAvgReadable();
            return delay;
        }
        return std::nullopt;
    }
    // Poll on the SurfaceTexture in small intervalls until either
    // 1) a new frame was dequeued
    // 2) the timeout was reached
    // 3) the calling java thread was interrupted
    std::optional<std::chrono::steady_clock::duration> waitUntilFrameAvailable(JNIEnv* env,const std::chrono::steady_clock::time_point& maxWaitTimePoint){
        JThread jThread(env);
        while(true){
            if(const auto delay=updateAndCheck(env)){
                return delay;
            }
            //const auto leftSleepTime=maxWaitTimePoint-std::chrono::steady_clock::now();
            if(std::chrono::steady_clock::now()>=maxWaitTimePoint){
                break;
            }
            if(jThread.isInterrupted()){
                break;
            }
            TestSleep::sleep(std::chrono::milliseconds(1));
        }
        return std::nullopt;
    }
    unsigned int getTextureId()const{
        return textureId;
    }
};


#endif //FPV_VR_OS_SURFACETEXTUREUPDATE_HPP
