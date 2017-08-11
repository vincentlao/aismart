// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file is autogenerated by
//     base/android/jni_generator/jni_generator.py
// For
//     org/chromium/net/AndroidKeyStore

#ifndef org_chromium_net_AndroidKeyStore_JNI
#define org_chromium_net_AndroidKeyStore_JNI

#include <jni.h>

#include "base/android/jni_generator/jni_generator_helper.h"

// Step 1: forward declarations.
JNI_REGISTRATION_EXPORT extern const char
    kClassPath_org_chromium_net_AndroidKeyStore[];
const char kClassPath_org_chromium_net_AndroidKeyStore[] =
    "org/chromium/net/AndroidKeyStore";

// Leaking this jclass as we cannot use LazyInstance from some threads.
JNI_REGISTRATION_EXPORT base::subtle::AtomicWord
    g_org_chromium_net_AndroidKeyStore_clazz = 0;
#ifndef org_chromium_net_AndroidKeyStore_clazz_defined
#define org_chromium_net_AndroidKeyStore_clazz_defined
inline jclass org_chromium_net_AndroidKeyStore_clazz(JNIEnv* env) {
  return base::android::LazyGetClass(env,
      kClassPath_org_chromium_net_AndroidKeyStore,
      &g_org_chromium_net_AndroidKeyStore_clazz);
}
#endif

namespace net {
namespace android {

// Step 2: method stubs.

static base::subtle::AtomicWord
    g_org_chromium_net_AndroidKeyStore_signWithPrivateKey = 0;
static base::android::ScopedJavaLocalRef<jbyteArray>
    Java_AndroidKeyStore_signWithPrivateKey(JNIEnv* env, const
    base::android::JavaRef<jobject>& privateKey,
    const base::android::JavaRef<jstring>& algorithm,
    const base::android::JavaRef<jbyteArray>& message) {
  CHECK_CLAZZ(env, org_chromium_net_AndroidKeyStore_clazz(env),
      org_chromium_net_AndroidKeyStore_clazz(env), NULL);
  jmethodID method_id =
      base::android::MethodID::LazyGet<
      base::android::MethodID::TYPE_STATIC>(
      env, org_chromium_net_AndroidKeyStore_clazz(env),
      "signWithPrivateKey",
"("
"Ljava/security/PrivateKey;"
"Ljava/lang/String;"
"[B"
")"
"[B",
      &g_org_chromium_net_AndroidKeyStore_signWithPrivateKey);

  jbyteArray ret =
static_cast<jbyteArray>(env->CallStaticObjectMethod(org_chromium_net_AndroidKeyStore_clazz(env),
          method_id, privateKey.obj(), algorithm.obj(), message.obj()));
  jni_generator::CheckException(env);
  return base::android::ScopedJavaLocalRef<jbyteArray>(env, ret);
}

static base::subtle::AtomicWord
    g_org_chromium_net_AndroidKeyStore_getOpenSSLHandleForPrivateKey = 0;
static jlong Java_AndroidKeyStore_getOpenSSLHandleForPrivateKey(JNIEnv* env,
    const base::android::JavaRef<jobject>& privateKey) {
  CHECK_CLAZZ(env, org_chromium_net_AndroidKeyStore_clazz(env),
      org_chromium_net_AndroidKeyStore_clazz(env), 0);
  jmethodID method_id =
      base::android::MethodID::LazyGet<
      base::android::MethodID::TYPE_STATIC>(
      env, org_chromium_net_AndroidKeyStore_clazz(env),
      "getOpenSSLHandleForPrivateKey",
"("
"Ljava/security/PrivateKey;"
")"
"J",
      &g_org_chromium_net_AndroidKeyStore_getOpenSSLHandleForPrivateKey);

  jlong ret =
      env->CallStaticLongMethod(org_chromium_net_AndroidKeyStore_clazz(env),
          method_id, privateKey.obj());
  jni_generator::CheckException(env);
  return ret;
}

static base::subtle::AtomicWord
    g_org_chromium_net_AndroidKeyStore_getOpenSSLEngineForPrivateKey = 0;
static base::android::ScopedJavaLocalRef<jobject>
    Java_AndroidKeyStore_getOpenSSLEngineForPrivateKey(JNIEnv* env, const
    base::android::JavaRef<jobject>& privateKey) {
  CHECK_CLAZZ(env, org_chromium_net_AndroidKeyStore_clazz(env),
      org_chromium_net_AndroidKeyStore_clazz(env), NULL);
  jmethodID method_id =
      base::android::MethodID::LazyGet<
      base::android::MethodID::TYPE_STATIC>(
      env, org_chromium_net_AndroidKeyStore_clazz(env),
      "getOpenSSLEngineForPrivateKey",
"("
"Ljava/security/PrivateKey;"
")"
"Ljava/lang/Object;",
      &g_org_chromium_net_AndroidKeyStore_getOpenSSLEngineForPrivateKey);

  jobject ret =
      env->CallStaticObjectMethod(org_chromium_net_AndroidKeyStore_clazz(env),
          method_id, privateKey.obj());
  jni_generator::CheckException(env);
  return base::android::ScopedJavaLocalRef<jobject>(env, ret);
}

}  // namespace android
}  // namespace net

#endif  // org_chromium_net_AndroidKeyStore_JNI
