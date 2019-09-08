package constantin.video.example;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;

import com.google.android.gms.tasks.OnCompleteListener;
import com.google.android.gms.tasks.Task;
import com.google.firebase.firestore.FirebaseFirestore;
import com.google.firebase.firestore.Query;
import com.google.firebase.firestore.QueryDocumentSnapshot;
import com.google.firebase.firestore.QuerySnapshot;

public class ViewDataActivity extends AppCompatActivity {
    private final static String TAG="VideoDataActivity";
    private TextView textView;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_view_data);
        textView=findViewById(R.id.textView2);

        final FirebaseFirestore db = FirebaseFirestore.getInstance();
        //.whereEqualTo("Build.VERSION.SDK_INT", 28);
        db.collection("Decoding info").whereEqualTo("Build_MODEL", Build.MODEL).
                whereEqualTo("Build_VERSION_SDK_INT",Build.VERSION.SDK_INT).get()
                .addOnCompleteListener(new OnCompleteListener<QuerySnapshot>() {
                    @Override
                    public void onComplete(@NonNull Task<QuerySnapshot> task) {
                        if (task.isSuccessful()) {
                            Log.d(TAG,"success");
                            final QuerySnapshot snapshot=task.getResult();
                            final StringBuilder builder=new StringBuilder();
                            textView.setText("");
                            for (QueryDocumentSnapshot document : snapshot) {
                                Log.d(TAG, document.getId() + " => " + document.getData());
                                textView.setText(document.getData().toString());
                                break;
                            }
                        } else {
                            Log.d(TAG, "Error getting documents: ", task.getException());
                        }
                    }
                });

    }
}
