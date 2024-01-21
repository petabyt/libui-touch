#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <android/log.h>
#include <stdlib.h>
#include <stdarg.h>
#include <jni.h>

#include "ui.h"
#include "uifw.h"

// zeroed in bss
static struct UILibAndroidEnv libui = {0};

/*
 * uiAndroidThreadStart(JNIEnv *env, jobject ctx);
 * detect PID , add env & pid to linked list
 *
 * uiNewButton -> get_env(); get_ctx(); -> get env/ctx from PID, abort if not found
 * uiControl *uiNewButton(char *str) {
 *   struct UILibAndroidEnv *z = getj(); -> get from PID
 *
 * uiAndroidThreadEnd();
 * free PID/env/ctx from list
 *
 *
 * Create thread from existing thread
 */

static int check_exception() {
	JNIEnv *env = libui.env;
	if ((*env)->ExceptionCheck(env)) {
		(*env)->ExceptionDescribe(env);
		(*env)->ExceptionClear(env);
		return -1;
	}

	return 0;
}

uiControl *uiControlFromView(jobject obj) {
	// ...
	return NULL;
}

static jobject view_from_ctrl(void *c) {
	return ((struct uiAndroidControl *)c)->o;
}

static void view_set_text_size(jobject obj, float size) {
	JNIEnv *env = libui.env;
	jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, obj), "setTextSize", "(F)V");
	(*env)->CallVoidMethod(env, obj, method, size);
}

jobject view_get_by_id(const char *id) {
	JNIEnv *env = libui.env;
	jmethodID method = (*env)->GetMethodID(env, libui.class, "getView", "(Ljava/lang/String;)Landroid/view/View;");
	jobject view = (*env)->CallObjectMethod(env, libui.class, method,
											(*env)->NewStringUTF(env, id)
	);

	return view;
}

void uiSwitchScreen(uiControl *content, const char *title) {
	JNIEnv *env = libui.env;
	jmethodID method = (*env)->GetStaticMethodID(env, libui.class, "switchScreen", "(Landroid/view/View;Ljava/lang/String;)V");
	(*env)->CallStaticVoidMethod(env, libui.class, method,
								 ((struct uiAndroidControl *)content)->o, (*env)->NewStringUTF(env, title)
	);
}

void ctx_set_content_view(jobject view) {
	JNIEnv *env = libui.env;
	jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, libui.ctx), "setContentView", "(Landroid/view/View;)V");
	(*env)->CallVoidMethod(env, libui.ctx, method, view );
}

void uiAndroidSetContent(uiControl *c) {
	ctx_set_content_view(view_from_ctrl(c));
}

void uiControlCenter(uiControl *c) {
	JNIEnv *env = libui.env;
	const jobject view = view_from_ctrl(c);
	jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, view), "setGravity", "(I)V");
	(*env)->CallVoidMethod(env, view, method, 0x00000010);
}

void uiLabelAlignment(uiControl *c, int align) {
	JNIEnv *env = libui.env;
	const jobject view = view_from_ctrl(c);
	jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, view), "setTextAlignment", "(I)V");
	jint alignment = 0;
	switch (align) {
	case uiDrawTextAlignCenter:
		alignment = 0x00000004;
		break;
	}

	(*env)->CallVoidMethod(env, view, method, alignment);
}

void view_set_view_enabled(jobject view, int b) {
	JNIEnv *env = libui.env;
	jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, view), "setEnabled", "(Z)V");
	(*env)->CallVoidMethod(env, view, method, b);

	check_exception();
}

void uiLabelSetTextSize(uiLabel *l, float size) {
	view_set_text_size(view_from_ctrl(l), size);
}

void uiMultilineEntrySetReadOnly(uiMultilineEntry *e, int readonly) {
	view_set_view_enabled(((uiAndroidControl *)e)->o, readonly);
}

char *uiMultilineEntryText(uiMultilineEntry *e) {
	// GetText returns 'Editable'/CharSequence , toString
}

