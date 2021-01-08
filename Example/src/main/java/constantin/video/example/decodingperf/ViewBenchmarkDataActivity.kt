package constantin.video.example.decodingperf

import android.annotation.SuppressLint
import android.content.Context
import android.os.Bundle
import android.text.Layout
import android.text.Spannable
import android.text.SpannableString
import android.text.style.AlignmentSpan
import android.util.Log
import android.util.Pair
import android.view.View
import android.widget.*
import androidx.appcompat.app.AppCompatActivity
import com.google.android.gms.tasks.OnCompleteListener
import com.google.firebase.firestore.FirebaseFirestore
import com.jaredrummler.android.device.DeviceName
import constantin.video.example.Helper
import constantin.video.example.MainActivity
import constantin.video.example.databinding.ActivityViewDataBinding
import java.text.DecimalFormat
import java.util.*


@SuppressLint("SetTextI18n")
class ViewBenchmarkDataActivity : AppCompatActivity() {
    private lateinit var binding: ActivityViewDataBinding

    private val TAG = this::class.java.simpleName
    //They become valid once the firebase request finished
    //private val deviceNames = ArrayList<Pair<String,String>>()
    private val deviceNames = ArrayList<String>()
    private val deviceNamesSimple=ArrayList<String>();

    private val osVersions = ArrayList<List<String>>()

    private lateinit var context: Context
    //Avoid fetching the same data multiple times
    private var lastFetchedConfiguration = Pair("", "")
    internal val db = FirebaseFirestore.getInstance()

    private val DECODING_INFO: String ="DecodingInfo3";

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        //setContentView(R.layout.activity_view_data)
        binding = ActivityViewDataBinding.inflate(layoutInflater)
        setContentView(binding.root)

        context = this
        //textView=findViewById(R.id.textView2);
        val device = Helper.getManufacturerAndDeviceName()
        val os = Helper.getBuildVersionRelease()

        binding.textViewDevice.text = "Device:$device"
        binding.textViewOS.text = "OS:$os"
        
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
                    val simpleDeviceName = DeviceName.getDeviceName(doc.getString(ID_BUILD_DEVICE), doc.getString(ID_BUILD_MODEL), doc.id)
                    deviceNamesSimple.add(simpleDeviceName)
                    deviceNames.add(doc.id)
                    val osVersionsForThisDevice: List<String> = doc.data!!.get(ID_OS_VERSIONS) as List<String>
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
        val adapter1 = ArrayAdapter(context, android.R.layout.simple_spinner_item, deviceNamesSimple)
        binding.spinnerDevice.adapter = adapter1
        binding.spinnerDevice.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parent: AdapterView<*>, view: View, position: Int, id: Long) {
                setupSpinnerOSVersions(position)
                //System.out.println("Selected device:"+deviceNames.get(position));
            }

            override fun onNothingSelected(parent: AdapterView<*>) {}
        }
        val thisDeviceName = Helper.getManufacturerAndDeviceName()
        if (deviceNames.indexOf(thisDeviceName) >= 0) {
            println("Yor device exists in the database")
            binding.spinnerDevice.setSelection(deviceNames.indexOf(thisDeviceName))
        } else {
            println("Yor device does not exist in database$thisDeviceName")
        }
    }

    private fun setupSpinnerOSVersions(selectedDevice: Int) {
        Log.d(TAG, "Selected Device:" + deviceNames[selectedDevice])
        //The Spinner with the OS versions changes depending on the selected device name
        val osVersionsForThisDevice = osVersions[selectedDevice]
        val adapter2 = ArrayAdapter(context, android.R.layout.simple_spinner_item, osVersionsForThisDevice)
        binding.spinnerOs.adapter = adapter2
        binding.spinnerOs.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parent2: AdapterView<*>, view2: View, position2: Int, id: Long) {
                fetchDataForSelectedConfiguration(
                        deviceNames[binding.spinnerDevice.selectedItemPosition],
                        osVersionsForThisDevice[position2]
                )
            }

            override fun onNothingSelected(parent: AdapterView<*>) {}
        }
        //spinnerOSVersions.setSelection(0,false);
        val thisOS = Helper.getBuildVersionRelease()
        if (osVersionsForThisDevice.indexOf(thisOS) >= 0) {
            println("Your OS version exists in the database")
            binding.spinnerDevice.setSelection(deviceNames.indexOf(thisOS))
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
            //binding.tvData1.text="";
            //binding.tvData2.text="";
            binding.tableLayout.removeAllViews();
            for (i in 0..6) {
                addEntryToTableLayout(selectedDevice, selectedOS, MainActivity.Companion.VIDEO_TEST_FILES_FOR_DB(context)[i]);
            }
        }
    }

    @SuppressLint("SetTextI18n")
    private fun addEntryToTableLayout(selectedDevice: String, selectedOS: String, testFileName: String) {
        val tvData1 = TextView(this)
        val tvData2 = TextView(this)
        val tableRow = TableRow(this)
        //linearLayout.layoutParams = LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT)
        tableRow.addView(tvData1)
        tableRow.addView(tvData2)
        binding.tableLayout.addView(tableRow);

        tvData1.text=testFileName

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
                            val tmp= java.lang.Float.valueOf(df.format(testResultDecodingInfo!!.avgDecodingTime_ms)).toString() + "ms"
                            // Note: Watch out for thread safety
                            tvData2.text=tmp
                            Log.d(TAG, testResultDecodingInfo.toString())
                        }
                    } else {
                        Log.d(TAG, "Error getting documents: ", task.exception)
                        tvData2.text = "Error"
                    }
                }.addOnFailureListener {
                    tvData2.text ="Error"
                }
    }

    private fun setTextLeftAndRight(resultTextView: TextView, leftText: String, rightText: String){
        val resultText: String = leftText + "\n" + rightText
        val styledResultText = SpannableString(resultText)
        styledResultText.setSpan(AlignmentSpan.Standard(Layout.Alignment.ALIGN_OPPOSITE), leftText.length + 1, leftText.length + 1 + rightText.length, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE)
        resultTextView.text = styledResultText

    }



}
