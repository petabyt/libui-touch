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

static struct AndroidLocal {
	JNIEnv *env;
	jobject ctx;
	jclass class;
}local;

// zeroed in bss
struct UILibAndroidEnv libui = {0};

#pragma GCC visibility push(internal)

jobject uiViewFromControl(void *c) {
	return ((struct uiAndroidControl *)c)->o;
}

void *uiAndroidGetCtx() {
	return local.ctx;
}

void *uiAndroidGetEnv() {
	return local.env;
}

static int check_exception() {
	JNIEnv *env = uiAndroidGetEnv();
	if ((*env)->ExceptionCheck(env)) {
		(*env)->ExceptionDescribe(env);
		(*env)->ExceptionClear(env);
		return -1;
	}

	return 0;
}

static void view_set_text_size(jobject obj, float size) {
	JNIEnv *env = uiAndroidGetEnv();
	static jmethodID method = 0; // faster?
	if (!method) method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, obj), "setTextSize", "(F)V");
	(*env)->CallVoidMethod(env, obj, method, size);
}

static jint get_res_id(const char *key, const char *name) {
	JNIEnv *env = uiAndroidGetEnv();
	(*env)->PushLocalFrame(env, 10);
	jobject res = jni_get_resources(env, uiAndroidGetCtx());

	jmethodID get_identifier = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, res), "getIdentifier", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I");

	jstring key_s = (*env)->NewStringUTF(env, name);
	jstring name_s = (*env)->NewStringUTF(env, key);
	jstring pkg_s = jni_get_package_name(env, uiAndroidGetCtx());

	jint id = (*env)->CallIntMethod(
		env, res, get_identifier,
		key_s, name_s,
		pkg_s
	);

	(*env)->PopLocalFrame(env, NULL);

	return id;
}

jobject view_get_by_id(const char *id) {
	JNIEnv *env = uiAndroidGetEnv();

	int res_id = get_res_id("id", id);

	jclass activity_class = (*env)->FindClass(env, "android/app/Activity");
	jmethodID find_view_m = (*env)->GetMethodID(env, activity_class, "findViewById", "(I)Landroid/view/View;");

	return (*env)->CallObjectMethod(env, uiAndroidGetCtx(), find_view_m, res_id);
}

void ctx_set_content_view(jobject view) {
	JNIEnv *env = uiAndroidGetEnv();
	jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, uiAndroidGetCtx()), "setContentView", "(Landroid/view/View;)V");
	(*env)->CallVoidMethod(env, uiAndroidGetCtx(), method, view );
}

void uiAndroidSetContent(uiControl *c) {
	ctx_set_content_view(uiViewFromControl(c));
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
	view_set_text_size(uiViewFromControl(l), size);
}

void uiMultilineEntrySetReadOnly(uiMultilineEntry *e, int readonly) {
	view_set_view_enabled(((uiAndroidControl *)e)->o, readonly);
}

void view_set_visibility(jobject view, int v) {
	// 0 = visible
	// 4 = invisible
	// 8 = gone
	JNIEnv *env = uiAndroidGetEnv();
	jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, view), "setVisibility", "(I)V");
	(*env)->CallVoidMethod(env, view, method, v);
}

