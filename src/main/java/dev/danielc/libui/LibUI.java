package dev.danielc.libui;

import android.app.Activity;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.Typeface;
import android.os.Handler;
import android.os.Looper;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.LinearLayout.LayoutParams;
import android.widget.PopupWindow;
import android.widget.TabHost;
import android.widget.TabWidget;
import android.widget.TextView;
import android.widget.Toast;
import android.app.ActionBar;

public class LibUI {
    public static Context ctx = null;
    public static ActionBar actionBar = null;

    // uiWindow (popup) background drawable style resource
    public static int popupDrawableResource = 0;

    // Background drawable resource for buttons
    public static int buttonBackgroundResource = 0;

    public static Boolean useActionBar = true;

    public static void init(Activity act) {
        ctx = (Context)act;
    }

    public static void start(Activity act) {
        init(act);
        initThiz(ctx);
    }

    // Common way of telling when activity is done loading
    private static void waitUntilActivityLoaded(Activity activity) {
        ViewTreeObserver viewTreeObserver = activity.getWindow().getDecorView().getViewTreeObserver();
        viewTreeObserver.addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
            @Override
            public void onGlobalLayout() {
                activity.getWindow().getDecorView().getViewTreeObserver().removeOnGlobalLayoutListener(this);
                init();
            }
        });
    }

    private static void init() {
        if (useActionBar) {
            actionBar = ((Activity)ctx).getActionBar();
            actionBar.setDisplayHomeAsUpEnabled(true);
        }
    }

    public class CustomAdapter extends BaseAdapter {
        @Override
        public native int getCount();
        @Override
        public native Object getItem(int i);
        @Override
        public native long getItemId(int i);
        @Override
        public native View getView(int i, View view, ViewGroup viewGroup);
    }

    private static class MySelectListener implements AdapterView.OnItemSelectedListener {
        byte[] struct;
        @Override
        public native void onItemSelected(AdapterView<?> parent, View view, int position, long id);
        @Override
        public native void onNothingSelected(AdapterView<?> parent);
    }

    private static class MyOnClickListener implements View.OnClickListener {
        byte[] struct;
        @Override
        public native void onClick(View v);
    }

    @SuppressWarnings("deprecation")
    private static class MyTabFactory implements TabHost.TabContentFactory {
        byte[] struct;
        View child;
        public native View createTabContent(String tag);
    }

    private static class MyRunnable implements Runnable {
        byte[] struct;
        @Override
        public native void run();
    }

    public static native void callFunction(byte[] struct);
    public static native void initThiz(Context ctx);
}