void view_set_visibility(jobject view, int v) {
	// 0 = visible
	// 4 = invisible
	// 8 = gone
	JNIEnv *env = libui.env;
	jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, view), "setVisibility", "(I)V");
	(*env)->CallVoidMethod(env, view, method, v);
}
 
void view_set_dimensions(jobject view, int w, int h) {
	JNIEnv *env = libui.env;

	jmethodID method = (*env)->GetMethodID(env,
										   (*env)->GetObjectClass(env, view),
										   "getLayoutParams", "()Landroid/view/ViewGroup$LayoutParams;");
	jobject obj = (*env)->CallObjectMethod(env, view, method);

	jclass class = (*env)->GetObjectClass(env, obj);
	jfieldID width_f = (*env)->GetFieldID(env, class, "width", "I");
	jfieldID height_f = (*env)->GetFieldID(env, class, "height", "I");

	if (w != 0)
		(*env)->SetIntField(env, obj, width_f, w);

	if (h != 0)
		(*env)->SetIntField(env, obj, height_f, h);
}

void view_set_layout(jobject view, int x, int y) {
	const JNIEnv *env = libui.env;

	jclass class = (*env)->FindClass(env, "android/widget/LinearLayout$LayoutParams");
	jmethodID constructor = (*env)->GetMethodID(env, class, "<init>", "(II)V");
	jobject obj = (*env)->NewObject(env, class, constructor, x, y);

	jmethodID method = (*env)->GetMethodID(env,
										   (*env)->GetObjectClass(env, view),
										   "setLayoutParams", "(Landroid/view/ViewGroup$LayoutParams;)V");
	(*env)->CallVoidMethod(env, view, method, obj);
}

static void view_set_padding_px(jobject obj, int padding) {
	int p = 10 * padding; // approx dp
	JNIEnv *env = libui.env;
	jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, obj), "setPadding", "(IIII)V");
	(*env)->CallVoidMethod(env, obj, method, p, p, p, p);
}

void uiBoxSetPadded(uiBox *b, int padded) {
	view_set_padding_px(b->c.o, padded);
}

void uiFormSetPadded(uiForm *f, int padded) {
	view_set_padding_px(f->c.o, padded);
}

static inline struct uiAndroidControl *new_view_control(int signature) {
	struct uiAndroidControl *b = calloc(1, sizeof(struct uiAndroidControl));
	b->c.Signature = signature;
	b->c.Handle = (uintptr_t (*)(uiControl *))view_from_ctrl;
	b->c.Disable = uiControlDisable;
	b->c.Enable = uiControlEnable;

	// TODO:
	//	void (*Destroy)(uiControl *);
	//	uiControl *(*Parent)(uiControl *);
	//	void (*SetParent)(uiControl *, uiControl *);
	//	int (*Toplevel)(uiControl *);
	//	int (*Visible)(uiControl *);
	//	void (*Show)(uiControl *);
	//	void (*Hide)(uiControl *);
	//	int (*Enabled)(uiControl *);

	return b;
}

static uiBox *new_uibox(int type) {
	struct uiAndroidControl *b = new_view_control(uiBoxSignature);
	jobject box = (*libui.env)->CallStaticObjectMethod(
			libui.env, libui.class, libui.layout_m,
			type
	);
	b->o = box;

	return (uiBox *)b;
}

static void view_set_text(jobject view, const char *text) {
	JNIEnv *env = libui.env;
	jstring jtext = (*libui.env)->NewStringUTF(libui.env, text);
	jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, view), "setText", "(Ljava/lang/CharSequence;)V");
	(*env)->CallVoidMethod(env, view, method, jtext);
	(*env)->DeleteLocalRef(env, jtext);
}

static struct uiAndroidControl *view_new_separator() {
	struct uiAndroidControl *c = new_view_control(uiSeparatorSignature);

	JNIEnv *env = libui.env;
	jclass class = (*env)->FindClass(env, "android/widget/Space");
	jmethodID constructor = (*env)->GetMethodID(env, class, "<init>", "(Landroid/content/Context;)V");
	jobject obj = (*env)->NewObject(env, class, constructor, libui.ctx);

