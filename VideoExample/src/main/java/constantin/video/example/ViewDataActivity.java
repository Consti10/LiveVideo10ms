package constantin.video.example;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.util.Pair;
import android.widget.TextView;

import com.google.android.gms.tasks.OnCompleteListener;
import com.google.android.gms.tasks.Task;
import com.google.firebase.firestore.DocumentSnapshot;
import com.google.firebase.firestore.FirebaseFirestore;
import com.google.firebase.firestore.Query;
import com.google.firebase.firestore.QueryDocumentSnapshot;
import com.google.firebase.firestore.QuerySnapshot;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class ViewDataActivity extends AppCompatActivity {
    private final static String TAG="VideoDataActivity";
    private TextView textView;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_view_data);
        textView=findViewById(R.id.textView2);
        final String device=VideoActivity.getDeviceName();
        final String os=VideoActivity.getBuildVersionRelease();
        ((TextView)findViewById(R.id.textViewDevice)).setText("Device:"+device);
        ((TextView)findViewById(R.id.textViewOS)).setText("OS:"+os);

        final FirebaseFirestore db = FirebaseFirestore.getInstance();
        //This one is to populate the spinner with all tested device names
        /*db.collection("Decoding info").get().addOnCompleteListener(new OnCompleteListener<QuerySnapshot>() {
            @Override
            public void onComplete(@NonNull Task<QuerySnapshot> task) {
                if(task.isSuccessful()){
                    final QuerySnapshot documentSnapshots= task.getResult();
                    final ArrayList<String> deviceNames=new ArrayList<>();
                    for(DocumentSnapshot snapshot : documentSnapshots.getDocuments()){
                        deviceNames.add(snapshot.getId());
                    }
                    Log.d(TAG,"Devices:"+deviceNames.toString());
                }else{
                    Log.d(TAG, "Error getting documents: ", task.getException());
                }
            }
        });*/
        //.whereEqualTo("Build.VERSION.SDK_INT", 28);
        db.collection("Decoding info").document(device).collection(os).
                whereEqualTo("VS_ASSETS_FILENAME_TEST_ONLY","rpi.h264"). //only rpi decoding
                //orderBy("nNALUSFeeded", Query.Direction.ASCENDING). //Get the document with the most n of fed nalus
                limit(1).
                get()
                .addOnCompleteListener(new OnCompleteListener<QuerySnapshot>() {
                    @Override
                    public void onComplete(@NonNull Task<QuerySnapshot> task) {
                        if (task.isSuccessful()) {
                            Log.d(TAG,"success");
                            final QuerySnapshot snapshot=task.getResult();
                            textView.setText("");
                            DocumentSnapshot document=snapshot.getDocuments().get(0);
                            if(document!=null){
                                StringBuilder builder=new StringBuilder();
                                final List<Pair<String,Object>> readableData=toReadableArrayList(document.getData());
                                for(final Pair<String,Object> pair : readableData){
                                    builder.append(pair.first).append(":").append(pair.second).append("\n");
                                }
                                textView.setText(builder.toString());
                                Log.d(TAG, document.getId() + " => " + builder.toString());
                            }
                        } else {
                            Log.d(TAG, "Error getting documents: ", task.getException());
                        }
                    }
                });
    }

    private static List<Pair<String,Object>> toReadableArrayList(final Map<String,Object> map){
        final ArrayList<Pair<String,Object>> ret=new ArrayList<>();
        if(map==null)return ret;
        //we want avgTotalDecodingTime at the beginning of our list
        final String avgTotalDecodingTime="avgTotalDecodingTime_ms";
        for(final String key:map.keySet()){
            if(!key.equals(avgTotalDecodingTime)){
                ret.add(new Pair<String, Object>(key,map.get(key)));
            }
        }
        //sort by characters of key
        Collections.sort(ret, new Comparator<Pair<String, Object>>() {
            @Override
            public int compare(Pair<String, Object> o1, Pair<String, Object> o2) {
                return o1.first.compareTo(o2.first);
            }
        });
        if(map.get(avgTotalDecodingTime)!=null){
            ret.add(0,new Pair<String, Object>(avgTotalDecodingTime,map.get(avgTotalDecodingTime)));
        }
        return ret;
    }
}
