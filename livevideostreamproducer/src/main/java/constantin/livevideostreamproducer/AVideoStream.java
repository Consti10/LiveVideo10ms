package constantin.livevideostreamproducer;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.SurfaceTexture;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CaptureRequest;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.os.Bundle;
import android.os.Process;
import android.util.Log;
import android.util.Range;
import android.view.Surface;
import android.view.TextureView;
import android.widget.Toast;

import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.Objects;

//Note: Pausing /resuming is not supported.
//Once started,everything runs until onDestroy() is called

public class AVideoStream extends AppCompatActivity{
    private static final String TAG="AVideoStream";

    private TextureView previewTextureView;
    private SurfaceTexture previewTexture;
    private Surface previewSurface;

    private Surface encoderInputSurface;

    private static final int W=1920;
    private static final int H=1080;
    private static final int MDEIACODEC_ENCODER_TARGET_FPS=60;
    private static final int MDEIACODEC_TARGET_KEY_BIT_RATE=5*1024*1024;

    private CameraDevice cameraDevice;
    private MediaCodec codec;

    //The MediaCodec output callbacks runs on this Handler
    //private Handler mBackgroundHandler;
    //private HandlerThread mBackgroundThread;

    private VideoTransmitter mUDPSender;
    private Thread drainEncoderThread;
    // Every n frames send sps pps data to allow re-starting of the stream
    private boolean SEND_SPS_PPS_EVERY_N_FRAMES=false;
    private ByteBuffer currentCSD0;
    private ByteBuffer currentCSD1;
    private int haveToManuallySendKeyFrame=0;


