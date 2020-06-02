package constantin.test;

public class SimpleTranscoder implements Runnable {
    static{
        System.loadLibrary("UVCReceiverDecoder");
    }
    final String INPUT_FILE_PATH;
    public SimpleTranscoder(final String INPUT_FILE_PATH){
        this.INPUT_FILE_PATH =INPUT_FILE_PATH;
    }

    private static native long nativeCreate(String inputFilename,String TEST_FILE_DIRECTORY);
    private static native long nativeDelete(long p);
    private static native void nativeLoop(long p);

    @Override
    public void run() {
        long p=nativeCreate(INPUT_FILE_PATH,UVCReceiverDecoder.getDirectoryToSaveDataTo());
        // runs until EOF is reached (success) or thread is interrupted (error)
        nativeLoop(p);
        nativeDelete(p);
    }

}
