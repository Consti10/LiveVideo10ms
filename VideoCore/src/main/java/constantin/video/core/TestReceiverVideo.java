package constantin.video.core;

import android.app.Activity;
import android.content.Context;
import android.graphics.Color;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import constantin.video.core.VideoNative.VideoNative;


public class TestReceiverVideo implements Runnable {

    private final Context context;
    private final TextView receivedVideoDataTV;
    private final Button button;
    private final VideoPlayer videoPlayer;
    private Thread mThread;

    public TestReceiverVideo(final Context c, TextView receivedVideoDataTV, Button connectB){
        this.context=c;
        this.receivedVideoDataTV = receivedVideoDataTV;
        this.button=connectB;
        videoPlayer=new VideoPlayer(c,null);
    }

    public void startReceiving(){
        videoPlayer.prepare(null);
        videoPlayer.addAndStartReceiver();
        mThread=new Thread(this);
        mThread.setName("TestReceiverVideo");
        mThread.start();
    }

    public void stopReceiving(){
        mThread.interrupt();
        try {mThread.join();} catch (InterruptedException e) {e.printStackTrace();}
        videoPlayer.stopAndRemovePlayerReceiver();
    }

    //to have as less work on the UI thread and GPU, we check if the content of the string has changed
    //before updating the TV.
    private static void updateViewIfStringChanged(final Context context,final TextView tv, final String newContent,final boolean colorRed){
        final String prev=tv.getText().toString();
        if(!prev.contentEquals(newContent)){
            ((Activity)context).runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    tv.setText(newContent);
                    if(colorRed){
                        tv.setTextColor(Color.RED);
                    }else{
                        tv.setTextColor(Color.GREEN);
                    }
                }
            });
        }
    }

    @Override
    public void run(){
        long lastCheckMS = System.currentTimeMillis() - 2*1000;
        while (!Thread.currentThread().isInterrupted()){
            //if the receivedVideoDataTV is !=null, we should update its content with the
            //number of received bytes usw
            if(receivedVideoDataTV !=null){
                final String s= VideoNative.getVideoInfoString(videoPlayer.getNativeInstance());
                updateViewIfStringChanged(context,receivedVideoDataTV,s,!VideoNative.anyVideoDataReceived(videoPlayer.getNativeInstance()));
            }
            if(button!=null){
                final boolean currentlyReceivingData=VideoNative.anyVideoBytesParsedSinceLastCall(videoPlayer.getNativeInstance());
                ((Activity)context).runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        if(currentlyReceivingData){
                            button.setTextColor(Color.GREEN);
                        }else{
                            button.setTextColor(Color.RED);
                        }
                    }
                });
            }
            //Every 3.5s we check if we are receiving video data, but cannot parse the data. Probably the user did mix up
            //rtp and raw. Make a warning toast
            if(System.currentTimeMillis()-lastCheckMS>=3500){
                final boolean errorVideo=VideoNative.receivingVideoButCannotParse(videoPlayer.getNativeInstance());
                lastCheckMS =System.currentTimeMillis();
                if(errorVideo){
                    ((Activity)context).runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            Toast.makeText(context,"You are receiving video data, but FPV-VR cannot parse it. If you are using RAW, please disable" +
                                    " rtp parsing via NETWORK-->UseRTP or change ez-wb config to rtp",Toast.LENGTH_LONG).show();
                        }
                    });
                }
            }
            //Refresh every 200ms
            try {Thread.sleep(200);} catch (InterruptedException e) {return;}
        }
    }

}
