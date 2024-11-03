#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <android/log.h>
#include <stdlib.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <jni.h>

#include "ui.h"
#include "uifw.h"
#include "uifw_priv.h"
#include "android.h"

#pragma GCC visibility push(internal)

inline jobject uiViewFromControl(void *c) {
	return ((struct uiAndroidControl *)c)->o;
}

static int check_exception(void) {
	JNIEnv *env = get_jni_env();
	if ((*env)->ExceptionCheck(env)) {
		(*env)->ExceptionDescribe(env);
		(*env)->ExceptionClear(env);
		return -1;
	}

	return 0;
}

static jclass libui_class(JNIEnv *env) {
	return (*env)->FindClass(env, "dev/danielc/libui/LibUI");
}

void uiAndroidSetContent(uiControl *c) {
	ctx_set_content_view(get_jni_env(), get_jni_ctx(), uiViewFromControl(c));
}

void uiControlCenter(uiControl *c) {
	JNIEnv *env = get_jni_env();
	const jobject view = uiViewFromControl(c);
	jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, view), "setGravity", "(I)V");
	(*env)->CallVoidMethod(env, view, method, 0x00000010);
}

void uiLabelAlignment(uiControl *c, int align) {
	JNIEnv *env = get_jni_env();
	const jobject view = uiViewFromControl(c);
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
	JNIEnv *env = get_jni_env();
	jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, view), "setEnabled", "(Z)V");
	(*env)->CallVoidMethod(env, view, method, b);

	check_exception();
}

void uiLabelSetTextSize(uiLabel *l, float size) {
	view_set_text_size(get_jni_env(), uiViewFromControl(l), size);
}

void uiMultilineEntrySetReadOnly(uiMultilineEntry *e, int readonly) {
	view_set_view_enabled(((uiAndroidControl *)e)->o, readonly);
}

void uiBoxSetPadded(uiBox *b, int padded) {
	view_set_padding_px(get_jni_env(), b->c.o, padded);
}

int uiBoxPadded(uiBox *b) { return 0; }

void uiControlShow(uiControl *c) {
	if (c->Signature == uiWindowSignature) {
		popupwindow_open(get_jni_env(), get_jni_ctx(), uiViewFromControl(c));
	}
}
void uiControlHide(uiControl *c) {}

static void view_destroy(jobject v) {
	JNIEnv *env = get_jni_env();

	const jboolean is_group = (*env)->IsInstanceOf(env, v, (*env)->FindClass(env, "android/view/ViewGroup"));
	if (is_group) {
		jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, v), "getChildCount", "()I");
		int child_count = (*env)->CallIntMethod(env, v, method);
		jmethodID get_child_at_m = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, v), "getChildAt", "(I)Landroid/view/View;");
		for (int i = 0; i < child_count; i++) {
			jobject child = (*env)->CallObjectMethod(env, v, get_child_at_m, i);
			view_destroy(child);
		}
	}

	jobjectRefType type = (*env)->GetObjectRefType(env, v);
	if (type == JNIWeakGlobalRefType || type == JNIGlobalRefType) {
		(*env)->DeleteGlobalRef(env, v);
	} else if (type == JNILocalRefType) {
		(*env)->DeleteLocalRef(env, v);
	}
}

void uiControlDestroy(uiControl *c) {
	view_destroy(uiViewFromControl((c)));
	free(c); // We only will free the parent wrapper :(
	// TODO: Track children/parents for views
}

void uiFormSetPadded(uiForm *f, int padded) {
	view_set_padding_px(get_jni_env(), f->c.o, padded);
}

static inline struct uiAndroidControl *new_view_control(int signature) {
	struct uiAndroidControl *b = calloc(1, sizeof(struct uiAndroidControl));
	b->c.Signature = signature;
	b->c.Handle = (uintptr_t (*)(uiControl *)) uiViewFromControl;
	b->c.Disable = uiControlDisable;
	b->c.Enable = uiControlEnable;
	b->c.Show = uiControlShow;
	b->c.Hide = uiControlHide;

	b->c.Destroy = uiControlDestroy;

	// TODO:
	//	uiControl *(*Parent)(uiControl *);
	//	void (*SetParent)(uiControl *, uiControl *);
	//	int (*Toplevel)(uiControl *);
	//	int (*Visible)(uiControl *);
	//	int (*Enabled)(uiControl *);

	return b;
}

