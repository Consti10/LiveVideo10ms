package constantin.video.example

import android.Manifest
import android.annotation.SuppressLint
import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.view.View
import android.widget.ArrayAdapter
import androidx.appcompat.app.AppCompatActivity
import constantin.uvcintegration.TranscodeService
import constantin.video.core.AVideoSettings
import constantin.video.core.IsConnected
import constantin.video.core.player.VideoSettings
import constantin.video.example.databinding.ActivityMainBinding
import constantin.video.example.decodingperf.VideoActivityWithDatabase
import constantin.video.example.decodingperf.ViewBenchmarkDataActivity
import constantin.video.transmitter.AVideoTransmitter
import constantin.video.transmitter.AVideoTransmitterSettings

class MainActivity : AppCompatActivity(), View.OnClickListener {
    private lateinit var binding: ActivityMainBinding

    private lateinit var context: Context;
    private val permissionHelper= constantin.helper.RequestPermissionHelper(
            arrayOf(Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE,
                    Manifest.permission.ACCESS_FINE_LOCATION, Manifest.permission.CAMERA)
    );

    @SuppressLint("ApplySharedPref")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        permissionHelper.checkAndRequestPermissions(this)
        VideoSettings.initializePreferences(this, false)
        context = this

        val adapter = ArrayAdapter.createFromResource(this,
                R.array.video_test_files, android.R.layout.simple_spinner_item)


        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        binding.sVideoFileSelector.adapter = adapter
        //

        binding.bStartVideoActivity.setOnClickListener {
            val selectedTestFile = binding.sVideoFileSelector.getSelectedItem() as String
            val preferences = getSharedPreferences("pref_video", Context.MODE_PRIVATE)
            preferences.edit().putString(getString(R.string.VS_ASSETS_FILENAME_TEST_ONLY), selectedTestFile).commit()
            val intentVideoActivity = Intent()
            intentVideoActivity.setClass(context, VideoActivityWithDatabase::class.java)
            startActivity(intentVideoActivity)
        }
        binding.bStartSettingsActivity.setOnClickListener {
            val intent = Intent()
            intent.setClass(context, AVideoSettings::class.java)
            intent.putExtra(AVideoSettings.EXTRA_KEY, true)
            startActivity(intent)
        }
        binding.bStartTelemetry.setOnClickListener { IsConnected.openUSBTetherSettings(context) }
        binding.bStartViewDatabase.setOnClickListener { startActivity(Intent().setClass(context, ViewBenchmarkDataActivity::class.java)) }
        binding.bStartColorFormatsTester.setOnClickListener { startActivity(Intent().setClass(context, AColorFormatTester::class.java)) }
        binding.bStartTelemetry.setOnClickListener { startActivity(Intent().setClass(context, ATelemetryExample::class.java)) }
        binding.bStartGraphView.setOnClickListener { startActivity(Intent().setClass(context, GraphViewA::class.java)) }

        binding.bStartTranscodeService.setOnClickListener {
            TranscodeService.startTranscoding(context, "");
        }
        binding.bStopTranscodeService.setOnClickListener {
            TranscodeService.stopTranscoding(context);
        }
        binding.bStartStreamingServer.setOnClickListener {
            val intent = Intent().setClass(context, AVideoTransmitter::class.java)
            startActivity(intent)
        }
        binding.bStreamingServerSettings.setOnClickListener {
            // User chose the "Settings" item, show the app settings UI...
            val i = Intent()
            i.setClass(this, AVideoTransmitterSettings::class.java)
            startActivity(i)
        }
        binding.bTestFEC.setOnClickListener {
            //TestFEC.nativeTestFec();
        }
    }

    override fun onResume() {
        binding.sVideoFileSelector.isEnabled =
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
        fun VIDEO_TEST_FILES_FOR_DB(context: Context): Array<String> {
            return context.resources.getStringArray(R.array.video_test_files_for_db)
        }
    }
}
