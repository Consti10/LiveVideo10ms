package constantin.video.example;

import androidx.appcompat.app.AppCompatActivity;

import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.SurfaceTexture;
import android.os.Bundle;
import android.view.Surface;
import android.view.TextureView;

import constantin.video.core.external.AspectFrameLayout;
import constantin.video.example.databinding.ActivityColorformatTestBinding;

public class ColorFormatTester extends AppCompatActivity implements TextureView.SurfaceTextureListener {
    private ActivityColorformatTestBinding binding;
    private Thread mUpdateSurfaceThread;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        binding = ActivityColorformatTestBinding.inflate(getLayoutInflater());
        binding.tvColorTest.setSurfaceTextureListener(this);
        setContentView(binding.getRoot());
    }

    @Override
    public void onSurfaceTextureAvailable(final SurfaceTexture surfaceTexture, int width, int height) {
        surfaceTexture.setDefaultBufferSize(640,480);
        final Surface surface=new Surface(surfaceTexture);
        mUpdateSurfaceThread =new Thread(new Runnable() {
            @Override
            public void run() {
                loopUpdateSurface(surface);
            }
        });
        mUpdateSurfaceThread.start();
    }

    @Override
    public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) { }

    @Override
    public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
        mUpdateSurfaceThread.interrupt();
        try {
            mUpdateSurfaceThread.join();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        return false;
    }

    @Override
    public void onSurfaceTextureUpdated(SurfaceTexture surface) { }


    private void loopUpdateSurface(final Surface surface){
        while (!Thread.currentThread().isInterrupted()){
            System.out.println("update");
            Canvas canvas=surface.lockCanvas(null);
            canvas.drawColor(Color.RED);
            surface.unlockCanvasAndPost(canvas);
        }
    }
}