uiControl *uiControlFromView(jobject obj) {
	JNIEnv *env = get_jni_env();
	int signature = uiLabelSignature;
	if ((*env)->IsInstanceOf(env, obj, (*env)->FindClass(env, "android/widget/Button"))) {
		signature = uiButtonSignature;
	} else if ((*env)->IsInstanceOf(env, obj, (*env)->FindClass(env, "android/widget/TextView"))) {
		signature = uiLabelSignature;
	}

	struct uiAndroidControl *ctl = new_view_control(uiButtonSignature);
	ctl->o = obj;

	return uiControl(ctl);
}

uiControl *uiControlFromID(const char *id) {
	JNIEnv *env = get_jni_env();
	jobject obj = (*env)->NewGlobalRef(env, view_get_by_id(env, get_jni_ctx(), id));
	return (uiControl *)uiControlFromView(obj);
}

static uiBox *new_uibox(int type) {
	JNIEnv *env = get_jni_env();
	struct uiAndroidControl *b = new_view_control(uiBoxSignature);
	b->o = view_new_linearlayout(env, get_jni_ctx(), type, ANDROID_LAYOUT_MATCH_PARENT, ANDROID_LAYOUT_MATCH_PARENT);
	return (uiBox *)b;
}

char *uiMultilineEntryText(uiMultilineEntry *e) {
	return view_get_text(get_jni_env(), uiViewFromControl(e));
}

void uiFreeText(char *text) {
	free(text);
}

static struct uiAndroidControl *view_new_separator() {
	struct uiAndroidControl *c = new_view_control(uiSeparatorSignature);

	JNIEnv *env = get_jni_env();
	jclass class = (*env)->FindClass(env, "android/widget/Space");
	jmethodID constructor = (*env)->GetMethodID(env, class, "<init>", "(Landroid/content/Context;)V");
	jobject obj = (*env)->NewObject(env, class, constructor, get_jni_ctx());

	c->request_height = 50;

	c->o = obj;
	return c;
}

void uiComboboxSetSelected(uiCombobox *c, int index) {
	JNIEnv *env = get_jni_env();

	jobject view = uiViewFromControl(c);

	jmethodID add_m = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, view), "setSelection", "(I)V");
	(*env)->CallVoidMethod(env, view, add_m, (jint)index);
}

int uiComboboxSelected(uiCombobox *c) {
	JNIEnv *env = get_jni_env();

	jobject view = uiViewFromControl(c);

	jmethodID get_m = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, view), "getSelectedItemPosition", "()I");
	return (*env)->CallIntMethod(env, view, get_m);
}

void uiComboboxOnSelected(uiCombobox *c, void (*f)(uiCombobox *sender, void *senderData), void *data) {
	JNIEnv *env = get_jni_env();
	jobject view = uiViewFromControl(c);
	view_add_native_select_listener(env, view, (void *)f, 2, c, data);
}

void uiComboboxClear(uiCombobox *c) {
	JNIEnv *env = get_jni_env();

	jobject view = uiViewFromControl(c);

	jobject adapter = combobox_get_adapter(env, get_jni_ctx(), view);

	jclass adapter_c = (*env)->GetObjectClass(env, adapter);
	jmethodID add_m = (*env)->GetMethodID(env, adapter_c, "clear", "()V");
	(*env)->CallVoidMethod(env, adapter, add_m);

	(*env)->DeleteLocalRef(env, adapter);
}

void uiComboboxAppend(uiCombobox *c, const char *text) {
	JNIEnv *env = get_jni_env();

	jobject view = uiViewFromControl(c);

	jstring jtext = (*env)->NewStringUTF(env, text);

	jobject adapter = combobox_get_adapter(env, get_jni_ctx(), view);

	jclass adapter_c = (*env)->GetObjectClass(env, adapter);
	jmethodID add_m = (*env)->GetMethodID(env, adapter_c, "add", "(Ljava/lang/Object;)V");
	(*env)->CallVoidMethod(env, adapter, add_m, jtext);

	(*env)->DeleteLocalRef(env, jtext);
	(*env)->DeleteLocalRef(env, adapter);
}

