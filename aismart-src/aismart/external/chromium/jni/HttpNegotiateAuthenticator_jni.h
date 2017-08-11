// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file is autogenerated by
//     base/android/jni_generator/jni_generator.py
// For
//     org/chromium/net/HttpNegotiateAuthenticator

#ifndef org_chromium_net_HttpNegotiateAuthenticator_JNI
#define org_chromium_net_HttpNegotiateAuthenticator_JNI

#include <jni.h>

#include "base/android/jni_generator/jni_generator_helper.h"

// Step 1: forward declarations.
JNI_REGISTRATION_EXPORT extern const char
    kClassPath_org_chromium_net_HttpNegotiateAuthenticator[];
const char kClassPath_org_chromium_net_HttpNegotiateAuthenticator[] =
    "org/chromium/net/HttpNegotiateAuthenticator";

// Leaking this jclass as we cannot use LazyInstance from some threads.
JNI_REGISTRATION_EXPORT base::subtle::AtomicWord
    g_org_chromium_net_HttpNegotiateAuthenticator_clazz = 0;
#ifndef org_chromium_net_HttpNegotiateAuthenticator_clazz_defined
#define org_chromium_net_HttpNegotiateAuthenticator_clazz_defined
inline jclass org_chromium_net_HttpNegotiateAuthenticator_clazz(JNIEnv* env) {
  return base::android::LazyGetClass(env,
      kClassPath_org_chromium_net_HttpNegotiateAuthenticator,
      &g_org_chromium_net_HttpNegotiateAuthenticator_clazz);
}
#endif

namespace net {
namespace android {

// Step 2: method stubs.
JNI_GENERATOR_EXPORT void
    Java_org_chromium_net_HttpNegotiateAuthenticator_nativeSetResult(JNIEnv*
    env, jobject jcaller,
    jlong nativeJavaNegotiateResultWrapper,
    jint status,
    jstring authToken) {
  TRACE_NATIVE_EXECUTION_SCOPED("SetResult");
  JavaNegotiateResultWrapper* native =
      reinterpret_cast<JavaNegotiateResultWrapper*>(nativeJavaNegotiateResultWrapper);
  CHECK_NATIVE_PTR(env, jcaller, native, "SetResult");
  return native->SetResult(env, base::android::JavaParamRef<jobject>(env,
      jcaller), status, base::android::JavaParamRef<jstring>(env, authToken));
}

static base::subtle::AtomicWord
    g_org_chromium_net_HttpNegotiateAuthenticator_create = 0;
static base::android::ScopedJavaLocalRef<jobject>
    Java_HttpNegotiateAuthenticator_create(JNIEnv* env, const
    base::android::JavaRef<jstring>& accountType) {
  CHECK_CLAZZ(env, org_chromium_net_HttpNegotiateAuthenticator_clazz(env),
      org_chromium_net_HttpNegotiateAuthenticator_clazz(env), NULL);
  jmethodID method_id =
      base::android::MethodID::LazyGet<
      base::android::MethodID::TYPE_STATIC>(
      env, org_chromium_net_HttpNegotiateAuthenticator_clazz(env),
      "create",
"("
"Ljava/lang/String;"
")"
"Lorg/chromium/net/HttpNegotiateAuthenticator;",
      &g_org_chromium_net_HttpNegotiateAuthenticator_create);

  jobject ret =
env->CallStaticObjectMethod(org_chromium_net_HttpNegotiateAuthenticator_clazz(env),
          method_id, accountType.obj());
  jni_generator::CheckException(env);
  return base::android::ScopedJavaLocalRef<jobject>(env, ret);
}

static base::subtle::AtomicWord
    g_org_chromium_net_HttpNegotiateAuthenticator_getNextAuthToken = 0;
static void Java_HttpNegotiateAuthenticator_getNextAuthToken(JNIEnv* env, const
    base::android::JavaRef<jobject>& obj, jlong nativeResultObject,
    const base::android::JavaRef<jstring>& principal,
    const base::android::JavaRef<jstring>& authToken,
    jboolean canDelegate) {
  CHECK_CLAZZ(env, obj.obj(),
      org_chromium_net_HttpNegotiateAuthenticator_clazz(env));
  jmethodID method_id =
      base::android::MethodID::LazyGet<
      base::android::MethodID::TYPE_INSTANCE>(
      env, org_chromium_net_HttpNegotiateAuthenticator_clazz(env),
      "getNextAuthToken",
"("
"J"
"Ljava/lang/String;"
"Ljava/lang/String;"
"Z"
")"
"V",
      &g_org_chromium_net_HttpNegotiateAuthenticator_getNextAuthToken);

     env->CallVoidMethod(obj.obj(),
          method_id, nativeResultObject, principal.obj(), authToken.obj(),
              canDelegate);
  jni_generator::CheckException(env);
}

}  // namespace android
}  // namespace net

#endif  // org_chromium_net_HttpNegotiateAuthenticator_JNI
