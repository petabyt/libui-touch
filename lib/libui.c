#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <android/log.h>
#include <stdlib.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <jni.h>

#include "ui.h"
#include "uifw.h"
#include "uifw_priv.h"
#include "android.h"

struct AndroidLocal {
	JNIEnv *env;
	jobject ctx;
};
__thread struct AndroidLocal local = {0, 0};

#pragma GCC visibility push(internal)

jobject uiViewFromControl(void *c) {
	return ((struct uiAndroidControl *)c)->o;
}

void ui_android_set_env_ctx(JNIEnv *env, jobject ctx) {
	local.ctx = ctx;
	local.env = env;
	// TODO: Set LibUI.ctx
}

void *uiAndroidGetCtx(void) {
	if (local.ctx == 0) {
		__android_log_write(ANDROID_LOG_ERROR, "libui", "NULL ctx");
		abort();
	}
	return local.ctx;
}

void *uiAndroidGetEnv(void) {
	if (local.env == NULL) {
		__android_log_write(ANDROID_LOG_ERROR, "libui", "NULL env");
		abort();
	}
	return local.env;
}

static int check_exception(void) {
	JNIEnv *env = uiAndroidGetEnv();
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
	ctx_set_content_view(uiAndroidGetEnv(), uiAndroidGetCtx(), uiViewFromControl(c));
}

void uiControlCenter(uiControl *c) {
	JNIEnv *env = uiAndroidGetEnv();
	const jobject view = uiViewFromControl(c);
	jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, view), "setGravity", "(I)V");
	(*env)->CallVoidMethod(env, view, method, 0x00000010);
}

void uiLabelAlignment(uiControl *c, int align) {
	JNIEnv *env = uiAndroidGetEnv();
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
	JNIEnv *env = uiAndroidGetEnv();
	jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, view), "setEnabled", "(Z)V");
	(*env)->CallVoidMethod(env, view, method, b);

	check_exception();
}

void uiLabelSetTextSize(uiLabel *l, float size) {
	view_set_text_size(uiAndroidGetEnv(), uiViewFromControl(l), size);
}

void uiMultilineEntrySetReadOnly(uiMultilineEntry *e, int readonly) {
	view_set_view_enabled(((uiAndroidControl *)e)->o, readonly);
}

void uiBoxSetPadded(uiBox *b, int padded) {
	view_set_padding_px(uiAndroidGetEnv(), b->c.o, padded);
}

int uiBoxPadded(uiBox *b) { return 0; }

void uiControlShow(uiControl *c) {
	if (c->Signature == uiWindowSignature) {
		popupwindow_open(uiAndroidGetEnv(), uiAndroidGetCtx(), uiViewFromControl(c));
	}
}
void uiControlHide(uiControl *c) {}

static void view_destroy(jobject v) {
	JNIEnv *env = uiAndroidGetEnv();

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
}

void uiFormSetPadded(uiForm *f, int padded) {
	view_set_padding_px(uiAndroidGetEnv(), f->c.o, padded);
}

static inline struct uiAndroidControl *new_view_control(int signature) {
	struct uiAndroidControl *b = calloc(1, sizeof(struct uiAndroidControl));
	b->c.Signature = signature;
	b->c.Handle = (uintptr_t (*)(uiControl *)) uiViewFromControl;
	b->c.Disable = uiControlDisable;
	b->c.Enable = uiControlEnable;
	b->c.Show = uiControlShow;
	b->c.Hide = uiControlHide;

	// TODO:
	//	void (*Destroy)(uiControl *);
	//	uiControl *(*Parent)(uiControl *);
	//	void (*SetParent)(uiControl *, uiControl *);
	//	int (*Toplevel)(uiControl *);
	//	int (*Visible)(uiControl *);
	//	int (*Enabled)(uiControl *);

	return b;
}

uiControl *uiControlFromView(jobject obj) {
	JNIEnv *env = uiAndroidGetEnv();
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
	JNIEnv *env = uiAndroidGetEnv();
	jobject obj = (*env)->NewGlobalRef(env, view_get_by_id(env, uiAndroidGetCtx(), id));
	return (uiControl *)uiControlFromView(obj);
}

