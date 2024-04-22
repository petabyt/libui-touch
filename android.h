#ifndef UIFW_ANDROID_H
#define UIFW_ANDROID_H

#include <jni.h>

// https://android.googlesource.com/platform/frameworks/base/+/2ca2c87/core/res/res/values/public.xml
#define ANDROID_progressBarStyleHorizontal 0x01010078

#define ANDROID_LAYOUT_MATCH_PARENT 0xffffffff
#define ANDROID_LAYOUT_WRAP_CONTENT 0xfffffffe

#define ANDROID_simple_spinner_dropdown_item 0x01090009
#define ANDROID_simple_spinner_item 0x01090008

#define ANDROID_MODE_PRIVATE 0x0

static inline jobject jni_get_package_name(JNIEnv *env, jobject context) {
	jmethodID get_package_name = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, context), "getPackageName", "()Ljava/lang/String;");
	return (*env)->CallObjectMethod(env, context, get_package_name);
}

static inline jobject jni_get_layout_inflater(JNIEnv *env, jobject context) {
	jmethodID get_inflater = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, context), "getLayoutInflater", "()Landroid/view/LayoutInflater;");
	return (*env)->CallObjectMethod(env, context, get_inflater);
}

static inline jobject jni_get_resources(JNIEnv *env, jobject context) {
	jmethodID get_res = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, context), "getResources", "()Landroid/content/res/Resources;");
	return (*env)->CallObjectMethod(env, context, get_res);
}

static inline jobject jni_get_theme(JNIEnv *env, jobject context) {
	jmethodID get_theme = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, context), "getTheme", "()Landroid/content/res/Resources$Theme;");
	return (*env)->CallObjectMethod(env, context, get_theme);
}

static inline jstring jni_concat_strings2(JNIEnv *env, jstring a, jstring b) {
	const char *a_ascii = (*env)->GetStringUTFChars(env, a, NULL);
	const char *b_ascii = (*env)->GetStringUTFChars(env, b, NULL);

	char *result = malloc(strlen(a_ascii) + strlen(b_ascii) + 1);
	strcpy(result, a_ascii);
	strcat(result, b_ascii);

	jstring result_s = (*env)->NewStringUTF(env, result);

	(*env)->ReleaseStringUTFChars(env, a, a_ascii);
	(*env)->ReleaseStringUTFChars(env, b, b_ascii);

	free(result);

	return result_s;
}

static inline jstring jni_concat_strings3(JNIEnv *env, jstring a, jstring b, jstring c) {
	const char *a_ascii = (*env)->GetStringUTFChars(env, a, NULL);
	const char *b_ascii = (*env)->GetStringUTFChars(env, b, NULL);
	const char *c_ascii = (*env)->GetStringUTFChars(env, c, NULL);

	char *result = malloc(strlen(a_ascii) + strlen(b_ascii) + strlen(c_ascii) + 1);
	strcpy(result, a_ascii);
	strcat(result, b_ascii);
	strcat(result, c_ascii);

	jstring result_s = (*env)->NewStringUTF(env, result);

	(*env)->ReleaseStringUTFChars(env, a, a_ascii);
	(*env)->ReleaseStringUTFChars(env, b, b_ascii);
	(*env)->ReleaseStringUTFChars(env, c, c_ascii);

	free(result);

	return result_s;
}

jobject jni_set_pref_str(JNIEnv *env, jobject ctx, char *key, char *str);
jobject jni_get_pref(JNIEnv *env, jobject ctx, char *key);

#endif