void view_set_dimensions(jobject view, int w, int h) {
	JNIEnv *env = uiAndroidGetEnv();

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
	JNIEnv *env = uiAndroidGetEnv();

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
	JNIEnv *env = uiAndroidGetEnv();
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
	view_set_padding_px(f->c.o, padded);
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

uiControl *uiControlFromID(const char *id) {
	JNIEnv *env = uiAndroidGetEnv();
	jobject obj = (*env)->NewGlobalRef(env, view_get_by_id(id));

	int signature = uiLabelSignature;
	if ((*env)->IsInstanceOf(env, obj, (*env)->FindClass(env, "android/widget/Button"))) {
		signature = uiButtonSignature;
	}

	struct uiAndroidControl *ctl = new_view_control(uiButtonSignature);
	ctl->o = obj;

	return (uiControl *)ctl;
}

static void set_focusable(jobject obj) {
	JNIEnv *env = uiAndroidGetEnv();
	jclass class = (*env)->GetObjectClass(env, obj);
	jmethodID set_clickable_m = (*env)->GetMethodID(env, class, "setClickable", "(Z)V");
	(*env)->CallVoidMethod(env, obj, set_clickable_m, 1);

	jmethodID focusable_m = (*env)->GetMethodID(env, class, "setFocusableInTouchMode", "(Z)V");
	(*env)->CallVoidMethod(env, obj, focusable_m, 1);

	jmethodID setFocusable_m = (*env)->GetMethodID(env, class, "setFocusable", "(Z)V");
	(*env)->CallVoidMethod(env, obj, setFocusable_m, 1);
}

static uiBox *new_uibox(int type) {
	JNIEnv *env = uiAndroidGetEnv();

	struct uiAndroidControl *b = new_view_control(uiBoxSignature);

	jclass class = (*env)->FindClass(env, "android/widget/LinearLayout");
	jmethodID constructor = (*env)->GetMethodID(env, class, "<init>", "(Landroid/content/Context;)V");
	jobject obj = (*env)->NewObject(env, class, constructor, uiAndroidGetCtx());

	jmethodID set_orientation_m = (*env)->GetMethodID(env, class, "setOrientation", "(I)V");
	(*env)->CallVoidMethod(env, obj, set_orientation_m, type);

	b->o = obj;

	set_focusable(obj);

	view_set_layout(obj, ANDROID_LAYOUT_MATCH_PARENT, ANDROID_LAYOUT_MATCH_PARENT);

	return (uiBox *)b;
}

static char *view_get_text(jobject view) {
	JNIEnv *env = uiAndroidGetEnv();
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
	return view_get_text(uiViewFromControl(e));
}

void uiFreeText(char *text) {
	JNIEnv *env = uiAndroidGetEnv();
	free(text);
}

static void view_set_text(jobject view, const char *text) {
	JNIEnv *env = uiAndroidGetEnv();
	jstring jtext = (*env)->NewStringUTF(env, text);
	jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, view), "setText", "(Ljava/lang/CharSequence;)V");
	(*env)->CallVoidMethod(env, view, method, jtext);
	(*env)->DeleteLocalRef(env, jtext);
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

static jobject combobox_get_adapter(jobject view) {
	JNIEnv *env = uiAndroidGetEnv();
	jclass view_c = (*env)->GetObjectClass(env, view);
	jmethodID get_adapter_m = (*env)->GetMethodID(env, view_c, "getAdapter", "()Landroid/widget/SpinnerAdapter;");
	jobject adapter = (*env)->CallObjectMethod(env, view, get_adapter_m);

	if (adapter == NULL) {
		jclass arr_adapter_c = (*env)->FindClass(env, "android/widget/ArrayAdapter");
		jmethodID init_adapter_m = (*env)->GetMethodID(env, arr_adapter_c, "<init>", "(Landroid/content/Context;I)V");
		adapter = (*env)->NewObject(env, arr_adapter_c, init_adapter_m, uiAndroidGetCtx(), ANDROID_simple_spinner_item);

		jmethodID set_adapter_m = (*env)->GetMethodID(env, view_c, "setAdapter", "(Landroid/widget/SpinnerAdapter;)V");
		(*env)->CallVoidMethod(env, view, set_adapter_m, adapter);

		jmethodID set_resource_id_m = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, adapter), "setDropDownViewResource", "(I)V");
		(*env)->CallVoidMethod(env, adapter, set_resource_id_m, ANDROID_simple_spinner_dropdown_item);
	}

	return adapter;
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

	struct CallbackData call_data;
	call_data.fn_ptr = (uintptr_t)f;
	call_data.arg1 = (uintptr_t)c;
	call_data.arg2 = (uintptr_t)data;

	jobject *view = uiViewFromControl(c);

	jbyteArray arr = (*env)->NewByteArray(env, sizeof(call_data));
	(*env)->SetByteArrayRegion(env, arr, 0, sizeof(call_data), (const jbyte *)&call_data);

	jclass listener_c = (*env)->FindClass(env, "libui/LibUI$MySelectListener");
	jmethodID click_init = (*env)->GetMethodID(env, listener_c, "<init>", "([B)V");
	jobject listener = (*env)->NewObject(env, listener_c, click_init, arr);

	jmethodID set_click = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, view), "setOnItemSelectedListener", "(Landroid/widget/AdapterView$OnItemSelectedListener;)V");
	(*env)->CallVoidMethod(env, view, set_click, listener);
}

void uiComboboxClear(uiCombobox *c) {
	JNIEnv *env = uiAndroidGetEnv();

	jobject view = uiViewFromControl(c);

	jobject adapter = combobox_get_adapter(view);

	jclass adapter_c = (*env)->GetObjectClass(env, adapter);
	jmethodID add_m = (*env)->GetMethodID(env, adapter_c, "clear", "()V");
	(*env)->CallVoidMethod(env, adapter, add_m);

	(*env)->DeleteLocalRef(env, adapter);
}

