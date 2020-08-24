package constantin.video.core.video_player;

import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ImageDecoder;
import android.graphics.SurfaceTexture;
import android.media.Image;
import android.media.ImageReader;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.media.MediaFormat;
import android.os.Build;
import android.util.Log;
import android.view.Surface;

import androidx.annotation.NonNull;

import java.io.IOException;
import java.nio.ByteBuffer;

public class SimpleEncoder2 extends MediaCodec.Callback implements Runnable {
    private Thread mThread;
    private final long start=System.currentTimeMillis();

    public void start(){
        mThread=new Thread(this);
        mThread.start();
    }

    public void stop(){
        mThread.interrupt();
        try {
            mThread.join();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    static void lol(){
        MediaCodecList mediaCodecList=new MediaCodecList(MediaCodecList.REGULAR_CODECS);
        MediaFormat mediaFormat=new MediaFormat();
        mediaFormat.setInteger(MediaFormat.KEY_COLOR_FORMAT,MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV444Flexible);
        final String tmp=mediaCodecList.findEncoderForFormat(mediaFormat);
        System.out.println("found enc "+tmp);

        MediaCodecInfo[] codecInfos = mediaCodecList.getCodecInfos();
        for (final MediaCodecInfo info : codecInfos) {
            for(final String type:info.getSupportedTypes()){
                final MediaCodecInfo.CodecCapabilities capabilities=info.getCapabilitiesForType(type);
                for(int i=0;i<capabilities.colorFormats.length;i++){
                    System.out.println("Codec "+type+" Supports "+capabilities.colorFormats[i]);
                }
            }
        }


    }

    static void lol2(){
        try {
            MediaCodec codec=MediaCodec.createEncoderByType("video/avc");


            codec.release();

            MediaCodecInfo info=codec.getCodecInfo();
            MediaCodecInfo.CodecCapabilities abilities=info.getCapabilitiesForType("video/avc");
            for(int i=0;i<abilities.colorFormats.length;i++){
                System.out.println("Supports "+abilities.colorFormats[i]);
            }

        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    @Override
    public void run() {
        try {
            MediaCodec codec=MediaCodec.createEncoderByType("video/avc");
            codec.setCallback(this);

            final int width = 640;
            final int height = 480;
            final int frameRate = 30;
            final int bitRate = 1024*1024;

            MediaFormat format=new MediaFormat();
            format.setString(MediaFormat.KEY_MIME, "video/avc");
            format.setInteger(MediaFormat.KEY_HEIGHT, height);
            format.setInteger( MediaFormat.KEY_WIDTH, width);
            format.setInteger( MediaFormat.KEY_BIT_RATE, bitRate);
            format.setInteger( MediaFormat.KEY_FRAME_RATE, frameRate);
            format.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL,30);
            format.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Flexible);
            //MediaFormat.KEY_STRIDE

            codec.configure(format,null,null,MediaCodec.CONFIGURE_FLAG_ENCODE);
            //final Surface surface=codec.createInputSurface();

            codec.start();

            final MediaFormat format2=codec.getInputFormat();
            log("Format is "+format2.getInteger(MediaFormat.KEY_COLOR_FORMAT));

            while(!Thread.currentThread().isInterrupted()) {
                //log("Hi from worker");
                // STop itself after 10 seconds
                if(System.currentTimeMillis()-start>10*1000){
                    mThread.interrupt();
                }

                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                    //final Canvas canvas=surface.lockHardwareCanvas();
                    //canvas.drawColor(Color.RED);
                    //surface.unlockCanvasAndPost(canvas);
                }

                //final Canvas canvas=surface.lockCanvas(null);
                //System.out.println("Surface has format "+canvas)
                //surface.unlockCanvasAndPost(canvas);


            }
            codec.stop();
            codec.release();

        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    @Override
    public void onInputBufferAvailable(@NonNull MediaCodec codec, int index) {
        log("onInputBufferAvailable");
        final ByteBuffer buffer=codec.getInputBuffer(index);
        //codec.queueInputBuffer(index,0,10,0,);
    }

    @Override
    public void onOutputBufferAvailable(@NonNull MediaCodec codec, int index, @NonNull MediaCodec.BufferInfo info) {
        log("onOutputBufferAvailable");
    }

    @Override
    public void onError(@NonNull MediaCodec codec, @NonNull MediaCodec.CodecException e) {
        log("onError");
    }

    @Override
    public void onOutputFormatChanged(@NonNull MediaCodec codec, @NonNull MediaFormat format) {
        log("onOutputFormatChanged");
    }

    private void log(final String s){
        Log.d("SimpleEncoderJ",s);
    }
}