static uiBox *new_uibox(int type) {
	JNIEnv *env = uiAndroidGetEnv();
	struct uiAndroidControl *b = new_view_control(uiBoxSignature);
	b->o = view_new_linearlayout(env, uiAndroidGetCtx(), type, ANDROID_LAYOUT_MATCH_PARENT, ANDROID_LAYOUT_MATCH_PARENT);
	return (uiBox *)b;
}

char *uiMultilineEntryText(uiMultilineEntry *e) {
	return view_get_text(uiAndroidGetEnv(), uiViewFromControl(e));
}

void uiFreeText(char *text) {
	JNIEnv *env = uiAndroidGetEnv();
	free(text);
}

static struct uiAndroidControl *view_new_separator() {
	struct uiAndroidControl *c = new_view_control(uiSeparatorSignature);

	JNIEnv *env = uiAndroidGetEnv();
	jclass class = (*env)->FindClass(env, "android/widget/Space");
	jmethodID constructor = (*env)->GetMethodID(env, class, "<init>", "(Landroid/content/Context;)V");
	jobject obj = (*env)->NewObject(env, class, constructor, uiAndroidGetCtx());

	c->request_height = 50;

	c->o = obj;
	return c;
}

void uiComboboxSetSelected(uiCombobox *c, int index) {
	JNIEnv *env = uiAndroidGetEnv();

	jobject view = uiViewFromControl(c);

	jmethodID add_m = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, view), "setSelection", "(I)V");
	(*env)->CallVoidMethod(env, view, add_m, (jint)index);
}

int uiComboboxSelected(uiCombobox *c) {
	JNIEnv *env = uiAndroidGetEnv();

	jobject view = uiViewFromControl(c);

	jmethodID get_m = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, view), "getSelectedItemPosition", "()I");
	return (*env)->CallIntMethod(env, view, get_m);
}

void uiComboboxOnSelected(uiCombobox *c, void (*f)(uiCombobox *sender, void *senderData), void *data) {
	JNIEnv *env = uiAndroidGetEnv();
	jobject view = uiViewFromControl(c);
	view_add_native_select_listener(env, view, (void *)f, 2, c, data);
}

void uiComboboxClear(uiCombobox *c) {
	JNIEnv *env = uiAndroidGetEnv();

	jobject view = uiViewFromControl(c);

	jobject adapter = combobox_get_adapter(env, uiAndroidGetCtx(), view);

	jclass adapter_c = (*env)->GetObjectClass(env, adapter);
	jmethodID add_m = (*env)->GetMethodID(env, adapter_c, "clear", "()V");
	(*env)->CallVoidMethod(env, adapter, add_m);

	(*env)->DeleteLocalRef(env, adapter);
}

void uiComboboxAppend(uiCombobox *c, const char *text) {
	JNIEnv *env = uiAndroidGetEnv();

	jobject view = uiViewFromControl(c);

	jstring jtext = (*env)->NewStringUTF(env, text);

	jobject adapter = combobox_get_adapter(env, uiAndroidGetCtx(), view);

	jclass adapter_c = (*env)->GetObjectClass(env, adapter);
	jmethodID add_m = (*env)->GetMethodID(env, adapter_c, "add", "(Ljava/lang/Object;)V");
	(*env)->CallVoidMethod(env, adapter, add_m, jtext);

	(*env)->DeleteLocalRef(env, jtext);
	(*env)->DeleteLocalRef(env, adapter);
}

struct uiScroll *uiNewScroll() {
	struct uiAndroidControl *c = new_view_control(uiBoxSignature);

	JNIEnv *env = uiAndroidGetEnv();
	jclass class = (*env)->FindClass(env, "android/widget/ScrollView");
	jmethodID constructor = (*env)->GetMethodID(env, class, "<init>", "(Landroid/content/Context;)V");
	jobject obj = (*env)->NewObject(env, class, constructor, uiAndroidGetCtx());

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
	JNIEnv *env = uiAndroidGetEnv();