void uiComboboxAppend(uiCombobox *c, const char *text) {
	JNIEnv *env = uiAndroidGetEnv();

	jobject view = uiViewFromControl(c);

	jstring jtext = (*env)->NewStringUTF(env, text);

	jobject adapter = combobox_get_adapter(view);

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

	set_focusable(obj);

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
	jfieldID button_bg_f = (*env)->GetStaticFieldID(env, libui.class, "buttonBackgroundResource", "I");
	jint button_bg = (*env)->GetStaticIntField(env, libui.class, button_bg_f);
	if (button_bg != 0) {
		jmethodID get_drawable_m = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, res), "getDrawable", "(ILandroid/content/res/Resources$Theme;)Landroid/graphics/drawable/Drawable;");

		jobject drawable = (*env)->CallObjectMethod(env, res, get_drawable_m, button_bg, theme);

		jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, obj), "setBackground", "(Landroid/graphics/drawable/Drawable;)V");
		(*env)->CallVoidMethod(env, obj, method, drawable);
	}
#endif

	obj = (*env)->PopLocalFrame(env, obj);
	b->o = obj;

	view_set_text(obj, text);
	view_set_text_size(obj, 14.0);

	return (uiButton *)b;
}

uiLabel *uiNewLabel(const char *text) {
	JNIEnv *env = uiAndroidGetEnv();

	struct uiAndroidControl *l = new_view_control(uiLabelSignature);

	jclass class = (*env)->FindClass(env, "android/widget/TextView");
	jmethodID constructor = (*env)->GetMethodID(env, class, "<init>", "(Landroid/content/Context;)V");
	jobject obj = (*env)->NewObject(env, class, constructor, uiAndroidGetCtx());
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
	JNIEnv *env = uiAndroidGetEnv();
	struct uiAndroidControl *t = new_view_control(uiTabSignature);

	jobject tab = (*env)->CallStaticObjectMethod(env, libui.class, libui.tab_layout_m);
	t->o = tab;

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

	view_set_layout(obj, ANDROID_LAYOUT_MATCH_PARENT, ANDROID_LAYOUT_WRAP_CONTENT);

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
	jclass class = (*env)->FindClass(env, "libui/LibUI$Popup");
	jmethodID constructor = (*env)->GetMethodID(env, class, "<init>", "(Ljava/lang/String;I)V");
	jobject obj = (*env)->NewObject(env, class, constructor, (*env)->NewStringUTF(env, title), 0);
	c->o = obj;

	return (uiWindow *)c;
}

uiGroup *uiNewGroup(const char *title) {
	JNIEnv *env = uiAndroidGetEnv();
	struct uiAndroidControl *f = new_view_control(uiGroupSignature);

	jobject form = (*env)->CallStaticObjectMethod(
			env, libui.class, libui.form_m,
			(*env)->NewStringUTF(env, title)
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
	JNIEnv *env = uiAndroidGetEnv();
	(*env)->CallStaticVoidMethod(
			env, libui.class, libui.form_add_m,
			f->c.o, (*env)->NewStringUTF(env, label), ((uiAndroidControl *)c)->o
	);
}

void uiButtonSetText(uiButton *b, const char *text) {
	view_set_text(uiViewFromControl(b), text);
}

void uiLabelSetText(uiLabel *l, const char *text) {
	view_set_text(uiViewFromControl(l), text);
}

void uiEntrySetText(uiEntry *e, const char *text) {
	view_set_text(uiViewFromControl(e), text);
}

void uiMultilineEntrySetText(uiMultilineEntry *e, const char *text) {
	view_set_text(uiViewFromControl(e), text);
}

void uiWindowSetChild(uiWindow *w, uiControl *child) {
	JNIEnv *env = uiAndroidGetEnv();
	if (((struct uiAndroidControl *)w)->is_activity) {
		ctx_set_content_view(uiViewFromControl((child)));
		return;
	}
	jclass class = (*env)->FindClass(env, "libui/LibUI$Popup");
	jmethodID m_set_child = (*env)->GetMethodID(env, class, "setChild", "(Landroid/view/View;)V");
	(*env)->CallVoidMethod(env, w->c.o, m_set_child, ((struct uiAndroidControl *)child)->o);
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

	jclass class = (*env)->FindClass(env, "android/view/ViewGroup");
	jmethodID add_view = (*env)->GetMethodID(env, class, "addView", "(Landroid/view/View;)V");

	(*env)->CallVoidMethod(env, b->c.o, add_view, ((struct uiAndroidControl *)child)->o);

	// Controls can optionally request to be set a certain width (only can be set after appending)
	if (ctl->request_width) view_set_dimensions(ctl->o, ctl->request_width, 0);
	if (ctl->request_height) view_set_dimensions(ctl->o, 0, ctl->request_height);
	(*env)->DeleteLocalRef(env, class);
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

	// setOnItemSelectedListener
	// setOnClickListener

	struct CallbackData call_data;
	call_data.fn_ptr = (uintptr_t)f;
	call_data.arg1 = (uintptr_t)b;
	call_data.arg2 = (uintptr_t)data;

	jobject *view = uiViewFromControl(b);

	jbyteArray arr = (*env)->NewByteArray(env, sizeof(call_data));
	(*env)->SetByteArrayRegion(env, arr, 0, sizeof(call_data), (const jbyte *)&call_data);

	jclass listener_c = (*env)->FindClass(env, "libui/LibUI$MyOnClickListener");
	jmethodID click_init = (*env)->GetMethodID(env, listener_c, "<init>", "([B)V");
	jobject listener = (*env)->NewObject(env, listener_c, click_init, arr);

	jmethodID set_click = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, view), "setOnClickListener", "(Landroid/view/View$OnClickListener;)V");
	(*env)->CallVoidMethod(env, view, set_click, listener);
}

