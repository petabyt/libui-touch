#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <jni.h>
#include <android/log.h>
#include "android.h"

jobject jni_get_display_metrics(JNIEnv *env, jobject ctx) {
	jclass cls_activity = (*env)->FindClass(env, "android/app/Activity");
	jclass cls_window_manager = (*env)->FindClass(env, "android/view/WindowManager");
	jclass cls_display_metrics = (*env)->FindClass(env, "android/util/DisplayMetrics");
	jclass cls_display = (*env)->FindClass(env, "android/view/Display");

	jmethodID mid_get_window_manager = (*env)->GetMethodID(env, cls_activity, "getWindowManager", "()Landroid/view/WindowManager;");
	jmethodID mid_get_default_display = (*env)->GetMethodID(env, cls_window_manager, "getDefaultDisplay", "()Landroid/view/Display;");
	jmethodID mid_get_metrics = (*env)->GetMethodID(env, cls_display, "getMetrics", "(Landroid/util/DisplayMetrics;)V");

	jobject wm = (*env)->CallObjectMethod(env, ctx, mid_get_window_manager);
	jobject display = (*env)->CallObjectMethod(env, wm, mid_get_default_display);

	jobject display_metrics = (*env)->NewObject(env, cls_display_metrics, (*env)->GetMethodID(env, cls_display_metrics, "<init>", "()V"));

	(*env)->CallVoidMethod(env, display, mid_get_metrics, display_metrics);
	return display_metrics;
}

jobject jni_get_main_looper(JNIEnv *env) {
	jclass c = (*env)->FindClass(env, "android/os/Looper");
	jmethodID m = (*env)->GetStaticMethodID(env, c, "getMainLooper", "()Landroid/os/Looper;");
	return (*env)->CallStaticObjectMethod(env, c, m);
}

jobject jni_get_package_name(JNIEnv *env, jobject context) {
	jmethodID get_package_name = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, context), "getPackageName", "()Ljava/lang/String;");
	return (*env)->CallObjectMethod(env, context, get_package_name);
}

jobject jni_get_layout_inflater(JNIEnv *env, jobject context) {
	jmethodID get_inflater = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, context), "getLayoutInflater", "()Landroid/view/LayoutInflater;");
	return (*env)->CallObjectMethod(env, context, get_inflater);
}

jobject jni_get_resources(JNIEnv *env, jobject context) {
	jmethodID get_res = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, context), "getResources", "()Landroid/content/res/Resources;");
	return (*env)->CallObjectMethod(env, context, get_res);
}

jobject jni_get_theme(JNIEnv *env, jobject context) {
	jmethodID get_theme = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, context), "getTheme", "()Landroid/content/res/Resources$Theme;");
	return (*env)->CallObjectMethod(env, context, get_theme);
}

jstring jni_concat_strings2(JNIEnv *env, jstring a, jstring b) {
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

jstring jni_concat_strings3(JNIEnv *env, jstring a, jstring b, jstring c) {
	const char *a_ascii = (*env)->GetStringUTFChars(env, a, NULL);
	const char *b_ascii = (*env)->GetStringUTFChars(env, b, NULL);
	const char *c_ascii = (*env)->GetStringUTFChars(env, c, NULL);

	char *result = malloc(strlen(a_ascii) + strlen(b_ascii) + strlen(c_ascii) + 1);
	strcpy(result, a_ascii);
	strcat(result, b_ascii);
	strcat(result, c_ascii);

	void plat_dbg(char *fmt, ...);

	jstring result_s = (*env)->NewStringUTF(env, result);

	(*env)->ReleaseStringUTFChars(env, a, a_ascii);
	(*env)->ReleaseStringUTFChars(env, b, b_ascii);
	(*env)->ReleaseStringUTFChars(env, c, c_ascii);

	free(result);

	return result_s;
}

void *jni_get_assets_file(JNIEnv *env, jobject ctx, const char *filename, int *length) {
	(*env)->PushLocalFrame(env, 10);
	jmethodID get_assets_m = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, ctx), "getAssets", "()Landroid/content/res/AssetManager;");
	jobject asset_manager = (*env)->CallObjectMethod(env, ctx, get_assets_m);

	jstring jfile = (*env)->NewStringUTF(env, filename);

	jmethodID open_m = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, asset_manager), "open", "(Ljava/lang/String;)Ljava/io/InputStream;");
	jobject input_stream = (*env)->CallObjectMethod(env, asset_manager, open_m, jfile);

	jmethodID close_m = (*env)->GetMethodID(env, (*env)->FindClass(env, "java/io/InputStream"), "close", "()V");
	jmethodID read_m = (*env)->GetMethodID(env, (*env)->FindClass(env, "java/io/InputStream"), "read", "([B)I");
	jmethodID available_m = (*env)->GetMethodID(env, (*env)->FindClass(env, "java/io/InputStream"), "available", "()I");

	int file_size = (*env)->CallIntMethod(env, input_stream, available_m);

	jbyteArray buffer = (*env)->NewByteArray(env, file_size);
	(*env)->CallIntMethod(env, input_stream, read_m, buffer);
	(*env)->CallVoidMethod(env, input_stream, close_m);

	jbyte *bytes = (*env)->GetByteArrayElements(env, buffer, 0);
	(*length) = file_size;

	void *new = malloc(*length);
	memcpy(new, bytes, *length);

	(*env)->DeleteLocalRef(env, jfile);
	(*env)->ReleaseByteArrayElements(env, buffer, bytes, 0);

	(*env)->PopLocalFrame(env, NULL);

	return new;
}

