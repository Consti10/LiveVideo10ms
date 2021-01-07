package constantin.video.example;

// Play all the test vides for N_ITERATIONS times

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;

import androidx.test.rule.ActivityTestRule;
import androidx.test.rule.GrantPermissionRule;

import org.junit.Rule;
import org.junit.Test;

import java.util.Objects;

import javax.annotation.Nullable;

import constantin.video.core.player.DecodingInfo;
import constantin.video.core.player.VideoSettings;
import constantin.video.example.decodingperf.VideoActivityWithDatabase;

public class PlayTestVideosRepeated {
    private static final int WAIT_TIME_SHORT = 5*1000; //n seconds
    private static final int N_ITERATIONS=1;

    @Rule
    public ActivityTestRule<MainActivity> mActivityTestRule = new ActivityTestRule<>(MainActivity.class);
    @Rule
    public ActivityTestRule<VideoActivityWithDatabase> mVideoActivityTestRule = new ActivityTestRule<>(VideoActivityWithDatabase.class,false,false);

    @Rule
    public GrantPermissionRule mGrantPermissionRule =
            GrantPermissionRule.grant(
                    "android.permission.READ_EXTERNAL_STORAGE",
                    "android.permission.WRITE_EXTERNAL_STORAGE",
                    "android.permission.INTERNET");


    private void writeVideoSource(final VideoSettings.VS_SOURCE videoSource){
        final Context context=mActivityTestRule.getActivity();
        VideoSettings.setVS_SOURCE(context,videoSource);
    }

    //Dang, I cannot get the Spinner work with an Espresso test
    private void selectVideoFilename(final int selectedFile){
        final Context context=mActivityTestRule.getActivity();
        final String filename= Objects.requireNonNull(MainActivity.Companion.VIDEO_TEST_FILES_FOR_DB(context))[selectedFile];
        VideoSettings.setVS_ASSETS_FILENAME_TEST_ONLY(context,filename);
    }

    private void testPlayVideo(){
        Intent i = new Intent();
        mVideoActivityTestRule.launchActivity(i);
        try { Thread.sleep(WAIT_TIME_SHORT); } catch (InterruptedException e) { e.printStackTrace(); }
        final DecodingInfo info= mVideoActivityTestRule.getActivity().getMDecodingInfo();
        validateDecodingInfo(info);
        mVideoActivityTestRule.finishActivity();
    }

    private static void validateDecodingInfo(final @Nullable DecodingInfo info){
        assert info!=null : "info!=null";
        System.out.println(info.toString());
        assert info.nNALU>0 : "nNalu<=0";
        assert info.nNALUSFeeded>0 : "nNaluFeeded<=0";
        assert info.currentFPS>10 : "info.currentFPS<=10";
    }


    @Test
    public void playAllTestVideosTest() {
        writeVideoSource(VideoSettings.VS_SOURCE.ASSETS);
        //Alternating, play x264 test video ,rpi cam, webcam
        for(int i=0;i<N_ITERATIONS;i++){
            for(int j=0;j<7;j++){
                selectVideoFilename(j);
                testPlayVideo();
            }
        }
    }

}

