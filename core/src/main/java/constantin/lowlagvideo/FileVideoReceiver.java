package constantin.lowlagvideo;

import android.content.Context;
import android.content.res.AssetManager;
import android.os.Environment;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;


public class FileVideoReceiver {
    private static final int BUFER_SIZE=1024*1024;
    private static final int CHUNK_SIZE=1024;

    public enum REC_MODE{
        FILE,ASSETS
    }

    private final Context mContext;
    private final REC_MODE mReceivingMode;
    private final String mFileName;
    private final IVideoDataRaw mIVideoDataRaw;
    private Thread receiverThread;


    public FileVideoReceiver(final Context context, final IVideoDataRaw iVideoDataRaw, final REC_MODE receivingMode, final String fileName){
        mContext=context;
        mIVideoDataRaw =iVideoDataRaw;
        mReceivingMode=receivingMode;
        mFileName=fileName;
    }

    public void startReceiving(){
        receiverThread=new Thread() {
            @Override
            public void run() {
                switch (mReceivingMode){
                    case ASSETS:
                        setName("AssetsRecT");
                        receiveFromAssets(mFileName);
                        break;
                    case FILE:
                        setName("FileRecT");
                        receiveFromFile(mFileName);
                        break;
                }
            }
        };
        receiverThread.setPriority(Thread.NORM_PRIORITY);
        receiverThread.start();
    }

    public void stopReceivingAndWait(){
        receiverThread.interrupt();
        try {
            receiverThread.join();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    private void receiveFromAssets(final String fileName){
        AssetManager assetManager=mContext.getAssets();
        InputStream in;
        byte[] buffer= new byte[BUFER_SIZE];
        try {
            in=assetManager.open(fileName);
        } catch (IOException e) {e.printStackTrace();return;}
        while(!receiverThread.isInterrupted()) {
            int sampleSize = 0;
            try {
                sampleSize=in.read(buffer,0,BUFER_SIZE);
            } catch (IOException e) {
                e.printStackTrace();}
            if(sampleSize>0){
                passDataInChunks(buffer,sampleSize);
            }else {
                try{ in.reset(); }catch (IOException e){e.printStackTrace();}
            }
        }
        try {in.close();} catch (IOException e) {e.printStackTrace();}
        //System.out.println("Receive from Assets ended");
    }


    private void receiveFromFile(final String fileName) {
        java.io.FileInputStream in;
        byte[] buffer= new byte[BUFER_SIZE];
        final String s=Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DCIM)+"/FPV_VR/"+fileName;
        try {
            in=new java.io.FileInputStream(s);
        } catch (FileNotFoundException e) {
            System.out.println("Error opening video file:"+s);
            return;
        }
        while(!receiverThread.isInterrupted()) {
            int sampleSize = 0;
            try {
                sampleSize=in.read(buffer,0,BUFER_SIZE);
            } catch (IOException e) {e.printStackTrace();}
            if(sampleSize>0){
                passDataInChunks(buffer,sampleSize);
            }else {
                try {in.close();} catch (IOException e) {e.printStackTrace();}
                try {
                    in=new java.io.FileInputStream(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DCIM)+"/FPV_VR/"+fileName);
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }
        //System.out.println("Receive from file ended");
    }

    /*
     * The buffer for receiving from file is quite big.
     * When sending too much data to the decoder in one pass,
     * stopping it takes longer, since it has to wait until no more data is
     * fed by the parser, or in other words: wait until the input pipeline is empty
     */
    private void passDataInChunks(byte[] buffer, int length){
        int offset=0;
        while (!receiverThread.isInterrupted()){
            if(length>CHUNK_SIZE){
                if(mIVideoDataRaw !=null)
                    mIVideoDataRaw.onNewVideoData(buffer,offset,CHUNK_SIZE);
                offset+=CHUNK_SIZE;
                length-=CHUNK_SIZE;
            }else{
                if(mIVideoDataRaw !=null){
                    mIVideoDataRaw.onNewVideoData(buffer,offset,length);
                }
                break;
            }
        }
    }

    public interface IVideoDataRaw{
        void onNewVideoData(byte[] buffer, int offset, int length);
    }
}
