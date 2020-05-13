//
// Created by geier on 12/05/2020.
//

#ifndef LIVEVIDEO10MS_NDKTHREAD_H
#define LIVEVIDEO10MS_NDKTHREAD_H

#include <jni.h>
#include <MDebug.hpp>

// Java Thread utility methods
namespace JThread{
    static int getThreadPriority(JNIEnv* env){
        jclass jcThread = env->FindClass("java/lang/Thread");
        jmethodID jmCurrentThread=env->GetStaticMethodID(jcThread,"currentThread","()Ljava/lang/Thread;");
        jmethodID jmGetPriority=env->GetMethodID(jcThread,"getPriority","()I");
        jobject joCurrentThread=env->CallStaticObjectMethod(jcThread,jmCurrentThread);
        return (int)env->CallIntMethod(joCurrentThread,jmGetPriority);
    }
    static void setThreadPriority(JNIEnv* env,int wantedPriority){
        jclass jcThread = env->FindClass("java/lang/Thread");
        jmethodID jmCurrentThread=env->GetStaticMethodID(jcThread,"currentThread","()Ljava/lang/Thread;");
        jmethodID jmSetPriority=env->GetMethodID(jcThread,"setPriority","(I)V");
        jobject joCurrentThread=env->CallStaticObjectMethod(jcThread,jmCurrentThread);
        env->CallVoidMethod(joCurrentThread,jmSetPriority,(jint)wantedPriority);
    }
    static void printThreadPriority(JNIEnv* env){
        LOG::D("JThread","printThreadPriority %d",getThreadPriority(env));
    }
}
// Java android.os.Process utility methods (not java/lang/Process !)
namespace JProcess{
    // calls android.os.Process.setThreadPriority()
    static void setThreadPriority(JNIEnv* env, int wantedPriority){
        jclass jcProcess = env->FindClass("android/os/Process");
        jmethodID jmSetThreadPriority=env->GetStaticMethodID(jcProcess,"setThreadPriority","(I)V");
        env->CallStaticVoidMethod(jcProcess,jmSetThreadPriority,(jint)wantedPriority);
    }
    // calls android.os.Process.getThreadPriority(android.os.Process.myTid())
    static int getThreadPriority(JNIEnv* env){
        jclass jcProcess = env->FindClass("android/os/Process");
        jmethodID jmGetThreadPriority=env->GetStaticMethodID(jcProcess,"getThreadPriority","(I)I");
        jmethodID jmMyTid=env->GetStaticMethodID(jcProcess,"myTid","()I");
        jint myTid=env->CallStaticIntMethod(jcProcess,jmMyTid);
        return (int)env->CallStaticIntMethod(jcProcess,jmGetThreadPriority,(jint)myTid);
    }
    static void printThreadPriority(JNIEnv* env){
        LOG::D("JProcess","printThreadPriority %d",getThreadPriority(env));
    }
}

namespace NDKThreadHelper{
    static JNIEnv* attachThread(JavaVM* jvm){
        JNIEnv* myNewEnv;
        JavaVMAttachArgs args;
        args.version = JNI_VERSION_1_6; // choose your JNI version
        args.name = "Thread name"; // you might want to give the java thread a name
        args.group = NULL; // you might want to assign the java thread to a ThreadGroup
        jvm->AttachCurrentThread(&myNewEnv, &args);
        return myNewEnv;
    }
    static void detachThread(JavaVM* jvm){
        jvm->DetachCurrentThread();
    }
    // If the current thread is already bound to the Java VM only call JProcess::setThreadPriority
    // If no Java VM is attached (e.g. the thread was created via ndk std::thread or equivalent)
    // attach the java vm, set priority and then DETACH again
    static void attachAndSetProcessThreadPriority(JavaVM* vm,int wantedPriority,const char* TAG="Unknown"){
        JNIEnv* env=nullptr;
        vm->GetEnv((void**)&env,JNI_VERSION_1_6);
        bool detachWhenDone=false;
        if(env== nullptr){
            LOG::D(TAG,"Attaching thread");
            env=attachThread(vm);
            detachWhenDone=true;
        }
        const int currentPriority=JProcess::getThreadPriority(env);
        JProcess::setThreadPriority(env, wantedPriority);
        const int newPriority=JProcess::getThreadPriority(env);
        if(newPriority==wantedPriority){
            LOG::D(TAG,"Successfully set priority from %d to %d",currentPriority,wantedPriority);
        }else{
            LOG::D(TAG,"Cannot set priority from %d to %d is %d instead",currentPriority,wantedPriority,newPriority);
        }
        if(detachWhenDone){
            detachThread(vm);
        }
    }
}

namespace NDKTHREADHELPERTEST{
    // attach java VM, set Thread priority & print it.
    // Then detach, reattach JVM and print thread priority
    static void doSomething(JavaVM* jvm) {
        JNIEnv* env=NDKThreadHelper::attachThread(jvm);
        JThread::printThreadPriority(env);
        JThread::setThreadPriority(env,5);
        JThread::printThreadPriority(env);
        NDKThreadHelper::detachThread(jvm);
        env=NDKThreadHelper::attachThread(jvm);
        JThread::printThreadPriority(env);
        NDKThreadHelper::detachThread(jvm);
    }
    // same as doSomething() but uses ProcessThreadPriority
    static void doSomething2(JavaVM* jvm,int prio) {
        JNIEnv* env=NDKThreadHelper::attachThread(jvm);
        JProcess::printThreadPriority(env);
        JProcess::setThreadPriority(env, prio);
        JProcess::printThreadPriority(env);
        NDKThreadHelper::detachThread(jvm);
        env=NDKThreadHelper::attachThread(jvm);
        JProcess::printThreadPriority(env);
        NDKThreadHelper::detachThread(jvm);
    }
    static void test(JNIEnv* env){
        JavaVM* jvm;
        env->GetJavaVM(&jvm);
        std::thread* thread1=new std::thread(doSomething2,jvm,-20);
    }
}

#endif //LIVEVIDEO10MS_NDKTHREAD_H