	struct uiAndroidControl *b = new_view_control(uiButtonSignature);

	(*env)->PushLocalFrame(env, 10);

	jclass class = (*env)->FindClass(env, "android/widget/Button");
	jmethodID constructor = (*env)->GetMethodID(env, class, "<init>", "(Landroid/content/Context;)V");
	jobject obj = (*env)->NewObject(env, class, constructor, uiAndroidGetCtx());

	jobject res = jni_get_resources(env, uiAndroidGetCtx());
	jobject theme = jni_get_theme(env, uiAndroidGetCtx());

	// TODO: condense
#if 1
	jfieldID button_bg_f = (*env)->GetStaticFieldID(env, libui_class(env), "buttonBackgroundResource", "I");
	jint button_bg = (*env)->GetStaticIntField(env, libui_class(env), button_bg_f);
	if (button_bg != 0) {
		jmethodID get_drawable_m = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, res), "getDrawable", "(ILandroid/content/res/Resources$Theme;)Landroid/graphics/drawable/Drawable;");

		jobject drawable = (*env)->CallObjectMethod(env, res, get_drawable_m, button_bg, theme);

		jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, obj), "setBackground", "(Landroid/graphics/drawable/Drawable;)V");
		(*env)->CallVoidMethod(env, obj, method, drawable);
	}
#endif

	obj = (*env)->PopLocalFrame(env, obj);
	b->o = obj;

	view_set_text(env, obj, text);
	view_set_text_size(env, obj, 14.0);

	return (uiButton *)b;
}

uiLabel *uiNewLabel(const char *text) {
	JNIEnv *env = uiAndroidGetEnv();

	struct uiAndroidControl *l = new_view_control(uiLabelSignature);

	jclass class = (*env)->FindClass(env, "android/widget/TextView");
	jmethodID constructor = (*env)->GetMethodID(env, class, "<init>", "(Landroid/content/Context;)V");
	jobject obj = (*env)->NewObject(env, class, constructor, uiAndroidGetCtx());
	l->o = obj;

	view_set_text(env, obj, text);
	view_set_text_size(env, obj, 14.0);

	return (uiLabel *)l;
}

char *uiLabelText(uiLabel *l) {return "NULL";}

uiBox *uiNewVerticalBox() {
	return new_uibox(1);
}

uiBox *uiNewHorizontalBox() {
	return new_uibox(0);
}

void uiMsgBox(uiWindow *parent, const char *title, const char *description) {

}

int jni_does_class_exist(JNIEnv *env, const char *name) {
	(*env)->FindClass(env, name);
	if ((*env)->ExceptionCheck(env)) {
		(*env)->ExceptionClear(env);
		return 0;
	}

	return 1;
}

uiTab *uiNewTab() {
	JNIEnv *env = uiAndroidGetEnv();
	struct uiAndroidControl *t = new_view_control(uiTabSignature);

	if (jni_does_class_exist(env, "androidx/ViewPager2")) {
		jmethodID new_tab_m = (*env)->GetStaticMethodID(env, libui_class(env), "tabLayout", "()Landroid/view/View;");
		jobject tab = (*env)->CallStaticObjectMethod(env, libui_class(env), new_tab_m);
		t->o = tab;
	} else {
		t->o = tabhost_new(env, uiAndroidGetCtx());
	}

	return (uiTab *)t;
}

uiMultilineEntry *uiNewMultilineEntry() {
	struct uiAndroidControl *c = new_view_control(uiMultilineEntrySignature);

	JNIEnv *env = uiAndroidGetEnv();
	jclass class = (*env)->FindClass(env, "android/widget/EditText");
	jmethodID constructor = (*env)->GetMethodID(env, class, "<init>", "(Landroid/content/Context;)V");
	jobject obj = (*env)->NewObject(env, class, constructor, uiAndroidGetCtx());

	c->o = obj;
	return (uiMultilineEntry *)c;
}

