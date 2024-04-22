#ifndef LIBUI_ANDROID
#define LIBUI_ANDROID

#include <jni.h>

/**
 * Get localized string from name.
 * @param name
 * @return
 */
const char *uiGet(const char *name);

/**
 * Generic - Set an attribute of a control.
 * @param c
 * @param name
 * @param value
 */
void uiControlSetAttr(uiControl *c, const char *name, const char *value);

/**
 * Set text alignment of a label.
 * @param c
 * @param align Accepts enum uiDrawTextAlign
 */
void uiLabelAlignment(uiControl *c, int align);

/**
 * Initializes LibUI for the current context (activity)
 * @param env
 * @param context
 * @return
 */
int uiAndroidInit(JNIEnv *env, jobject context);

/**
 * Sets the activity main view/control
 * @param c
 */
void uiAndroidSetContent(uiControl *c);
/**
 * Sets the gravity of a control
 * @param c
 */
void uiControlCenter(uiControl *c);

/**
 * Standard Android toast
 * @param format
 * @param ...
 */
void uiToast(const char *format, ...);

/**
 * Add an icon to the action bar
 * @param id
 * @param title
 * @param f
 */
void uiScreenAddIcon(const char *id, const char *title, void (*f)(void *data));

/**
 * Get child at index in box
 * @param box
 * @param index
 * @return
 */
uiControl *uiBoxChild(uiBox *box, int index);

/**
 * Get control from native string ID (R.id.*)
 * @param id
 * @return
 */
uiControl *uiControlFromID(const char *id);

/**
 * Inflate layout from native layout name (R.layout.*)
 * @param name
 * @return
 */
uiControl *uiExpandControl(char *name);

/**
 * Get a file from the 'assets' directory of the app
 * @param env
 * @param ctx
 * @param filename Plain filename
 * @param length Will store length of file here
 * @return
 */
void *jni_get_assets_file(JNIEnv *env, jobject ctx, char *filename, int *length);
/**
 * Extension of jni_get_assets_file, returns a NULL-terminated text file
 * @param env
 * @param ctx
 * @param filename
 * @return
 */
void *ui_get_txt_file(JNIEnv *env, jobject ctx, char *filename);

#ifdef lua_h
int luaopen_libuilua(lua_State *L);
#endif

/**
 * Get jobject ctx based on the caller's thread info
 * @return
 */
void *uiAndroidGetCtx();

/**
 * Get JNIEnv based on the caller's thread info
 * @return
 */
void *uiAndroidGetEnv();

#endif
