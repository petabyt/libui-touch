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
	typedef void fn2(uintptr_t a, uintptr_t b);
	typedef void fn1(uintptr_t a);
	if (data->argc == 2) {
		fn2 *ptr_f = (fn2 *)data->fn_ptr;
		ptr_f(data->arg1, data->arg2);
	} else if (data->argc == 1) {
		fn1 *ptr_f = (fn1 *)data->fn_ptr;
		ptr_f(data->arg1);
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


void jni_native_runnable(JNIEnv *env, void *fn, int argc, void *arg1, void *arg2) {
	struct CallbackData call_data;
	call_data.fn_ptr = (uintptr_t)fn;
	call_data.argc = argc;
	call_data.arg1 = (uintptr_t)arg1;
	call_data.arg2 = (uintptr_t)arg2;

	jbyteArray arr = (*env)->NewByteArray(env, sizeof(call_data));
	(*env)->SetByteArrayRegion(env, arr, 0, sizeof(call_data), (const jbyte *)&call_data);

	jclass listener_c = (*env)->FindClass(env, "dev/danielc/libui/LibUI$MyRunnable");
	jobject listener = (*env)->AllocObject(env, listener_c);

	jfieldID field = (*env)->GetFieldID(env, listener_c, "struct", "[B");
	(*env)->SetObjectField(env, listener, field, arr);

	jobject handler = jni_get_handler(env);

	jmethodID post_m = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, handler), "post", "(Ljava/lang/Runnable;)V");
	(*env)->CallVoidMethod(env, handler, post_m, listener);
}

LIBUI(jobject, 00024MyTabFactory_createTabContent)(JNIEnv* env, jobject thiz, jstring tab) {
	jfieldID field = (*env)->GetFieldID(env, (*env)->GetObjectClass(env, thiz), "child", "Landroid/view/View;");
	return (*env)->GetObjectField(env, thiz, field);
}

LIBUI(void, 00024MyOnClickListener_onClick)(JNIEnv* env, jobject thiz, jobject view) {
	jfieldID field = (*env)->GetFieldID(env, (*env)->GetObjectClass(env, thiz), "struct", "[B");
	jobject arr = (*env)->GetObjectField(env, thiz, field);
	function_callback(env, arr);
}

LIBUI(void, 00024MySelectListener_onNothingSelected)(JNIEnv* env, jobject thiz, jobject view) {
	// ...
}

LIBUI(void, 00024MySelectListener_onItemSelected)(JNIEnv* env, jobject thiz, jobject parent,
		jobject view, jint position, jlong id) {
	jfieldID field = (*env)->GetFieldID(env, (*env)->GetObjectClass(env, thiz), "struct", "[B");
	jobject arr = (*env)->GetObjectField(env, thiz, field);
	function_callback(env, arr);
}

LIBUI(void, 00024MyRunnable_run)(JNIEnv* env, jobject thiz) {
	jfieldID field = (*env)->GetFieldID(env, (*env)->GetObjectClass(env, thiz), "struct", "[B");
	jobject arr = (*env)->GetObjectField(env, thiz, field);
	function_callback(env, arr);
}

LIBUI(void, callFunction)(JNIEnv *env, jobject thiz, jbyteArray arr) {
	function_callback(env, arr);
}

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