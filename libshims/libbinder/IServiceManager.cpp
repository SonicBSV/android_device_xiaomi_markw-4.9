/*
 * Copyright (C) 2023 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <utils/String16.h>

namespace android {

bool checkCallingPermission(const String16& /* permission */) {
    return true;
};

}  // namespace android