	c->request_height = 50;

	c->o = obj;
	return c;
}

struct uiScroll *uiNewScroll() {
	struct uiAndroidControl *c = new_view_control(uiBoxSignature);

	JNIEnv *env = libui.env;
	jclass class = (*env)->FindClass(env, "android/widget/ScrollView");
	jmethodID constructor = (*env)->GetMethodID(env, class, "<init>", "(Landroid/content/Context;)V");
	jobject obj = (*env)->NewObject(env, class, constructor, libui.ctx);

	c->o = obj;
	return (uiScroll *)c;
}

uiSeparator *uiNewHorizontalSeparator(void) {
	return (uiSeparator *)view_new_separator();
}

uiSeparator *uiNewVerticalSeparator(void) {
	return (uiSeparator *)view_new_separator();
}

void uiControlEnable(uiControl *c) {
	jobject obj = ((struct uiAndroidControl *)c)->o;
	view_set_view_enabled(obj, 1);
}

void uiControlDisable(uiControl *c) {
	jobject obj = ((struct uiAndroidControl *)c)->o;
	view_set_view_enabled(obj, 0);
}

uiButton *uiNewButton(const char *text) {
	JNIEnv *env = libui.env;

	struct uiAndroidControl *b = new_view_control(uiButtonSignature);

	jclass class = (*env)->FindClass(env, "android/widget/Button");
	jmethodID constructor = (*env)->GetMethodID(env, class, "<init>", "(Landroid/content/Context;)V");
	jobject obj = (*env)->NewObject(env, class, constructor, libui.ctx);
	b->o = obj;

	// TODO: condense
#if 1
	jfieldID button_bg_f = (*env)->GetStaticFieldID(env, libui.class, "buttonBackgroundResource", "I");
	jint button_bg = (*env)->GetStaticIntField(env, libui.class, button_bg_f);
	if (button_bg != 0) {
		jmethodID get_drawable_m = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, libui.res), "getDrawable", "(ILandroid/content/res/Resources$Theme;)Landroid/graphics/drawable/Drawable;");

		jobject drawable = (*env)->CallObjectMethod(libui.env, libui.res, get_drawable_m, button_bg, libui.theme);

		jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, obj), "setBackground", "(Landroid/graphics/drawable/Drawable;)V");
		(*env)->CallVoidMethod(env, obj, method, drawable);
	}
#endif

	view_set_text(obj, text);
	view_set_text_size(obj, 14.0);

	return (uiButton *)b;
}

uiLabel *uiNewLabel(const char *text) {
	JNIEnv *env = libui.env;

	struct uiAndroidControl *l = new_view_control(uiLabelSignature);

	jclass class = (*env)->FindClass(env, "android/widget/TextView");
	jmethodID constructor = (*env)->GetMethodID(env, class, "<init>", "(Landroid/content/Context;)V");
	jobject obj = (*env)->NewObject(env, class, constructor, libui.ctx);
	l->o = obj;

	view_set_text(obj, text);
	view_set_text_size(obj, 14.0);

	return (uiLabel *)l;
}

uiBox *uiNewVerticalBox() {
	return new_uibox(1);
}

uiBox *uiNewHorizontalBox() {
	return new_uibox(0);
}

uiTab *uiNewTab() {
	struct uiAndroidControl *t = new_view_control(uiTabSignature);

	jobject tab = (*libui.env)->CallStaticObjectMethod(
			libui.env, libui.class, libui.tab_layout_m
	);
	t->o = tab;

	return (uiTab *)t;
}

uiMultilineEntry *uiNewMultilineEntry() {
	struct uiAndroidControl *c = new_view_control(uiMultilineEntrySignature);

	JNIEnv *env = libui.env;
	jclass class = (*env)->FindClass(env, "android/widget/EditText");
	jmethodID constructor = (*env)->GetMethodID(env, class, "<init>", "(Landroid/content/Context;)V");
	jobject obj = (*env)->NewObject(env, class, constructor, libui.ctx);

	c->o = obj;
	return (uiMultilineEntry *)c;
}

