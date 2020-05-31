package constantin.video.example;

import androidx.appcompat.app.AppCompatActivity;

import android.graphics.Camera;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.SurfaceTexture;
import android.hardware.camera2.CaptureRequest;
import android.os.Bundle;
import android.view.Surface;
import android.view.TextureView;

import constantin.test.ColorFormatTester;
import constantin.video.example.databinding.ActivityColorformatTestBinding;

public class AColorFormatTester extends AppCompatActivity implements TextureView.SurfaceTextureListener {
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
        surfaceTexture.setDefaultBufferSize(640*1,480*1);
        final Surface surface=new Surface(surfaceTexture);
        ColorFormatTester.nativeSetSurface(surface);
        mUpdateSurfaceThread =new Thread(new Runnable() {
            @Override
            public void run() {
                loopUpdateSurface();
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
        ColorFormatTester.nativeSetSurface(null);
        return false;
    }

    @Override
    public void onSurfaceTextureUpdated(SurfaceTexture surface) { }


    private void loopUpdateSurface(){
        while (!Thread.currentThread().isInterrupted()){
            //System.out.println("update");
            //Canvas canvas=surface.lockCanvas(null);
            //canvas.drawColor(Color.RED);
            //surface.unlockCanvasAndPost(canvas);
            ColorFormatTester.nativeTestUpdateSurface();
            try {
                Thread.sleep(100);
            } catch (InterruptedException e) {
                return;
            }
        }
    }
}