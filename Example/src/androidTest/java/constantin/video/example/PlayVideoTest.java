package constantin.video.example;


import android.content.Intent;
import android.util.Log;

import androidx.test.filters.LargeTest;
import androidx.test.rule.ActivityTestRule;
import androidx.test.rule.GrantPermissionRule;

import org.junit.Rule;
import org.junit.Test;

import javax.annotation.Nullable;

import constantin.video.core.player.DecodingInfo;
import constantin.video.example.decodingperf.VideoActivityWithDatabase;

@LargeTest
public class PlayVideoTest {
    private static final int WAIT_TIME_LONG = 10*1000; //30 seconds

    @Rule
    public ActivityTestRule<VideoActivityWithDatabase> mVideoActivityTestRule = new ActivityTestRule<>(VideoActivityWithDatabase.class,false,false);

    @Rule
    public GrantPermissionRule mGrantPermissionRule =
            GrantPermissionRule.grant(
                    "android.permission.READ_EXTERNAL_STORAGE",
                    "android.permission.WRITE_EXTERNAL_STORAGE");

    @Test
    public void testFull() {
        Intent i = new Intent();
        mVideoActivityTestRule.launchActivity(i);
        try { Thread.sleep(WAIT_TIME_LONG); } catch (InterruptedException e) { e.printStackTrace(); }
        final DecodingInfo info= mVideoActivityTestRule.getActivity().getMDecodingInfo();
        validateDecodingInfo(info);
        mVideoActivityTestRule.finishActivity();
    }



    private static void validateDecodingInfo(final @Nullable DecodingInfo info){
        Log.d("XX",info.toString());
        assert info!=null : "info!=null";
        assert info.nNALU<=0 : "nNalu<=0";
        assert info.nNALUSFeeded<=0 : "nNaluFeeded<=0";
        assert info.currentFPS<=10 : "info.currentFPS<=10";
        System.out.println(info.toString());
    }

}

