#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <android/log.h>
#include <stdlib.h>
#include <jni.h>

#include "android.h"

jobject view_new_linearlayout(JNIEnv *env, jobject ctx, int is_vertical, int x, int y) {
	jclass linear_layout_class = (*env)->FindClass(env, "android/widget/LinearLayout");
	jmethodID linear_layout_constructor = (*env)->GetMethodID(env, linear_layout_class, "<init>", "(Landroid/content/Context;)V");
	jobject linear_layout = (*env)->NewObject(env, linear_layout_class, linear_layout_constructor, ctx);

	view_set_layout(env, linear_layout, x, y);
	jmethodID set_orientation = (*env)->GetMethodID(env, linear_layout_class, "setOrientation", "(I)V");
	(*env)->CallVoidMethod(env, linear_layout, set_orientation, is_vertical);

	view_set_focusable(env, linear_layout);

	return linear_layout;
}

jobject view_new_tabhost(JNIEnv *env, jobject ctx) {
	jclass tab_host_class = (*env)->FindClass(env, "android/widget/TabHost");
	jmethodID tab_host_constructor = (*env)->GetMethodID(env, tab_host_class, "<init>", "(Landroid/content/Context;Landroid/util/AttributeSet;)V");
	jobject tab_host = (*env)->NewObject(env, tab_host_class, tab_host_constructor, ctx, NULL);
	view_set_layout(env, tab_host, ANDROID_LAYOUT_MATCH_PARENT, ANDROID_LAYOUT_MATCH_PARENT);

	jobject linear_layout = view_new_linearlayout(env, ctx, 1, ANDROID_LAYOUT_MATCH_PARENT, ANDROID_LAYOUT_MATCH_PARENT);

	jclass tab_widget_class = (*env)->FindClass(env, "android/widget/TabWidget");
	jmethodID tab_widget_constructor = (*env)->GetMethodID(env, tab_widget_class, "<init>", "(Landroid/content/Context;)V");
	jobject tab_widget = (*env)->NewObject(env, tab_widget_class, tab_widget_constructor, ctx);

	jclass view_class = (*env)->FindClass(env, "android/view/View");
	jmethodID set_id = (*env)->GetMethodID(env, view_class, "setId", "(I)V");
	(*env)->CallVoidMethod(env, tab_widget, set_id, 0x01020013); // android.R.id.tabs

	jclass frame_layout_class = (*env)->FindClass(env, "android/widget/FrameLayout");
	jmethodID frame_layout_constructor = (*env)->GetMethodID(env, frame_layout_class, "<init>", "(Landroid/content/Context;)V");
	jobject frame_layout = (*env)->NewObject(env, frame_layout_class, frame_layout_constructor, ctx);
	(*env)->CallVoidMethod(env, frame_layout, set_id, 0x01020011); // android.R.id.tabcontent

	jmethodID add_view = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, linear_layout), "addView", "(Landroid/view/View;)V");
	(*env)->CallVoidMethod(env, linear_layout, add_view, tab_widget);
	(*env)->CallVoidMethod(env, linear_layout, add_view, frame_layout);
	(*env)->CallVoidMethod(env, tab_host, add_view, linear_layout);

	jmethodID setup = (*env)->GetMethodID(env, tab_host_class, "setup", "()V");
	(*env)->CallVoidMethod(env, tab_host, setup);

	return tab_host;
}