    @SuppressLint("MissingPermission")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_avideo_stream);

        //mBackgroundThread = new HandlerThread("Encoder output");
        //mBackgroundThread.start();
        //mBackgroundHandler = new Handler(mBackgroundThread.getLooper());
        previewTextureView =findViewById(R.id.mTextureView);
        //Initialize UDP sender
        mUDPSender=new VideoTransmitter(this);


        //This thread will be started once the MediaCodec encoder has been created.
        //When decoding in LiveVideo10ms, testing indicates lower latency when constantly pulling on the output with one Thread.
        //Also, we have to use another Thread for networking anyways when using the 'callback' api
        drainEncoderThread=new Thread(new Runnable() {
            @Override
            public void run() {
                Process.setThreadPriority(Process.THREAD_PRIORITY_URGENT_AUDIO);
                Thread.currentThread().setName("DrainEnc");
                while (!Thread.currentThread().isInterrupted()){
                    final MediaCodec.BufferInfo bufferInfo=new MediaCodec.BufferInfo();
                    if(codec!=null){
                        final int outputBufferId = codec.dequeueOutputBuffer(bufferInfo,1000*2);
                        if (outputBufferId >= 0) {
                            //Log.d(TAG,"NEW OUTPUT BUFFER"+outputBufferId);
                            final ByteBuffer outputBuffer = codec.getOutputBuffer(outputBufferId);
                            if(SEND_SPS_PPS_EVERY_N_FRAMES){
                                if(haveToManuallySendKeyFrame==0){
                                    //Log.d(TAG,"Manually inserting SPS & PPS");
                                    //Log.d(TAG,"csd0"+currentCSD0.isDirect());
                                    mUDPSender.sendOnCurrentThread(currentCSD0);
                                    mUDPSender.sendOnCurrentThread(currentCSD1);
                                }
                                haveToManuallySendKeyFrame++;
                                haveToManuallySendKeyFrame=haveToManuallySendKeyFrame % 2;
                            }
                            mUDPSender.sendOnCurrentThread(outputBuffer);
                            codec.releaseOutputBuffer(outputBufferId,false);

                        } else if (outputBufferId == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                            // Subsequent data will conform to new format.
                            final MediaFormat currentOutputFormat= codec.getOutputFormat();
                            Log.d(TAG,"INFO_OUTPUT_FORMAT_CHANGED "+currentOutputFormat.toString());
                            // Every sync frame has sps and pps prepended - wo do not need to send the
                            // sps and pps data manually
                            if(currentOutputFormat.containsKey(MediaFormat.KEY_PREPEND_HEADER_TO_SYNC_FRAMES)){
                                SEND_SPS_PPS_EVERY_N_FRAMES=false;
                            }else{
                                SEND_SPS_PPS_EVERY_N_FRAMES=true;
                            }
                            currentCSD0= VideoTransmitter.createDirectByteBuffer(Objects.requireNonNull(currentOutputFormat.getByteBuffer("csd-0")));
                            currentCSD1= VideoTransmitter.createDirectByteBuffer(Objects.requireNonNull(currentOutputFormat.getByteBuffer("csd-1")));
                        }
                    }
                }
            }
        });
        //Create Decoder. We don't have to wait for anything here
        try {
            codec= MediaCodec.createEncoderByType("video/avc");
            MediaFormat format = MediaFormat.createVideoFormat("video/avc",W,H);
            format.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface);
            format.setInteger(MediaFormat.KEY_BIT_RATE,MDEIACODEC_TARGET_KEY_BIT_RATE); //X MBit/s
            format.setInteger(MediaFormat.KEY_FRAME_RATE,MDEIACODEC_ENCODER_TARGET_FPS);
            format.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL,1);
            //format.setInteger(MediaFormat.KEY_INTRA_REFRESH_PERIOD,1);
            if(android.os.Build.VERSION.SDK_INT>28){
                format.setInteger(MediaFormat.KEY_LATENCY,0);
                format.setInteger(MediaFormat.KEY_PRIORITY,0);
            }
            //format.setInteger(MediaFormat.KEY_LEVEL,MediaCodecInfo.CodecProfileLevel.AVCLevel32);
            //format.setInteger(MediaFormat.KEY_PROFILE,MediaCodecInfo.CodecProfileLevel.AVCProfileBaseline);
            codec.configure(format, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
            encoderInputSurface = codec.createInputSurface();
            //codec.setCallback(mediaCodecCallback);
            codec.start();
            drainEncoderThread.start();
        } catch (Exception e) {
            e.printStackTrace();
            //There is no point of continuing when unable to create Encoder
            notifyUserAndFinishActivity( "Error MediaCodec.createEncoderByType");
        }
        //Open main camera. We don't need to wait for the surface texture, since we set the callback once camera was opened
        CameraManager manager = (CameraManager) getSystemService(Context.CAMERA_SERVICE);
        Log.d(TAG,"Opening camera");
        try {
            //Just assume there is a front camera
            final String cameraId = manager.getCameraIdList()[0];
            final CameraCharacteristics characteristics = manager.getCameraCharacteristics(cameraId);
            Range<Integer>[] fpsRanges = characteristics.get(CameraCharacteristics.CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES);
            Log.d(TAG,"Available fps range(s):"+Arrays.toString(fpsRanges));

            //StreamConfigurationMap map = characteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
            //imageDimension = map.getOutputSizes(SurfaceTexture.class)[0];
            manager.openCamera(cameraId, stateCallback, null);
        } catch (CameraAccessException | NullPointerException e) {
            e.printStackTrace();
            //No point in continuing if we cannot open camera
            notifyUserAndFinishActivity("Cannot open camera");
        }
    }


    private final CameraDevice.StateCallback stateCallback = new CameraDevice.StateCallback() {
        @Override
        public void onOpened(CameraDevice camera) {
            Log.d(TAG, "CameraDevice onOpened");
            cameraDevice = camera;
            //Once the camera was opened, we add the listener for the TextureView.
            //Which will then call the startStream()
            previewTextureView.setSurfaceTextureListener(surfaceTextureListener);
            // If the textureView is already available, the callback won't get called - do it manually
            if(previewTextureView.isAvailable()){
                surfaceTextureListener.onSurfaceTextureAvailable(previewTextureView.getSurfaceTexture(), previewTextureView.getWidth(), previewTextureView.getHeight());
            }
        }
        @Override
        public void onDisconnected(CameraDevice camera) {
            cameraDevice.close();
        }
        @Override
        public void onError(CameraDevice camera, int error) {
            cameraDevice.close();
            cameraDevice = null;
        }
    };
    private final TextureView.SurfaceTextureListener surfaceTextureListener=new TextureView.SurfaceTextureListener() {
        @Override
        public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {
            Log.d(TAG,"onSurfaceTextureAvailable");
            previewTexture=surface;
            startPreviewAndEncoding();
        }
        @Override
        public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) { }
        @Override
        public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
            return false;
        }
        @Override
        public void onSurfaceTextureUpdated(SurfaceTexture surface) { }
    };

    /*private final MediaCodec.Callback mediaCodecCallback=new MediaCodec.Callback() {
        @Override
        public void onInputBufferAvailable(@NonNull MediaCodec codec, int index) {
            Log.d(TAG,"MediaCodec::onInputBufferAvailable");
            //This should never happen
        }

        @Override
        public void onOutputBufferAvailable(@NonNull MediaCodec codec, int index, @NonNull MediaCodec.BufferInfo info) {
            //Log.d(TAG,"MediaCodec::onOutputBufferAvailable");
            final ByteBuffer bb= codec.getOutputBuffer(index);
            mUDPSender.sendAsync(bb);
            //DO something with the data
            codec.releaseOutputBuffer(index,false);
        }

        @Override
        public void onError(@NonNull MediaCodec codec, @NonNull MediaCodec.CodecException e) {
            Log.d(TAG,"MediaCodec::onError");
        }

        @Override
        public void onOutputFormatChanged(@NonNull MediaCodec codec, @NonNull MediaFormat format) {
            final ByteBuffer csd0=format.getByteBuffer("csd-0");
            final ByteBuffer csd1=format.getByteBuffer("csd-1");
            mUDPSender.sendAsync(csd0);
            mUDPSender.sendAsync(csd1);
        }
    };*/


    private void startPreviewAndEncoding(){
        Log.d(TAG,"startPreviewAndEncoding(). Camera device id"+cameraDevice.getId()+"Surface Texture"+previewTexture.toString());
        //All Preconditions are fulfilled:
        //The Camera has been opened
        //The preview Surface texture has been created
        //The encoder surface has been created
        previewTexture.setDefaultBufferSize(W,H);
        previewSurface = new Surface(previewTexture);

        try {
            final CaptureRequest.Builder captureRequestBuilder = cameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
            captureRequestBuilder.addTarget(previewSurface);
            captureRequestBuilder.addTarget(encoderInputSurface);

            //Log.d("FPS", "SYNC_MAX_LATENCY_PER_FRAME_CONTROL: " + Arrays.toString(fpsRanges));
            Range<Integer> fpsRange=new Range<>(30,60);
            captureRequestBuilder.set(CaptureRequest.CONTROL_AE_TARGET_FPS_RANGE,fpsRange);

            cameraDevice.createCaptureSession(Arrays.asList(previewSurface,encoderInputSurface), new CameraCaptureSession.StateCallback() {
                @Override
                public void onConfigured(@NonNull CameraCaptureSession cameraCaptureSession) {
                    // When the session is ready, we start displaying the preview.
                    try {
                        cameraCaptureSession.setRepeatingRequest(captureRequestBuilder.build(), null,null);
                    } catch (CameraAccessException e) {
                        e.printStackTrace();
                    }
                }
                @Override
                public void onConfigureFailed(@NonNull CameraCaptureSession cameraCaptureSession) {
                    Toast.makeText(AVideoStream.this, "Configuration change", Toast.LENGTH_SHORT).show();
                }
            }, null);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    @Override
    protected void onDestroy(){
        super.onDestroy();
        if(cameraDevice!=null){
            cameraDevice.close();
            cameraDevice=null;
        }
        if(drainEncoderThread!=null){
            drainEncoderThread.interrupt();
            try {
                drainEncoderThread.join();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
        if(codec!=null){
            codec.stop();
            codec.release();
            codec=null;
        }
        if(previewSurface!=null){
            previewSurface.release();
            previewSurface=null;
        }
        if(encoderInputSurface!=null){
            encoderInputSurface.release();
            encoderInputSurface=null;
        }
    }

    //has to be called on the ui thread
    private void notifyUserAndFinishActivity(final String message){
        Log.d(TAG,message);
        Toast.makeText(getApplicationContext(),message,Toast.LENGTH_LONG).show();
        finish();
    }


}
