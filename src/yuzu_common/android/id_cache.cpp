// SPDX-FileCopyrightText: Copyright 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "yuzu_common/android/id_cache.h"

static JavaVM * s_java_vm = nullptr;
static constexpr jint kJniVersion = JNI_VERSION_1_6;

void SetJavaVM(JavaVM * vm)
{
    s_java_vm = vm;
}

JavaVM * GetJavaVM()
{
    return s_java_vm;
}

JNIEnv * GetEnvForThread()
{
    if (s_java_vm == nullptr)
    {
        return nullptr;
    }

    thread_local static struct OwnedEnv
    {
        OwnedEnv()
        {
            status = s_java_vm->GetEnv(reinterpret_cast<void **>(&env), kJniVersion);
            if (status == JNI_EDETACHED)
            {
                s_java_vm->AttachCurrentThread(&env, nullptr);
            }
        }

        ~OwnedEnv()
        {
            if (status == JNI_EDETACHED)
            {
                s_java_vm->DetachCurrentThread();
            }
        }

        int status{};
        JNIEnv * env = nullptr;
    } owned;

    return owned.env;
}