struct uiScroll *uiNewScroll(void) {
	struct uiAndroidControl *c = new_view_control(uiBoxSignature);
	JNIEnv *env = get_jni_env();
	jobject obj = view_new_scroll(env, get_jni_ctx());
	view_set_focusable(env, obj);
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
	view_set_view_enabled(uiViewFromControl(c), 1);
}

void uiControlDisable(uiControl *c) {
	view_set_view_enabled(uiViewFromControl(c), 0);
}

// TODO: jnifunc
uiButton *uiNewButton(const char *text) {
	JNIEnv *env = get_jni_env();

	struct uiAndroidControl *b = new_view_control(uiButtonSignature);

	(*env)->PushLocalFrame(env, 10);

	jobject obj = view_new_button(env, get_jni_ctx());

	jfieldID button_bg_f = (*env)->GetStaticFieldID(env, libui_class(env), "buttonBackgroundResource", "I");
	jint button_bg = (*env)->GetStaticIntField(env, libui_class(env), button_bg_f);
	if (button_bg != 0) {
		view_set_button_style(env, get_jni_ctx(), obj, button_bg);
	}

	obj = (*env)->PopLocalFrame(env, obj);
	b->o = obj;

	view_set_text(env, obj, text);
	view_set_text_size(env, obj, 14.0);

	return (uiButton *)b;
}

uiLabel *uiNewLabel(const char *text) {
	JNIEnv *env = get_jni_env();

	struct uiAndroidControl *l = new_view_control(uiLabelSignature);

	jobject obj = view_new_textview(env, get_jni_ctx());
	l->o = obj;

	view_set_text(env, obj, text);
	view_set_text_size(env, obj, 14.0);

	return (uiLabel *)l;
}

char *uiLabelText(uiLabel *l) {return "NULL";}

uiBox *uiNewVerticalBox(void) {
	return new_uibox(1);
}

uiBox *uiNewHorizontalBox(void) {
	return new_uibox(0);
}

void uiMsgBox(uiWindow *parent, const char *title, const char *description) {

}

static int jni_does_class_exist(JNIEnv *env, const char *name) {
	(*env)->FindClass(env, name);
	if ((*env)->ExceptionCheck(env)) {
		(*env)->ExceptionClear(env);
		return 0;
	}

	return 1;
}

uiTab *uiNewTab(void) {
	JNIEnv *env = get_jni_env();
	struct uiAndroidControl *t = new_view_control(uiTabSignature);
	t->o = view_new_tabhost(env, get_jni_ctx());
	return (uiTab *)t;
}

uiMultilineEntry *uiNewMultilineEntry(void) {
	struct uiAndroidControl *c = new_view_control(uiMultilineEntrySignature);

	JNIEnv *env = get_jni_env();
	jclass class = (*env)->FindClass(env, "android/widget/EditText");
	jmethodID constructor = (*env)->GetMethodID(env, class, "<init>", "(Landroid/content/Context;)V");
	jobject obj = (*env)->NewObject(env, class, constructor, get_jni_ctx());

	c->o = obj;
	return (uiMultilineEntry *)c;
}

uiEntry *uiNewEntry() {
	struct uiAndroidControl *c = new_view_control(uiEntrySignature);

	JNIEnv *env = get_jni_env();
	jclass class = (*env)->FindClass(env, "android/widget/EditText");
	jmethodID constructor = (*env)->GetMethodID(env, class, "<init>", "(Landroid/content/Context;)V");
	jobject obj = (*env)->NewObject(env, class, constructor, get_jni_ctx());

	view_set_layout(get_jni_env(), obj, ANDROID_LAYOUT_MATCH_PARENT, ANDROID_LAYOUT_WRAP_CONTENT);

	c->o = obj;
	return (uiEntry *)c;
}

uiProgressBar *uiNewProgressBar() {
	JNIEnv *env = get_jni_env();
	struct uiAndroidControl *c = new_view_control(uiProgressBarSignature);
	jclass class = (*env)->FindClass(env, "android/widget/ProgressBar");
	jmethodID constructor = (*env)->GetMethodID(env, class, "<init>", "(Landroid/content/Context;Landroid/util/AttributeSet;I)V");
	jobject obj = (*env)->NewObject(env, class, constructor, get_jni_ctx(), NULL, ANDROID_progressBarStyleHorizontal);
	c->o = obj;
	return (uiProgressBar *)c;
}

