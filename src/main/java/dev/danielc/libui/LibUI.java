package dev.danielc.libui;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.Application;
import android.content.Context;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.MenuItem;
import android.view.SurfaceHolder;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.CompoundButton;
import android.widget.TabHost;

import java.util.List;

public class LibUI {
    public static Context ctx = null; // leaks memory, needs weakreference

    // uiWindow (popup) background drawable style resource
    public static int popupDrawableResource = 0;

    // Background drawable resource for buttons
    public static int buttonBackgroundResource = 0;

    public static class MyActivityLifecycle implements Application.ActivityLifecycleCallbacks {
        @Override
        public native void onActivityCreated(Activity activity, Bundle bundle);
        @Override
        public native void onActivityStarted(Activity activity);
        @Override
        public native void onActivityResumed(Activity activity);
        @Override
        public native void onActivityPaused(Activity activity);
        @Override
        public native void onActivityStopped(Activity activity);
        @Override
        public native void onActivitySaveInstanceState(Activity activity, Bundle bundle);
        @Override
        public native void onActivityDestroyed(Activity activity);
    }

    public static class CustomAdapter extends BaseAdapter {
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

    private static class MySurface implements SurfaceHolder.Callback {
        byte[] struct;
        @Override
        public native void surfaceCreated(SurfaceHolder surfaceHolder);
        @Override
        public native void surfaceChanged(SurfaceHolder surfaceHolder, int i, int i1, int i2);
        @Override
        public native void surfaceDestroyed(SurfaceHolder surfaceHolder);
    }

    private static class MyCheckedListener implements CompoundButton.OnCheckedChangeListener {
        byte[] struct;
        @Override
        public native void onCheckedChanged(CompoundButton buttonView, boolean isChecked);
    }

    private static class MyTextWatcher implements TextWatcher {
        View view;
        byte[] struct;
        @Override
        public native void beforeTextChanged(CharSequence s, int start, int count, int after);
        @Override
        public native void onTextChanged(CharSequence s, int start, int before, int count);
        @Override
        public native void afterTextChanged(Editable s);
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
        Context ctx;
        @Override
        public native void run();
    }

    public static class DummyActivity extends Activity {
        byte[] struct;
        @SuppressLint("MissingSuperCall")
        @Override
        protected native void onCreate(Bundle savedInstanceState);
        @Override
        public native boolean onOptionsItemSelected(MenuItem item);
        @Override
        @SuppressLint("MissingSuperCall")
        public native void onDestroy();
    }
}
