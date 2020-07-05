package constantin.telemetry.example;


import android.content.Intent;

import androidx.test.filters.LargeTest;
import androidx.test.rule.ActivityTestRule;
import androidx.test.rule.GrantPermissionRule;
import androidx.test.runner.AndroidJUnit4;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import constantin.telemetry.core.ASettingsTelemetry;

@LargeTest
public class MainActivityTest {
    private static final int WAIT_TIME_SHORT = 5*1000; //n seconds
    private static final int N_ITERATIONS=1;

    @Rule
    public ActivityTestRule<MainActivity> mActivityTestRule = new ActivityTestRule<>(MainActivity.class);
    @Rule
    public ActivityTestRule<ASettingsTelemetry> mSettingsActivityTestRule = new ActivityTestRule<>(ASettingsTelemetry.class,false,false);

    @Rule
    public GrantPermissionRule mGrantPermissionRule =
            GrantPermissionRule.grant(
                    "android.permission.ACCESS_FINE_LOCATION",
                    "android.permission.WRITE_EXTERNAL_STORAGE");

    @Test
    public void mainActivityTest() {
        for(int n=0;n<N_ITERATIONS;n++){
            Intent i = new Intent();
            mSettingsActivityTestRule.launchActivity(i);
            try { Thread.sleep(WAIT_TIME_SHORT); } catch (InterruptedException e) { e.printStackTrace(); }
            mSettingsActivityTestRule.finishActivity();
            try { Thread.sleep(WAIT_TIME_SHORT); } catch (InterruptedException e) { e.printStackTrace(); }
        }
    }
}