uiEntry *uiNewEntry() {
	struct uiAndroidControl *c = new_view_control(uiEntrySignature);

	JNIEnv *env = uiAndroidGetEnv();
	jclass class = (*env)->FindClass(env, "android/widget/EditText");
	jmethodID constructor = (*env)->GetMethodID(env, class, "<init>", "(Landroid/content/Context;)V");
	jobject obj = (*env)->NewObject(env, class, constructor, uiAndroidGetCtx());

	view_set_layout(uiAndroidGetEnv(), obj, ANDROID_LAYOUT_MATCH_PARENT, ANDROID_LAYOUT_WRAP_CONTENT);

	c->o = obj;
	return (uiEntry *)c;
}

uiProgressBar *uiNewProgressBar() {
	JNIEnv *env = uiAndroidGetEnv();
	struct uiAndroidControl *c = new_view_control(uiProgressBarSignature);
	jclass class = (*env)->FindClass(env, "android/widget/ProgressBar");
	jmethodID constructor = (*env)->GetMethodID(env, class, "<init>", "(Landroid/content/Context;Landroid/util/AttributeSet;I)V");
	jobject obj = (*env)->NewObject(env, class, constructor, uiAndroidGetCtx(), NULL, ANDROID_progressBarStyleHorizontal);
	c->o = obj;
	return (uiProgressBar *)c;
}

uiWindow *uiNewWindow(const char *title, int width, int height, int hasMenubar) {
	struct uiAndroidControl *c = new_view_control(uiWindowSignature);

	JNIEnv *env = uiAndroidGetEnv();

	jstring jtitle = (*env)->NewStringUTF(env, title);

	c->o = popupwindow_new(env, uiAndroidGetCtx(), 0);

	return (uiWindow *)c;
}

uiGroup *uiNewGroup(const char *title) {
	JNIEnv *env = uiAndroidGetEnv();
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
	JNIEnv *env = uiAndroidGetEnv();
//	(*env)->CallStaticVoidMethod(
//			env, libui_class(env), libui.form_add_m,
//			f->c.o, (*env)->NewStringUTF(env, label), ((uiAndroidControl *)c)->o
//	);
}

void uiButtonSetText(uiButton *b, const char *text) {
	view_set_text(uiAndroidGetEnv(), uiViewFromControl(b), text);
}

void uiLabelSetText(uiLabel *l, const char *text) {
	view_set_text(uiAndroidGetEnv(), uiViewFromControl(l), text);
}

void uiEntrySetText(uiEntry *e, const char *text) {
	view_set_text(uiAndroidGetEnv(), uiViewFromControl(e), text);
}

void uiMultilineEntrySetText(uiMultilineEntry *e, const char *text) {
	view_set_text(uiAndroidGetEnv(), uiViewFromControl(e), text);
}

void uiWindowSetChild(uiWindow *w, uiControl *child) {
	JNIEnv *env = uiAndroidGetEnv();
	if (((struct uiAndroidControl *)w)->is_activity) {
		ctx_set_content_view(env, uiAndroidGetCtx(), uiViewFromControl((child)));
		return;
	}

	popupwindow_set_content(env, uiViewFromControl(w), uiViewFromControl(child));
}

void uiProgressBarSetValue(uiProgressBar *p, int n) {
	JNIEnv *env = uiAndroidGetEnv();
	jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, p->c.o), "setProgress", "(I)V");
	(*env)->CallVoidMethod(env, p->c.o, method, n);
}

void uiBoxAppend(uiBox *b, uiControl *child, int stretchy) {
	JNIEnv *env = uiAndroidGetEnv();
	struct uiAndroidControl *ctl = (struct uiAndroidControl *)child;
	ctl->o = (*env)->NewGlobalRef(env, ctl->o);

	viewgroup_addview(env, b->c.o, uiViewFromControl(child));

	// Controls can optionally request to be set a certain width (only can be set after appending)
	if (ctl->request_width) view_set_dimensions(env, ctl->o, ctl->request_width, 0);
	if (ctl->request_height) view_set_dimensions(env, ctl->o, 0, ctl->request_height);
}