void uiQueueMain(void (*f)(void *data), void *data) {
	JNIEnv *env = uiAndroidGetEnv();

	struct CallbackData call_data;
	call_data.fn_ptr = (uintptr_t)f;
	call_data.arg1 = (uintptr_t)data;
	call_data.arg2 = (uintptr_t)NULL;

	jbyteArray arr = (*env)->NewByteArray(env, sizeof(call_data));
	(*env)->SetByteArrayRegion(env, arr, 0, sizeof(call_data), (const jbyte *)&call_data);

	(*env)->CallStaticVoidMethod(
			env, libui.class, libui.add_runnable_m, arr
	);
}

void uiTabAppend(uiTab *t, const char *name, uiControl *c) {
	JNIEnv *env = uiAndroidGetEnv();
	((uiAndroidControl *)c)->o = (*env)->NewGlobalRef(env, ((uiAndroidControl *)c)->o);

	jstring jname = (*env)->NewStringUTF(env, name);
	(*env)->CallStaticVoidMethod(
			env, libui.class, libui.add_tab_m,
			t->c.o, jname, ((uiAndroidControl *)c)->o
	);

	(*env)->DeleteLocalRef(env, jname);
}

void uiToast(const char *format, ...) {
	va_list args;
	va_start(args, format);
	char buffer[256];
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	JNIEnv *env = uiAndroidGetEnv();
	(*env)->PushLocalFrame(env, 10);

	jstring jbuffer = (*env)->NewStringUTF(env, buffer);

	jclass toast_c = (*env)->FindClass(env, "android/widget/Toast");
	jmethodID make_text_m = (*env)->GetStaticMethodID(env, toast_c, "makeText", "(Landroid/content/Context;Ljava/lang/CharSequence;I)Landroid/widget/Toast;");
	jmethodID show_m = (*env)->GetMethodID(env, toast_c, "show", "()V");

	jobject toast = (*env)->CallStaticObjectMethod(env, toast_c, make_text_m, uiAndroidGetCtx(), jbuffer, 0x0);
	(*env)->CallVoidMethod(env, toast, show_m);

	(*env)->PopLocalFrame(env, NULL);
}

static jobject get_drawable_id(const char *name) {
	JNIEnv *env = uiAndroidGetEnv();
	jobject res = jni_get_resources(env, uiAndroidGetCtx());

	int id = get_res_id("drawable", name);
	if (id == 0) return NULL;

	jmethodID get_drawable = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, res), "getDrawable", "(ILandroid/content/res/Resources$Theme;)Landroid/graphics/drawable/Drawable;");

	jobject drawable = (*env)->CallObjectMethod(
		env, res, get_drawable, id, jni_get_theme(env, uiAndroidGetCtx())
	);

	(*env)->DeleteLocalRef(env, res);

	return drawable;
}

