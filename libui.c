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

// zeroed in bss
static struct UILibAndroidEnv libui = {0};

#pragma GCC visibility push(internal)

struct CallbackData {
	uintptr_t fn_ptr;
	uintptr_t arg1;
	uintptr_t arg2;
};

static int check_exception() {
	JNIEnv *env = libui.env;
	if ((*env)->ExceptionCheck(env)) {
		(*env)->ExceptionDescribe(env);
		(*env)->ExceptionClear(env);
		return -1;
	}

	return 0;
}

void *uiAndroidGetCtx() {
	return libui.ctx;
}

void *uiAndroidGetEnv() {
	return libui.env;
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
	static jmethodID method = 0; // faster?
	if (!method) method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, obj), "setTextSize", "(F)V");
	(*env)->CallVoidMethod(env, obj, method, size);
}

static jint get_res_id(const char *key, const char *name) {
	JNIEnv *env = libui.env;
	jobject res = libui.res;

	jmethodID get_identifier = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, res), "getIdentifier", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I");

	jint id = (*env)->CallIntMethod(
		libui.env, res, get_identifier,
		(*env)->NewStringUTF(env, name), (*env)->NewStringUTF(env, key), libui.package
	);

	return id;
}

jobject view_get_by_id(const char *id) {
	JNIEnv *env = libui.env;

	int res_id = get_res_id("id", id);

	jclass activity_class = (*env)->FindClass(env, "android/app/Activity");
	jmethodID find_view_m = (*env)->GetMethodID(env, activity_class, "findViewById", "(I)Landroid/view/View;");

	return (*env)->CallObjectMethod(env, libui.ctx, find_view_m, res_id);
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
	JNIEnv *env = libui.env;

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

int uiBoxPadded(uiBox *b) { return 0; }

void uiControlShow(uiControl *c) {}
void uiControlHide(uiControl *c) {}

static void view_destroy(jobject v) {
	JNIEnv *env = libui.env;

	jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, v), "getChildCount", "()I");
	if (!check_exception()) {
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
	view_destroy(view_from_ctrl((c)));
	free(c); // not advanced enough to free wrappers
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

uiControl *uiControlFromID(const char *id) {
	JNIEnv *env = libui.env;
	jobject obj = (*env)->NewGlobalRef(env, view_get_by_id(id));

	int signature = uiLabelSignature;
	if ((*env)->IsInstanceOf(env, obj, (*env)->FindClass(env, "android/widget/Button"))) {
		signature = uiButtonSignature;
	}

	struct uiAndroidControl *ctl = new_view_control(uiButtonSignature);
	ctl->o = obj;

	return (uiControl *)ctl;
}

static uiBox *new_uibox(int type) {
	JNIEnv *env = libui.env;

	struct uiAndroidControl *b = new_view_control(uiBoxSignature);

	jclass class = (*env)->FindClass(env, "android/widget/LinearLayout");
	jmethodID constructor = (*env)->GetMethodID(env, class, "<init>", "(Landroid/content/Context;)V");
	jobject obj = (*env)->NewObject(env, class, constructor, libui.ctx);

	jmethodID set_orientation_m = (*env)->GetMethodID(env, class, "setOrientation", "(I)V");
	(*env)->CallVoidMethod(env, obj, set_orientation_m, type);

	b->o = obj;

	view_set_layout(obj, ANDROID_LAYOUT_MATCH_PARENT, ANDROID_LAYOUT_MATCH_PARENT);

	return (uiBox *)b;
}

static char *view_get_text(jobject view) {
	JNIEnv *env = libui.env;
	jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, view), "getText", "()Landroid/text/Editable;");
	jobject editable = (*env)->CallObjectMethod(env, view, method);

	jmethodID to_string = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, editable), "toString", "()Ljava/lang/String;");
	jstring output = (*env)->CallObjectMethod(env, editable, to_string);

	const char *utf = (*env)->GetStringUTFChars(env, output, 0);

	size_t len = strlen(utf);
	char *new = malloc(strlen(utf) + 1);
	memcpy(new, utf, len + 1);

	(*env)->ReleaseStringUTFChars(env, output, utf);
	(*env)->DeleteLocalRef(env, output);

	return new;
}

char *uiMultilineEntryText(uiMultilineEntry *e) {
	return view_get_text(view_from_ctrl(e));
}

