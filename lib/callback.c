#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <android/log.h>
#include <stdlib.h>
#include <jni.h>

#include "android.h"
#include "uifw_priv.h"

struct CallbackData {
	uintptr_t fn_ptr;
	int argc;
	uintptr_t arg1;
	uintptr_t arg2;
};

static void function_callback(JNIEnv *env, jbyteArray arr) {
	struct CallbackData *data = (struct CallbackData *)(*env)->GetByteArrayElements(env, arr, 0);
	if (data->fn_ptr == 0x0) abort();
	if (data->argc == 2) {
		typedef void fn2(uintptr_t a, uintptr_t b);
		fn2 *ptr_f = (fn2 *)data->fn_ptr;
		ptr_f(data->arg1, data->arg2);
	} else if (data->argc == 1) {
		typedef void fn1(uintptr_t a);
		fn1 *ptr_f = (fn1 *)data->fn_ptr;
		ptr_f(data->arg1);
	} else if (data->argc == 0) {
		typedef void fn0(void);
		fn0 *ptr_f = (fn0 *)data->fn_ptr;
		ptr_f();
	} else {
		abort();
	}
}

void view_add_native_click_listener(JNIEnv *env, jobject view, void *fn, int argc, void *arg1, void *arg2) {
	struct CallbackData call_data;
	call_data.fn_ptr = (uintptr_t)fn;
	call_data.argc = argc;
	call_data.arg1 = (uintptr_t)arg1;
	call_data.arg2 = (uintptr_t)arg2;

	jbyteArray arr = (*env)->NewByteArray(env, sizeof(call_data));
	(*env)->SetByteArrayRegion(env, arr, 0, sizeof(call_data), (const jbyte *) &call_data);

	jclass listener_c = (*env)->FindClass(env, "dev/danielc/libui/LibUI$MyOnClickListener");
	jobject listener = (*env)->AllocObject(env, listener_c);

	jfieldID field = (*env)->GetFieldID(env, listener_c, "struct", "[B");
	(*env)->SetObjectField(env, listener, field, arr);

	jmethodID set_click = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, view),
											  "setOnClickListener",
											  "(Landroid/view/View$OnClickListener;)V");
	(*env)->CallVoidMethod(env, view, set_click, listener);
}

void view_add_native_select_listener(JNIEnv *env, jobject view, void *fn, int argc, void *arg1, void *arg2) {
	struct CallbackData call_data;
	call_data.argc = argc;
	call_data.fn_ptr = (uintptr_t)fn;
	call_data.arg1 = (uintptr_t)arg1;
	call_data.arg2 = (uintptr_t)arg2;

	jbyteArray arr = (*env)->NewByteArray(env, sizeof(call_data));
	(*env)->SetByteArrayRegion(env, arr, 0, sizeof(call_data), (const jbyte *)&call_data);

	jclass listener_c = (*env)->FindClass(env, "dev/danielc/libui/LibUI$MySelectListener");
	jmethodID click_init = (*env)->GetMethodID(env, listener_c, "<init>", "()V");
	jobject listener = (*env)->NewObject(env, listener_c, click_init);

	jfieldID field = (*env)->GetFieldID(env, listener_c, "struct", "[B");
	(*env)->SetObjectField(env, listener, field, arr);

	jmethodID set_click = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, view), "setOnItemSelectedListener", "(Landroid/widget/AdapterView$OnItemSelectedListener;)V");
	(*env)->CallVoidMethod(env, view, set_click, listener);
}

