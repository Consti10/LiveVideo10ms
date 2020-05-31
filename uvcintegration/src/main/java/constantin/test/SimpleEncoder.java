package constantin.test;

public class SimpleEncoder {
    static{
        System.loadLibrary("UVCReceiverDecoder");
    }
    public static native long nativeStartConvertFile(String GroundRecordingDirectory);
    public static native long nativeStopConvertFile(long p);

}
