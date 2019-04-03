package constantin.video.example;

import android.content.Context;
import android.content.Intent;

import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.Spinner;

public class MainActivity extends AppCompatActivity implements View.OnClickListener {
    public static final String INNTENT_EXTRA_VIDEO_MODE ="INTENT_EXTRA_MODE";
    public static final String INNTENT_EXTRA_VIDEO_TEST_FILE ="INTENT_EXTRA_TEST_FILE";
    Spinner spinnerVideoMode;
    Spinner spinnerVideoTestFile;
    Context context;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        context=this;
        spinnerVideoMode =  findViewById(R.id.s_videoModeSelector);
        ArrayAdapter<CharSequence> adapter1 = ArrayAdapter.createFromResource(this,
                R.array.video_modes, android.R.layout.simple_spinner_item);
        adapter1.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinnerVideoMode.setAdapter(adapter1);

        spinnerVideoTestFile =findViewById(R.id.s_videoFileSelector);
        ArrayAdapter<CharSequence> adapter2 = ArrayAdapter.createFromResource(this,
                R.array.video_test_files, android.R.layout.simple_spinner_item);
        adapter2.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinnerVideoTestFile.setAdapter(adapter2);
        //
        Button startVideoActivity=findViewById(R.id.b_startVideoActivity);
        startVideoActivity.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                final int selectedMode= spinnerVideoMode.getSelectedItemPosition();
                final int selectedTestFile=spinnerVideoTestFile.getSelectedItemPosition();
                Intent intentVideoActivity = new Intent();
                intentVideoActivity.setClass(context, VideoActivity.class);
                intentVideoActivity.putExtra(INNTENT_EXTRA_VIDEO_MODE,selectedMode);
                intentVideoActivity.putExtra(INNTENT_EXTRA_VIDEO_TEST_FILE,selectedTestFile);
                startActivity(intentVideoActivity);
            }
        });
    }


    @Override
    public void onClick(View v) {

    }
}