void view_add_native_checked_listener(JNIEnv *env, jobject view, void *fn, int argc, void *arg1, void *arg2) {
	struct CallbackData call_data;
	call_data.argc = argc;
	call_data.fn_ptr = (uintptr_t)fn;
	call_data.arg1 = (uintptr_t)arg1;
	call_data.arg2 = (uintptr_t)arg2;

	jbyteArray arr = (*env)->NewByteArray(env, sizeof(call_data));
	(*env)->SetByteArrayRegion(env, arr, 0, sizeof(call_data), (const jbyte *)&call_data);

	jclass listener_c = (*env)->FindClass(env, "dev/danielc/libui/LibUI$MyCheckedListener");
	jmethodID click_init = (*env)->GetMethodID(env, listener_c, "<init>", "()V");
	jobject listener = (*env)->NewObject(env, listener_c, click_init);

	jfieldID field = (*env)->GetFieldID(env, listener_c, "struct", "[B");
	(*env)->SetObjectField(env, listener, field, arr);

	jmethodID set_click = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, view), "setOnCheckedChangeListener", "(Landroid/widget/CompoundButton$OnCheckedChangeListener;)V");
	(*env)->CallVoidMethod(env, view, set_click, listener);
}

void view_add_native_input_listener(JNIEnv *env, jobject view, void *fn, int argc, void *arg1, void *arg2) {
	struct CallbackData call_data;
	call_data.argc = argc;
	call_data.fn_ptr = (uintptr_t)fn;
	call_data.arg1 = (uintptr_t)arg1;
	call_data.arg2 = (uintptr_t)arg2;

	jbyteArray arr = (*env)->NewByteArray(env, sizeof(call_data));
	(*env)->SetByteArrayRegion(env, arr, 0, sizeof(call_data), (const jbyte *)&call_data);

	jclass listener_c = (*env)->FindClass(env, "dev/danielc/libui/LibUI$MyTextWatcher");
	jmethodID click_init = (*env)->GetMethodID(env, listener_c, "<init>", "()V");
	jobject listener = (*env)->NewObject(env, listener_c, click_init);

	jfieldID field = (*env)->GetFieldID(env, listener_c, "struct", "[B");
	(*env)->SetObjectField(env, listener, field, arr);

	jfieldID view_f = (*env)->GetFieldID(env, listener_c, "view", "Landroid/view/View;");
	(*env)->SetObjectField(env, listener, view_f, view);

	jmethodID set_click = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, view), "addTextChangedListener", "(Landroid/text/TextWatcher;)V");
	(*env)->CallVoidMethod(env, view, set_click, listener);
}

void jni_native_runnable(JNIEnv *env, jobject ctx, void *fn, int argc, void *arg1, void *arg2) {
	struct CallbackData call_data;
	call_data.fn_ptr = (uintptr_t)fn;
	call_data.argc = argc;
	call_data.arg1 = (uintptr_t)arg1;
	call_data.arg2 = (uintptr_t)arg2;

	jbyteArray arr = (*env)->NewByteArray(env, sizeof(call_data));
	(*env)->SetByteArrayRegion(env, arr, 0, sizeof(call_data), (const jbyte *)&call_data);

	jclass listener_c = (*env)->FindClass(env, "dev/danielc/libui/LibUI$MyRunnable");
	jobject listener = (*env)->AllocObject(env, listener_c);

	jfieldID ctx_f = (*env)->GetFieldID(env, listener_c, "ctx", "Landroid/content/Context;");
	(*env)->SetObjectField(env, listener, ctx_f, ctx);

	jfieldID field = (*env)->GetFieldID(env, listener_c, "struct", "[B");
	(*env)->SetObjectField(env, listener, field, arr);

	jobject handler = jni_get_handler(env);

	jmethodID post_m = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, handler), "post", "(Ljava/lang/Runnable;)Z");
	(*env)->CallBooleanMethod(env, handler, post_m, listener);
}

LIBUI(void, 00024MyTextWatcher_beforeTextChanged)(JNIEnv *env, jobject thiz, jobject s,
													 jint start, jint count, jint after) {

}

LIBUI(void, 00024MyTextWatcher_onTextChanged)(JNIEnv *env, jobject thiz, jobject s, jint start,
											  jint before, jint count) {

}

