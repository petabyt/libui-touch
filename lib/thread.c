#include <stdio.h>
#include <android/log.h>
#include <jni.h>

// This will be put in a __emutls_t.* variable
// It's up to the compiler to decide how to implement it
struct JNILocal {
	JNIEnv *env;
	jobject ctx;
};
__thread struct JNILocal local = {0, 0};

void set_jni_env_ctx(JNIEnv *env, jobject ctx) {
	plat_dbg("Setting env/ctx %d, %d: %d", local.env, local.ctx, gettid());
	local.env = env;
	local.ctx = ctx;
}

struct JNILocal push_jni_env_ctx(JNIEnv *env, jobject ctx) {
	struct JNILocal l;
	l.env = local.env;
	l.ctx = local.ctx;
	plat_dbg("env: %u, ctx: %u, tid: %d", local.env, local.ctx, gettid());
	local.env = env;
	local.ctx = ctx;
	return l;
}

void pop_jni_env_ctx(struct JNILocal l) {
	set_jni_env_ctx(l.env, l.ctx);
}

JNIEnv *get_jni_env() {
	if (local.env == NULL) {
		plat_dbg("JNIEnv not set for this thread");
		abort();
	}

	return local.env;
}

jobject get_jni_ctx() {
	if (local.ctx == NULL) {
		plat_dbg("ctx not set for this thread");
		abort();
	}

	return local.ctx;
}