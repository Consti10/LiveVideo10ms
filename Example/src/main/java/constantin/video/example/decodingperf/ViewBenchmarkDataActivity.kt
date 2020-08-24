package constantin.video.example.decodingperf

import android.annotation.SuppressLint
import android.content.Context
import android.os.Bundle
import android.util.Log
import android.util.Pair
import android.view.View
import android.widget.AdapterView
import android.widget.ArrayAdapter
import android.widget.Spinner
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import com.google.android.gms.tasks.OnCompleteListener
import com.google.firebase.firestore.FirebaseFirestore
import com.jaredrummler.android.device.DeviceName
import constantin.video.example.Helper
import constantin.video.example.R
import java.text.DecimalFormat
import java.util.*


@SuppressLint("SetTextI18n")
class ViewBenchmarkDataActivity : AppCompatActivity() {
    private val TAG = this::class.java.simpleName
    //They become valid once the firebase request finished
    //private val deviceNames = ArrayList<Pair<String,String>>()
    private val deviceNames = ArrayList<String>()
    private val deviceNamesSimple=ArrayList<String>();

    private val osVersions = ArrayList<List<String>>()

    private var spinnerDeviceNames: Spinner? = null
    private var spinnerOSVersions: Spinner? = null
    private var context: Context? = null
    //Avoid fetching the same data multiple times
    private var lastFetchedConfiguration = Pair("", "")
    internal val db = FirebaseFirestore.getInstance()

    private var tvDataRpiCam: TextView? = null
    private var tvDataX264: TextView? = null
    private var tvDataInsta360: TextView? = null

