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

enum AndroidViewVisibilities {
	ANDROID_VIEW_VISIBLE = 0,
	ANDROID_VIEW_INVISIBLE = 4,
	ANDROID_VIEW_GONE = 9,
};

jobject jni_get_package_name(JNIEnv *env, jobject context);
jobject jni_get_layout_inflater(JNIEnv *env, jobject context);
jobject jni_get_resources(JNIEnv *env, jobject context);
jobject jni_get_theme(JNIEnv *env, jobject context);
jstring jni_concat_strings2(JNIEnv *env, jstring a, jstring b);
jstring jni_concat_strings3(JNIEnv *env, jstring a, jstring b, jstring c);

/**
 * Get a file from the 'assets' directory of the app
 * @param env
 * @param ctx
 * @param filename Plain filename
 * @param length Will store length of file here
 * @return
 */
void *jni_get_assets_file(JNIEnv *env, jobject ctx, const char *filename, int *length);
/**
 * Extension of jni_get_assets_file, returns a NULL-terminated text file
 * @param env
 * @param ctx
 * @param filename
 * @return
 */
void *jni_get_txt_file(JNIEnv *env, jobject ctx, const char *filename);

jobject jni_get_pref_str(JNIEnv *env, jobject ctx, char *key);
jint jni_get_pref_int(JNIEnv *env, jobject ctx, char *key);
void jni_set_pref_str(JNIEnv *env, jobject ctx, char *key, char *str);
void jni_set_pref_int(JNIEnv *env, jobject ctx, char *key, int x);
const char *jni_get_string(JNIEnv *env, jobject ctx, const char *id);

// view.c
jobject jni_get_main_looper(JNIEnv *env);
void view_set_text_size(JNIEnv *env, jobject obj, float size);
jint view_get_res_id(JNIEnv *env, jobject ctx, const char *key, const char *name);
jobject view_get_by_id(JNIEnv *env, jobject ctx, const char *id);
void ctx_set_content_layout(JNIEnv *env, jobject ctx, const char *name);
void ctx_set_content_view(JNIEnv *env, jobject ctx, jobject view);
void view_set_visibility(JNIEnv *env, jobject view, int v);
void view_set_dimensions(JNIEnv *env, jobject view, int w, int h);
void view_set_layout(JNIEnv *env, jobject view, int x, int y);
void view_set_padding_px(JNIEnv *env, jobject obj, int padding);
void view_set_focusable(JNIEnv *env, jobject obj);
char *view_get_text(JNIEnv *env, jobject view);
void view_set_text(JNIEnv *env, jobject view, const char *text);
jobject combobox_get_adapter(JNIEnv *env, jobject ctx, jobject view);
jobject get_drawable_id(JNIEnv *env, jobject ctx, const char *name);
jobject view_expand(JNIEnv *env, jobject ctx, const char *name);
jobject view_new_tabhost(JNIEnv *env, jobject ctx);

// callback.c
jclass get_click_listener_class(JNIEnv *env);

#endif