void view_tabhost_add(JNIEnv *env, const char *name, jobject parent, jobject child) {
#if 0
	@SuppressWarnings("deprecation")
    public static void addTab(View parent, String name, View child) {
        TabHost tabHost = (TabHost)parent;
        TabHost.TabSpec tab1Spec = tabHost.newTabSpec(name);
        tab1Spec.setIndicator(name);
        tab1Spec.setContent(new TabHost.TabContentFactory() {
            public View createTabContent(String tag) {
                return child;
            }
        });
        tabHost.addTab(tab1Spec);
    }
#endif
	jclass tab_spec_class = (*env)->FindClass(env, "android/widget/TabHost$TabSpec");
	jclass tab_host_class = (*env)->FindClass(env, "android/widget/TabHost");
	jclass factory_c = (*env)->FindClass(env, "dev/danielc/libui/LibUI$MyTabFactory");
	jstring name_j = (*env)->NewStringUTF(env, name);
	jmethodID new_tab_spec_method = (*env)->GetMethodID(env, tab_host_class, "newTabSpec", "(Ljava/lang/String;)Landroid/widget/TabHost$TabSpec;");
	jobject tab_spec = (*env)->CallObjectMethod(env, parent, new_tab_spec_method, name_j);
	jmethodID set_indicator_method = (*env)->GetMethodID(env, tab_spec_class, "setIndicator", "(Ljava/lang/CharSequence;)Landroid/widget/TabHost$TabSpec;");
	(*env)->CallObjectMethod(env, tab_spec, set_indicator_method, name_j);

	jobject factory = (*env)->AllocObject(env, factory_c);

	jfieldID field = (*env)->GetFieldID(env, factory_c, "child", "Landroid/view/View;");
	(*env)->SetObjectField(env, factory, field, child);

	jmethodID set_content_m = (*env)->GetMethodID(env, tab_spec_class, "setContent", "(Landroid/widget/TabHost$TabContentFactory;)Landroid/widget/TabHost$TabSpec;");
	(*env)->CallObjectMethod(env, tab_spec, set_content_m, name_j);

	jmethodID add_tab_m = (*env)->GetMethodID(env, tab_host_class, "addTab", "(Landroid/widget/TabHost$TabSpec;)V");
	(*env)->CallVoidMethod(env, parent, add_tab_m, tab_spec);
}

void view_set_text_size(JNIEnv *env, jobject obj, float size) {
	static jmethodID method = 0; // faster?
	if (!method) method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, obj), "setTextSize", "(F)V");
	(*env)->CallVoidMethod(env, obj, method, size);
}

jint view_get_res_id(JNIEnv *env, jobject ctx, const char *key, const char *name) {
	(*env)->PushLocalFrame(env, 10);
	jobject res = jni_get_resources(env, ctx);

	jmethodID get_identifier = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, res), "getIdentifier", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I");

	jstring key_s = (*env)->NewStringUTF(env, name);
	jstring name_s = (*env)->NewStringUTF(env, key);
	jstring pkg_s = jni_get_package_name(env, ctx);

	jint id = (*env)->CallIntMethod(
		env, res, get_identifier,
		key_s, name_s,
		pkg_s
	);

	(*env)->PopLocalFrame(env, NULL);

	return id;
}

jobject view_get_by_id(JNIEnv *env, jobject ctx, const char *id) {
	int res_id = view_get_res_id(env, ctx, "id", id);

	jclass activity_class = (*env)->FindClass(env, "android/app/Activity");
	jmethodID find_view_m = (*env)->GetMethodID(env, activity_class, "findViewById", "(I)Landroid/view/View;");

	return (*env)->CallObjectMethod(env, ctx, find_view_m, res_id);
}

void ctx_set_content_view(JNIEnv *env, jobject ctx, jobject view) {
	jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, ctx), "setContentView", "(Landroid/view/View;)V");
	(*env)->CallVoidMethod(env, ctx, method, view);
}

void ctx_set_content_layout(JNIEnv *env, jobject ctx, const char *name) {
	jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, ctx), "setContentView", "(I)V");
	int id = view_get_res_id(env, ctx, "layout", name);
	if (id == 0) abort();
	(*env)->CallVoidMethod(env, ctx, method, id);
}

void view_set_visibility(JNIEnv *env, jobject view, int v) {
	// 0 = visible
	// 4 = invisible
	// 8 = gone
	jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, view), "setVisibility", "(I)V");
	(*env)->CallVoidMethod(env, view, method, v);
}

void view_set_dimensions(JNIEnv *env, jobject view, int w, int h) {
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

void view_set_layout(JNIEnv *env, jobject view, int x, int y) {
	jclass class = (*env)->FindClass(env, "android/widget/LinearLayout$LayoutParams");
	jmethodID constructor = (*env)->GetMethodID(env, class, "<init>", "(II)V");
	jobject obj = (*env)->NewObject(env, class, constructor, x, y);

	jmethodID method = (*env)->GetMethodID(env,
										   (*env)->GetObjectClass(env, view),
										   "setLayoutParams", "(Landroid/view/ViewGroup$LayoutParams;)V");
	(*env)->CallVoidMethod(env, view, method, obj);
}

void view_set_padding_px(JNIEnv *env, jobject obj, int padding) {
	int p = 10 * padding; // approx dp
	jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, obj), "setPadding", "(IIII)V");
	(*env)->CallVoidMethod(env, obj, method, p, p, p, p);
}

