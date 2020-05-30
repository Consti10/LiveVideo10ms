package constantin.video.example;

import androidx.appcompat.app.AppCompatActivity;

import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.SurfaceTexture;
import android.os.Bundle;
import android.view.Surface;
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
    public void onSurfaceTextureAvailable(final SurfaceTexture surfaceTexture, int width, int height) {
        surfaceTexture.setDefaultBufferSize(640,480);
        updateSurfaceThread=new Thread(new Runnable() {
            @Override
            public void run() {
                loopUpdateSurface(new Surface(surfaceTexture));
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


    private void loopUpdateSurface(final Surface surface){
        while (!Thread.currentThread().isInterrupted()){
            System.out.println("update");
            Canvas canvas=surface.lockCanvas(null);
            canvas.drawColor(Color.RED);
            surface.unlockCanvasAndPost(canvas);
        }
    }
}