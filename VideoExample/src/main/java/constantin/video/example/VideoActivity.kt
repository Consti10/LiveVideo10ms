package constantin.video.example

import android.content.Context
import android.os.Build
import android.os.Bundle
import android.util.ArrayMap
import android.util.Log
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.View
import android.widget.Button
import android.widget.TextView

import androidx.appcompat.app.AppCompatActivity
import com.google.firebase.firestore.FieldValue

import com.google.firebase.firestore.FirebaseFirestore
import com.google.firebase.firestore.SetOptions

import constantin.video.core.DecodingInfo
import constantin.video.core.External.AspectFrameLayout
import constantin.video.core.IVideoParamsChanged
import constantin.video.core.VideoPlayer.VideoPlayer
import constantin.video.core.VideoPlayerSurfaceHolder

const val ID_OS_VERSIONS : String ="OSVersions";
const val ID_BUILD_MODEL : String ="BuildModel";
const val ID_BUILD_DEVICE : String ="BuildDevice";
const val ID_BUILD_MANUFACTURER : String = "BuildManufacturer";

class VideoActivity : AppCompatActivity(), SurfaceHolder.Callback, IVideoParamsChanged {
    private val TAG = this::class.java.simpleName
    private var context: Context? = null
    private var mAspectFrameLayout: AspectFrameLayout? = null
    //private var mVideoPlayer: VideoPlayer? = null
    private var mVideoPlayer: VideoPlayerSurfaceHolder?=null
    private var mTextViewStatistics : TextView?=null;

    var mDecodingInfo: DecodingInfo? = null
        private set
    internal var lastLogMS = System.currentTimeMillis()
    private var VS_SOURCE: Int = 0
    private var VS_ASSETS_FILENAME_TEST_ONLY: String? = ""
    private val DECODING_INFO: String ="DecodingInfo2";


    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        context = this
        setContentView(R.layout.activity_video)
        //
        val surfaceView = findViewById<SurfaceView>(R.id.sv_video)
        surfaceView.holder.addCallback(this)
        mVideoPlayer= VideoPlayerSurfaceHolder(this,surfaceView);
        mVideoPlayer!!.setIVideoParamsChanged(this)

        mAspectFrameLayout = findViewById(R.id.afl_video)
        mTextViewStatistics=findViewById(R.id.tv_decoding_stats)
        //Find the toggle button. If user taps on it, the toggle button itself
        //is disabled and the view showing decoding stats is enabled
        val toggleButton=findViewById<Button>(R.id.tb_show_decoding_info);
        toggleButton.setOnClickListener{
            mTextViewStatistics!!.visibility= View.VISIBLE;
            toggleButton.visibility=View.INVISIBLE;
        }
        mTextViewStatistics!!.setOnClickListener{
            mTextViewStatistics!!.visibility= View.INVISIBLE;
            toggleButton.visibility=View.VISIBLE;
        }
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        val preferences = getSharedPreferences("pref_video", Context.MODE_PRIVATE)
        VS_SOURCE = preferences.getInt(getString(R.string.VS_SOURCE), 0)
        VS_ASSETS_FILENAME_TEST_ONLY = preferences.getString(getString(R.string.VS_ASSETS_FILENAME_TEST_ONLY), "")
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {}

    override fun surfaceDestroyed(holder: SurfaceHolder) {}

    override fun onVideoRatioChanged(videoW: Int, videoH: Int) {
        runOnUiThread { mAspectFrameLayout!!.setAspectRatio(videoW.toDouble() / videoH) }
    }

    public override fun onDestroy() {
        super.onDestroy()
        writeTestResult();
    }

    private fun writeTestResult() {
        if (mDecodingInfo != null && mDecodingInfo!!.nNALUSFeeded > 120) {
            val db = FirebaseFirestore.getInstance()
            val mTestResult=TestResultDecodingInfoConstructor.create(VS_SOURCE,(if (VS_SOURCE == VideoPlayer.VS_SOURCE.ASSETS.ordinal)
                VS_ASSETS_FILENAME_TEST_ONLY
            else
                "Unknown")!!,mDecodingInfo!!);
            val writeBatch = db.batch()
            //val allCombinationsReference=db.collection(DECODING_INFO).document("ALL_DEVICE_OS_COMBINATIONS")
            //writeBatch.set(allCombinationsReference,"",FieldValue.arrayUnion(Helper.getBuildVersionRelease()),SetOptions.merge())
            val thisDeviceReference = db.collection(DECODING_INFO).document(Helper.getManufacturerAndDeviceName())
            val dummyMap = ArrayMap<String, Any>()
            //dummyMap["OsVersions."+Helper.getBuildVersionRelease().replace(".","_")] = null
            dummyMap[ID_BUILD_MODEL] = Build.MODEL
            dummyMap[ID_BUILD_MANUFACTURER] = Build.MANUFACTURER
            dummyMap[ID_BUILD_DEVICE]=Build.DEVICE
            writeBatch.set(thisDeviceReference, dummyMap, SetOptions.merge())
            writeBatch.update(thisDeviceReference,ID_OS_VERSIONS,FieldValue.arrayUnion(Helper.getBuildVersionRelease()))
            val thisDeviceOsNewTestData = thisDeviceReference.collection(Helper.getBuildVersionRelease()).document()
            writeBatch.set(thisDeviceOsNewTestData, mTestResult)
            //writeBatch.update(thisDeviceOsNewTestData,mDecodingInfo!!.toMap());
            writeBatch.commit().addOnSuccessListener {
                Log.d(TAG, "WriteBatch added DocumentSnapshot with id: " + thisDeviceOsNewTestData.id)
            }
        }
    }

    override fun onDecodingInfoChanged(decodingInfo: DecodingInfo) {
        runOnUiThread(java.lang.Runnable {
            mTextViewStatistics!!.setText(decodingInfo.toString(true))
        })
        mDecodingInfo = decodingInfo
        /*if (System.currentTimeMillis() - lastLogMS > 5 * 1000) {
            Log.d(TAG,mDecodingInfo!!.toString());
            lastLogMS = System.currentTimeMillis()
        }*/
    }


}
