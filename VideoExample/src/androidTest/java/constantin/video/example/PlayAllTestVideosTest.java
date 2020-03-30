package constantin.video.example;


import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;

import androidx.test.filters.LargeTest;
import androidx.test.rule.ActivityTestRule;
import androidx.test.rule.GrantPermissionRule;
import androidx.test.runner.AndroidJUnit4;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import javax.annotation.Nullable;

import constantin.video.core.DecodingInfo;
import constantin.video.core.VideoPlayer.VideoPlayer.VideoNative;


@LargeTest
@RunWith(AndroidJUnit4.class)
public class PlayAllTestVideosTest {
    private static final int WAIT_TIME_LONG = 10*1000; //10 seconds

    @Rule
    public ActivityTestRule<MainActivity> mActivityTestRule = new ActivityTestRule<>(MainActivity.class);

    @Rule
    public ActivityTestRule<VideoActivity> mVideoActivityTestRule = new ActivityTestRule<>(VideoActivity.class,false,false);

    @Rule
    public GrantPermissionRule mGrantPermissionRule =
            GrantPermissionRule.grant(
                    "android.permission.READ_EXTERNAL_STORAGE",
                    "android.permission.WRITE_EXTERNAL_STORAGE",
                    "android.permission.INTERNET");


    private void writeVideoSource(final int videoSource){
        final Context context=mActivityTestRule.getActivity();
        SharedPreferences sharedPreferences=context.getSharedPreferences("pref_video",Context.MODE_PRIVATE);
        sharedPreferences.edit().putInt(context.getString(R.string.VS_SOURCE),videoSource).commit();
    }

    //Dang, I cannot get the Spinner work with an Espresso test
    private void selectVideoFilename(final int selectedFile){
        final Context context=mActivityTestRule.getActivity();
        SharedPreferences sharedPreferences=context.getSharedPreferences("pref_video",Context.MODE_PRIVATE);
        sharedPreferences.edit().putString(context.getString(R.string.VS_ASSETS_FILENAME_TEST_ONLY), MainActivity.Companion.getASSETS_TEST_VIDEO_FILE_NAMES()[selectedFile]).commit();
    }

    private void testPlayVideo(){
        Intent i = new Intent();
        mVideoActivityTestRule.launchActivity(i);
        try { Thread.sleep(WAIT_TIME_LONG); } catch (InterruptedException e) { e.printStackTrace(); }
        final DecodingInfo info= mVideoActivityTestRule.getActivity().getMDecodingInfo();
        validateDecodingInfo(info);
        mVideoActivityTestRule.finishActivity();
    }

    private static void validateDecodingInfo(final @Nullable DecodingInfo info){
        assert info!=null : "info!=null";
        assert info.nNALU<=0 : "nNalu<=0";
        assert info.nNALUSFeeded<=0 : "nNaluFeeded<=0";
        assert info.currentFPS<=10 : "info.currentFPS<=10";
        System.out.println(info.toString());
    }


    @Test
    public void playAllTestVideosTest() {
        writeVideoSource(VideoNative.VS_SOURCE_ASSETS);
        for(int i=0;i<7;i++){
            selectVideoFilename(i);
            testPlayVideo();
        }
    }

}
