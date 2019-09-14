package constantin.video.example;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import android.annotation.SuppressLint;
import android.content.Context;
import android.os.Bundle;
import android.util.Log;
import android.util.Pair;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Spinner;
import android.widget.TextView;

import com.google.android.gms.tasks.OnCompleteListener;
import com.google.android.gms.tasks.OnFailureListener;
import com.google.android.gms.tasks.Task;
import com.google.firebase.firestore.DocumentSnapshot;
import com.google.firebase.firestore.FirebaseFirestore;
import com.google.firebase.firestore.Query;
import com.google.firebase.firestore.QuerySnapshot;

import java.text.DecimalFormat;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public class ViewDataActivity extends AppCompatActivity {
    private final static String TAG="VideoDataActivity";
    //They become valid once the firebase request finished
    private final ArrayList<String> deviceNames=new ArrayList<>();
    private final ArrayList<ArrayList<String>> osVersions=new ArrayList<>();

    private Spinner spinnerDeviceNames;
    private Spinner spinnerOSVersions;
    private Context context;
    //Avoid fetching the same data multiple times
    private Pair<String,String> lastFetchedConfiguration=new Pair<>("","");
    final FirebaseFirestore db = FirebaseFirestore.getInstance();

    private TextView tvDataRpiCam;
    private TextView tvDataX264;
    private TextView tvDataInsta360;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_view_data);
        context=this;
        //textView=findViewById(R.id.textView2);
        final String device= Helper.getDeviceName();
        final String os= Helper.getBuildVersionRelease();
        ((TextView)findViewById(R.id.textViewDevice)).setText("Device:"+device);
        ((TextView)findViewById(R.id.textViewOS)).setText("OS:"+os);
        spinnerDeviceNames=findViewById(R.id.spinner_device);
        spinnerOSVersions=findViewById(R.id.spinner_os);
        tvDataRpiCam=findViewById(R.id.tv_data_rpiCam);
        tvDataX264=findViewById(R.id.tv_data_x264);
        tvDataInsta360=findViewById(R.id.tv_data_insta360);

        //This one is to populate the spinner with all tested device names / OS combos
        db.collection("Decoding info").get().addOnCompleteListener(new OnCompleteListener<QuerySnapshot>() {
            @Override
            public void onComplete(@NonNull Task<QuerySnapshot> task) {
                if(task.isSuccessful()){
                    final QuerySnapshot documentSnapshots= task.getResult();
                    if(documentSnapshots==null)return;
                    final List<DocumentSnapshot> documents=documentSnapshots.getDocuments();
                    for(int i=0;i<documents.size();i++){
                        final DocumentSnapshot doc=documentSnapshots.getDocuments().get(i);
                        deviceNames.add(doc.getId());
                        final ArrayList<String> osVersionsForThisDevice=new ArrayList<>();
                        for(Map.Entry<String,Object> entry : doc.getData().entrySet()){
                            osVersionsForThisDevice.add(entry.getKey());
                        }
                        osVersions.add(osVersionsForThisDevice);
                    }
                    Log.d(TAG,"Devices:"+deviceNames.toString());
                    Log.d(TAG,"OS versions:"+osVersions.toString());
                    //Populate the Spinner with the right values
                    setupSpinner();
                }else{
                    Log.d(TAG, "Error getting documents: ", task.getException());
                }
            }
        });
    }

    private void setupSpinner(){
        //The spinner with the device names does not change
        final ArrayAdapter<String> adapter1=new ArrayAdapter<>(context,android.R.layout.simple_spinner_item,deviceNames);
        spinnerDeviceNames.setAdapter(adapter1);
        spinnerDeviceNames.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                setupSpinnerOSVersions(position);
                //System.out.println("Selected device:"+deviceNames.get(position));
            }
            @Override
            public void onNothingSelected(AdapterView<?> parent) { }
        });
        final String thisDeviceName= Helper.getDeviceName();
        if(deviceNames.indexOf(thisDeviceName)>=0){
            System.out.println("Yor device exists in the database");
            spinnerDeviceNames.setSelection(deviceNames.indexOf(thisDeviceName));
        }else{
            System.out.println("Yor device does not exist in database"+thisDeviceName);
        }
    }

    private void setupSpinnerOSVersions(final int selectedDevice){
        Log.d(TAG,"Selected Device:"+deviceNames.get(selectedDevice));
        //The Spinner with the OS versions changes depending on the selected device name
        final ArrayList<String> osVersionsForThisDevice=osVersions.get(selectedDevice)==null ? new ArrayList<String>()
                : osVersions.get(selectedDevice);
        final ArrayAdapter<String> adapter2=new ArrayAdapter<>(context,android.R.layout.simple_spinner_item,osVersionsForThisDevice);
        spinnerOSVersions.setAdapter(adapter2);
        spinnerOSVersions.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent2, View view2, int position2, long id) {
                fetchDataForSelectedConfiguration(
                        deviceNames.get(spinnerDeviceNames.getSelectedItemPosition()),
                        osVersionsForThisDevice.get(position2)
                );
            }
            @Override
            public void onNothingSelected(AdapterView<?> parent) { }
        });
        //spinnerOSVersions.setSelection(0,false);
        final String thisOS= Helper.getBuildVersionRelease();
        if(osVersionsForThisDevice.indexOf(thisOS)>=0){
            System.out.println("Your OS version exists in the database");
            spinnerDeviceNames.setSelection(deviceNames.indexOf(thisOS));
        }else{
            System.out.println("Your OS version exists in the database"+thisOS);
        }
    }


    private void fetchDataForSelectedConfiguration(final String selectedDevice,final String selectedOS){
        if(selectedDevice.equals(lastFetchedConfiguration.first) &&
                selectedOS.equals(lastFetchedConfiguration.second)){
            Log.d(TAG,"Already fetched data for:"+selectedDevice+":"+selectedOS);
        }else {
            Log.d(TAG, "Fetching data for:" + selectedDevice + ":" + selectedOS);
            lastFetchedConfiguration = new Pair<>(selectedDevice, selectedOS);
            //Query data for all possible test files
            updateValuesInsideTextViews(selectedDevice,selectedOS,"rpi.h264",tvDataRpiCam);
            updateValuesInsideTextViews(selectedDevice,selectedOS,"testVideo.h264",tvDataX264);
            updateValuesInsideTextViews(selectedDevice,selectedOS,"Recording_360_short.h264",tvDataInsta360);
        }
    }

    @SuppressLint("SetTextI18n")
    private void updateValuesInsideTextViews(final String selectedDevice,final String selectedOS,final String testFileName,final TextView tvData){
        db.collection("Decoding info").document(selectedDevice).collection(selectedOS).
                whereEqualTo("VS_ASSETS_FILENAME_TEST_ONLY",testFileName).
                //orderBy("nNALUSFeeded", Query.Direction.ASCENDING).
                limit(1).get()
                .addOnCompleteListener(new OnCompleteListener<QuerySnapshot>() {
                    @Override
                    public void onComplete(@NonNull Task<QuerySnapshot> task) {
                        if (task.isSuccessful() && task.getResult()!=null
                                && task.getResult().getDocuments().size()>0) {
                            Log.d(TAG,"success getting data:"+selectedDevice+":"+selectedOS+":"+testFileName);
                            final QuerySnapshot snapshot=task.getResult();
                            DocumentSnapshot document=snapshot.getDocuments().get(0);
                            if(document!=null ){
                                final Map<String,Object> testData=document.getData();
                                final Object o=testData.get("avgTotalDecodingTime_ms");
                                if(o!=null){
                                    DecimalFormat df = new DecimalFormat("####.##");
                                    float val=(float)((double)o);
                                    tvData.setText(Float.valueOf(df.format(val))+"ms");
                                }
                            }
                        } else {
                            Log.d(TAG, "Error getting documents: ", task.getException());
                            tvData.setText("Error");
                        }
                    }
                }).addOnFailureListener(new OnFailureListener() {
            @Override
            public void onFailure(@NonNull Exception e) {
                tvData.setText("No data available");
            }
        });
    }

    
}