uiEntry *uiNewEntry() {
	struct uiAndroidControl *c = new_view_control(uiEntrySignature);

	JNIEnv *env = libui.env;
	jclass class = (*env)->FindClass(env, "android/widget/EditText");
	jmethodID constructor = (*env)->GetMethodID(env, class, "<init>", "(Landroid/content/Context;)V");
	jobject obj = (*env)->NewObject(env, class, constructor, libui.ctx);

	view_set_layout(obj, ANDROID_LAYOUT_MATCH_PARENT, ANDROID_LAYOUT_WRAP_CONTENT);

	c->o = obj;
	return (uiEntry *)c;
}

uiProgressBar *uiNewProgressBar() {
	struct uiAndroidControl *c = new_view_control(uiProgressBarSignature);
	JNIEnv *env = libui.env;
	jclass class = (*env)->FindClass(env, "android/widget/ProgressBar");
	jmethodID constructor = (*env)->GetMethodID(env, class, "<init>", "(Landroid/content/Context;Landroid/util/AttributeSet;I)V");
	jobject obj = (*env)->NewObject(env, class, constructor, libui.ctx, NULL, ANDROID_progressBarStyleHorizontal);
	c->o = obj;
	return (uiProgressBar *)c;
}

uiWindow *uiNewWindow(const char *title, int width, int height, int hasMenubar) {
	struct uiAndroidControl *c = new_view_control(uiWindowSignature);

	JNIEnv *env = libui.env;
	jclass class = (*env)->FindClass(env, "libui/LibUI$Popup");
	jmethodID constructor = (*env)->GetMethodID(env, class, "<init>", "(Ljava/lang/String;I)V");
	jobject obj = (*env)->NewObject(env, class, constructor, (*libui.env)->NewStringUTF(libui.env, title), 0);
	c->o = obj;

	return (uiWindow *)c;
}

uiGroup *uiNewGroup(const char *title) {
	struct uiAndroidControl *f = new_view_control(uiGroupSignature);

	jobject form = (*libui.env)->CallStaticObjectMethod(
			libui.env, libui.class, libui.form_m,
			(*libui.env)->NewStringUTF(libui.env, title)
	);
	f->o = form;

	return (uiGroup *)f;
}

void uiGroupSetMargined(uiGroup *g, int margined) {}

void uiGroupSetChild(uiGroup *g, uiControl *c) {
	uiBoxAppend((uiBox *)g, c, 0);
}

uiForm *uiNewForm() {
	return (uiForm *)uiNewVerticalBox();
}

void uiFormAppend(uiForm *f, const char *label, uiControl *c, int stretchy) {
	(*libui.env)->CallStaticVoidMethod(
			libui.env, libui.class, libui.form_add_m,
			f->c.o, (*libui.env)->NewStringUTF(libui.env, label), ((uiAndroidControl *)c)->o
	);
}

void uiButtonSetText(uiButton *b, const char *text) {
	view_set_text(b->c.o, text);
}

void uiLabelSetText(uiLabel *l, const char *text) {
	view_set_text(l->c.o, text);
}

void uiEntrySetText(uiEntry *e, const char *text) {
	view_set_text(e->c.o, text);
}

void uiWindowSetChild(uiWindow *w, uiControl *child) {
	JNIEnv *env = libui.env;
	jclass class = (*env)->FindClass(env, "libui/LibUI$Popup");
	jmethodID m_set_child = (*env)->GetMethodID(env, class, "setChild", "(Landroid/view/View;)V");
	(*env)->CallVoidMethod(env, w->c.o, m_set_child, ((struct uiAndroidControl *)child)->o);
}

void uiProgressBarSetValue(uiProgressBar *p, int n) {
	JNIEnv *env = libui.env;
	jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, p->c.o), "setProgress", "(I)V");
	(*env)->CallVoidMethod(env, p->c.o, method, n);
}

