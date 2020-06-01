package constantin.test;

public class SimpleEncoder implements Runnable {
    static{
        System.loadLibrary("UVCReceiverDecoder");
    }
    private static native long nativeCreate(String GroundRecordingDirectory);
    private static native long nativeDelete(long p);
    private static native void nativeLoop(long p);

    @Override
    public void run() {
        long p=nativeCreate(UVCReceiverDecoder.getDirectoryToSaveDataTo());
        // runs until EOF is reached (success) or thread is interrupted (error)
        nativeLoop(p);
        nativeDelete(p);
    }

}
