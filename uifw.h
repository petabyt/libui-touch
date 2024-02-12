#ifndef LIBUI_ANDROID
#define LIBUI_ANDROID

#include <jni.h>

#ifndef LIBUI
#define LIBUI(ret, name) JNIEXPORT ret JNICALL Java_libui_LibUI_##name
#endif

// https://android.googlesource.com/platform/frameworks/base/+/2ca2c87/core/res/res/values/public.xml
#define ANDROID_progressBarStyleHorizontal 0x01010078

#define ANDROID_LAYOUT_MATCH_PARENT 0xffffffff
#define ANDROID_LAYOUT_WRAP_CONTENT 0xfffffffe

struct UILibAndroidEnv {
    int pid;
    JNIEnv *env;
    jobject ctx;
    jclass class;
    jstring package;
    jobject res;
    jobject theme;

    jmethodID layout_m;
    jmethodID form_m;
    jmethodID tab_layout_m;

    jmethodID form_add_m;
    jmethodID toast_m;
    jmethodID set_click_m;
    jmethodID add_tab_m;
    jmethodID add_runnable_m;
};

#define uiAreaSignature 0x41726561
#define uiBoxSignature 0x426F784C
#define uiButtonSignature 0x42746F6E
#define uiCheckboxSignature 0x43686B62
#define uiColorButtonSignature 0x436F6C42
#define uiComboboxSignature 0x436F6D62
#define uiDateTimePickerSignature 0x44545069
#define uiEditableComboboxSignature 0x45644362
#define uiEntrySignature 0x456E7472
#define uiFontButtonSignature 0x466F6E42
#define uiFormSignature 0x466F726D
#define uiGridSignature 0x47726964
#define uiGroupSignature 0x47727062
#define uiLabelSignature 0x4C61626C
#define uiMultilineEntrySignature 0x4D6C6E45
#define uiProgressBarSignature 0x50426172
#define uiRadioButtonsSignature 0x5264696F
#define uiSeparatorSignature 0x53657061
#define uiSliderSignature 0x536C6964
#define uiSpinboxSignature 0x5370696E
#define uiTabSignature 0x54616273
#define uiTableSignature 0x5461626C
#define uiWindowSignature 0x57696E64
#define uiScrollSignature 0x1234567

struct uiAndroidControl {
	uiControl c;
	jobject o;
	short request_width;
	short request_height;
};

typedef struct uiAndroidControl uiAndroidControl;

struct uiButton { struct uiAndroidControl c; };
struct uiLabel { struct uiAndroidControl c; };
struct uiWindow { struct uiAndroidControl c; int is_activity; };
struct uiBox { struct uiAndroidControl c; };
struct uiTab { struct uiAndroidControl c; };
struct uiProgressBar { struct uiAndroidControl c; };
struct uiSeparator { struct uiAndroidControl c; };
struct uiMultilineEntry { struct uiAndroidControl c; };
struct uiEntry { struct uiAndroidControl c; };
struct uiForm { struct uiAndroidControl c; };
struct uiScroll { struct uiAndroidControl c; };

typedef struct uiScroll uiScroll;
struct uiScroll *uiNewScroll();

const char *uiGet(const char *name);
void uiControlSetAttr(uiControl *c, const char *name, const char *value);

void uiLabelAlignment(uiControl *c, int align);
int uiAndroidInit(JNIEnv *env, jobject context);
void uiAndroidSetContent(uiControl *c);
void uiControlCenter(uiControl *c);

void uiToast(const char *format, ...);
void uiSwitchScreen(uiControl *content, const char *title);
void uiScreenAddIcon(const char *id, const char *title, void (*f)(void *data));

// Temporary JNI I/O routines
int libu_write_file(JNIEnv *env, char *path, void *data, size_t length);
void *libu_get_assets_file(JNIEnv *env, jobject ctx, char *filename, int *length);
void *libu_get_txt_file(JNIEnv *env, jobject ctx, char *filename);

void *uiAndroidGetEnv();
void *uiAndroidGetCtx();

#ifdef lua_h
int luaopen_libuilua(lua_State *L);
#endif

#endif