void uiFreeText(char *text) {
	JNIEnv *env = libui.env;
	free(text);
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

static jobject combobox_get_adapter(jobject view) {
	JNIEnv *env = libui.env;
	jclass view_c = (*env)->GetObjectClass(env, view);
	jmethodID get_adapter_m = (*env)->GetMethodID(env, view_c, "getAdapter", "()Landroid/widget/SpinnerAdapter;");
	jobject adapter = (*env)->CallObjectMethod(env, view, get_adapter_m);

	if (adapter == NULL) {
		jclass arr_adapter_c = (*env)->FindClass(env, "android/widget/ArrayAdapter");
		jmethodID init_adapter_m = (*env)->GetMethodID(env, arr_adapter_c, "<init>", "(Landroid/content/Context;I)V");
		adapter = (*env)->NewObject(env, arr_adapter_c, init_adapter_m, libui.ctx, ANDROID_simple_spinner_item);

		jmethodID set_adapter_m = (*env)->GetMethodID(env, view_c, "setAdapter", "(Landroid/widget/SpinnerAdapter;)V");
		(*env)->CallVoidMethod(env, view, set_adapter_m, adapter);

		jmethodID set_resource_id_m = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, adapter), "setDropDownViewResource", "(I)V");
		(*env)->CallVoidMethod(env, adapter, set_resource_id_m, ANDROID_simple_spinner_dropdown_item);
	}

	return adapter;
}

void uiComboboxSetSelected(uiCombobox *c, int index) {
	JNIEnv *env = libui.env;

	jobject view = view_from_ctrl(c);

	jmethodID add_m = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, view), "setSelection", "(I)V");
	(*env)->CallVoidMethod(env, view, add_m, (jint)index);
}

int uiComboboxSelected(uiCombobox *c) {
	JNIEnv *env = libui.env;

	jobject view = view_from_ctrl(c);

	jmethodID get_m = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, view), "getSelectedItemPosition", "()I");
	return (*env)->CallIntMethod(env, view, get_m);
}

void uiComboboxOnSelected(uiCombobox *c, void (*f)(uiCombobox *sender, void *senderData), void *data) {
	JNIEnv *env = libui.env;

	struct CallbackData call_data;
	call_data.fn_ptr = (uintptr_t)f;
	call_data.arg1 = (uintptr_t)c;
	call_data.arg2 = (uintptr_t)data;

	jobject *view = view_from_ctrl(c);

	jbyteArray arr = (*env)->NewByteArray(env, sizeof(call_data));
	(*env)->SetByteArrayRegion(env, arr, 0, sizeof(call_data), (const jbyte *)&call_data);

	jclass listener_c = (*env)->FindClass(env, "libui/LibUI$MySelectListener");
	jmethodID click_init = (*env)->GetMethodID(env, listener_c, "<init>", "([B)V");
	jobject listener = (*env)->NewObject(env, listener_c, click_init, arr);

	jmethodID set_click = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, view), "setOnItemSelectedListener", "(Landroid/widget/AdapterView$OnItemSelectedListener;)V");
	(*env)->CallVoidMethod(env, view, set_click, listener);
}

void uiComboboxClear(uiCombobox *c) {
	JNIEnv *env = libui.env;

	jobject view = view_from_ctrl(c);

	jobject adapter = combobox_get_adapter(view);

	jclass adapter_c = (*env)->GetObjectClass(env, adapter);
	jmethodID add_m = (*env)->GetMethodID(env, adapter_c, "clear", "()V");
	(*env)->CallVoidMethod(env, adapter, add_m);
}

void uiComboboxAppend(uiCombobox *c, const char *text) {
	JNIEnv *env = libui.env;

	jobject view = view_from_ctrl(c);

	jstring jtext = (*libui.env)->NewStringUTF(libui.env, text);

	jobject adapter = combobox_get_adapter(view);

	jclass adapter_c = (*env)->GetObjectClass(env, adapter);
	jmethodID add_m = (*env)->GetMethodID(env, adapter_c, "add", "(Ljava/lang/Object;)V");
	(*env)->CallVoidMethod(env, adapter, add_m, jtext);

	(*env)->DeleteLocalRef(env, jtext);
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
	view_set_view_enabled(view_from_ctrl(c), 1);
}

