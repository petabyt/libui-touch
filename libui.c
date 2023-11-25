#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <android/log.h>
#include <stdlib.h>
#include <stdarg.h>
#include <jni.h>

#include "ui.h"
#include "ui_android.h"

static void view_set_view_enabled(jobject view, int b) {
	JNIEnv *env = uilib.env;
	jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, view), "setEnabled", "(Z)V");
	(*env)->CallVoidMethod(env, view, method, b);
}

static void view_set_visibility(jobject view, int v) {
	// 0 = visible
	// 4 = invisible
	// 8 = gone
	JNIEnv *env = uilib.env;
	jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, view), "setVisibility", "(I)V");
	(*env)->CallVoidMethod(env, view, method, v);
}

static void view_set_dimensions(jobject view, int w, int h) {
	(*uilib.env)->CallStaticVoidMethod(
		uilib.env, uilib.class, uilib.set_dimensions_m,
		view, w, h
	);
}

void uiBoxSetPadded(uiBox *b, int padded) {
	int p = 10 * padded;

	(*uilib.env)->CallStaticVoidMethod(
		uilib.env, uilib.class, uilib.set_padding_m,
		b->c.o, p, p, p, p
	);
}

static inline struct uiAndroidControl *new_view_control(int signature) {
	struct uiAndroidControl *b = calloc(1, sizeof(struct uiAndroidControl));
	b->c.Signature = signature;
	b->c.Disable = uiControlDisable;
	b->c.Enable = uiControlEnable;
	return b;
}

static uiBox *new_uibox(int type) {
	struct uiAndroidControl *b = new_view_control(uiBoxSignature);
	jobject box = (*uilib.env)->CallStaticObjectMethod(
		uilib.env, uilib.class, uilib.layout_m,
		type
	);
	b->o = box;

	return (uiBox *)b;
}

static int view_set_view_text(jobject view, const char *text) {
	JNIEnv *env = uilib.env;
	jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, view), "setText", "(Ljava/lang/CharSequence;)V");
	(*env)->CallVoidMethod(env, view, method, (*uilib.env)->NewStringUTF(uilib.env, text));
}

static struct uiAndroidControl *view_new_separator() {
	struct uiAndroidControl *c = new_view_control(uiSeparatorSignature);

	JNIEnv *env = uilib.env;
	jclass class = (*env)->FindClass(env, "android/widget/Space");
	jmethodID constructor = (*env)->GetMethodID(env, class, "<init>", "(Landroid/content/Context;)V");
	jobject obj = (*env)->NewObject(env, class, constructor, uilib.ctx);

	c->request_height = 50;

	c->o = obj;
	return c;
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
	struct uiAndroidControl *b = new_view_control(uiButtonSignature);
	jobject button = (*uilib.env)->CallStaticObjectMethod(
		uilib.env, uilib.class, uilib.button_m,
		(*uilib.env)->NewStringUTF(uilib.env, text)
	);
	b->o = button;
	return (uiButton *)b;
}

uiLabel *uiNewLabel(const char *text) {
	struct uiAndroidControl *l = new_view_control(uiLabelSignature);
	jobject label = (*uilib.env)->CallStaticObjectMethod(
		uilib.env, uilib.class, uilib.label_m,
		(*uilib.env)->NewStringUTF(uilib.env, text)
	);
	l->o = label;
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

	jobject tab = (*uilib.env)->CallStaticObjectMethod(
		uilib.env, uilib.class, uilib.tab_layout_m
	);
	t->o = tab;

	return (uiTab *)t;
}

uiProgressBar *uiNewProgressBar() {
	struct uiAndroidControl *c = new_view_control(uiProgressBarSignature);

	JNIEnv *env = uilib.env;
	jclass class = (*env)->FindClass(env, "android/widget/ProgressBar");
	jmethodID constructor = (*env)->GetMethodID(env, class, "<init>", "(Landroid/content/Context;Landroid/util/AttributeSet;I)V");

	jobject obj = (*env)->NewObject(env, class, constructor, uilib.ctx, NULL, progressBarStyleHorizontal);

	c->o = obj;
	return (uiProgressBar *)c;
}

uiWindow *uiNewWindow(const char *title, int width, int height, int hasMenubar) {
	struct uiAndroidControl *c = new_view_control(uiWindowSignature);

	JNIEnv *env = uilib.env;
	jclass class = (*env)->FindClass(env, "libui/LibUI$Popup");

	jmethodID constructor = (*env)->GetMethodID(env, class, "<init>", "(Ljava/lang/String;I)V");

	jobject obj = (*env)->NewObject(env, class, constructor, (*uilib.env)->NewStringUTF(uilib.env, title), 0);

	c->o = obj;

	return (uiWindow *)c;
}

void uiButtonSetText(uiButton *b, const char *text) {
	view_set_view_text(b->c.o, text);
}

void uiLabelSetText(uiLabel *l, const char *text) {
	view_set_view_text(l->c.o, text);	
}

void uiWindowSetChild(uiWindow *w, uiControl *child) {
	JNIEnv *env = uilib.env;
	jclass class = (*env)->FindClass(env, "libui/LibUI$Popup");
	jmethodID m_set_child = (*env)->GetMethodID(env, class, "setChild", "(Landroid/view/View;)V");
	(*env)->CallVoidMethod(env, w->c.o, m_set_child, ((struct uiAndroidControl *)child)->o);
}

uiSeparator *uiNewHorizontalSeparator(void) {
	return (uiSeparator *)view_new_separator();
}