void view_set_focusable(JNIEnv *env, jobject obj) {
	jclass class = (*env)->GetObjectClass(env, obj);
	jmethodID set_clickable_m = (*env)->GetMethodID(env, class, "setClickable", "(Z)V");
	(*env)->CallVoidMethod(env, obj, set_clickable_m, 1);

	jmethodID focusable_m = (*env)->GetMethodID(env, class, "setFocusableInTouchMode", "(Z)V");
	(*env)->CallVoidMethod(env, obj, focusable_m, 1);

	jmethodID setFocusable_m = (*env)->GetMethodID(env, class, "setFocusable", "(Z)V");
	(*env)->CallVoidMethod(env, obj, setFocusable_m, 1);
}

char *view_get_text(JNIEnv *env, jobject view) {
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

void view_set_text(JNIEnv *env, jobject view, const char *text) {
	jstring jtext = (*env)->NewStringUTF(env, text);
	jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, view), "setText", "(Ljava/lang/CharSequence;)V");
	(*env)->CallVoidMethod(env, view, method, jtext);
	(*env)->DeleteLocalRef(env, jtext);
}

jobject combobox_get_adapter(JNIEnv *env, jobject ctx, jobject view) {
	jclass view_c = (*env)->GetObjectClass(env, view);
	jmethodID get_adapter_m = (*env)->GetMethodID(env, view_c, "getAdapter", "()Landroid/widget/SpinnerAdapter;");
	jobject adapter = (*env)->CallObjectMethod(env, view, get_adapter_m);

	if (adapter == NULL) {
		jclass arr_adapter_c = (*env)->FindClass(env, "android/widget/ArrayAdapter");
		jmethodID init_adapter_m = (*env)->GetMethodID(env, arr_adapter_c, "<init>", "(Landroid/content/Context;I)V");
		adapter = (*env)->NewObject(env, arr_adapter_c, init_adapter_m, ctx, ANDROID_simple_spinner_item);

		jmethodID set_adapter_m = (*env)->GetMethodID(env, view_c, "setAdapter", "(Landroid/widget/SpinnerAdapter;)V");
		(*env)->CallVoidMethod(env, view, set_adapter_m, adapter);

		jmethodID set_resource_id_m = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, adapter), "setDropDownViewResource", "(I)V");
		(*env)->CallVoidMethod(env, adapter, set_resource_id_m, ANDROID_simple_spinner_dropdown_item);
	}

	return adapter;
}

jobject get_drawable_id(JNIEnv *env, jobject ctx, const char *name) {
	jobject res = jni_get_resources(env, ctx);

	int id = view_get_res_id(env, ctx, "drawable", name);
	if (id == 0) return NULL;

	jmethodID get_drawable = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, res), "getDrawable", "(ILandroid/content/res/Resources$Theme;)Landroid/graphics/drawable/Drawable;");

	jobject drawable = (*env)->CallObjectMethod(
		env, res, get_drawable, id, jni_get_theme(env, ctx)
	);

	(*env)->DeleteLocalRef(env, res);

	return drawable;
}

jobject view_expand(JNIEnv *env, jobject ctx, const char *name) {
	(*env)->PushLocalFrame(env, 10);

	int id = view_get_res_id(env, ctx, "layout", name);
	if (id == 0) return NULL;

	jobject inflater = jni_get_layout_inflater(env, ctx);

	jmethodID inflate_m = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, inflater), "inflate", "(ILandroid/view/ViewGroup;)Landroid/view/View;");

	jobject view = (*env)->CallObjectMethod(
		env, inflater, inflate_m, id, NULL
	);

	view = (*env)->PopLocalFrame(env, view);
	return view;
}

void viewgroup_addview(JNIEnv *env, jobject parent, jobject child) {
	jclass class = (*env)->FindClass(env, "android/view/ViewGroup");
	jmethodID add_view = (*env)->GetMethodID(env, class, "addView", "(Landroid/view/View;)V");
	(*env)->CallVoidMethod(env, parent, add_view, child);
}

