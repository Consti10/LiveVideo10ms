//
// Created by geier on 30/04/2020.
//

#ifndef RENDERINGX_CLASSMEMBERFROMJAVA_H
#define RENDERINGX_CLASSMEMBERFROMJAVA_H

#include "NDKArrayHelper.hpp"
#include <AndroidLogger.hpp>

// The purpose of this class is to make it easier to obtain member values of a java class instance
// Only dependencies are standard libraries and the android java NDK

// return the java name for a simple cpp primitive
// e.g. float -> F, int -> I
template <class T> struct cppTypeToNdkName { static const char * const value; };
template <> constexpr const char *cppTypeToNdkName<int>::value = "I";
template <> constexpr const char *cppTypeToNdkName<float>::value = "F";
template <> constexpr const char *cppTypeToNdkName<std::vector<int>>::value = "[I";
template <> constexpr const char *cppTypeToNdkName<std::vector<float>>::value = "[F";

class ClassMemberFromJava {
private:
    static constexpr auto TAG="ClassMemberFromJava";
    JNIEnv* env;
    jclass jclass1;
    jobject jobject1;
    /**
     * @tparam T a generic cpp data type (like int, float)
     * @param name the name of the member object instance in the java class
     * @return On success ( name found and data type matches) return a valid ndk handle
     * On failure, return nullptr but clear java exception
     */
    template <class T> jfieldID getFieldId(const char* name){
        const char* ndkTypeName=cppTypeToNdkName<T>::value;
        jfieldID field=env->GetFieldID(jclass1,name,ndkTypeName);
        if(field==nullptr){
            MLOGE<<"cannot find member "<<name<<" of type "<<ndkTypeName;
            env->ExceptionClear();
            return nullptr;
        }
        return field;
    }
public:
    /**
     * We can obtain the java class (name) from the instance
     * When @param env becomes invalid this instance mustn't be used anymore
     * @param jobject1 instance of the jobject you want to obtain member values from
     */
    ClassMemberFromJava(JNIEnv* env,jobject jobject1){
        this->env=env;
        this->jclass1=env->GetObjectClass(jobject1);
        this->jobject1=jobject1;
    }
    template <typename T>
    struct always_false : std::false_type {};
    /**
     * If a class member with the name @param name is found AND
     * Its type can be translated into a generic cpp type
     * (e.g. java int == cpp int but a special java 'Car' class cannot be translated into cpp)
     * @return the value of the java class member as generic cpp type or 0 when not found
     */
    template <typename T>
    T get(const char *name) {
        jfieldID field=ClassMemberFromJava::getFieldId<T>(name);
        if(field== nullptr)return T();
        //switch case for all supported cpp types
        if constexpr (std::is_same_v<T, float>){
            return env->GetFloatField(jobject1,field);
        }else if constexpr (std::is_same_v<T, int>){
            return env->GetIntField(jobject1,field);
        }else if constexpr (std::is_same_v<T, std::vector<float>>){
            jfloatArray array=(jfloatArray)env->GetObjectField(jobject1,field);
            return NDKArrayHelper::DynamicSizeArray<float>(env, array);
        }else{
            static_assert(always_false<T>::value, "Type wrong.");
        }
        return T();
    }
    template<std::size_t S> std::array<float,S> getFloatArrayFixed(const char *name) {
        jfieldID field=ClassMemberFromJava::getFieldId<std::vector<float>>(name);
        if(field== nullptr)return std::array<float,S>();
        jfloatArray array=(jfloatArray)env->GetObjectField(jobject1,field);
        return NDKArrayHelper::FixedSizeArray<float,S>(env,array);
    }
};

#endif //RENDERINGX_CLASSMEMBERFROMJAVA_H
