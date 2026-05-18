// SPDX-FileCopyrightText: 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#ifdef ANDROID

#include <jni.h>

void SetJavaVM(JavaVM * vm);
JavaVM * GetJavaVM();
JNIEnv * GetEnvForThread();

#endif
