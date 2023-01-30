//
// Created by Constantin on 24.10.2017.
//

#ifndef FPV_VR_HELPER_SHARED_PREFERENCES_HPP
#define FPV_VR_HELPER_SHARED_PREFERENCES_HPP

#include <jni.h>
#include <string>
#include <cassert>

///Example reading values
///SharedPreferences sharedPref(env,context,"pref_telemetry");
///T_Protocol=sharedPref.getInt("T_Protocol");
///Example writing values
///SharedPreferences_Editor editor=sharedPref.edit();
///editor.putString("MY_KEY","HELLO");
///editor.commit();

class SharedPreferences_Editor{
public:
    SharedPreferences_Editor(JNIEnv* env,const jobject joSharedPreferences_Edit):env(env),joSharedPreferences_Edit(joSharedPreferences_Edit){
        //find the methods for putting values into Shared preferences via the editor
        jclass jcSharedPreferences_Editor = env->GetObjectClass(joSharedPreferences_Edit);
        jmPutBoolean=env->GetMethodID(jcSharedPreferences_Editor,"putBoolean","(Ljava/lang/String;Z)Landroid/content/SharedPreferences$Editor;");
        jmPutInt=env->GetMethodID(jcSharedPreferences_Editor,"putInt","(Ljava/lang/String;I)Landroid/content/SharedPreferences$Editor;");
        jmPutString=env->GetMethodID(jcSharedPreferences_Editor,"putString","(Ljava/lang/String;Ljava/lang/String;)Landroid/content/SharedPreferences$Editor;");
        jmCommit=env->GetMethodID(jcSharedPreferences_Editor,"commit","()Z");
    }
    //return itself for method chaining
    const SharedPreferences_Editor& putBoolean(const char* key,const bool value)const{
        env->CallObjectMethod(joSharedPreferences_Edit,jmPutBoolean,env->NewStringUTF(key),(jboolean)value);
        return *this;
    }
    const SharedPreferences_Editor& putInt(const char* key,const int value)const{
        env->CallObjectMethod(joSharedPreferences_Edit,jmPutInt,env->NewStringUTF(key),(jint)value);
        return *this;
    }
    const SharedPreferences_Editor& putString(const char* key,const char* value)const{
        env->CallObjectMethod(joSharedPreferences_Edit,jmPutString,env->NewStringUTF(key),env->NewStringUTF(value));
        return *this;
    }
    bool commit()const{
        return (bool)env->CallBooleanMethod(joSharedPreferences_Edit,jmCommit);
    }
private:
    JNIEnv* env;
    jobject joSharedPreferences_Edit;
    jmethodID jmPutBoolean;
    jmethodID jmPutInt;
    jmethodID jmPutString;
    jmethodID jmCommit;
};


class SharedPreferences {
public:
    SharedPreferences(SharedPreferences const &) = delete;
    void operator=(SharedPreferences const &)= delete;
public:
    //Note: Per default, this doesn't keep the reference to the sharedPreferences java object alive
    //longer than the lifetime of the JNIEnv.
    //With keepReference=true the joSharedPreferences is kept 'alive' and you can still use the class after the original JNIEnv* has become invalid -
    //but make sure to refresh the JNIEnv* object with a new valid reference via replaceJNI()
    SharedPreferences(JNIEnv *env, jobject androidContext,const char* name,const bool keepReference=false){
        this->env=env;
        //Find the 2 java classes we need to make calls with
        jclass jcContext = env->FindClass("android/content/Context");
        jclass jcSharedPreferences = env->FindClass("android/content/SharedPreferences");
        //jclass jcSharedPreferences_Editor=env->FindClass("android/content/SharedPreferences$Editor");
        assert(jcContext!=nullptr && jcSharedPreferences!=nullptr);
        //find the 3 functions we need to get values from an SharedPreferences instance
        jmGetBoolean=env->GetMethodID(jcSharedPreferences,"getBoolean","(Ljava/lang/String;Z)Z");
        jmGetInt=env->GetMethodID(jcSharedPreferences,"getInt","(Ljava/lang/String;I)I");
        jmGetFloat=env->GetMethodID(jcSharedPreferences,"getFloat","(Ljava/lang/String;F)F");
        jmGetString=env->GetMethodID(jcSharedPreferences,"getString","(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
        //find the 1 function we need to create the SharedPreferences.Editor object
        jmEdit=env->GetMethodID(jcSharedPreferences,"edit", "()Landroid/content/SharedPreferences$Editor;");
        //create a instance of SharedPreferences and store it in @joSharedPreferences
        jmethodID jmGetSharedPreferences=env->GetMethodID(jcContext,"getSharedPreferences","(Ljava/lang/String;I)Landroid/content/SharedPreferences;");
        joSharedPreferences=env->CallObjectMethod(androidContext,jmGetSharedPreferences,env->NewStringUTF(name),MODE_PRIVATE);
        //jmEdit_commit=env->GetMethodID(jcSharedPreferences_Editor,"putString","(Ljava/lang/String;Ljava/lang/String;)Landroid/content/SharedPreferences$Editor;");
        if(keepReference){
            joSharedPreferences=env->NewWeakGlobalRef(joSharedPreferences);
        }
    }
    void replaceJNI(JNIEnv* newEnv){
        env=newEnv;
    }
private:
    JNIEnv* env;
    jobject joSharedPreferences;
    jmethodID jmGetBoolean;
    jmethodID jmGetInt;
    jmethodID jmGetFloat;
    jmethodID jmGetString;
    jmethodID jmEdit;
public:
    bool getBoolean(const char* id,bool defaultValue=false)const{
        return (bool)(env->CallBooleanMethod(joSharedPreferences,jmGetBoolean,env->NewStringUTF(id),(jboolean)defaultValue));
    }
    int getInt(const char* id,int defaultValue=0)const{
        return (int)(env->CallIntMethod(joSharedPreferences,jmGetInt,env->NewStringUTF(id),(jint)defaultValue));
    }
    float getFloat(const char* id,float defaultValue=0.0f)const{
        return (float)(env->CallFloatMethod(joSharedPreferences,jmGetFloat,env->NewStringUTF(id),(jfloat)defaultValue));
    }
    std::string getString(const char* id,const char* defaultValue="")const{
        auto value=(jstring)(env->CallObjectMethod(joSharedPreferences,jmGetString,env->NewStringUTF(id),env->NewStringUTF(defaultValue)));
        const char* valueP = env->GetStringUTFChars(value, nullptr);
        const std::string ret=std::string(valueP);
        env->ReleaseStringUTFChars(value,valueP);
        return ret;
    }
    SharedPreferences_Editor edit()const{
        //create a instance of SharedPreferences.Editor and store it in @joSharedPreferences_Edit
        jobject joSharedPreferences_Edit=env->CallObjectMethod(joSharedPreferences,jmEdit);
        SharedPreferences_Editor editor(env,joSharedPreferences_Edit);
        return editor;
    }
private:
    static constexpr const int  MODE_PRIVATE = 0; //taken directly from java, assuming this value stays constant in java
};


#endif //FPV_VR_HELPER_SHARED_PREFERENCES_HPP

