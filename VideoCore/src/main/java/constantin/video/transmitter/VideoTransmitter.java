package constantin.video.transmitter;


import android.content.Context;
import android.os.AsyncTask;
import android.util.Log;

import java.nio.ByteBuffer;

//UDP -> Put data in, hope the data comes out at the other end of the network
//Uses ndk to send UDP packets to specified ip and port

public class VideoTransmitter {
    private static final String TAG="VideoTransmitter";
    private static final int PORT=5600;
    static {
        System.loadLibrary("VideoTransmitter");
    }
    native long nativeConstruct(String IP,int port);
    native void nativeDelete(long p);
    //Called by sendAsync / sendOnCurrentThread

    native void nativeSend(long p,ByteBuffer data,int dataSize,int mode);

    private final long nativeInstance;

    final int streamMode;

    VideoTransmitter(final Context context){
        //"10.183.84.95"
        final String IP = ASettings.getSP_UDP_IP(context);
        Log.d("UDPSender","Sending to IP "+IP);
        nativeInstance=nativeConstruct(IP,PORT);
        streamMode=ASettings.getSTREAM_MODE(context);
    }

    //Send the UDP data on another thread,since networking is strictly forbidden on the UI thread
    public void sendAsync(final ByteBuffer data){
        AsyncTask.execute(new Runnable() {
            @Override
            public void run() {
                sendOnCurrentThread(data);
            }
        });
    }

    //When using a special Handler for the Encoder*s callbacks
    //We do not need to create an extra thread
    public void sendOnCurrentThread(final ByteBuffer data){
        if(!data.isDirect()){
            // We need to create a direct byte buffer such that the native code can access it
            //final ByteBuffer tmpDirectByteBuffer=ByteBuffer.allocateDirect(data.remaining());
            Log.e(TAG,"Cannot send non-direct byte buffer.Convert to direct first.");
        }
        nativeSend(nativeInstance,data,data.remaining(),streamMode);
    }


    @Override
    protected void finalize() throws Throwable {
        try {
            nativeDelete(nativeInstance);
        } finally {
            super.finalize();
        }
    }

    public static ByteBuffer createDirectByteBuffer(final ByteBuffer source){
        ByteBuffer ret=ByteBuffer.allocateDirect(source.capacity());
        ret.rewind();
        ret.put(source);
        ret.flip();
        return ret;
    }

}
