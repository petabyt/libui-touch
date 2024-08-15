package dev.danielc.libui;

import android.app.Activity;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.Typeface;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.DisplayMetrics;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.view.animation.AnimationUtils;
import android.widget.AdapterView;
import android.widget.LinearLayout;
import android.widget.LinearLayout.LayoutParams;
import android.widget.PopupWindow;
import android.widget.ScrollView;
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

public class AndroidX {
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

    public static View tabLayout(Context ctx) {
        LinearLayout layout = new LinearLayout(ctx);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setLayoutParams(new LinearLayout.LayoutParams(
                LayoutParams.MATCH_PARENT,
                LayoutParams.MATCH_PARENT,
                1.0f
        ));

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
            public void onTabUnselected(TabLayout.Tab tab) {}
            @Override
            public void onTabReselected(TabLayout.Tab tab) {}
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

    public static void addTab(Context ctx, View parent, String name, View child) {
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
}