package constantin.livevideostreamproducer;


import android.content.Context;
import android.os.AsyncTask;
import android.util.Log;

import androidx.preference.PreferenceManager;

import java.nio.ByteBuffer;

//UDP -> Put data in, hope the data comes out at the other end of the network
//Uses ndk to send UDP packets to specified ip and port

public class UDPSender {
    private static final String TAG="UDPSender";
    private static final int PORT=5600;
    static {
        System.loadLibrary("UDPSender");
    }
    native long nativeConstruct(String IP,int port);
    native void nativeDelete(long p);
    //Called by sendAsync / sendOnCurrentThread
    //If length exceeds the max UDP packet size,
    //The data is split into smaller chunks and the method calls itself recursively
    native void nativeSplitAndSend(long p,ByteBuffer data,int dataSize);
    native void nativeSendFEC(long p,ByteBuffer data,int dataSize);

    native void nativeSend(long p,ByteBuffer data,int dataSize,int mode);

    private final long nativeInstance;

    final int streamMode;

    UDPSender(final Context context){
        //"10.183.84.95"
        final String IP = PreferenceManager.getDefaultSharedPreferences(context).getString(context.getString(R.string.KEY_SP_UDP_IP), "192.168.1.172");
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
