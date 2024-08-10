#include <stdio.h>
#include <stdlib.h>
#include <android/log.h>
#include <jni.h>
#include "android.h"

// This will be put in a __emutls_t.* variable
// It's up to the compiler to decide how to implement it
__thread struct AndroidLocal local = {0, 0};

__attribute__((weak))
void set_jni_env_ctx(JNIEnv *env, jobject ctx) {
	local.env = env;
	local.ctx = ctx;
}

__attribute__((weak))
struct AndroidLocal push_jni_env_ctx(JNIEnv *env, jobject ctx) {
	struct AndroidLocal l;
	l.env = local.env;
	l.ctx = local.ctx;
	local.env = env;
	local.ctx = ctx;
	return l;
}

__attribute__((weak))
void pop_jni_env_ctx(struct AndroidLocal l) {
	set_jni_env_ctx(l.env, l.ctx);
}

__attribute__((weak))
JNIEnv *get_jni_env() {
	if (local.env == NULL) {
		puts("JNIEnv not set for this thread");
		abort();
	}

	return local.env;
}

jobject get_jni_ctx() {
	if (local.ctx == NULL) {
		puts("ctx not set for this thread");
		abort();
	}

	return local.ctx;
}