void uiBoxAppend(uiBox *b, uiControl *child, int stretchy) {
	struct uiAndroidControl *ctl = (struct uiAndroidControl *)child;
	ctl->o = (*libui.env)->NewGlobalRef(libui.env, ctl->o);

	JNIEnv *env = libui.env;
	jclass class = (*env)->FindClass(env, "android/view/ViewGroup");
	jmethodID add_view = (*env)->GetMethodID(env, class, "addView", "(Landroid/view/View;)V");

	(*env)->CallVoidMethod(env, b->c.o, add_view, ((struct uiAndroidControl *)child)->o);

	// Controls can optionally request to be set a certain width (only can be set after appending)
	if (ctl->request_width) view_set_dimensions(ctl->o, ctl->request_width, 0);
	if (ctl->request_height) view_set_dimensions(ctl->o, 0, ctl->request_height);
}

void uiButtonOnClicked(uiButton *b, void (*f)(uiButton *sender, void *senderData), void *data) {
	(*libui.env)->CallStaticVoidMethod(
			libui.env, libui.class, libui.set_click_m,
			b->c.o, (uintptr_t)f, (uintptr_t)b, (uintptr_t)data
	);
}

void uiQueueMain(void (*f)(void *data), void *data) {
	(*libui.env)->CallStaticVoidMethod(
			libui.env, libui.class, libui.add_runnable_m,
			(uintptr_t)f, data, 0
	);
}

void uiTabAppend(uiTab *t, const char *name, uiControl *c) {
	((uiAndroidControl *)c)->o = (*libui.env)->NewGlobalRef(libui.env, ((uiAndroidControl *)c)->o);

	jstring jname = (*libui.env)->NewStringUTF(libui.env, name);
	(*libui.env)->CallStaticVoidMethod(
			libui.env, libui.class, libui.add_tab_m,
			t->c.o, jname, ((uiAndroidControl *)c)->o
	);

	(*libui.env)->DeleteLocalRef(libui.env, jname);
}

void uiToast(const char *format, ...) {
	va_list args;
	va_start(args, format);
	char buffer[256];
	vsnprintf(buffer, sizeof(buffer), format, args);
	jstring jbuffer = (*libui.env)->NewStringUTF(libui.env, buffer);
	(*libui.env)->CallStaticVoidMethod(
			libui.env, libui.class, libui.toast_m,
			jbuffer
	);

	(*libui.env)->DeleteLocalRef(libui.env, jbuffer);

	va_end(args);
}

static jint get_res_id(const char *key, const char *name) {
	const JNIEnv *env = libui.env;
	jobject res = libui.res;

	jmethodID get_identifier = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, res), "getIdentifier", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I");

	jint id = (*env)->CallIntMethod(
			libui.env, res, get_identifier,
			(*env)->NewStringUTF(env, name), (*env)->NewStringUTF(env, key), libui.package
	);

	return id;
}

static jobject get_drawable_id(const char *name) {
	const JNIEnv *env = libui.env;
	jobject res = libui.res;

	int id = get_res_id("drawable", name);
	if (id == 0) return NULL;

	jmethodID get_drawable = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, res), "getDrawable", "(ILandroid/content/res/Resources$Theme;)Landroid/graphics/drawable/Drawable;");

	jobject drawable = (*env)->CallObjectMethod(
			libui.env, res, get_drawable, id, libui.theme
	);

	return drawable;
}

const char *uiGet(const char *name) {
	const JNIEnv *env = libui.env;
	jobject res = libui.res;

	int id = get_res_id("string", name);
	if (id == 0) return (const char *)"NULL";

	jmethodID get_string = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, res), "getString", "(I)Ljava/lang/String;");

	jstring val = (*env)->CallObjectMethod(
			libui.env, res, get_string, id
	);

	const char *c_string = (*libui.env)->GetStringUTFChars(libui.env, val, 0);

	// Memory will be leaked
	// env->ReleaseStringUTFChars(str, utf_string);

	return c_string;
}

