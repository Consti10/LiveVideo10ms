package constantin.video.example;


import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;

import org.hamcrest.Description;
import org.hamcrest.Matcher;
import org.hamcrest.TypeSafeMatcher;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import androidx.test.espresso.DataInteraction;
import androidx.test.espresso.ViewInteraction;
import androidx.test.filters.LargeTest;
import androidx.test.rule.ActivityTestRule;
import androidx.test.rule.GrantPermissionRule;
import androidx.test.runner.AndroidJUnit4;
import constantin.video.core.DecodingInfo;
import constantin.video.core.VideoNative.VideoNative;

import static androidx.test.espresso.Espresso.onData;
import static androidx.test.espresso.Espresso.onView;
import static androidx.test.espresso.Espresso.pressBack;
import static androidx.test.espresso.action.ViewActions.click;
import static androidx.test.espresso.matcher.ViewMatchers.isDisplayed;
import static androidx.test.espresso.matcher.ViewMatchers.withClassName;
import static androidx.test.espresso.matcher.ViewMatchers.withId;
import static androidx.test.espresso.matcher.ViewMatchers.withText;
import static org.hamcrest.Matchers.allOf;
import static org.hamcrest.Matchers.anything;
import static org.hamcrest.Matchers.is;

@LargeTest
@RunWith(AndroidJUnit4.class)
public class MainActivityTest {
    private static final int WAIT_TIME_LONG = 10*1000; //30 seconds

    @Rule
    public ActivityTestRule<MainActivity> mActivityTestRule = new ActivityTestRule<>(MainActivity.class);

    @Rule
    public ActivityTestRule<VideoActivity> mVideoActivityTestRule = new ActivityTestRule<>(VideoActivity.class,false,false);

    @Rule
    public GrantPermissionRule mGrantPermissionRule =
            GrantPermissionRule.grant(
                    "android.permission.READ_EXTERNAL_STORAGE",
                    "android.permission.WRITE_EXTERNAL_STORAGE");


    private void writeVideoSource(final int videoSource){
        final Context context=mActivityTestRule.getActivity();
        SharedPreferences sharedPreferences=context.getSharedPreferences("pref_video",Context.MODE_PRIVATE);
        sharedPreferences.edit().putInt(context.getString(R.string.VS_SOURCE),videoSource).commit();
    }


    private void testPlayVideo(){
        Intent i = new Intent();
        mVideoActivityTestRule.launchActivity(i);
        try { Thread.sleep(WAIT_TIME_LONG); } catch (InterruptedException e) { e.printStackTrace(); }
        final DecodingInfo info= mVideoActivityTestRule.getActivity().getDecodingInfo();
        validateDecodingInfo(info);
        mVideoActivityTestRule.finishActivity();
    }

    private static void validateDecodingInfo(final DecodingInfo info){
        assert info.nNALU<=0 : "nNalu<=0";
        assert info.nNALUSFeeded<=0 : "nNaluFeeded<=0";
        assert info.currentFPS<=10 : "info.currentFPS<=10";
        System.out.println(info.toString());
    }

    private void selectVideoFilename(final int which){
        onView(withId(R.id.s_videoFileSelector)).perform(click());
        DataInteraction appCompatCheckedTextView = onData(anything())
                .inAdapterView(childAtPosition(
                        withClassName(is("android.widget.PopupWindow$PopupBackgroundView")),
                        0))
                .atPosition(which);
        appCompatCheckedTextView.perform(click());
    }

    @Test
    public void mainActivityTest() {
        writeVideoSource(VideoNative.SOURCE_TYPE_ASSETS);
        selectVideoFilename(0);
        testPlayVideo();
        selectVideoFilename(1);
        testPlayVideo();
        selectVideoFilename(2);
        testPlayVideo();
        selectVideoFilename(3);
        testPlayVideo();
    }

    private static Matcher<View> childAtPosition(
            final Matcher<View> parentMatcher, final int position) {

        return new TypeSafeMatcher<View>() {
            @Override
            public void describeTo(Description description) {
                description.appendText("Child at position " + position + " in parent ");
                parentMatcher.describeTo(description);
            }

            @Override
            public boolean matchesSafely(View view) {
                ViewParent parent = view.getParent();
                return parent instanceof ViewGroup && parentMatcher.matches(parent)
                        && view.equals(((ViewGroup) parent).getChildAt(position));
            }
        };
    }
}