uiControl *uiBoxChild(uiBox *box, int index) {
	JNIEnv *env = uiAndroidGetEnv();
	//(*env)->PushLocalFrame(env, 10);
	jclass class = (*env)->FindClass(env, "android/view/ViewGroup");
	jmethodID get_child_m = (*env)->GetMethodID(env, class, "getChildAt", "(I)Landroid/view/View;");

	jobject view = (*env)->CallObjectMethod(env, uiViewFromControl(box), get_child_m, (jint)index);

	jobject viewn = (*env)->NewGlobalRef(env, view);
	(*env)->DeleteLocalRef(env, view);

	struct uiAndroidControl *c = new_view_control(uiBoxSignature);
	c->o = viewn;

	//(*env)->PopLocalFrame(env, NULL);

	(*env)->DeleteLocalRef(env, class);

	return (uiControl *)c;
}

void uiBoxDelete(uiBox *box, int index) {
	JNIEnv *env = uiAndroidGetEnv();
	jclass class = (*env)->FindClass(env, "android/view/ViewGroup");
	jmethodID remove_view_m = (*env)->GetMethodID(env, class, "removeViewAt", "(I)V");
	(*env)->DeleteLocalRef(env, class);
	(*env)->CallVoidMethod(env, uiViewFromControl(box), remove_view_m, (jint)index);
}

int uiBoxNumChildren(uiBox *box) {
	JNIEnv *env = uiAndroidGetEnv();
	jclass class = (*env)->FindClass(env, "android/view/ViewGroup");
	jmethodID get_child_count_m = (*env)->GetMethodID(env, class, "getChildCount", "()I");
	(*env)->DeleteLocalRef(env, class);
	return (*env)->CallIntMethod(env, uiViewFromControl(box), get_child_count_m);
}

void uiButtonOnClicked(uiButton *b, void (*f)(uiButton *sender, void *senderData), void *data) {
	JNIEnv *env = uiAndroidGetEnv();
	jobject *view = uiViewFromControl(b);
	view_add_native_click_listener(env, view, (void *)f, 2, b, data);
}

void uiQueueMain(void (*f)(void *data), void *data) {
	JNIEnv *env = uiAndroidGetEnv();
	jni_native_runnable(env, uiAndroidGetCtx(), (void *)f, 1, data, NULL);
}

void uiTabAppend(uiTab *t, const char *name, uiControl *c) {
	JNIEnv *env = uiAndroidGetEnv();
	((uiAndroidControl *)c)->o = (*env)->NewGlobalRef(env, ((uiAndroidControl *)c)->o);

	view_tabhost_add(env, name, uiViewFromControl(t), uiViewFromControl(c));

//	jstring jname = (*env)->NewStringUTF(env, name);
//	(*env)->CallStaticVoidMethod(
//			env, libui_class(env), libui.add_tab_m,
//			t->c.o, jname, ((uiAndroidControl *)c)->o
//	);

//	(*env)->DeleteLocalRef(env, jname);
}

void uiToast(const char *format, ...) {
	va_list args;
	va_start(args, format);
	char buffer[256];
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	JNIEnv *env = uiAndroidGetEnv();
	jni_toast(env, uiAndroidGetCtx(), buffer);
}

uiControl *uiExpandControl(const char *name) {
	JNIEnv *env = uiAndroidGetEnv();
	struct uiAndroidControl *c = new_view_control(uiBoxSignature); // ???
	c->o = view_expand(env, uiAndroidGetCtx(), name);
	return (uiControl *)c;
}

const char *uiGet(const char *name) {
	JNIEnv *env = uiAndroidGetEnv();
	return jni_get_string(uiAndroidGetEnv(), uiAndroidGetCtx(), name);
}

uintptr_t uiControlHandle(uiControl *c) {
	return (uintptr_t)((uiAndroidControl *)c)->o;
}

void uiControlSetAttr(uiControl *c, const char *key, const char *value) {
	JNIEnv *env = uiAndroidGetEnv();
	if (!strcmp(key, "drawable")) {
		jobject drawable = get_drawable_id(env, uiAndroidGetCtx(), value);
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
	ui_android_set_env_ctx(env, context);

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