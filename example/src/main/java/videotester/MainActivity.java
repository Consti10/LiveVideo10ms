package videotester;

import android.content.Context;
import android.content.Intent;
import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.Spinner;

public class MainActivity extends AppCompatActivity implements View.OnClickListener {
    public static final String INNTENT_EXTRA_NAME="selectedVideoName";
    Spinner spinner;
    Context context;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        context=this;
        spinner =  findViewById(R.id.s_videoFileSelector);
        ArrayAdapter<CharSequence> adapter = ArrayAdapter.createFromResource(this,
                R.array.test_videos, android.R.layout.simple_spinner_item);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinner.setAdapter(adapter);
        //
        Button startVideoActivity=findViewById(R.id.b_startVideoActivity);
        startVideoActivity.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                final int selected=spinner.getSelectedItemPosition();
                System.out.println("Selected:"+selected);
                Intent intentVideoActivity = new Intent();
                intentVideoActivity.setClass(context, VideoActivity.class);
                intentVideoActivity.putExtra(INNTENT_EXTRA_NAME,selected);
                startActivity(intentVideoActivity);
            }
        });
    }


    @Override
    public void onClick(View v) {

    }
}
