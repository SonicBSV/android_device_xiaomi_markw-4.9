/*
 * Copyright (C) 2021-2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

void perf_event(int arg1, char* arg2, int arg3, int* arg4) {}

int perf_get_feedback(int arg1, char* arg2) {
    return 233;
}

int perf_get_feedback_extn(int arg1, char* arg2, unsigned int arg3, char* arg4) {
    return 233;
}

void perf_hint(int arg1, char* arg2, int arg3, int arg4) {}

int perf_hint_renew(int arg1, int arg2, const char* arg3, int arg4, int arg5, int arg6,
                    int arg7[]) {
    return 233;
}

int perf_lock_acq(int handle, int duration, int list[], int numArgs) {
    return handle ?: 233;
}

void perf_lock_cmd(int arg1) {}

int perf_lock_rel(int handle) {
    return handle ?: 233;
}

int perf_lock_use_profile(int handle, int profile) {
    return handle ?: 233;
}

void perf_wait_get_prop(char* arg1, char* arg2) {}