uiSeparator *uiNewVerticalSeparator(void) {
	return (uiSeparator *)view_new_separator();
}

void uiProgressBarSetValue(uiProgressBar *p, int n) {
	JNIEnv *env = uilib.env;
	jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, p->c.o), "setProgress", "(I)V");
	(*env)->CallVoidMethod(env, p->c.o, method, n);
}

void uiBoxAppend(uiBox *b, uiControl *child, int stretchy) {
	struct uiAndroidControl *ctl = (struct uiAndroidControl *)child;
	ctl->o = (*uilib.env)->NewGlobalRef(uilib.env, ctl->o);

	(*uilib.env)->CallStaticVoidMethod(
		uilib.env, uilib.class, uilib.add_view_m,
		b->c.o, ((uiButton *)child)->c.o
	);

	if (ctl->request_width) view_set_dimensions(ctl->o, ctl->request_width, 0);
	if (ctl->request_height) view_set_dimensions(ctl->o, 0, ctl->request_height);
}

void uiButtonOnClicked(uiButton *b, void (*f)(uiButton *sender, void *senderData), void *data) {
	(*uilib.env)->CallStaticVoidMethod(
		uilib.env, uilib.class, uilib.set_click_m,
		b->c.o, (uintptr_t)f, (uintptr_t)b, (uintptr_t)data
	);
}

void uiQueueMain(void (*f)(void *data), void *data) {
	(*uilib.env)->CallStaticVoidMethod(
		uilib.env, uilib.class, uilib.add_runnable_m,
		(uintptr_t)f, data, 0
	);
}

void uiTabAppend(uiTab *t, const char *name, uiControl *c) {
	(*uilib.env)->CallStaticVoidMethod(
		uilib.env, uilib.class, uilib.add_tab_m,
		t->c.o, (*uilib.env)->NewStringUTF(uilib.env, name), ((uiButton *)c)->c.o
	);
}

void uiToast(const char *format, ...) {
    va_list args;
    va_start(args, format);

    char buffer[256];

    vsnprintf(buffer, sizeof(buffer), format, args);
    (*uilib.env)->CallStaticVoidMethod(
        uilib.env, uilib.class, uilib.toast_m,
        (*uilib.env)->NewStringUTF(uilib.env, buffer)
    );

    va_end(args);
}

const char *uiGet(const char *id) {
	jstring *ret = (*uilib.env)->CallStaticObjectMethod(
		uilib.env, uilib.class, uilib.get_string_m,
		(*uilib.env)->NewStringUTF(uilib.env, id)
	);

	const char *c_string = (*uilib.env)->GetStringUTFChars(uilib.env, ret, 0);

	return c_string;
}

int uiAndroidInit(JNIEnv *env, jobject context, jobject parent) {
	uilib.pid = getpid();
	jclass class = (*env)->FindClass(env, "libui/LibUI");
	uilib.class = (*env)->NewGlobalRef(env, class);

	// TODO: Handle exception

	jfieldID ctx_f = (*env)->GetStaticFieldID(env, class, "ctx", "Landroid/content/Context;");
	(*env)->SetStaticObjectField(env, class, ctx_f, context);

	uilib.env = env;
	uilib.ctx = context;

	uilib.button_m = (*env)->GetStaticMethodID(env, class, "button", "(Ljava/lang/String;)Landroid/view/View;");
	uilib.label_m = (*env)->GetStaticMethodID(env, class, "label", "(Ljava/lang/String;)Landroid/view/View;");
	uilib.layout_m = (*env)->GetStaticMethodID(env, class, "linearLayout", "(I)Landroid/view/ViewGroup;");
	uilib.tab_layout_m = (*env)->GetStaticMethodID(env, class, "tabLayout", "()Landroid/view/View;");

	uilib.add_tab_m = (*env)->GetStaticMethodID(env, class, "addTab", "(Landroid/view/View;Ljava/lang/String;Landroid/view/View;)V");
	uilib.add_view_m = (*env)->GetStaticMethodID(env, class, "addView", "(Landroid/view/View;Landroid/view/View;)V");
	uilib.toast_m = (*env)->GetStaticMethodID(env, class, "toast", "(Ljava/lang/String;)V");
	uilib.set_click_m = (*env)->GetStaticMethodID(env, class, "setClickListener", "(Landroid/view/View;JJJ)V");
	uilib.add_runnable_m = (*env)->GetStaticMethodID(env, class, "runRunnable", "(JJJ)V");
	uilib.set_padding_m = (*env)->GetStaticMethodID(env, class, "setPadding", "(Landroid/view/View;IIII)V");
	uilib.get_string_m = (*env)->GetStaticMethodID(env, class, "getString", "(Ljava/lang/String;)Ljava/lang/String;");
	uilib.set_dimensions_m = (*env)->GetStaticMethodID(env, class, "setDimensions", "(Landroid/view/View;II)V");
	
	return 0;
}

uiBox *uiAndroidBox(JNIEnv *env, jobject context, jobject parent) {
	struct uiBox *box = malloc(sizeof(struct uiBox));
	uiAndroidInit(env, context, parent);
	box->c.o = parent;
	return box;
}

#define LIBUI(ret, name) JNIEXPORT ret JNICALL Java_libui_LibUI_##name

LIBUI(void, callFunction)(JNIEnv *env, jobject thiz, jlong ptr, jlong arg1, jlong arg2) {
	void (*ptr_f)(void *, void *) = (void *)ptr;
	ptr_f((void *)arg1, (void *)arg2);
}