void uiControlDisable(uiControl *c) {
	view_set_view_enabled(view_from_ctrl(c), 0);
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

char *uiLabelText(uiLabel *l) {return "NULL";}

uiBox *uiNewVerticalBox() {
	return new_uibox(1);
}

uiBox *uiNewHorizontalBox() {
	return new_uibox(0);
}

void uiMsgBox(uiWindow *parent, const char *title, const char *description) {

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

char *uiGroupTitle(uiGroup *g) {
	return "NULL";
}

void uiGroupSetTitle(uiGroup *g, const char *title) {}
int uiGroupMargined(uiGroup *g) {return 0;}

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
	view_set_text(view_from_ctrl(b), text);
}

void uiLabelSetText(uiLabel *l, const char *text) {
	view_set_text(view_from_ctrl(l), text);
}

void uiEntrySetText(uiEntry *e, const char *text) {
	view_set_text(view_from_ctrl(e), text);
}

void uiMultilineEntrySetText(uiMultilineEntry *e, const char *text) {
	view_set_text(view_from_ctrl(e), text);
}

void uiWindowSetChild(uiWindow *w, uiControl *child) {
	JNIEnv *env = libui.env;
	if (w->is_activity) {
		ctx_set_content_view(view_from_ctrl((child)));
		return;
	}
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

uiControl *uiBoxChild(uiBox *box, int index) {
	JNIEnv *env = libui.env;
	jclass class = (*env)->FindClass(env, "android/view/ViewGroup");
	jmethodID get_child_m = (*env)->GetMethodID(env, class, "getChildAt", "(I)Landroid/view/View;");

	jobject view = (*env)->CallObjectMethod(env, view_from_ctrl(box), get_child_m, (jint)index);

	struct uiAndroidControl *c = new_view_control(uiBoxSignature);
	c->o = view;
	return (uiControl *)c;
}

void uiBoxDelete(uiBox *box, int index) {
	JNIEnv *env = libui.env;
	jclass class = (*env)->FindClass(env, "android/view/ViewGroup");
	jmethodID remove_view_m = (*env)->GetMethodID(env, class, "removeViewAt", "(I)V");
	(*env)->CallVoidMethod(env, view_from_ctrl(box), remove_view_m, (jint)index);
}

int uiBoxNumChildren(uiBox *box) {
	JNIEnv *env = libui.env;
	jclass class = (*env)->FindClass(env, "android/view/ViewGroup");
	jmethodID get_child_count_m = (*env)->GetMethodID(env, class, "getChildCount", "()I");
	return (*env)->CallIntMethod(env, view_from_ctrl(box), get_child_count_m);
}

void uiButtonOnClicked(uiButton *b, void (*f)(uiButton *sender, void *senderData), void *data) {
	JNIEnv *env = libui.env;

	// setOnItemSelectedListener
	// setOnClickListener

	struct CallbackData call_data;
	call_data.fn_ptr = (uintptr_t)f;
	call_data.arg1 = (uintptr_t)b;
	call_data.arg2 = (uintptr_t)data;

	jobject *view = view_from_ctrl(b);

	jbyteArray arr = (*env)->NewByteArray(env, sizeof(call_data));
	(*env)->SetByteArrayRegion(env, arr, 0, sizeof(call_data), (const jbyte *)&call_data);

	jclass listener_c = (*env)->FindClass(env, "libui/LibUI$MyOnClickListener");
	jmethodID click_init = (*env)->GetMethodID(env, listener_c, "<init>", "([B)V");
	jobject listener = (*env)->NewObject(env, listener_c, click_init, arr);

	jmethodID set_click = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, view), "setOnClickListener", "(Landroid/view/View$OnClickListener;)V");
	(*env)->CallVoidMethod(env, view, set_click, listener);
}

void uiQueueMain(void (*f)(void *data), void *data) {
	JNIEnv *env = libui.env;

	struct CallbackData call_data;
	call_data.fn_ptr = (uintptr_t)f;
	call_data.arg1 = (uintptr_t)data;
	call_data.arg2 = (uintptr_t)NULL;

	jbyteArray arr = (*env)->NewByteArray(env, sizeof(call_data));
	(*env)->SetByteArrayRegion(env, arr, 0, sizeof(call_data), (const jbyte *)&call_data);

	(*libui.env)->CallStaticVoidMethod(
			libui.env, libui.class, libui.add_runnable_m, arr
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
	va_end(args);

	JNIEnv *env = libui.env;

	jstring jbuffer = (*libui.env)->NewStringUTF(libui.env, buffer);

	jclass toast_c = (*env)->FindClass(env, "android/widget/Toast");
	jmethodID make_text_m = (*env)->GetStaticMethodID(env, toast_c, "makeText", "(Landroid/content/Context;Ljava/lang/CharSequence;I)Landroid/widget/Toast;");
	jmethodID show_m = (*env)->GetMethodID(env, toast_c, "show", "()V");

	jobject toast = (*env)->CallStaticObjectMethod(env, toast_c, make_text_m, libui.ctx, jbuffer, 0x0);
	(*env)->CallVoidMethod(env, toast, show_m);

	(*libui.env)->DeleteLocalRef(libui.env, jbuffer);
	(*libui.env)->DeleteLocalRef(libui.env, toast);
}

static jobject get_drawable_id(const char *name) {
	JNIEnv *env = libui.env;
	jobject res = libui.res;

	int id = get_res_id("drawable", name);
	if (id == 0) return NULL;

	jmethodID get_drawable = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, res), "getDrawable", "(ILandroid/content/res/Resources$Theme;)Landroid/graphics/drawable/Drawable;");

	jobject drawable = (*env)->CallObjectMethod(
			libui.env, res, get_drawable, id, libui.theme
	);

	return drawable;
}

