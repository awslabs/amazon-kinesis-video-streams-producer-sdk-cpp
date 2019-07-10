/**
 * Some boilerplate code used by all of our JNI interfaces.
 */

#ifndef __JNICOMMON_H__
#define __JNICOMMON_H__

#include <jni.h>                  // Basic native API
#include <string.h>

#define EXCEPTION_NAME "com/amazonaws/kinesisvideo/producer/ProducerException"

inline void throwNativeException(JNIEnv* env, const char* name, const char* msg, STATUS status)
{
    if (env->ExceptionCheck())
    {
        env->ExceptionClear(); // Discard pending exception (should never happen)
        DLOGW("Had to clear a pending exception found when throwing \"%s\" (code 0x%x)", msg, status);
    }

    DLOGI("Throwing %s with message: %s", name, msg);

    jclass exceptionClass = env->FindClass(name);
    CHECK(exceptionClass != NULL);

    jmethodID constructor = env->GetMethodID(exceptionClass, "<init>", "(Ljava/lang/String;I)V");
    CHECK(constructor != NULL);

    jstring msgString = env->NewStringUTF(msg);
    CHECK(msgString != NULL);

    // Convert status to an unsigned 64-bit value
    const jint intStatus = (jint) status;
    const jobject exception = env->NewObject(exceptionClass, constructor, msgString, intStatus);
    CHECK(exception != NULL);

    if (env->Throw(jthrowable(exception)) != JNI_OK)
    {
        DLOGE("Failed throwing %s: %s (status 0x%x)", name, msg, status);
    }

    env->DeleteLocalRef(msgString);
    env->DeleteLocalRef(exception);
}

#endif // __JNICOMMON_H__