uiWindow *uiNewWindow(const char *title, int width, int height, int hasMenubar) {
	struct uiAndroidControl *c = new_view_control(uiWindowSignature);

	JNIEnv *env = get_jni_env();

	jstring jtitle = (*env)->NewStringUTF(env, title);

	c->o = popupwindow_new(env, get_jni_ctx(), 0);

	return (uiWindow *)c;
}

uiGroup *uiNewGroup(const char *title) {
	JNIEnv *env = get_jni_env();
	struct uiAndroidControl *f = new_view_control(uiGroupSignature);

//	jobject form = (*env)->CallStaticObjectMethod(
//			env, libui_class(env), libui.form_m,
//			(*env)->NewStringUTF(env, title)
//	);
//	f->o = form;

	return (uiGroup *)f;
}

void uiGroupSetMargined(uiGroup *g, int margined) {}

void uiGroupSetChild(uiGroup *g, uiControl *c) {
	uiBoxAppend((uiBox *)g, c, 0);
}

char *uiGroupTitle(uiGroup *g) {
	return "NULL";
}

void uiGroupSetTitle(uiGroup *g, const char *title) {}
int uiGroupMargined(uiGroup *g) {return 0;}

uiForm *uiNewForm() {
	return (uiForm *)uiNewVerticalBox();
}

void uiFormAppend(uiForm *f, const char *label, uiControl *c, int stretchy) {
	JNIEnv *env = get_jni_env();
//	(*env)->CallStaticVoidMethod(
//			env, libui_class(env), libui.form_add_m,
//			f->c.o, (*env)->NewStringUTF(env, label), ((uiAndroidControl *)c)->o
//	);
}

void uiButtonSetText(uiButton *b, const char *text) {
	view_set_text(get_jni_env(), uiViewFromControl(b), text);
}

void uiLabelSetText(uiLabel *l, const char *text) {
	view_set_text(get_jni_env(), uiViewFromControl(l), text);
}

void uiEntrySetText(uiEntry *e, const char *text) {
	view_set_text(get_jni_env(), uiViewFromControl(e), text);
}

void uiMultilineEntrySetText(uiMultilineEntry *e, const char *text) {
	view_set_text(get_jni_env(), uiViewFromControl(e), text);
}

void uiWindowSetChild(uiWindow *w, uiControl *child) {
	JNIEnv *env = get_jni_env();
	if (((struct uiAndroidControl *)w)->is_activity) {
		ctx_set_content_view(env, get_jni_ctx(), uiViewFromControl((child)));
		return;
	}

	popupwindow_set_content(env, uiViewFromControl(w), uiViewFromControl(child));
}

void uiProgressBarSetValue(uiProgressBar *p, int n) {
	JNIEnv *env = get_jni_env();
	jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, p->c.o), "setProgress", "(I)V");
	(*env)->CallVoidMethod(env, p->c.o, method, n);
}

void uiBoxAppend(uiBox *b, uiControl *child, int stretchy) {
	JNIEnv *env = get_jni_env();
	struct uiAndroidControl *ctl = (struct uiAndroidControl *)child;
	ctl->o = (*env)->NewGlobalRef(env, ctl->o);

	viewgroup_addview(env, b->c.o, uiViewFromControl(child));

	// Controls can optionally request to be set a certain width (only can be set after appending)
	if (ctl->request_width) view_set_dimensions(env, ctl->o, ctl->request_width, 0);
	if (ctl->request_height) view_set_dimensions(env, ctl->o, 0, ctl->request_height);
}

uiControl *uiBoxChild(uiBox *box, int index) {
	JNIEnv *env = get_jni_env();
	jclass class = (*env)->FindClass(env, "android/view/ViewGroup");
	jmethodID get_child_m = (*env)->GetMethodID(env, class, "getChildAt", "(I)Landroid/view/View;");

	jobject view = (*env)->CallObjectMethod(env, uiViewFromControl(box), get_child_m, (jint)index);

	jobject viewn = (*env)->NewGlobalRef(env, view);
	(*env)->DeleteLocalRef(env, view);

	struct uiAndroidControl *c = new_view_control(uiBoxSignature);
	c->o = viewn;

	(*env)->DeleteLocalRef(env, class);

	return (uiControl *)c;
}

