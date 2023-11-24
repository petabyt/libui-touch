#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <android/log.h>
#include <stdlib.h>
#include <jni.h>

#include "ui.h"
#include "ui_android.h"

uiButton *uiNewButton(const char *text) {
	struct uiButton *b = malloc(sizeof(struct uiButton));
	b->c.c.Signature = uiButtonSignature;
	jobject button = (*uilib.env)->CallStaticObjectMethod(
		uilib.env, uilib.class, uilib.button_m,
		(*uilib.env)->NewStringUTF(uilib.env, text)
	);
	b->c.o = button;
	return b;
}

uiLabel *uiNewLabel(const char *text) {
	struct uiLabel *b = malloc(sizeof(struct uiLabel));
	b->c.c.Signature = uiLabelSignature;
	jobject label = (*uilib.env)->CallStaticObjectMethod(
		uilib.env, uilib.class, uilib.label_m,
		(*uilib.env)->NewStringUTF(uilib.env, text)
	);
	b->c.o = label;
	return b;
}

uiBox *uiNewVerticalBox() {
	struct uiAndroidControl *b = malloc(sizeof(struct uiAndroidControl));	
	b->c.Signature = uiBoxSignature;
	jobject box = (*uilib.env)->CallStaticObjectMethod(
		uilib.env, uilib.class, uilib.layout_m,
		1
	);
	b->o = box;

	return (uiBox *)b;
}

uiTab *uiNewTab() {
	struct uiTab *t = malloc(sizeof(struct uiTab));	

	jobject tab = (*uilib.env)->CallStaticObjectMethod(
		uilib.env, uilib.class, uilib.tab_layout_m
	);
	t->c.o = tab;

	return t;
}

void uiBoxAppend(uiBox *b, uiControl *child, int stretchy) {
	(*uilib.env)->CallStaticVoidMethod(
		uilib.env, uilib.class, uilib.add_view_m,
		b->c.o, ((uiButton *)child)->c.o
	);
}

void uiButtonOnClicked(uiButton *b, void (*f)(uiButton *sender, void *senderData), void *data) {
	(*uilib.env)->CallStaticVoidMethod(
		uilib.env, uilib.class, uilib.set_click_m,
		b->c.o, (uintptr_t)f, (uintptr_t)data
	);
}

void uiTabAppend(uiTab *t, const char *name, uiControl *c) {
	(*uilib.env)->CallStaticVoidMethod(
		uilib.env, uilib.class, uilib.add_tab_m,
		t->c.o, (*uilib.env)->NewStringUTF(uilib.env, name), ((uiButton *)c)->c.o
	);
}

void uiBoxSetPadded(uiBox *b, int padded) {
	int p = 10 * padded;

	(*uilib.env)->CallStaticVoidMethod(
		uilib.env, uilib.class, uilib.set_padding_m,
		b->c.o, p, p, p, p
	);
}

void uiToast(const char *text) {
	(*uilib.env)->CallStaticVoidMethod(
		uilib.env, uilib.class, uilib.toast_m,
		(*uilib.env)->NewStringUTF(uilib.env, text)
	);	
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
	uilib.set_click_m = (*env)->GetStaticMethodID(env, class, "setClickListener", "(Landroid/view/View;JJ)V");
	uilib.set_padding_m = (*env)->GetStaticMethodID(env, class, "setPadding", "(Landroid/view/View;IIII)V");
	uilib.get_string_m = (*env)->GetStaticMethodID(env, class, "getString", "(Ljava/lang/String;)Ljava/lang/String;");

	return 0;
}

uiBox *uiAndroidBox(JNIEnv *env, jobject context, jobject parent) {
	struct uiBox *box = malloc(sizeof(struct uiBox));
	uiAndroidInit(env, context, parent);
	box->c.o = parent;
	return box;
}

#define LIBUI(ret, name) JNIEXPORT ret JNICALL Java_libui_LibUI_##name

LIBUI(void, callFunction)(JNIEnv *env, jobject thiz, jlong ptr, jlong arg) {
	void (*ptr_f)(void *) = (void *)ptr;
	ptr_f((void *)arg);
}

#define FUNC(ret, name) JNIEXPORT ret JNICALL Java_dev_petabyt_camcontrol_Testbed_##name

static void clickevent(uiButton *button, void *arg) {
	uiToast("Hello");
}

static int view_set_view_text(jobject view, const char *text) {
	JNIEnv *env = uilib.env;
	jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, view), "setText", "(Ljava/lang/CharSequence;)V");
	(*env)->CallVoidMethod(env, view, method, (*uilib.env)->NewStringUTF(uilib.env, text));
}

static int view_set_view_enabled(jobject view, int b) {
	JNIEnv *env = uilib.env;
	jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, view), "setEnabled", "(Z)V");
	(*env)->CallVoidMethod(env, view, method, b);
}

static int view_set_view_visible(jobject view, int v) {
	// 0 = visible
	// 4 = invisible
	// 8 = gone
	JNIEnv *env = uilib.env;
	jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, view), "setVisibility", "(I)V");
	(*env)->CallVoidMethod(env, view, method, v);
}

void uiButtonSetText(uiButton *b, const char *text) {
	view_set_view_text(b->c.o, text);
}

void uiLabelSetText(uiLabel *l, const char *text) {
	view_set_view_text(l->c.o, text);	
}

FUNC(jint, startMyUI)(JNIEnv *env, jobject thiz, jobject context, jobject view) {
	struct uiBox *frame = uiAndroidBox(env, context, view);

	// ---

	uiTab *tab;
	uiButton *btn;

	uiBox *box1 = uiNewVerticalBox();
	uiBoxSetPadded(box1, 1);

	btn = uiNewButton("Hello page 1");
	uiButtonOnClicked(btn, clickevent, 0);
	uiBoxAppend(box1, uiControl(btn), 0);

	uiLabel *lbl = uiNewLabel("New UI label\nnewline test");
	uiBoxAppend(box1, uiControl(lbl), 0);

	uiButtonSetText(btn, "Changed text");
	uiLabelSetText(lbl, "Text has been changed");

	view_set_view_enabled(btn->c.o, 0);
	view_set_view_visible(btn->c.o, 0);

	btn = uiNewButton("Another button page 1");
	uiBoxAppend(box1, uiControl(btn), 0);

	uiBox *box2 = uiNewVerticalBox();
	uiBoxSetPadded(box2, 1);

	btn = uiNewButton("Hello");

	uiBoxAppend(box2, uiControl(btn), 0);
	uiButtonOnClicked(btn, clickevent, 0);

	btn = uiNewButton("asd");
	uiBoxAppend(box2, uiControl(btn), 0);

	tab = uiNewTab();
	uiTabAppend(tab, "tab1", uiControl(box1));
	uiTabAppend(tab, "tab2", uiControl(box2));

	uiBoxAppend(frame, uiControl(tab), 0);

	//	---

	return 0;
}