#if 0
public static View form(String name) {
        LinearLayout layout = new LinearLayout(ctx);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setLayoutParams(new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT));

        TextView title = new TextView(ctx);
        title.setPadding(5, 5, 5, 5);
        title.setTypeface(Typeface.DEFAULT_BOLD);
        title.setText(name);
        title.setLayoutParams(new LinearLayout.LayoutParams(
                LayoutParams.MATCH_PARENT,
                LayoutParams.WRAP_CONTENT
        ));

        layout.addView(title);

        return layout;
    }

    public static void formAppend(View form, String name, View child) {
        LinearLayout entry = new LinearLayout(ctx);
        entry.setOrientation(LinearLayout.HORIZONTAL);
        entry.setLayoutParams(new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT));

        TextView entryName = new TextView(ctx);
        entryName.setPadding(20, 10, 20, 10);
        entryName.setText(name);
        entryName.setLayoutParams(new LinearLayout.LayoutParams(
                LayoutParams.WRAP_CONTENT,
                LayoutParams.WRAP_CONTENT
        ));
        entry.addView(entryName);

        entry.addView(child);

        ((LinearLayout)form).addView(entry);
    }

    @SuppressWarnings("deprecation")
    public static View tabLayout() {
        TabHost tabHost = new TabHost(ctx, null);
        tabHost.setLayoutParams(new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT));

        LinearLayout linearLayout = new LinearLayout(ctx);
        linearLayout.setLayoutParams(new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT));
        linearLayout.setOrientation(LinearLayout.VERTICAL);

        TabWidget tabWidget = new TabWidget(ctx);
        tabWidget.setId(android.R.id.tabs);

        FrameLayout frameLayout = new FrameLayout(ctx);
        frameLayout.setId(android.R.id.tabcontent);

        linearLayout.addView(tabWidget);
        linearLayout.addView(frameLayout);
        tabHost.addView(linearLayout);

        tabHost.setup();

        return tabHost;
    }

    @SuppressWarnings("deprecation")
    public static void addTab(View parent, String name, View child) {
        TabHost tabHost = (TabHost)parent;
        TabHost.TabSpec tab1Spec = tabHost.newTabSpec(name);
        tab1Spec.setIndicator(name);
        tab1Spec.setContent(new TabHost.TabContentFactory() {
            public View createTabContent(String tag) {
                return child;
            }
        });
        tabHost.addTab(tab1Spec);
    }

    public static boolean handleBack(boolean allowBack) {
        if (allowBack) {
            ((Activity)ctx).finish();
            return true;
        }
        return false;
    }

    public static boolean handleOptions(MenuItem item, boolean allowBack) {
        switch (item.getItemId()) {
            case android.R.id.home:
                handleBack(allowBack);
                return true;
        }

        return ((Activity)ctx).onOptionsItemSelected(item);
    }

    // Being too fast doesn't feel right, brain need delay
    public static void userSleep() {
        try {
            Thread.sleep(100);
        } catch (Exception e) {}
    }

    public static class Popup {
        PopupWindow popupWindow;
        public void dismiss() {
            this.popupWindow.dismiss();
        }

        String title;

        public void setChild(View v) {
            LinearLayout rel = new LinearLayout(ctx);

            LinearLayout bar = new LinearLayout(ctx);
            rel.setPadding(10, 10, 10, 10);
            rel.setOrientation(LinearLayout.HORIZONTAL);
            rel.setLayoutParams(new ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT));

            TextView tv = new TextView(ctx);
            tv.setText(title);
            tv.setLayoutParams(new ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.WRAP_CONTENT,
                    ViewGroup.LayoutParams.MATCH_PARENT));
            tv.setPadding(20, 0, 0, 0);
            tv.setTextSize(20f);
            tv.setGravity(Gravity.CENTER);
            bar.addView(tv);

            rel.setOrientation(LinearLayout.VERTICAL);
            rel.setLayoutParams(new ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.MATCH_PARENT));

            rel.addView(bar);

            LinearLayout layout = new LinearLayout(ctx);
            layout.setPadding(20, 20, 20, 20);
            layout.setOrientation(LinearLayout.VERTICAL);
            layout.setLayoutParams(new ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT));

            layout.addView(v);
            rel.addView(layout);

            this.popupWindow.setContentView(rel);
            this.popupWindow.showAtLocation(((Activity)ctx).getWindow().getDecorView().getRootView(), Gravity.CENTER, 0, 0);
        }

        Popup(String title, int options) {
            userSleep();
            DisplayMetrics displayMetrics = new DisplayMetrics();
            ((Activity)ctx).getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
            int height = displayMetrics.heightPixels;
            int width = displayMetrics.widthPixels;
            this.title = title;

            this.popupWindow = new PopupWindow(
                    (int)(width / 1.2),
                    (int)(height / 1.9)
            );

            if (popupDrawableResource != 0) {
                this.popupWindow.setBackgroundDrawable(ctx.getResources().getDrawable(popupDrawableResource));
            }

            this.popupWindow.setOutsideTouchable(true);
        }
    }

    public static LibUI.Popup openWindow(String title, int options) {
        LibUI.Popup popup = new LibUI.Popup(title, options);
        return popup;
    }
#endif