LIBUI(void, 00024MyTextWatcher_afterTextChanged)(JNIEnv *env, jobject thiz, jobject s) {
	jfieldID view_f = (*env)->GetFieldID(env, (*env)->GetObjectClass(env, thiz), "view", "Landroid/view/View;");
	jobject input = (*env)->GetObjectField(env, thiz, view_f);
	set_jni_env_ctx(env, view_get_context(env, input));

	jfieldID field = (*env)->GetFieldID(env, (*env)->GetObjectClass(env, thiz), "struct", "[B");
	jobject arr = (*env)->GetObjectField(env, thiz, field);
	function_callback(env, arr);
}

LIBUI(void, 00024MyCheckedListener_onCheckedChanged)(JNIEnv* env, jobject thiz,
														jobject button_view, jboolean is_checked) {
	jobject ctx = view_get_context(env, button_view);
	struct AndroidLocal x = push_jni_env_ctx(env, ctx);
	//set_jni_env_ctx(env, ctx);
	jfieldID field = (*env)->GetFieldID(env, (*env)->GetObjectClass(env, thiz), "struct", "[B");
	jobject arr = (*env)->GetObjectField(env, thiz, field);
	function_callback(env, arr);
	//set_jni_env_ctx(env, ctx);
	pop_jni_env_ctx(x);
}

LIBUI(jobject, 00024MyTabFactory_createTabContent)(JNIEnv* env, jobject thiz, jstring tab) {
	jfieldID field = (*env)->GetFieldID(env, (*env)->GetObjectClass(env, thiz), "child", "Landroid/view/View;");
	return (*env)->GetObjectField(env, thiz, field);
}

LIBUI(void, 00024MyOnClickListener_onClick)(JNIEnv* env, jobject thiz, jobject view) {
	set_jni_env_ctx(env, view_get_context(env, view));
	jfieldID field = (*env)->GetFieldID(env, (*env)->GetObjectClass(env, thiz), "struct", "[B");
	jobject arr = (*env)->GetObjectField(env, thiz, field);
	function_callback(env, arr);
}

LIBUI(void, 00024MySelectListener_onNothingSelected)(JNIEnv* env, jobject thiz, jobject view) {
	// ...
}

LIBUI(void, 00024MySelectListener_onItemSelected)(JNIEnv* env, jobject thiz, jobject parent,
		jobject view, jint position, jlong id) {
	set_jni_env_ctx(env, view_get_context(env, view));
	jfieldID field = (*env)->GetFieldID(env, (*env)->GetObjectClass(env, thiz), "struct", "[B");
	jobject arr = (*env)->GetObjectField(env, thiz, field);
	function_callback(env, arr);
}

LIBUI(void, 00024MyRunnable_run)(JNIEnv* env, jobject thiz) {
	jfieldID ctx_f = (*env)->GetFieldID(env, (*env)->GetObjectClass(env, thiz), "ctx", "Landroid/content/Context;");
	set_jni_env_ctx(env, (*env)->GetObjectField(env, thiz, ctx_f));

	jfieldID field = (*env)->GetFieldID(env, (*env)->GetObjectClass(env, thiz), "struct", "[B");
	jobject arr = (*env)->GetObjectField(env, thiz, field);
	function_callback(env, arr);
}

LIBUI(void, callFunction)(JNIEnv *env, jobject thiz, jbyteArray arr) {
	function_callback(env, arr);
}
#if 0
LIBUI(jobject, callObjectFunction)(JNIEnv *env, jobject thiz, jbyteArray arr, jobjectArray args) {
	struct CallbackData *data = (struct CallbackData *)(*env)->GetByteArrayElements(env, arr, 0);

	if (data->fn_ptr == 0x0) abort();

	if ((*env)->GetArrayLength(env, args) >= 1) {
		data->arg1 = (uintptr_t)(*env)->GetObjectArrayElement(env, args, 0);
	}
	if ((*env)->GetArrayLength(env, args) >= 2) {
		data->arg2 = (uintptr_t)(*env)->GetObjectArrayElement(env, args, 1);
	}

	jobject (*ptr_f)(uintptr_t, uintptr_t) = (void *)data->fn_ptr;
	return ptr_f(data->arg1, data->arg2);
}
#endif