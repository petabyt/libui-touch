package libui;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.DisplayMetrics;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.LinearLayout.LayoutParams;
import android.widget.PopupWindow;
import android.widget.RelativeLayout;
import android.widget.ScrollView;
import android.widget.Space;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.content.ContextCompat;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.lifecycle.Lifecycle;
import androidx.viewpager2.adapter.FragmentStateAdapter;
import androidx.viewpager2.widget.ViewPager2;

import com.google.android.material.tabs.TabLayout;

import java.util.ArrayList;

import dev.petabyt.camcontrol.Backend;
import dev.petabyt.camcontrol.Gallery;
import dev.petabyt.camcontrol.MainActivity;

public class LibUI {
    public static Context ctx;

    // uiWindow (popup) background drawable style resource
    public static int popupDrawableResource = 0;

    // Background drawable resource for buttons
    public static int buttonBackgroundResource = 0;

    // Common way of telling when activity is done loading (onCreate isn't it)
    public static void waitUntilActivityLoaded(Activity activity) {
        ViewTreeObserver viewTreeObserver = activity.getWindow().getDecorView().getViewTreeObserver();
        viewTreeObserver.addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
            @Override
            public void onGlobalLayout() {
                activity.getWindow().getDecorView().getViewTreeObserver().removeOnGlobalLayoutListener(this);
            }
        });
    }

    public static class MyFragment extends Fragment {
        ViewGroup view;
        MyFragment(ViewGroup v) {
            view = v;
        }
        @Nullable
        @Override
        public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
            return view;
        }
    }

    public static class MyFragmentStateAdapter extends FragmentStateAdapter {
        private ArrayList<ViewGroup> arrayList = new ArrayList<>();
        public MyFragmentStateAdapter(@NonNull FragmentManager fragmentManager, @NonNull Lifecycle lifecycle) {
            super(fragmentManager, lifecycle);
        }

        public void addViewGroup(ViewGroup vg) {
            arrayList.add(vg);
        }

        @NonNull
        @Override
        public Fragment createFragment(int position) {
            return new MyFragment(arrayList.get(position));
        }

        @Override
        public int getItemCount() {
            return arrayList.size();
        }
    }

    public static class MyOnClickListener implements View.OnClickListener {
        private long ptr;
        private long arg1;
        private long arg2;
        public MyOnClickListener(long ptr, long arg1, long arg2) {
            this.ptr = ptr;
            this.arg1 = arg1;
            this.arg2 = arg2;
        }

        @Override
        public void onClick(View v) {
            LibUI.callFunction(ptr, arg1, arg2);
        }
    }

    public static View button(String text) {
        Button b = new Button(ctx);

        if (buttonBackgroundResource != 0) {
            b.setBackground(ContextCompat.getDrawable(ctx, buttonBackgroundResource));
        }

        b.setLayoutParams(new LinearLayout.LayoutParams(
                LayoutParams.MATCH_PARENT,
                LayoutParams.MATCH_PARENT,
                1.0f
        ));

        b.setTextSize(14f);

        b.setText(text);
        return (View)b;
    }

    public static View label(String text) {
        TextView lbl = new TextView(ctx);
        lbl.setText(text);
        lbl.setTextSize(15f);
        return (View)lbl;
    }

    public static View tabLayout() {
        LinearLayout layout = new LinearLayout(ctx);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setLayoutParams(new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT));

        TabLayout tl = new TabLayout(ctx);
        tl.setLayoutParams(new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT));

        ViewPager2 pager = new ViewPager2(ctx);
        pager.setLayoutParams(new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT));

        MyFragmentStateAdapter frag = new MyFragmentStateAdapter(
                ((AppCompatActivity)ctx).getSupportFragmentManager(),
                ((AppCompatActivity)ctx).getLifecycle()
        );
        pager.setAdapter(frag);

        tl.addOnTabSelectedListener(new TabLayout.OnTabSelectedListener() {
            @Override
            public void onTabSelected(TabLayout.Tab tab) {
                pager.setCurrentItem(tab.getPosition());
            }

            @Override
            public void onTabUnselected(TabLayout.Tab tab) {
            }

            @Override
            public void onTabReselected(TabLayout.Tab tab) {
            }
        });

        pager.registerOnPageChangeCallback(new ViewPager2.OnPageChangeCallback() {
            @Override
            public void onPageSelected(int position) {
                tl.selectTab(tl.getTabAt(position));
            }
        });

        layout.addView(tl);
        layout.addView(pager);

        return layout;
    }

    private static void addTab(View parent, String name, View child) {
        // TabLayout is child 0, add a new tab
        TabLayout tl = (TabLayout)(((ViewGroup)parent).getChildAt(0));
        TabLayout.Tab tab = tl.newTab();
        tab.setText(name);
        tl.addTab(tab);

        // ViewPager2 is the second child, we can get the custom fragment adapter from it
        ViewPager2 vp = (ViewPager2)(((ViewGroup)parent).getChildAt(1));
        MyFragmentStateAdapter frag = (MyFragmentStateAdapter)vp.getAdapter();

        ScrollView sv = new ScrollView(ctx);
        sv.addView(child);

        frag.addViewGroup(sv);
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
            //bar.setBackgroundColor(Color.BLACK);

            Button back = new Button(ctx);
            back.setText("Close");
            if (buttonBackgroundResource != 0) {
                back.setBackground(ContextCompat.getDrawable(ctx, buttonBackgroundResource));
            }

            back.setTextSize(14f);

            back.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    dismiss();
                }
            });

            bar.addView(back);
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
            rel.setBackground(ContextCompat.getDrawable(ctx, popupDrawableResource));
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
            DisplayMetrics displayMetrics = new DisplayMetrics();
            ((Activity)ctx).getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
            int height = displayMetrics.heightPixels;
            int width = displayMetrics.widthPixels;
            this.title = title;

            ActionBar actionBar = ((AppCompatActivity)ctx).getSupportActionBar();
            actionBar.setDisplayHomeAsUpEnabled(true);
            actionBar.setTitle(title);

            this.popupWindow = new PopupWindow(
                    (int)(width / 1.1),
                    (int)(height / 1.4)
            );

            this.popupWindow.setOutsideTouchable(false);
        }
    }

    private static LibUI.Popup openWindow(String title, int options) {
        LibUI.Popup popup = new LibUI.Popup(title, options);
        return popup;
    }

    private static void setClickListener(View v, long ptr, long arg1, long arg2) {
        v.setOnClickListener(new MyOnClickListener(ptr, arg1, arg2));
    }

    private static void addView(View parent, View child) {
        ViewGroup g = (ViewGroup)parent;
        g.addView(child);
    }

    private static ViewGroup linearLayout(int orientation) {
        LinearLayout layout = new LinearLayout(ctx);
        layout.setOrientation(orientation);
        layout.setLayoutParams(new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT));
        return (ViewGroup)layout;
    }

    private static void setPadding(View v, int l, int t, int r, int b) {
        v.setPadding(l, t, r, b);
    }

    private static void setDimensions(View v, int w, int h) {
        if (w != 0) {
            v.getLayoutParams().width = w;
        }
        if (h != 0) {
            v.getLayoutParams().height = h;
        }
    }

    private static String getString(String name) {
        Resources res = ctx.getResources();
        return res.getString(res.getIdentifier(name, "string", ctx.getPackageName()));
    }

    private static void toast(String text) {
        Toast.makeText(ctx, text, Toast.LENGTH_SHORT).show();
    }

    private static void runRunnable(long ptr, long arg1, long arg2) {
        Handler handler = new Handler(Looper.getMainLooper());
        handler.post(new Runnable() {
            @Override
            public void run() {
                callFunction(ptr, arg1, arg2);
            }
        });
    }

    private static native void callFunction(long ptr, long arg1, long arg2);
}
