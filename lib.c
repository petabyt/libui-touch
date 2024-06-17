#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <jni.h>
#include <android/log.h>
#include "android.h"

void *jni_get_assets_file(JNIEnv *env, jobject ctx, char *filename, int *length) {
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

void *jni_get_txt_file(JNIEnv *env, jobject ctx, char *filename) {
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

// Added in POSIX 2008, not C standard
__attribute__((weak))
char *stpcpy(char *dst, const char *src) {
	const size_t len = strlen(src);
	return (char *)memcpy (dst, src, len + 1) + len;
}