void *jni_get_txt_file(JNIEnv *env, jobject ctx, const char *filename) {
	int length = 0;
	char *bytes = jni_get_assets_file(env, ctx, filename, &length);
	bytes = realloc(bytes, length + 1);
	bytes[length] = '\0';
	return bytes;
}

const char *jni_get_external_storage_path(JNIEnv *env) {
	(*env)->PushLocalFrame(env, 10);
	// Get File object for the external storage directory.
	jclass environment_c = (*env)->FindClass(env, "android/os/Environment");
	jmethodID method = (*env)->GetStaticMethodID(env, environment_c, "getExternalStorageDirectory", "()Ljava/io/File;");
	jobject file_obj = (*env)->CallStaticObjectMethod(env, environment_c, method);

	jmethodID get_path_m = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, file_obj), "getAbsolutePath", "()Ljava/lang/String;");
	jstring path = (*env)->CallObjectMethod(env, file_obj, get_path_m);

	(*env)->DeleteLocalRef(env, file_obj);

	path = (*env)->PopLocalFrame(env, path);
	return (*env)->GetStringUTFChars(env, path, 0);
}

jobject jni_get_pref_str(JNIEnv *env, jobject ctx, char *key) {
	(*env)->PushLocalFrame(env, 10);
	jclass shared_pref_c = (*env)->FindClass(env, "android/content/SharedPreferences");
	jmethodID get_string_m = (*env)->GetMethodID(env, shared_pref_c, "getString", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
	jmethodID get_pref_m = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, ctx), "getSharedPreferences", "(Ljava/lang/String;I)Landroid/content/SharedPreferences;");

	jstring package_name_s = jni_get_package_name(env, ctx);

	jobject pref_o = (*env)->CallObjectMethod(env, ctx, get_pref_m, package_name_s, ANDROID_MODE_PRIVATE);

	jstring path = jni_concat_strings3(env, package_name_s, (*env)->NewStringUTF(env, "."), (*env)->NewStringUTF(env, key));
	jstring value = (*env)->CallObjectMethod(env, pref_o, get_string_m, path, NULL);

	value = (*env)->PopLocalFrame(env, value);
	return value;
}

jint jni_get_pref_int(JNIEnv *env, jobject ctx, char *key) {
	(*env)->PushLocalFrame(env, 10);
	jclass shared_pref_c = (*env)->FindClass(env, "android/content/SharedPreferences");
	jmethodID get_string_m = (*env)->GetMethodID(env, shared_pref_c, "getInt", "(Ljava/lang/String;I)I");
	jmethodID get_pref_m = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, ctx), "getSharedPreferences", "(Ljava/lang/String;I)Landroid/content/SharedPreferences;");

	jstring package_name_s = jni_get_package_name(env, ctx);

	jobject pref_o = (*env)->CallObjectMethod(env, ctx, get_pref_m, package_name_s, ANDROID_MODE_PRIVATE);

	jstring path = jni_concat_strings3(env, package_name_s, (*env)->NewStringUTF(env, "."), (*env)->NewStringUTF(env, key));
	jint rc = (*env)->CallIntMethod(env, pref_o, get_string_m, path, -1);

	(*env)->PopLocalFrame(env, NULL);
	return rc;
}