uintptr_t uiControlHandle(uiControl *c) {
	return (uintptr_t)((uiAndroidControl *)c)->o;
}

void uiControlSetAttr(uiControl *c, const char *key, const char *value) {
	JNIEnv *env = libui.env;
	if (!strcmp(key, "drawable")) {
		jobject drawable = get_drawable_id(value);
		if (drawable == NULL) return;

		jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, view_from_ctrl(c)), "setBackground", "(Landroid/graphics/drawable/Drawable;)V");
		(*env)->CallVoidMethod(env, view_from_ctrl(c), method, drawable);
	}
}

int uiAndroidInit(JNIEnv *env, jobject context) {
	libui.pid = getpid();
	libui.env = env;
	libui.ctx = (*env)->NewGlobalRef(env, context);

	jmethodID get_package_name = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, context), "getPackageName", "()Ljava/lang/String;");
	libui.package = (*env)->CallObjectMethod(env, context, get_package_name);

	jmethodID get_res = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, context), "getResources", "()Landroid/content/res/Resources;");
	libui.res = (*env)->CallObjectMethod(libui.env, libui.ctx, get_res);

	jmethodID get_theme = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, context), "getTheme", "()Landroid/content/res/Resources$Theme;");
	libui.theme = (*env)->CallObjectMethod(libui.env, libui.ctx, get_theme);

	libui.theme = (*env)->NewGlobalRef(env, libui.theme);
	libui.res = (*env)->NewGlobalRef(env, libui.res);
	libui.package = (*env)->NewGlobalRef(env, libui.package);

	jclass class = (*env)->FindClass(env, "libui/LibUI");
	libui.class = (*env)->NewGlobalRef(env, class);

	// TODO: Handle exception

	jfieldID ctx_f = (*env)->GetStaticFieldID(env, class, "ctx", "Landroid/content/Context;");
	(*env)->SetStaticObjectField(env, class, ctx_f, context);

	//jfieldID action_bar_f = (*env)->GetStaticFieldID(env, class, "actionBar", "Landroidx/appcompat/app/ActionBar;");

	libui.form_m = (*env)->GetStaticMethodID(env, class, "form", "(Ljava/lang/String;)Landroid/view/View;");
	libui.layout_m = (*env)->GetStaticMethodID(env, class, "linearLayout", "(I)Landroid/view/ViewGroup;");
	libui.tab_layout_m = (*env)->GetStaticMethodID(env, class, "tabLayout", "()Landroid/view/View;");

	libui.add_tab_m = (*env)->GetStaticMethodID(env, class, "addTab", "(Landroid/view/View;Ljava/lang/String;Landroid/view/View;)V");
	libui.form_add_m = (*env)->GetStaticMethodID(env, class, "formAppend", "(Landroid/view/View;Ljava/lang/String;Landroid/view/View;)V");
	libui.toast_m = (*env)->GetStaticMethodID(env, class, "toast", "(Ljava/lang/String;)V");
	libui.set_click_m = (*env)->GetStaticMethodID(env, class, "setClickListener", "(Landroid/view/View;JJJ)V");

	libui.add_runnable_m = (*env)->GetStaticMethodID(env, class, "runRunnable", "(JJJ)V");

	return 0;
}

uiBox *uiAndroidBox(JNIEnv *env, jobject context, jobject parent) {
	struct uiBox *box = malloc(sizeof(struct uiBox));
	uiAndroidInit(env, context);
	box->c.o = parent;
	return box;
}

#define LIBUI(ret, name) JNIEXPORT ret JNICALL Java_libui_LibUI_##name

LIBUI(void, callFunction)(JNIEnv *env, jobject thiz, uintptr_t ptr, uintptr_t arg1, uintptr_t arg2) {

	// TODO: jlong == long long breaks ptr hack, need to store pointer data in struct -> jbytearray

	void (*ptr_f)(uintptr_t, uintptr_t) = (void *)ptr;
	ptr_f((uintptr_t)arg1, (uintptr_t)arg2);
}