    private val DECODING_INFO: String ="DecodingInfo2";

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_view_data)
        context = this
        //textView=findViewById(R.id.textView2);
        val device = Helper.getManufacturerAndDeviceName()
        val os = Helper.getBuildVersionRelease()
        (findViewById<View>(R.id.textViewDevice) as TextView).text = "Device:$device"
        (findViewById<View>(R.id.textViewOS) as TextView).text = "OS:$os"
        spinnerDeviceNames = findViewById(R.id.spinner_device)
        spinnerOSVersions = findViewById(R.id.spinner_os)
        tvDataRpiCam = findViewById(R.id.tv_data_rpiCam)
        tvDataX264 = findViewById(R.id.tv_data_x264)
        tvDataInsta360 = findViewById(R.id.tv_data_insta360)

        //This one is to populate the spinner with all tested device names / OS combos
        db.collection(DECODING_INFO).get().addOnCompleteListener(OnCompleteListener { task ->
            if (task.isSuccessful) {
                val documentSnapshots = task.result ?: return@OnCompleteListener
                val documents = documentSnapshots.documents
                for (i in documents.indices) {
                    val doc = documentSnapshots.documents[i]
                    //DeviceName.getDeviceName();
                    //val buildModel=doc.getString(ID_BUILD_MODEL);
                    //val buildManufacturer=doc.getString(ID_BUILD_MANUFACTURER);
                    //DeviceName.getDeviceName(Build.DEVICE!,)
                    //val deviceName = Pair(doc.id,doc.id)
                    //System.out.println(""+DeviceName.getDeviceName("","","""))
                    val simpleDeviceName=DeviceName.getDeviceName(doc.getString(ID_BUILD_DEVICE),doc.getString(ID_BUILD_MODEL),doc.id)
                    deviceNamesSimple.add(simpleDeviceName)
                    deviceNames.add(doc.id)
                    val osVersionsForThisDevice : List<String> = doc.data!!.get(ID_OS_VERSIONS) as List<String>
                    osVersions.add(osVersionsForThisDevice)
                }
                Log.d(TAG, "Devices:$deviceNames")
                Log.d(TAG, "OS versions:$osVersions")
                //Populate the Spinner with the right values
                setupSpinner()
            } else {
                Log.d(TAG, "Error getting documents: ", task.exception)
            }
        })
    }

    private fun setupSpinner() {
        //The spinner with the device names does not change
        val adapter1 = ArrayAdapter(context!!, android.R.layout.simple_spinner_item, deviceNamesSimple)
        spinnerDeviceNames!!.adapter = adapter1
        spinnerDeviceNames!!.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parent: AdapterView<*>, view: View, position: Int, id: Long) {
                setupSpinnerOSVersions(position)
                //System.out.println("Selected device:"+deviceNames.get(position));
            }

            override fun onNothingSelected(parent: AdapterView<*>) {}
        }
        val thisDeviceName = Helper.getManufacturerAndDeviceName()
        if (deviceNames.indexOf(thisDeviceName) >= 0) {
            println("Yor device exists in the database")
            spinnerDeviceNames!!.setSelection(deviceNames.indexOf(thisDeviceName))
        } else {
            println("Yor device does not exist in database$thisDeviceName")
        }
    }

    private fun setupSpinnerOSVersions(selectedDevice: Int) {
        Log.d(TAG, "Selected Device:" + deviceNames[selectedDevice])
        //The Spinner with the OS versions changes depending on the selected device name
        val osVersionsForThisDevice = osVersions[selectedDevice]
        val adapter2 = ArrayAdapter(context!!, android.R.layout.simple_spinner_item, osVersionsForThisDevice)
        spinnerOSVersions!!.adapter = adapter2
        spinnerOSVersions!!.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parent2: AdapterView<*>, view2: View, position2: Int, id: Long) {
                fetchDataForSelectedConfiguration(
                        deviceNames[spinnerDeviceNames!!.selectedItemPosition],
                        osVersionsForThisDevice[position2]
                )
            }

            override fun onNothingSelected(parent: AdapterView<*>) {}
        }
        //spinnerOSVersions.setSelection(0,false);
        val thisOS = Helper.getBuildVersionRelease()
        if (osVersionsForThisDevice.indexOf(thisOS) >= 0) {
            println("Your OS version exists in the database")
            spinnerDeviceNames!!.setSelection(deviceNames.indexOf(thisOS))
        } else {
            println("Your OS version exists in the database$thisOS")
        }
    }


    private fun fetchDataForSelectedConfiguration(selectedDevice: String, selectedOS: String) {
        if (selectedDevice == lastFetchedConfiguration.first && selectedOS == lastFetchedConfiguration.second) {
            Log.d(TAG, "Already fetched data for:$selectedDevice:$selectedOS")
        } else {
            Log.d(TAG, "Fetching data for:$selectedDevice:$selectedOS")
            lastFetchedConfiguration = Pair(selectedDevice, selectedOS)
            //Query data for all possible test files
            updateValuesInsideTextViews(selectedDevice, selectedOS, "rpi.h264", tvDataRpiCam)
            updateValuesInsideTextViews(selectedDevice, selectedOS, "", tvDataX264)
            updateValuesInsideTextViews(selectedDevice, selectedOS, "webbn_second.h264", tvDataInsta360)
        }
    }

    @SuppressLint("SetTextI18n")
    private fun updateValuesInsideTextViews(selectedDevice: String, selectedOS: String, testFileName: String, tvData: TextView?) {
        //db.collection("").whereEqualTo("",0).whereEqualTo("",1).
        db.collection(DECODING_INFO).document(selectedDevice).collection(selectedOS).
                whereEqualTo("vs_assets_filename", testFileName).
                limit(1).get()
                .addOnCompleteListener { task ->
                    if (task.isSuccessful && task.result != null
                            && task.result!!.documents.size > 0) {
                        Log.d(TAG, "success getting data:$selectedDevice:$selectedOS:$testFileName")
                        val snapshot = task.result
                        val document = snapshot!!.documents[0]
                        if (document != null) {
                            val testResultDecodingInfo = document.toObject(TestResultDecodingInfo::class.java)
                            val df = DecimalFormat("####.##")
                            tvData!!.text = java.lang.Float.valueOf(df.format(testResultDecodingInfo!!.avgDecodingTime_ms)).toString() + "ms"
                            Log.d(TAG,testResultDecodingInfo.toString())
                        }
                    } else {
                        Log.d(TAG, "Error getting documents: ", task.exception)
                        tvData!!.text = "Error"
                    }
                }.addOnFailureListener { tvData!!.text = "No data available" }
    }



}
