package constantin.video.example

import android.Manifest
import android.annotation.SuppressLint
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.util.Log
import android.view.View
import android.widget.ArrayAdapter
import android.widget.Button
import android.widget.Spinner
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat

import java.util.ArrayList
import java.util.Arrays

import constantin.video.core.AVideoSettings
import constantin.video.core.IsConnected
import constantin.video.core.VideoNative.VideoNative

class MainActivity : AppCompatActivity(), View.OnClickListener {
    private var spinnerVideoTestFileFromAssets: Spinner?=null;
    private var context: Context?=null;
    private val missingPermission = ArrayList<String>()


    @SuppressLint("ApplySharedPref")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        checkAndRequestPermissions()
        VideoNative.initializePreferences(this, false)
        context = this

        spinnerVideoTestFileFromAssets = findViewById<Spinner>(R.id.s_videoFileSelector)
        val adapter = ArrayAdapter.createFromResource(this,
                R.array.video_test_files, android.R.layout.simple_spinner_item)
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        spinnerVideoTestFileFromAssets!!.adapter = adapter
        //
        val startVideoActivity = findViewById<Button>(R.id.b_startVideoActivity)
        startVideoActivity.setOnClickListener {
            val selectedTestFile = spinnerVideoTestFileFromAssets!!.selectedItemPosition
            val preferences = getSharedPreferences("pref_video", Context.MODE_PRIVATE)
            preferences.edit().putString(getString(R.string.VS_ASSETS_FILENAME_TEST_ONLY), ASSETS_TEST_VIDEO_FILE_NAMES[selectedTestFile]).commit()
            val intentVideoActivity = Intent()
            intentVideoActivity.setClass(context, VideoActivity::class.java)
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
    }

    override fun onResume() {
        spinnerVideoTestFileFromAssets!!.isEnabled =
                getSharedPreferences("pref_video", Context.MODE_PRIVATE).getInt(getString(R.string.VS_SOURCE), 0) == 2
        super.onResume()
    }


    override fun onClick(v: View) {}

    private fun checkAndRequestPermissions() {
        missingPermission.clear()
        for (eachPermission in REQUIRED_PERMISSION_LIST) {
            if (ContextCompat.checkSelfPermission(this, eachPermission) != PackageManager.PERMISSION_GRANTED) {
                missingPermission.add(eachPermission)
            }
        }
        if (!missingPermission.isEmpty()) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                val asArray = missingPermission.toTypedArray()
                Log.d("PermissionManager", "Request: " + Arrays.toString(asArray))
                ActivityCompat.requestPermissions(this, asArray, REQUEST_PERMISSION_CODE)
            }
        }
    }

    override fun onRequestPermissionsResult(requestCode: Int,
                                            permissions: Array<String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)

        // Check for granted permission and remove from missing list
        if (requestCode == REQUEST_PERMISSION_CODE) {
            for (i in grantResults.indices.reversed()) {
                if (grantResults[i] == PackageManager.PERMISSION_GRANTED) {
                    missingPermission.remove(permissions[i])
                }
            }
        }
        if (!missingPermission.isEmpty()) {
            checkAndRequestPermissions()
        }

    }

    public companion object {
        private val REQUIRED_PERMISSION_LIST = arrayOf(Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE)
        private val REQUEST_PERMISSION_CODE = 12345
        public val ASSETS_TEST_VIDEO_FILE_NAMES = arrayOf("testVideo.h264", "rpi.h264","720p_usb.h264", "Recording_360_short.h264",
                "360_test.h264","video360.h264","testRoom1_1920Mono.mp4")
    }
}