uiControl *uiExpandControl(char *name) {
	JNIEnv *env = libui.env;

	int id = get_res_id("layout", name);
	if (id == 0) return NULL;

	jmethodID inflate_m = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, libui.inflater), "inflate", "(ILandroid/view/ViewGroup;)Landroid/view/View;");

	jobject view = (*env)->CallObjectMethod(
			libui.env, libui.inflater, inflate_m, id, NULL
	);

	struct uiAndroidControl *c = new_view_control(uiBoxSignature);
	c->o = view;
	return (uiControl *)c;
}

const char *uiGet(const char *name) {
	JNIEnv *env = libui.env;
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

	jmethodID get_inflater = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, context), "getLayoutInflater", "()Landroid/view/LayoutInflater;");
	libui.inflater = (*env)->CallObjectMethod(env, libui.ctx, get_inflater);

	jmethodID get_res = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, context), "getResources", "()Landroid/content/res/Resources;");
	libui.res = (*env)->CallObjectMethod(env, libui.ctx, get_res);

	jmethodID get_theme = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, context), "getTheme", "()Landroid/content/res/Resources$Theme;");
	libui.theme = (*env)->CallObjectMethod(env, libui.ctx, get_theme);

	libui.theme = (*env)->NewGlobalRef(env, libui.theme);
	libui.res = (*env)->NewGlobalRef(env, libui.res);
	libui.package = (*env)->NewGlobalRef(env, libui.package);
	libui.inflater = (*env)->NewGlobalRef(env, libui.inflater);

	jclass class = (*env)->FindClass(env, "libui/LibUI");
	libui.class = (*env)->NewGlobalRef(env, class);

	// TODO: Handle exception

	jfieldID ctx_f = (*env)->GetStaticFieldID(env, class, "ctx", "Landroid/content/Context;");
	(*env)->SetStaticObjectField(env, class, ctx_f, context);

	libui.form_m = (*env)->GetStaticMethodID(env, class, "form", "(Ljava/lang/String;)Landroid/view/View;");
	libui.tab_layout_m = (*env)->GetStaticMethodID(env, class, "tabLayout", "()Landroid/view/View;");

	libui.add_tab_m = (*env)->GetStaticMethodID(env, class, "addTab", "(Landroid/view/View;Ljava/lang/String;Landroid/view/View;)V");
	libui.form_add_m = (*env)->GetStaticMethodID(env, class, "formAppend", "(Landroid/view/View;Ljava/lang/String;Landroid/view/View;)V");

	libui.add_runnable_m = (*env)->GetStaticMethodID(env, class, "runRunnable", "([B)V");

	return 0;
}

uiBox *uiAndroidBox(JNIEnv *env, jobject context, jobject parent) {
	struct uiBox *box = malloc(sizeof(struct uiBox));
	uiAndroidInit(env, context);
	box->c.o = parent;
	return box;
}

#pragma GCC visibility pop

#define LIBUI(ret, name) JNIEXPORT ret JNICALL Java_libui_LibUI_##name

LIBUI(void, callFunction)(JNIEnv *env, jobject thiz, jbyteArray arr) {

	struct CallbackData *data = (struct CallbackData *)(*env)->GetByteArrayElements(env, arr, 0);

	if (data->fn_ptr == NULL) abort();

	// TODO: jlong == long long breaks ptr hack, need to store pointer data in struct -> jbytearray

	void (*ptr_f)(uintptr_t, uintptr_t) = (void *)data->fn_ptr;
	ptr_f(data->arg1, data->arg2);
}

LIBUI(void, initThiz)(JNIEnv *env, jobject thiz, jobject ctx) {
	uiAndroidInit(env, ctx);
}