uiControl *uiExpandControl(char *name) {
	JNIEnv *env = uiAndroidGetEnv();

	(*env)->PushLocalFrame(env, 10);

	int id = get_res_id("layout", name);
	if (id == 0) return NULL;

	jobject inflater = jni_get_layout_inflater(env, uiAndroidGetCtx());

	jmethodID inflate_m = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, inflater), "inflate", "(ILandroid/view/ViewGroup;)Landroid/view/View;");

	jobject view = (*env)->CallObjectMethod(
			env, inflater, inflate_m, id, NULL
	);

	view = (*env)->PopLocalFrame(env, view);

	struct uiAndroidControl *c = new_view_control(uiBoxSignature);
	c->o = view;
	return (uiControl *)c;
}

const char *uiGet(const char *name) {
	JNIEnv *env = uiAndroidGetEnv();
	(*env)->PushLocalFrame(env, 10);
	jobject res = jni_get_resources(env, uiAndroidGetCtx());

	int id = get_res_id("string", name);
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

uintptr_t uiControlHandle(uiControl *c) {
	return (uintptr_t)((uiAndroidControl *)c)->o;
}

void uiControlSetAttr(uiControl *c, const char *key, const char *value) {
	JNIEnv *env = uiAndroidGetEnv();
	if (!strcmp(key, "drawable")) {
		jobject drawable = get_drawable_id(value);
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
	// If new context != old context, free or use hashOf crap solution blah blah
	local.env = env;
	local.ctx = (*env)->NewGlobalRef(env, context);

	if (libui.class == 0x0) {
		jclass class = (*env)->FindClass(env, "libui/LibUI");
		libui.class = (*env)->NewGlobalRef(env, class);

		jfieldID ctx_f = (*env)->GetStaticFieldID(env, class, "ctx", "Landroid/content/Context;");
		(*env)->SetStaticObjectField(env, class, ctx_f, context);

		libui.form_m = (*env)->GetStaticMethodID(env, class, "form", "(Ljava/lang/String;)Landroid/view/View;");
		libui.tab_layout_m = (*env)->GetStaticMethodID(env, class, "tabLayout", "()Landroid/view/View;");

		libui.add_tab_m = (*env)->GetStaticMethodID(env, class, "addTab", "(Landroid/view/View;Ljava/lang/String;Landroid/view/View;)V");
		libui.form_add_m = (*env)->GetStaticMethodID(env, class, "formAppend", "(Landroid/view/View;Ljava/lang/String;Landroid/view/View;)V");

		libui.add_runnable_m = (*env)->GetStaticMethodID(env, class, "runRunnable", "([B)V");
	}

	return 0;
}

uiBox *uiAndroidBox(JNIEnv *env, jobject context, jobject parent) {
	struct uiBox *box = malloc(sizeof(struct uiBox));
	uiAndroidInit(env, context);
	box->c.o = parent;
	return box;
}

#pragma GCC visibility pop

LIBUI(void, callFunction)(JNIEnv *env, jobject thiz, jbyteArray arr) {
	struct CallbackData *data = (struct CallbackData *)(*env)->GetByteArrayElements(env, arr, 0);

	if (data->fn_ptr == 0x0) abort();

	void (*ptr_f)(uintptr_t, uintptr_t) = (void *)data->fn_ptr;
	ptr_f(data->arg1, data->arg2);
}

LIBUI(jobject, callObjectFunction)(JNIEnv *env, jobject thiz, jbyteArray arr, jobjectArray args) {
	struct CallbackData *data = (struct CallbackData *)(*env)->GetByteArrayElements(env, arr, 0);

	if (data->fn_ptr == 0x0) abort();

	if ((*env)->GetArrayLength(env, args) >= 1) {
		data->arg1 = (uintptr_t)(*env)->GetObjectArrayElement(env, args, 0);
	}
	if ((*env)->GetArrayLength(env, args) >= 2) {
		data->arg2 = (uintptr_t)(*env)->GetObjectArrayElement(env, args, 1);
	}

	jobject (*ptr_f)(uintptr_t, uintptr_t) = (void *)data->fn_ptr;
	return ptr_f(data->arg1, data->arg2);
}

LIBUI(void, initThiz)(JNIEnv *env, jobject thiz, jobject ctx) {
	uiAndroidInit(env, ctx);
}

LIBUI(void, destructThiz)(JNIEnv *env, jobject thiz, jobject ctx) {
	//uiAndroidInit(env, ctx);
}