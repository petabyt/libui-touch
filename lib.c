#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <jni.h>
#include <android/log.h>

#define LIBU(ret, name) JNIEXPORT ret JNICALL Java_libui_LibU_##name

/* TODO:
public void getFilePermissions(Activity ctx) {
	// Require legacy Android write permissions
	if (ContextCompat.checkSelfPermission(ctx, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
		ActivityCompat.requestPermissions(ctx, new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, 1);
	}
}

public static String getDCIM() {
	String mainStorage = Environment.getExternalStorageDirectory().getAbsolutePath();
	return mainStorage + File.separator + "DCIM" + File.separator;
}

public static String getDocuments() {
	String mainStorage = Environment.getExternalStorageDirectory().getAbsolutePath();
	return mainStorage + File.separator + "Documents" + File.separator;
}

public static JSONObject getJSONSettings(Activity ctx, String key) throws Exception {
	SharedPreferences prefs = ctx.getSharedPreferences(ctx.getPackageName(), Context.MODE_PRIVATE);

	String value = prefs.getString(ctx.getPackageName() + "." + key, null);
	if (value == null) return null;

	return new JSONObject(value);
}

public static void storeJSONSettings(Activity ctx, String key, String value) throws Exception {
	SharedPreferences prefs = ctx.getSharedPreferences(ctx.getPackageName(), Context.MODE_PRIVATE);
	prefs.edit().putString(ctx.getPackageName() + "." + key, value).apply();
}
*/

void *libu_get_assets_file(JNIEnv *env, jobject ctx, char *filename, int *length) {
	jclass class = (*env)->FindClass(env, "libui/LibU");

#if 0
	InputStream inputStream = ctx.getAssets().open(file);
	byte buffer[] = new byte[inputStream.available()];
	inputStream.read(buffer);
	inputStream.close();
	return buffer;
#endif

	jmethodID method = (*env)->GetStaticMethodID(env, class, "readFileFromAssets", "(Landroid/content/Context;Ljava/lang/String;)[B");

	jstring jfile = (*env)->NewStringUTF(env, filename);

	jbyteArray array = (*env)->CallStaticObjectMethod(env, class, method,
													  ctx, jfile
	);

	jbyte *bytes = (*env)->GetByteArrayElements(env, array, 0);

	(*length) = (*env)->GetArrayLength(env, array);

	void *new = malloc(*length);
	memcpy(new, bytes, *length);

	(*env)->DeleteLocalRef(env, jfile);
	(*env)->ReleaseByteArrayElements(env, array, bytes, 0);

	return new;
}

void *libu_get_txt_file(JNIEnv *env, jobject ctx, char *filename) {
	int length = 0;
	char *bytes = libu_get_assets_file(env, ctx, filename, &length);
	bytes = realloc(bytes, length + 1);
	bytes[length] = '\0';
	return bytes;
}

const char *ui_get_external_storage_path(JNIEnv *env) {
	// Get File object for the external storage directory.
	jclass environment_c = (*env)->FindClass(env, "android/os/Environment");
	jmethodID method = (*env)->GetStaticMethodID(env, environment_c, "getExternalStorageDirectory", "()Ljava/io/File;");
	jobject file_obj = (*env)->CallStaticObjectMethod(env, environment_c, method);

	jmethodID get_path_m = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, file_obj), "getAbsolutePath", "()Ljava/lang/String;");
	jstring path = (*env)->CallObjectMethod(env, file_obj, get_path_m);

	(*env)->DeleteLocalRef(env, file_obj);

	return (*env)->GetStringUTFChars(env, path, 0);
}


// Added in POSIX 2008, not C standard
char *stpcpy(char *dst, const char *src) {
	const size_t len = strlen(src);
	return (char *)memcpy (dst, src, len + 1) + len;
}