jobject jni_get_pref_editor(JNIEnv *env, jobject ctx) {
	jclass shared_pref_c = (*env)->FindClass(env, "android/content/SharedPreferences");
	jmethodID edit_m = (*env)->GetMethodID(env, shared_pref_c, "edit", "()Landroid/content/SharedPreferences$Editor;");
	jmethodID get_pref_m = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, ctx), "getSharedPreferences", "(Ljava/lang/String;I)Landroid/content/SharedPreferences;");

	jstring package_name_s = jni_get_package_name(env, ctx);

	jobject pref_o = (*env)->CallObjectMethod(env, ctx, get_pref_m, package_name_s, ANDROID_MODE_PRIVATE);

	return (*env)->CallObjectMethod(env, pref_o, edit_m);
}

void jni_set_pref_str(JNIEnv *env, jobject ctx, char *key, char *str) {
	(*env)->PushLocalFrame(env, 10);
	jstring package_name_s = jni_get_package_name(env, ctx);
	jobject editor_o = jni_get_pref_editor(env, ctx);

	jmethodID put_string_m = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, editor_o), "putString", "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/SharedPreferences$Editor;");
	jmethodID apply_m = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, editor_o), "apply", "()V");

	jstring path = jni_concat_strings3(env, package_name_s, (*env)->NewStringUTF(env, "."), (*env)->NewStringUTF(env, key));
	(*env)->CallObjectMethod(env, editor_o, put_string_m, path, (*env)->NewStringUTF(env, str));
	(*env)->CallVoidMethod(env, editor_o, apply_m);

	(*env)->PopLocalFrame(env, NULL);
}

void jni_set_pref_int(JNIEnv *env, jobject ctx, char *key, int x) {
	(*env)->PushLocalFrame(env, 10);
	jstring package_name_s = jni_get_package_name(env, ctx);
	jobject editor_o = jni_get_pref_editor(env, ctx);

	jmethodID put_string_m = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, editor_o), "putInt", "(Ljava/lang/String;I)Landroid/content/SharedPreferences$Editor;");
	jmethodID apply_m = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, editor_o), "apply", "()V");

	jstring path = jni_concat_strings3(env, package_name_s, (*env)->NewStringUTF(env, "."), (*env)->NewStringUTF(env, key));
	(*env)->CallObjectMethod(env, editor_o, put_string_m, path, x);
	(*env)->CallVoidMethod(env, editor_o, apply_m);

	(*env)->PopLocalFrame(env, NULL);
}

jobject jni_get_application_ctx(JNIEnv *env)
{
	jclass activity_thread = (*env)->FindClass(env,"android/app/ActivityThread");
	jmethodID current_activity_thread = (*env)->GetStaticMethodID(env, activity_thread, "currentActivityThread", "()Landroid/app/ActivityThread;");
	jobject activity_thread_obj = (*env)->CallStaticObjectMethod(env, activity_thread, current_activity_thread);

	jmethodID get_application = (*env)->GetMethodID(env, activity_thread, "getApplication", "()Landroid/app/Application;");
	jobject context = (*env)->CallObjectMethod(env, activity_thread_obj, get_application);
	return context;
}

const char *jni_get_string(JNIEnv *env, jobject ctx, const char *key) {
	(*env)->PushLocalFrame(env, 10);
	jobject res = jni_get_resources(env, ctx);

	int id = view_get_res_id(env, ctx, "string", key);
	if (id == 0) return (const char *)"NULL";

	jmethodID get_string = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, res), "getString", "(I)Ljava/lang/String;");

	jstring val = (*env)->CallObjectMethod(
		env, res, get_string, id
	);

	const char *c_string = (*env)->GetStringUTFChars(env, val, 0);

	// Memory will be leaked
	// env->ReleaseStringUTFChars(str, utf_string);

	(*env)->PopLocalFrame(env, NULL);
	return c_string;
}

// Added in POSIX 2008, not C standard
__attribute__((weak))
char *stpcpy(char *dst, const char *src) {
	const size_t len = strlen(src);
	return (char *)memcpy (dst, src, len + 1) + len;
}