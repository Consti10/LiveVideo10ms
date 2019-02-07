package com.example.videotester;


import android.content.Intent;
import android.support.test.filters.LargeTest;
import android.support.test.rule.ActivityTestRule;
import android.support.test.runner.AndroidJUnit4;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import constantin.lowlagvideo.DecodingInfo;
import videotester.MainActivity;
import videotester.VideoActivity;

@LargeTest
@RunWith(AndroidJUnit4.class)
public class PlayVideoTest {
    private static final int WAIT_TIME_LONG = 30*1000; //30 seconds

    @Rule
    public ActivityTestRule<VideoActivity> mActivityTestRule = new ActivityTestRule<>(VideoActivity.class,false,false);


    @Test
    public void testFull() {
        testActivityWithVideoFile(0);
        testActivityWithVideoFile(1);
        testActivityWithVideoFile(2);
    }


    private void testActivityWithVideoFile(final int whichFile){
        Intent i = new Intent();
        i.putExtra(MainActivity.INNTENT_EXTRA_NAME, whichFile);
        mActivityTestRule.launchActivity(i);
        try { Thread.sleep(WAIT_TIME_LONG); } catch (InterruptedException e) { e.printStackTrace(); }

        final DecodingInfo info=mActivityTestRule.getActivity().getDecodingInfo();
        validateDecodingInfo(info);
        mActivityTestRule.finishActivity();
    }


    private static void validateDecodingInfo(final DecodingInfo info){
        assert info.nNALU<=0 : "nNalu<=0";
        assert info.nNALUSFeeded<=0 : "nNaluFeeded<=0";
        assert info.currentFPS<=10 : "info.currentFPS<=10";
        System.out.println(info.toString());
    }

}