void uiBoxDelete(uiBox *box, int index) {
	JNIEnv *env = get_jni_env();
	jclass class = (*env)->FindClass(env, "android/view/ViewGroup");
	jmethodID remove_view_m = (*env)->GetMethodID(env, class, "removeViewAt", "(I)V");
	(*env)->DeleteLocalRef(env, class);
	(*env)->CallVoidMethod(env, uiViewFromControl(box), remove_view_m, (jint)index);
}

int uiBoxNumChildren(uiBox *box) {
	JNIEnv *env = get_jni_env();
	jclass class = (*env)->FindClass(env, "android/view/ViewGroup");
	jmethodID get_child_count_m = (*env)->GetMethodID(env, class, "getChildCount", "()I");
	(*env)->DeleteLocalRef(env, class);
	return (*env)->CallIntMethod(env, uiViewFromControl(box), get_child_count_m);
}

void uiButtonOnClicked(uiButton *b, void (*f)(uiButton *sender, void *senderData), void *data) {
	JNIEnv *env = get_jni_env();
	jobject *view = uiViewFromControl(b);
	view_add_native_click_listener(env, view, (void *)f, 2, b, data);
}

void uiQueueMain(void (*f)(void *data), void *data) {
	JNIEnv *env = get_jni_env();
	jni_native_runnable(env, get_jni_ctx(), (void *)f, 1, data, NULL);
}

void uiTabAppend(uiTab *t, const char *name, uiControl *c) {
	JNIEnv *env = get_jni_env();
	((uiAndroidControl *)c)->o = (*env)->NewGlobalRef(env, ((uiAndroidControl *)c)->o);
	view_tabhost_add(env, name, uiViewFromControl(t), uiViewFromControl(c));
}

void uiToast(const char *format, ...) {
	va_list args;
	va_start(args, format);
	char buffer[256];
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	JNIEnv *env = get_jni_env();
	jni_toast(env, get_jni_ctx(), buffer);
}

uiControl *uiExpandControl(const char *name) {
	JNIEnv *env = get_jni_env();
	struct uiAndroidControl *c = new_view_control(uiBoxSignature); // ???
	c->o = view_expand(env, get_jni_ctx(), name);
	return (uiControl *)c;
}

const char *uiGet(const char *name) {
	return jni_get_string(get_jni_env(), get_jni_ctx(), name);
}

uintptr_t uiControlHandle(uiControl *c) {
	return (uintptr_t)((uiAndroidControl *)c)->o;
}

#if 0
uiScreen *uiNewScreen(const char *title, uiScreenInfo *info);
void uiScreenSetOnCreate();
void uiScreenSetOnClosed();

uiScreen *uiStartScreen(uiScreen *screen);

// For desktop only
void uiWindowSetScreen(uiScreen *screen);

#endif

void uiControlSetAttr(uiControl *c, const char *key, const char *value) {
	JNIEnv *env = get_jni_env();
	if (!strcmp(key, "drawable")) {
		jobject drawable = get_drawable_id(env, get_jni_ctx(), value);
		if (drawable == NULL) return;

		jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, uiViewFromControl(c)), "setBackground", "(Landroid/graphics/drawable/Drawable;)V");
		(*env)->CallVoidMethod(env, uiViewFromControl(c), method, drawable);
	}
}

int uiAndroidClose(JNIEnv *env) {
	// ...
	//(*env)->DeleteGlobalRef(env, libui_local->ctx);
	//libui_local = NULL;
	return 0;
}

int uiAndroidInit(JNIEnv *env, jobject context) {
	set_jni_env_ctx(env, context);

	jclass class = libui_class(env);
	jfieldID ctx_f = (*env)->GetStaticFieldID(env, class, "ctx", "Landroid/content/Context;");
	(*env)->SetStaticObjectField(env, class, ctx_f, context);

	return 0;
}

uiBox *uiAndroidBox(JNIEnv *env, jobject context, jobject parent) {
	struct uiBox *box = malloc(sizeof(struct uiBox));
	((uiAndroidControl *)box)->is_activity = 1;
	uiAndroidInit(env, context);
	box->c.o = parent;
	return box;
}

#pragma GCC visibility pop

LIBUI(void, initThiz)(JNIEnv *env, jobject thiz, jobject ctx) {
	uiAndroidInit(env, ctx);
}