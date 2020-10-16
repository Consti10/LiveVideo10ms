package constantin.video.example

import android.Manifest
import android.annotation.SuppressLint
import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.view.View
import android.widget.ArrayAdapter
import android.widget.Button
import android.widget.Spinner
import androidx.appcompat.app.AppCompatActivity
import constantin.uvcintegration.TranscodeService
import constantin.video.core.AVideoSettings
import constantin.video.core.IsConnected
import constantin.helper.RequestPermissionHelper
import constantin.video.core.TestFEC
import constantin.video.core.player.VideoSettings
import constantin.video.example.decodingperf.VideoActivityWithDatabase
import constantin.video.example.decodingperf.ViewBenchmarkDataActivity
import constantin.video.transmitter.AVideoTransmitterSettings
import constantin.video.transmitter.AVideoTransmitter

class MainActivity : AppCompatActivity(), View.OnClickListener {
    private lateinit var spinnerVideoTestFileFromAssets: Spinner;
    private lateinit var context: Context;
    private val permissionHelper= constantin.helper.RequestPermissionHelper(
            arrayOf(Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE,
                    Manifest.permission.ACCESS_FINE_LOCATION, Manifest.permission.CAMERA)
    );

    @SuppressLint("ApplySharedPref")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        permissionHelper.checkAndRequestPermissions(this)
        VideoSettings.initializePreferences(this, false)
        context = this

        spinnerVideoTestFileFromAssets = findViewById<Spinner>(R.id.s_videoFileSelector)
        val adapter = ArrayAdapter.createFromResource(this,
                R.array.video_test_files, android.R.layout.simple_spinner_item)
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        spinnerVideoTestFileFromAssets.adapter = adapter
        //
        val startVideoActivity = findViewById<Button>(R.id.b_startVideoActivity)
        startVideoActivity.setOnClickListener {
            val selectedTestFile = spinnerVideoTestFileFromAssets.selectedItemPosition
            val preferences = getSharedPreferences("pref_video", Context.MODE_PRIVATE)
            preferences.edit().putString(getString(R.string.VS_ASSETS_FILENAME_TEST_ONLY), ASSETS_TEST_VIDEO_FILE_NAMES[selectedTestFile]).commit()
            val intentVideoActivity = Intent()
            intentVideoActivity.setClass(context, VideoActivityWithDatabase::class.java)
            startActivity(intentVideoActivity)
        }
        val startSettingsActivity = findViewById<Button>(R.id.b_startSettingsActivity)
        startSettingsActivity.setOnClickListener {
            val intent = Intent()
            intent.setClass(context, AVideoSettings::class.java)
            intent.putExtra(AVideoSettings.EXTRA_KEY, true)
            startActivity(intent)
        }
        findViewById<View>(R.id.b_startTethering).setOnClickListener { IsConnected.openUSBTetherSettings(context) }
        findViewById<View>(R.id.b_startViewDatabase).setOnClickListener { startActivity(Intent().setClass(context, ViewBenchmarkDataActivity::class.java)) }
        findViewById<View>(R.id.b_startColorFormatsTester).setOnClickListener { startActivity(Intent().setClass(context, AColorFormatTester::class.java)) }
        findViewById<View>(R.id.b_startTelemetry).setOnClickListener { startActivity(Intent().setClass(context, ATelemetryExample::class.java)) }
        findViewById<View>(R.id.b_startGraphView).setOnClickListener { startActivity(Intent().setClass(context, GraphViewA::class.java)) }

        findViewById<View>(R.id.b_startTranscodeService).setOnClickListener {
            TranscodeService.startTranscoding(context, "");
        }
        findViewById<View>(R.id.b_stopTranscodeService).setOnClickListener {
            TranscodeService.stopTranscoding(context);
        }
        findViewById<View>(R.id.b_StartStreamingServer).setOnClickListener {
            val intent = Intent().setClass(context, AVideoTransmitter::class.java)
            startActivity(intent)
        }
        findViewById<View>(R.id.b_StreamingServerSettings).setOnClickListener {
            // User chose the "Settings" item, show the app settings UI...
            val i = Intent()
            i.setClass(this, AVideoTransmitterSettings::class.java)
            startActivity(i)
        }
        findViewById<View>(R.id.b_TestFEC).setOnClickListener {
            TestFEC.nativeTestFec();
        }
    }

    override fun onResume() {
        spinnerVideoTestFileFromAssets.isEnabled =
                getSharedPreferences("pref_video", Context.MODE_PRIVATE).getInt(getString(R.string.VS_SOURCE), 0) == 2
        super.onResume()
    }

    override fun onClick(v: View) {}


    override fun onRequestPermissionsResult(requestCode: Int,
                                            permissions: Array<String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        permissionHelper.onRequestPermissionsResult(requestCode, permissions, grantResults)
    }

    public companion object {
        public val ASSETS_TEST_VIDEO_FILE_NAMES = arrayOf("x264/testVideo.h264", "rpi_cam/rpi.h264", "webcam/720p_usb.h264", "360/insta_interference.h264",
                "360/insta_webbn_1_shortened.h264", "360/insta_webbn_2.h264", "360/insta_mjpeg_test.mp4"
                //,"360/insta_mjpeg_test.h264")
                , "360/testRoom1_1920Mono.mp4")
    }
}
