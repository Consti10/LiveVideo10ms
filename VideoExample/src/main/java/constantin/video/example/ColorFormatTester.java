package constantin.video.example;

import androidx.appcompat.app.AppCompatActivity;

import android.graphics.SurfaceTexture;
import android.os.Bundle;
import android.view.TextureView;

public class ColorFormatTester extends AppCompatActivity implements TextureView.SurfaceTextureListener {
    private TextureView mTextureView;
    private Thread updateSurfaceThread;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mTextureView = new TextureView(this);
        mTextureView.setSurfaceTextureListener(this);
        setContentView(mTextureView);
    }

    @Override
    public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {
        surface.setDefaultBufferSize(640,480);
        updateSurfaceThread=new Thread(new Runnable() {
            @Override
            public void run() {
                loopUpdateSurface();
            }
        });
        updateSurfaceThread.start();
    }

    @Override
    public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) { }

    @Override
    public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
        updateSurfaceThread.interrupt();
        try {
            updateSurfaceThread.join();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        return false;
    }

    @Override
    public void onSurfaceTextureUpdated(SurfaceTexture surface) { }


    private void loopUpdateSurface(){
        while (!Thread.currentThread().isInterrupted()){
            System.out.println("update");
        }
    }
}